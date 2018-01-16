#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>


#define nmax  150
#define N_PROCESSUS  3
typedef int semaphore;

semaphore mutex = -1, sem = -1;
int gvars_id = -1;
key_t cle1 = -1, cle2 = -1;
struct var_g *gvars = NULL;
int nbds[N_PROCESSUS];
int sleeps[N_PROCESSUS];


semaphore semaphore_init(key_t cle, int nsems, int val) {
    semaphore sem_id = semget(cle, 1, 0);
    if (sem_id < 0) {
        sem_id = semget(cle, nsems, IPC_CREAT | IPC_EXCL | 0666);
        if (sem_id < 0) {
            perror("semget");
            exit(EXIT_FAILURE);
        }
    }
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *tab;
    } su;
    su.val = val;
    semctl(sem_id, 0, SETVAL, su);
    printf("Init sem %d to %d\n", sem_id, semctl(sem_id, 0, GETVAL, 0));
    return sem_id;
}

void modifier_semaphore(semaphore sem_id, unsigned short semnum, size_t nsems, int val) {
    struct sembuf *b = malloc(sizeof(struct sembuf));
    b->sem_num = semnum;
    b->sem_op = (short) val;
    b->sem_flg = 0;
    int r = semop(sem_id, b, nsems);
    if (r < 0) {
        perror("semop");
        exit(EXIT_FAILURE);
    }
}

void P(semaphore sem_id) {
    modifier_semaphore(sem_id, 0, 1, -1);
}

void V(semaphore sem_id) {
    modifier_semaphore(sem_id, 0, 1, 1);
}

void Pi(semaphore sem_id, unsigned short indice) {
    modifier_semaphore(sem_id, indice, 1, -1);
}

void Vi(semaphore sem_id, unsigned short indice) {
    modifier_semaphore(sem_id, indice, 1, 1);
}

void detruire_semaphore(semaphore sem_id, int nsems) {
    int r = semctl(sem_id, 0, IPC_RMID, 0);
    if (r < 0) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
}

struct maillon {
    int idfp;
    int nb;
};
struct file {
    int tete;
    int queue;
    struct maillon liste[N_PROCESSUS];
};

struct var_g {
    int nbres;
    struct file pliste[1];
};


void inserer(int idfp, int nb) {
    gvars = (struct var_g *) shmat(gvars_id, NULL, SHM_R | SHM_W);
    if (gvars == NULL) {
        perror("shmat");
        exit(1);
    }
    if (gvars->pliste->queue < N_PROCESSUS) {
        gvars->pliste->liste[gvars->pliste->queue].idfp = idfp;
        gvars->pliste->liste[gvars->pliste->queue].nb = nb;
    }
    gvars->pliste->queue++;
    int r = shmdt(gvars);
    if (r < 0) {
        perror("shmdt");
        exit(1);
    }
}

int tete() {
    gvars = (struct var_g *) shmat(gvars_id, NULL, SHM_R | SHM_W);
    if (gvars == NULL) {
        perror("shmat");
        exit(1);
    }
    int ret = gvars->pliste->liste[gvars->pliste->tete].nb;
    int r = shmdt(gvars);
    if (r < 0) {
        perror("shmdt");
        exit(1);
    }
    return ret;
}

void retirer(int *idfp, int *nb) {
    gvars = (struct var_g *) shmat(gvars_id, NULL, SHM_R | SHM_W);
    if (gvars == NULL) {
        perror("shmat");
        exit(1);
    }
    *idfp = gvars->pliste->liste[gvars->pliste->tete].idfp;
    *nb = gvars->pliste->liste[gvars->pliste->tete].nb;
    gvars->pliste->tete++;
    int r = shmdt(gvars);
    if (r < 0) {
        perror("shmdt");
        exit(1);
    }
}

int get_nbres() {
    gvars = (struct var_g *) shmat(gvars_id, NULL, SHM_R | SHM_W);
    if (gvars == NULL) {
        perror("shmat");
        exit(1);
    }
    int ret = gvars->nbres;
    int r = shmdt(gvars);
    if (r < 0) {
        perror("shmdt");
        exit(1);
    }
    return ret;
}

void set_nbres(int nbres) {
    gvars = (struct var_g *) shmat(gvars_id, NULL, SHM_R | SHM_W);
    if (gvars == NULL) {
        perror("shmat");
        exit(1);
    }
    gvars->nbres = nbres;
    int r = shmdt(gvars);
    if (r < 0) {
        perror("shmdt");
        exit(1);
    }
}

int file_vide() {
    gvars = (struct var_g *) shmat(gvars_id, NULL, SHM_R | SHM_W);
    if (gvars == NULL) {
        perror("shmat");
        exit(1);
    }
    int ret = (gvars->pliste->tete == gvars->pliste->queue);
    int r = shmdt(gvars);
    if (r < 0) {
        perror("shmdt");
        exit(1);
    }
    return (ret);
}

void allouer(int ifdp, int nbd) {
    P(mutex);
    int nbres = get_nbres();
    if (nbd > nbres) {
        /*** Attente ***/
        printf("Attente proc %d with nbres = %d\n", ifdp, nbres);
        inserer(ifdp, nbd);
        V(mutex);
        Pi(sem, (unsigned short) ifdp);
    } else {
        printf("Allocation pour %d with nbres= %d\n", ifdp, nbres);
        set_nbres(nbres - nbd);
        printf("Allocation pour %d with nbres= %d\n", ifdp, get_nbres());
    }
    V(mutex);
}

void dump_file() {
    gvars = (struct var_g *) shmat(gvars_id, NULL, SHM_R | SHM_W);
    if (gvars == NULL) {
        perror("shmat");
        exit(1);
    }
    printf("tete = %d queue = %d\n", gvars->pliste->tete, gvars->pliste->queue);
    int i;
    for (i = gvars->pliste->tete; i < gvars->pliste->queue; i++) {
        printf("element id=%d   nb=%d\n", gvars->pliste->liste[i].idfp, gvars->pliste->liste[i].nb);
    }

    int r = shmdt(gvars);
    if (r < 0) {
        perror("shmdt");
        exit(1);
    }
}

void liberer(int nbr) {
    int nbd, idf;
    P(mutex);
    int nbres = get_nbres();
    printf("Before Liberation with nbres= %d\n", get_nbres());
    set_nbres(nbres + nbr);
    printf("Liberation with nbres= %d\n", get_nbres());
    /*** Satisfaire les demandes de clients en attente ***/
    dump_file();
    while (!file_vide() && tete() <= (nbres = get_nbres())) {
        retirer(&idf, &nbd);
        printf("Allocation in boucle %d with nbres= %d\n", idf, get_nbres());
        set_nbres(nbres - nbd);
        printf("Allocation %d with nbres= %d\n", idf, get_nbres());
        Vi(sem, (unsigned short) idf);
    }
    V(mutex);
}


void child_i(int i) {
    sleep((unsigned int) sleeps[i]);
    printf("Processus %d demande %d ressource\n", i, nbds[i]);
    allouer(i, nbds[i]);
    // Utilisation
    sleep((unsigned int) sleeps[i] * (sleeps[N_PROCESSUS - i - 1]));
    printf("Processus %d UTILIZE %d ressource\n", i, nbds[i]);
    liberer(nbds[i]);
    exit(0);
}

int main() {
    printf("Hello\n");
    /********** INITIALISATION ------------------------*/
    cle1 = ftok(getenv("HOME"), 'A');
    if (cle1 == -1) {
        perror("ftok");
        exit(1);
    }
    cle2 = ftok(getenv("HOME"), 'B');
    if (cle2 == -1) {
        perror("ftok");
        exit(1);
    }
    mutex = semaphore_init(cle1, 1, 1);
    sem = semaphore_init(cle2, N_PROCESSUS, 0);

    gvars_id = shmget(cle1, sizeof(struct var_g), 0);
    if (gvars_id == -1) {
        gvars_id = shmget(cle1, sizeof(struct var_g), IPC_CREAT | IPC_EXCL | 0666);
        if (gvars_id == -1) {
            perror("shmget");
            exit(1);
        }
    }

    gvars = (struct var_g *) shmat(gvars_id, NULL, SHM_R | SHM_W);
    gvars->nbres = nmax;
    gvars->pliste->tete = 0;
    gvars->pliste->queue = 0;
    shmdt(gvars);
    int p = -1, i = 0;
    for (i = 0; i < N_PROCESSUS; i++) {
        nbds[i] = rand() % nmax;
    }
    nbds[0] = 80;
    nbds[2] = 60;
    nbds[1] = 60;
    sleeps[0] = 1;
    sleeps[2] = 2;
    sleeps[1] = 3;
    for (i = 0; i < N_PROCESSUS; i++) {
        p = fork();
        if (p == -1) {
            printf("\nerreur de fork \n");
            exit(6);
        }
        if (p == 0) {
            child_i(i);
        }
    }
    while (wait(NULL) != -1); // attente des fils
    printf("Done , Destroying stuff\n");
    detruire_semaphore(sem, N_PROCESSUS);
    detruire_semaphore(mutex, 1);
    shmctl(gvars_id, IPC_RMID, NULL);
    return 0;
}
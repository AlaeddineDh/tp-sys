#include "m_semaphore.h"

void changer_semaphore(semaphore sem, unsigned short sem_num, int nsems, short val) {
    struct sembuf *buff = (struct sembuf *) malloc(sizeof(struct sembuf));
    buff->sem_num = sem_num;
    buff->sem_op = val;
    buff->sem_flg = 0;
    if (semop(sem, buff, (size_t) nsems) != 0) {
        perror("semop");
        exit(EXIT_FAILURE);
    }
}

semaphore semaphore_init(key_t cle, int val) {
    semaphore sem = semget(cle, 1, 0);
    if (sem < 0) {
        sem = semget(cle, 1, IPC_CREAT | IPC_EXCL | 0666);
        if (sem < 0) {
            perror("semget");
            exit(EXIT_FAILURE);
        }
        // Initialiser le semaphore
        changer_semaphore(sem, 0, 1, (short) val);
    }
    return sem;
}

void detruire_semaphore(semaphore sem) {
    int r = semctl(sem, 0, IPC_RMID, 0);
    if (r < 0) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
}


void P(semaphore sem) {
    changer_semaphore(sem, 0, 1, -1);
}

void V(semaphore sem) {
    changer_semaphore(sem, 0, 1, 1);
}

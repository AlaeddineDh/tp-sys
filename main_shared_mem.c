#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

int main() {
    key_t cle = ftok("/home/alaeddine/batata", 'A');

    if (cle == -1) {
        //Erreur lors de la creation de la cle
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    int id = shmget(cle, sizeof(int), 0 /*IPC_CREAT | IPC_EXCL | 0666*/);

    if (id == -1) {
        switch (errno) {
            case EEXIST:
                printf("Segment existe deja\n");
                break;
            default:
                perror("shmget");
                break;
        }
        exit(EXIT_FAILURE);
    }
    /*struct shmid_ds ds;
    shmctl(id,IPC_STAT,&ds);
    printf("pid %d",ds.shm_segsz);*/
    char *m = (char *) shmat(id, NULL, SHM_R | SHM_W);
    if (*m == -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
//    *m = 1997;
//    m[0] = 1;
//    m[1] = 2;
//    m[2] = 3;
//    m[3] = 4;
//    printf("m is %d",m[0]);
//    printf("m is %d",m[1]);
//    printf("m is %d",m[2]);
//    printf("m is %d",m[3]);
    if (shmctl(id,IPC_RMID,NULL) == -1){
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    if (shmctl(id,IPC_RMID,NULL) == -1){
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    return 0;
}
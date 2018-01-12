//
// Created by alaeddine on 1/11/18.
//
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "m_semaphore.h"
#include "m_semaphore.c"

struct common {
    int nb;
    int sum;
};

int main() {
    key_t cle = ftok("/home/alaeddine/batata", 'A');

    if (cle == -1) {
        //Erreur lors de la creation de la cle
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    semaphore mutex = semaphore_init(cle, 1);

    int id = shmget(cle, sizeof(struct common),0/* IPC_CREAT | IPC_EXCL | 0666*/);

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

    struct common *c = (struct common *) shmat(id, NULL, SHM_R | SHM_W);
    if (c == NULL) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    P(mutex);
    c->nb = 0;
    c->sum = 0;
    while (1) {
        int response;
        printf("+ ");
        if (scanf("%d", &response) != 1)
            break;
        c->nb++;
        c->sum += response;
        printf("sub-total %d= %d\n", c->nb,
               c->sum);
    }
    V(mutex);

    //Detach
    if (shmdt(c) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    //Destroy la zone
    if (shmctl(id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    return 0;
}

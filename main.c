//
// Created by alaeddine on 1/12/18.
//
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "m_semaphore.h"
#include "m_semaphore.c"


int main() {
    key_t cle = ftok("/home/alaeddine/batata", 'A');

    if (cle == -1) {
        //Erreur lors de la creation de la cle
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    semaphore sem = semaphore_init(cle, 2);
    printf("before section ----->\n");
    P(sem);
    printf("section critique : give number \n");
    int n;
    scanf("%d", &n);
    V(sem);
//    detruire_semaphore(sem);

    return 0;
}

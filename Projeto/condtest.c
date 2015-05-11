/* 
 * File:   condtest.c
 * Author: max
 *
 * Created on 30 de Abril de 2015, 10:26
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define LIMIT 12

int count = 0;
pthread_mutex_t mutex_count;
pthread_cond_t cond_limit;
pthread_cond_t cond_count;

void *counter(void *t) {
    long id = (long) t;
    while (1) {
        pthread_mutex_lock(&mutex_count);
        if (count == LIMIT) {
            printf("Thread %ld chegou ao limite - acordando o nullificador.\n", id);
            pthread_cond_signal(&cond_limit);
            pthread_cond_wait(&cond_count, &mutex_count);
        }
        printf("Thread %ld aumentou contador de %d para %d\n", id, count++, count+1);
        pthread_mutex_unlock(&mutex_count);
        usleep(200000);
    }

}

void *nullify(void *t) {
    long id = (long) t;
    while (1) {
        pthread_mutex_lock(&mutex_count);
        if (count < LIMIT) {
            printf("Thread %ld foi escalonada mas o contador não chegou ao limite.\n",id);
            pthread_cond_wait(&cond_limit, &mutex_count);
        }
        while (count > 0) {
            printf("Thread %ld diminuiu o contador de %d para %d\n", id, count--, count-1);
            usleep(200000);
        }
        pthread_mutex_unlock(&mutex_count);
        pthread_cond_broadcast(&cond_count);
        //pthread_cond_signal(&cond_count);
        //pthread_cond_signal(&cond_count);
    }
}

/*
 * 
 */
int main(int argc, char** argv) {
    long t1 = 1, t2 = 2, t3 = 3;
    pthread_t threads[3];
    pthread_attr_t attr;

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&mutex_count, NULL);
    pthread_cond_init(&cond_count, NULL);
    pthread_cond_init(&cond_limit, NULL);

    /* For portability, explicitly create threads in a joinable state */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&threads[0], &attr, nullify, (void *) t1);
    pthread_create(&threads[1], &attr, counter, (void *) t2);
    pthread_create(&threads[2], &attr, counter, (void *) t3);

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&mutex_count, NULL);
    pthread_cond_init(&cond_count, NULL);
    pthread_cond_init(&cond_limit, NULL);

    /* Wait for all threads to complete */
    int i;
    for (i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    return (EXIT_SUCCESS);
}


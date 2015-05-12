#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>

#define LIMIT 12

sem_t semCounter;
sem_t semNullify;

pthread_barrier_t barrier;

void *counter(void *t) {
    long id = (long) t;
    sem_wait(&semCounter);
    int count = 0;
    while (1) {
        if (count == LIMIT) {
            count = 0;
            if (id == 3) {
                printf("Thread %ld chegou ao limite - acordando o nullificador.\n", id);
                sem_post(&semNullify);
            }
            sem_wait(&semCounter);
        }
        printf("Thread %ld aumentou contador de %d para %d\n", id, count++, count + 1);
        sleep(1);
    }

}

void *nullify(void *t) {
    long id = (long) t;
    sem_wait(&semNullify);
    int count = 12;
    while (1) {
        while (count > 0) {
            printf("Thread %ld diminuiu o contador de %d para %d\n", id, count--, count - 1);
            sleep(1);
        }
        printf("Thread %ld chegou ao limite - acordando os counters.\n", id);
        count = 12;
        sem_post(&semCounter);
        sem_post(&semCounter);
        sem_wait(&semNullify);
    }
}

void *barrierTest(void *t) {
    long id = (long) t;
    while (1) {
        printf("%ld Executando\n", id);
        srand(time(NULL));
        int r = rand() % 3;
        sleep(r);
        printf("%ld Fim\n", id);
        pthread_barrier_wait(&barrier);
        printf("%ld Loop\n", id);
    }
}

/*
 * 
 */
int main(int argc, char** argv) {
    long t1 = 1, t2 = 2, t3 = 3;
    pthread_t threads[10];

    pthread_barrier_init(&barrier, NULL, 10);
    long int i = 0;
    for (i = 0; i < 10; i++) {
        pthread_create(&threads[i], NULL, barrierTest, (void*) i);
    }

    for (i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }



    /* Initialize semaphores variables objects */
    sem_init(&semCounter, 0, 2);
    sem_init(&semNullify, 0, 0);

    /* For portability, explicitly create threads in a joinable state */
    pthread_create(&threads[0], NULL, nullify, (void *) t1);
    pthread_create(&threads[1], NULL, counter, (void *) t2);
    pthread_create(&threads[2], NULL, counter, (void *) t3);

    /* Wait for all threads to complete */
    for (i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    return (EXIT_SUCCESS);
}


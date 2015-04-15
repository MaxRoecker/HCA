/* 
 * File:   main.c
 * Author: max
 *
 * Created on 15 de Abril de 2015, 16:16
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define NUM_THREADS 5

void *printHello(void *threadId){
    long tid;
    tid = (long) threadId;
    printf("Hello World! It's me, thread #%ld!\n",tid);
    pthread_exit(NULL);
}

/*
 * 
 */
int main(int argc, char** argv) {
    
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;
    for(t = 0; t< NUM_THREADS;t++){
        printf("In main: Creating thread %ld\n",t);
        rc = pthread_create(&threads[t],NULL,printHello, (void*)t);
        if(rc){
            printf("ERROR! Return code from pthread_create() is %d\n",rc);
            exit(-1);
        }
    }
    
    pthread_exit(NULL);
}


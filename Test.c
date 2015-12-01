#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#define INIT_TIMEOUT_LIMIT 2500
#define MAX_PAYLOAD_SIZE 1024

struct timeval init_timer;

typedef struct { 
    signed short source;
    signed int destination;
    size_t payload_length;
    char payload[MAX_PAYLOAD_SIZE];
} packet_t;

packet_t* globalpipe;

int initTimeout() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    if((current_time.tv_sec - init_timer.tv_sec)*1000 > INIT_TIMEOUT_LIMIT) {
        gettimeofday(&init_timer, NULL);
        return 1;
    } else {
        return 0;
    }
}

void * PrintName(void *id) {
    int threadid = (int)id;
    printf("Thread #%d created\n", threadid);
    globalpipe->source += 1;
    pthread_exit(NULL);
    return 0;
}

int main(){
    globalpipe = malloc(sizeof(packet_t));
    globalpipe->source = 0;
    globalpipe->destination = 0;
    globalpipe->payload_length = 0;
    globalpipe->payload = "hello world";
    printf(" ");
    pthread_t threads[5];
    gettimeofday(&init_timer,NULL);
    int check;
    for (int i = 0; i < 5; i++) {
        while(!initTimeout());
        check = pthread_create(&threads[i], NULL, PrintName, (void *)i);
        if (check) {
            printf("ERROR");
            exit(-1);
        }
    }
    printf("Globalpipe source=%d", globalpipe->source);
}
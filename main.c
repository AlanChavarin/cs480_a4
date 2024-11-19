#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "structs.h"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>  
#include "helpers.h"
#include "log.h"
#include <getopt.h>
//#include "seating.h"

int main(int argc, char **argv) {
    int total_requests = 120; // total number of seating requests
    int tx_time = 0; // time that TX robot uses on average for processing a seatign request. simulate this time for the tx robot to consume this request via sleeping for this many milliseconds
    int rev9_time = 0; // same as above 
    int general_time = 0; // time in miliseconds for the general greeter robot to insert a general table request
    int vip_time = 0; // time for the vip room greeter robot to insert into the vip room 
    
    int opt;
    while ((opt = getopt(argc, argv, "s:x:r:g:v:")) != -1) {
        switch (opt) {
            case 's':
                total_requests = atoi(optarg);
                break;
            case 'x':
                tx_time = atoi(optarg);
                break;
            case 'r':
                rev9_time = atoi(optarg);
                break;
            case 'g':
                general_time = atoi(optarg);
                break;
            case 'v':
                vip_time = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-s requests] [-x tx_time] [-r rev9_time] [-g general_time] [-v vip_time]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // TODO: Initialize semaphores, shared memory, and start producer/consumer threads
    
    // printf("Total requests: %d\n", total_requests);
    // printf("T-X service time: %d\n", tx_time);
    // printf("REV-9 service time: %d\n", rev9_time); 
    // printf("General table request interval: %d\n", general_time);
    // printf("VIP room request interval: %d\n", vip_time);

    // create the request queue
    //Queue line_outside_queue;
    //init_queue(&line_outside_queue, total_requests);
    Queue request_queue;
    init_queue(&request_queue, QUEUE_SIZE);

    // populate the request queue
    // for (int i = 0; i < total_requests; i++) {
    //     push_to_queue(&line_outside_queue, i % 2);
    // }

    pthread_mutex_t queue_mutex;
    pthread_mutex_init(&queue_mutex, NULL);
    
    pthread_mutex_t barrier;
    pthread_cond_t barrier_cond;
    int threads_completed = 0;  

    pthread_mutex_init(&barrier, NULL);
    pthread_cond_init(&barrier_cond, NULL);
    //pthread_mutex_lock(&barrier);

    struct greeter_args* args1 = malloc(sizeof(struct greeter_args));
    //args1->line_queue = &line_outside_queue;
    args1->time = &general_time;
    args1->vip_time = &vip_time;
    args1->queue_mutex = &queue_mutex;
    args1->customer_type = 0;
    args1->queue = &request_queue;
    args1->total_requests = &total_requests;
    args1->barrier = &barrier;
    args1->threads_completed = &threads_completed;
    args1->barrier_cond = &barrier_cond;
    
    struct greeter_args* args2 = malloc(sizeof(struct greeter_args));
    //args2->line_queue = &line_outside_queue;
    args2->time = &general_time;
    args2->vip_time = &vip_time;
    args2->queue_mutex = &queue_mutex;
    args2->customer_type = 1;
    args2->queue = &request_queue;
    args2->total_requests = &total_requests;
    args2->barrier = &barrier;
    args2->threads_completed = &threads_completed;
    args2->barrier_cond = &barrier_cond;

    pthread_t general_greeter_thread1;
    pthread_t general_greeter_thread2;

    struct concierge_args* concierge_args1 = malloc(sizeof(struct concierge_args));
    concierge_args1->queue = &request_queue;
    concierge_args1->time = &tx_time;
    concierge_args1->queue_mutex = &queue_mutex;
    concierge_args1->id = 0;
    concierge_args1->barrier = &barrier;
    concierge_args1->total_requests = &total_requests;
    concierge_args1->threads_completed = &threads_completed;
    concierge_args1->barrier_cond = &barrier_cond;

    struct concierge_args* concierge_args2 = malloc(sizeof(struct concierge_args));
    concierge_args2->queue = &request_queue;
    concierge_args2->time = &rev9_time;
    concierge_args2->queue_mutex = &queue_mutex;
    concierge_args2->id = 1;
    concierge_args2->barrier = &barrier;
    concierge_args2->total_requests = &total_requests;
    concierge_args2->threads_completed = &threads_completed;
    concierge_args2->barrier_cond = &barrier_cond;

    pthread_t tx_thread;
    pthread_t rev9_thread;

    pthread_create(&general_greeter_thread1, NULL, general_greeter, args1);
    pthread_create(&general_greeter_thread2, NULL, general_greeter, args2);
    pthread_create(&tx_thread, NULL, concierge_robot, concierge_args1);
    pthread_create(&rev9_thread, NULL, concierge_robot, concierge_args2);


    // continue when all threads are finished

    pthread_mutex_lock(&barrier);
    while (threads_completed < 4) {
        pthread_cond_wait(&barrier_cond, &barrier);
    }
    pthread_mutex_unlock(&barrier);

    // output results
    printf("Threads completed: %d\n", threads_completed);
    unsigned int produced[] = {request_queue.consumed[0][0] + request_queue.consumed[0][1], request_queue.consumed[1][0] + request_queue.consumed[1][1]};
    output_production_history(produced, request_queue.consumed);

    //printf("Total count of the queue: %d\n", request_queue.count);

    return 0;
}

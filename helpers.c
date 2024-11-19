#include <pthread.h>
#include "structs.h"
#include "seating.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#define MAX_VIP 6
#define max_QUEUE_SIZE 18

void init_queue(Queue* queue, int size) {
    queue->requests = (RequestType*)malloc(size * sizeof(RequestType));
    
    // allocat memory for each consumed counter arr
    queue->consumed[0] = (unsigned int*)malloc(2 * sizeof(unsigned int));
    queue->consumed[1] = (unsigned int*)malloc(2 * sizeof(unsigned int));
    
    // init all counters to 0
    queue->consumed[0][0] = 0;  // 1st concierge's normal count
    queue->consumed[0][1] = 0;  // 1st concierge's VIP count
    queue->consumed[1][0] = 0;  // 2nd concierge's normal count
    queue->consumed[1][1] = 0;  // 2nd concierge's VIP count
    
    queue->count = 0;
    queue->vip_count = 0;
    queue->normal_count = 0;
    queue->total_normal_count = 0;
    queue->total_vip_count = 0;
}

void push_to_queue(Queue* queue, RequestType request) {
    queue->count++;
    queue->requests[queue->count - 1] = request;

    if (request == VIPRoom) {
        queue->vip_count++;
        queue->total_vip_count++;
    } else {
        queue->normal_count++;
        queue->total_normal_count++;
    }
}

void print_queue(Queue* queue){
    printf("Queue count: %d\n", queue->count);
    printf("Queue: ");
    for (int i = 0; i < queue->count; i++) {
        printf("%d ", queue->requests[i]);
    }
    printf("\n");
}

int check_if_total_count_is_reached(Queue* queue, int total_requests){
    return queue->total_normal_count + queue->total_vip_count >= total_requests;
}

int check_if_queue_has_been_consumed(Queue* queue, int total_requests){
    int total_consumed = queue->consumed[0][0] + queue->consumed[0][1] + 
                        queue->consumed[1][0] + queue->consumed[1][1];
    return total_consumed >= total_requests && queue->total_normal_count + queue->total_vip_count >= total_requests;
}

RequestType pop_from_queue(Queue* queue) {
    if(queue->count <= 0){
        // throw an error
        printf("attempting to pop from Queue when empty\n");
        exit(1);
    }
    RequestType request = queue->requests[0];
    for (int i = 0; i < queue->count - 1; i++) {
        queue->requests[i] = queue->requests[i + 1];
    }
    // update the queue
    if (request == VIPRoom) {
        queue->vip_count--;
    } else {
        queue->normal_count--;
    }
    queue->count--;

    if(queue->vip_count < 0){
        // throw an error
        printf("VIP count is less than 0\n");
        exit(1);
    }

    if(queue->normal_count < 0){
        // throw an error
        printf("Normal count is less than 0\n");
        exit(1);
    }

    //printf("Popped from queue: %d\n", request);

    return request;
}

int is_empty(Queue* queue) {
    return queue->count == 0;
}

// robots

void* general_greeter(void* args) {
    // cast the args to the correct type
    struct greeter_args* greeter_args = (struct greeter_args*)args;
    //Queue* line_outside_queue = greeter_args->line_queue;
    int time = *greeter_args->time;
    int vip_time = *greeter_args->vip_time;
    pthread_mutex_t *queue_mutex = greeter_args->queue_mutex;
    int customer_type = greeter_args->customer_type;
    Queue* request_queue = greeter_args->queue;
    int total_requests = *greeter_args->total_requests;
    pthread_mutex_t* barrier = greeter_args->barrier;
    int* threads_completed = greeter_args->threads_completed;
    pthread_cond_t* barrier_cond = greeter_args->barrier_cond;

    //printf("General greeter %d is running\n", greeter_id);
    
    while (1) {

        if(customer_type == VIPRoom){
            struct timespec ts = {
                .tv_sec = vip_time / 1000,
                .tv_nsec = (vip_time % 1000) * 1000000
            };
            nanosleep(&ts, NULL);
        } else {
            struct timespec ts = {
                .tv_sec = time / 1000,
                .tv_nsec = (time % 1000) * 1000000
            };
            nanosleep(&ts, NULL);
        }

        pthread_mutex_lock(queue_mutex);

        if(check_if_total_count_is_reached(request_queue, total_requests)){
            pthread_mutex_unlock(queue_mutex);
            break;
        }

        if(check_if_queue_has_been_consumed(request_queue, total_requests)){
            pthread_mutex_unlock(queue_mutex);
            break;
        }

        //wait for the queue to have a spot if its full
        while (request_queue->count >= max_QUEUE_SIZE) {
            pthread_mutex_unlock(queue_mutex);
            struct timespec ts = {
                .tv_sec = time / 1000,
                .tv_nsec = (time % 1000) * 1000000
            };
            nanosleep(&ts, NULL);
            pthread_mutex_lock(queue_mutex);
        }

        // wait for the queue to have a spot if the vip count is greater than 4 and our request is not a vip
        while (request_queue->vip_count >= 5 && customer_type == 1) {
            pthread_mutex_unlock(queue_mutex);
            struct timespec ts = {
                .tv_sec = time / 1000,
                .tv_nsec = (time % 1000) * 1000000
            };
            nanosleep(&ts, NULL);
            pthread_mutex_lock(queue_mutex);
        }

        // for now we print
        // printf("General greeter %d met and pushed to queue customer of type: %d\n ", greeter_id, customer_type);

        push_to_queue(request_queue, customer_type);

        unsigned int produced[] = {request_queue->total_normal_count, request_queue->total_vip_count};
        unsigned int inRequestQueue[] = {request_queue->normal_count, request_queue->vip_count};

        //printf("%d, %d\n", inRequestQueue[0], inRequestQueue[1]);
        output_request_added(customer_type, produced, inRequestQueue);

        if(check_if_total_count_is_reached(request_queue, total_requests)){
            pthread_mutex_unlock(queue_mutex);
            break;
        }

        // unlock the queue once we are done working with it
        pthread_mutex_unlock(queue_mutex);
        
    }

    pthread_mutex_lock(barrier);
    (*threads_completed)++;
    pthread_cond_signal(barrier_cond);
    pthread_mutex_unlock(barrier);

    return NULL;
}

// normal seater
void* concierge_robot(void* args){
    //grab all the args
    struct concierge_args* concierge_args = (struct concierge_args*)args;
    Queue* queue = concierge_args->queue;
    int sleep_time = *concierge_args->time;
    pthread_mutex_t* queue_mutex = concierge_args->queue_mutex;
    int id = concierge_args->id;
    pthread_mutex_t* barrier = concierge_args->barrier;
    int* total_requests = concierge_args->total_requests;
    int* threads_completed = concierge_args->threads_completed;
    pthread_cond_t* barrier_cond = concierge_args->barrier_cond;

    //unsigned int consumedCounts[] = {0, 0};

    while(1){        
        struct timespec ts = {
            .tv_sec = sleep_time / 1000,
            .tv_nsec = (sleep_time % 1000) * 1000000
        };
        nanosleep(&ts, NULL);

        //lock the queue
        pthread_mutex_lock(queue_mutex);

        // check that the queue is not empty
        if (is_empty(queue)) {
            // Check completion while still holding the mutex
            if(check_if_queue_has_been_consumed(queue, *total_requests)){
                pthread_mutex_unlock(queue_mutex);
                break;
            }
            pthread_mutex_unlock(queue_mutex);
            continue;
        }
        
        // pop from the queue
        RequestType request = pop_from_queue(queue);
        if(request == VIPRoom){
            queue->consumed[id][1]++;
        } else {
            queue->consumed[id][0]++;
        }

        unsigned int normalAndVIPCounts[] = {queue->normal_count, queue->vip_count};
        output_request_removed(id, request, queue->consumed[id], normalAndVIPCounts);

        // unlock the queue
        pthread_mutex_unlock(queue_mutex);

    }
    
    pthread_mutex_lock(barrier);
    (*threads_completed)++;
    pthread_cond_signal(barrier_cond);
    pthread_mutex_unlock(barrier);

    return NULL;
}
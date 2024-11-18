#include <pthread.h>
#include "structs.h"
#include "seating.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
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

RequestType pop_from_queue(Queue* queue) {
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
    Queue* line_outside_queue = greeter_args->line_queue;
    int time = *greeter_args->time;
    int vip_time = *greeter_args->vip_time;
    pthread_mutex_t *queue_mutex = greeter_args->queue_mutex;
    pthread_mutex_t *line_outside_mutex = greeter_args->line_outside_mutex;
    //int greeter_id = greeter_args->greeter_id;
    Queue* request_queue = greeter_args->queue;

    //printf("General greeter %d is running\n", greeter_id);
    
    while (1) {

        //lock the line outside mutex
        pthread_mutex_lock(line_outside_mutex);

        if (is_empty(line_outside_queue)) {
            //printf("General greeter %d: No more customers in line\n", greeter_id);
            //printf("General greeter %d: Count of the queue: %d\n", greeter_id, request_queue->count);
            pthread_mutex_unlock(line_outside_mutex);

            // get the line outside mutex value
            // int line_outside_mutex_value;
            // sem_getvalue(line_outside_mutex, &line_outside_mutex_value);
            // printf("General greeter %d: Line outside mutex value: %d\n", greeter_id, line_outside_mutex_value);
            // int queue_mutex_value;
            // sem_getvalue(queue_mutex, &queue_mutex_value);
            // printf("General greeter %d: Queue mutex value: %d\n", greeter_id, queue_mutex_value);
            // printf("General greeter %d: Is empty: %d\n", greeter_id, is_empty(line_outside_queue));

            break;  // Exit if no more customers
        }

        RequestType customer_type = pop_from_queue(line_outside_queue);
        if(customer_type == VIPRoom){
            usleep(vip_time);
        } else {
            usleep(time);
        }

        // unlock the line outside mutex
        pthread_mutex_unlock(line_outside_mutex);

        //-----------------------------

        // now lock/wait for the queue once we have read in from the line outside
        pthread_mutex_lock(queue_mutex);


        //wait for the queue to have a spot if its full
        while (request_queue->count >= max_QUEUE_SIZE) {
            pthread_mutex_unlock(queue_mutex);
            usleep(time);
            pthread_mutex_lock(queue_mutex);
        }

        // wait for the queue to have a spot if the normal count is greater than 12 and our request is not a vip
        while (request_queue->normal_count >= 13 && customer_type != VIPRoom) {
            pthread_mutex_unlock(queue_mutex);
            usleep(time);
            pthread_mutex_lock(queue_mutex);
        }

        // wait for the queue to have a spot if the vip count is greater than 4 and our request is not a vip
        while (request_queue->vip_count >= 5 && customer_type != VIPRoom) {
            pthread_mutex_unlock(queue_mutex);
            usleep(time);
            pthread_mutex_lock(queue_mutex);
        }


        // for now we print instead of pushing to the real queue
        // printf("General greeter %d met and pushed to queue customer of type: %d\n ", greeter_id, customer_type);

        push_to_queue(request_queue, customer_type);

        unsigned int produced[] = {request_queue->total_normal_count, request_queue->total_vip_count};
        unsigned int inRequestQueue[] = {request_queue->normal_count, request_queue->vip_count};

        output_request_added(customer_type, produced, inRequestQueue);

        // unlock the queue once we are done working with it
        pthread_mutex_unlock(queue_mutex);
        
    }
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
        // lock the queue
        pthread_mutex_lock(queue_mutex);
        usleep(sleep_time);

        // check that the queue is not empty
        if (is_empty(queue)) {
            pthread_mutex_unlock(queue_mutex);

            // check if all the requests have been consumed and counts are zero, if so signal the barrier to be unlocked and increment the threads completed count 
            if(*total_requests == queue->consumed[0][0] + queue->consumed[0][1] + queue->consumed[1][0] + queue->consumed[1][1] && queue->count == 0 && queue->vip_count == 0 && queue->normal_count == 0){
                pthread_mutex_lock(barrier);
                (*threads_completed)++;
                pthread_cond_signal(barrier_cond);
                pthread_mutex_unlock(barrier);
            }

            continue;
        }

        // pop from the queue
        RequestType request = pop_from_queue(queue);
        if(request == VIPRoom){
            queue->consumed[id][1]++;
        } else {
            queue->consumed[id][0]++;
        }

        // unlock the queue
        pthread_mutex_unlock(queue_mutex);

        unsigned int normalAndVIPCounts[] = {queue->normal_count, queue->vip_count};
        output_request_removed(id, request, queue->consumed[id], normalAndVIPCounts);

        // check if all the requests have been consumed and counts are zero, if so signal the barrier to be unlocked and increment the threads completed count 
        if(*total_requests == queue->consumed[0][0] + queue->consumed[0][1] + queue->consumed[1][0] + queue->consumed[1][1] && queue->count == 0 && queue->vip_count == 0 && queue->normal_count == 0){
            pthread_mutex_lock(barrier);
            (*threads_completed)++;
            pthread_cond_signal(barrier_cond);
            pthread_mutex_unlock(barrier);
        }

        //check the type of request for now just print
        // if(request == VIPRoom){
        //     printf("Concierge robot %d has seated VIP customer\n", greeter_id);
        // } else {
        //     printf("Concierge robot %d has seated Normal customer\n", greeter_id);
        // }
    }

    return NULL;
}
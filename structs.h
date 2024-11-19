#include <pthread.h>
#include "seating.h"
#include <semaphore.h>

#define QUEUE_SIZE 18 // max queue size from requirements


typedef struct Queue {
       RequestType *requests;
       int count;                     
       int vip_count;                
       int normal_count;              
       int total_normal_count;        
       int total_vip_count;           

       // total consumed count
       unsigned int *consumed[2];
       
} Queue;

// create a struct to hold the multiple parameters
struct greeter_args {
    Queue* line_queue;
    Queue* queue;
    double* time;
    double* vip_time;
    pthread_mutex_t* queue_mutex;
    int customer_type;
    int* total_requests;

    pthread_mutex_t* barrier;
    int* threads_completed;
    pthread_cond_t* barrier_cond;
};

struct concierge_args {
    Queue* queue;
    double* time;
    pthread_mutex_t* queue_mutex;
    int id;
    int* total_requests;

    pthread_mutex_t* barrier;
    int* threads_completed;
    pthread_cond_t* barrier_cond;
};

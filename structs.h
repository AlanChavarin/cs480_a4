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
    int* time;
    int* vip_time;
    pthread_mutex_t* queue_mutex;
    pthread_mutex_t* line_outside_mutex;
    int greeter_id;
};

struct concierge_args {
    Queue* queue;
    int* time;
    pthread_mutex_t* queue_mutex;
    int id;
    pthread_mutex_t* barrier;
    int* total_requests;
    int* threads_completed;
    pthread_cond_t* barrier_cond;
};

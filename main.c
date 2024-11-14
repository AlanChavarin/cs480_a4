#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include "log.h"
//#include "seating.h"

int main(int argc, char **argv) {
    int total_requests = 120; // Default value
    int tx_time = 0;
    int rev9_time = 0; 
    int general_time = 0;
    int vip_time = 0;
    
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
    
    printf("Total requests: %d\n", total_requests);
    printf("T-X service time: %d\n", tx_time);
    printf("REV-9 service time: %d\n", rev9_time); 
    printf("General table request interval: %d\n", general_time);
    printf("VIP room request interval: %d\n", vip_time);

    return 0;
}

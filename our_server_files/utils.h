#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/time.h>
#include <pthread.h>

typedef struct Request{
    int connfd;
    struct timeval arrival;
    struct timeval dispatch;
} Request;

typedef struct{
    pthread_t thread;
    int thread_id;
    int req_counter;
    int st_counter;
    int dyn_counter;
} ThreadInfo;

typedef enum {
    DT,
    DH,
    BLOCK,
    RAND
} Pol_t;

#endif // __UTILS_H__

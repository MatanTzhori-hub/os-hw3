#include "segel.h"
#include "request.h"
#include "queue.h"
#include "utils.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// Define globals
pthread_cond_t work_cond, block_cond;
pthread_mutex_t mtx;
Queue wait_queue, exec_queue;
ThreadInfo* threads_arr;

void getargs(int *port, int *thread_n, int *req_n, Pol_t *policy, int argc, char *argv[])
{
    if (argc != 5) {
	fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <schedalg>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *thread_n = atoi(argv[2]);
    *req_n = atoi(argv[3]);
    if (strcmp(argv[4], "dt") == 0) {
        *policy = DT;
    } else if (strcmp(argv[4], "dh") == 0) {
        *policy = DH;
    } else if (strcmp(argv[4], "block") == 0) {
        *policy = BLOCK;
    } else {
        *policy = RAND;
    }
}

void* thread_main(void* temp){
    ThreadInfo* thread_info = (ThreadInfo*)temp;

    while(1){
        pthread_mutex_lock(&mtx);
        while(Q_isEmpty(wait_queue)){
            pthread_cond_wait(&work_cond, &mtx);
        }

        Request cur_request = Q_Pop(wait_queue);
        thread_info->req_counter++;
        gettimeofday(&cur_request.dispatch, NULL);
        timersub(&cur_request.dispatch, &cur_request.arrival, &cur_request.dispatch);
        Q_Insert(exec_queue, cur_request);
        pthread_mutex_unlock(&mtx);
        
        requestHandle(&cur_request, thread_info);
        Close(cur_request.connfd);

        pthread_mutex_lock(&mtx);
        Q_PopVal(exec_queue, cur_request.connfd);
        pthread_cond_signal(&block_cond);
        pthread_mutex_unlock(&mtx);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    int thread_n, req_n;
    Pol_t policy;

    // Init globals
    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&work_cond, NULL);
    pthread_cond_init(&block_cond, NULL);

    getargs(&port, &thread_n, &req_n, &policy, argc, argv);
    // For debugger:
    // port = 2001;
    // thread_n = 2;
    // req_n = 2;
    // policy = 0;
    //--------------

    // Create queues
    wait_queue = Q_CreateQueue(req_n);
    exec_queue = Q_CreateQueue(thread_n);
    threads_arr = calloc(thread_n, sizeof(*threads_arr));
    
    // Create workers threads
    for(int i = 0; i < thread_n; i++){
        threads_arr[i].thread_id = i;
        pthread_create(&(threads_arr[i].thread), NULL, thread_main, &threads_arr[i]);
    }

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        Request new_request = {.connfd = connfd};
        gettimeofday(&new_request.arrival, NULL);
        
        pthread_mutex_lock(&mtx);
        if(wait_queue->size + exec_queue->size >= req_n){
            switch (policy)
            {
            case DT:
                Close(connfd);
                pthread_mutex_unlock(&mtx);
                continue;
                break;
            
            case DH:
                if(Q_isEmpty(wait_queue)){
                    Close(connfd);
                    pthread_mutex_unlock(&mtx);
                    continue;
                    break;
                }
                else {
                    Request removed_request = Q_Pop(wait_queue);
                    Close(removed_request.connfd);
                }
                break;
            
            case BLOCK:
                while(Q_isFull(wait_queue)){
                    pthread_cond_wait(&block_cond, &mtx);
                }
                break;
            
            case RAND:
                // TODO: Add random implimentation
                if(Q_isEmpty(wait_queue)){
                    Close(connfd);
                    pthread_mutex_unlock(&mtx);
                    continue;
                    break;
                }
                else {
                    int req_to_drop = (wait_queue->size + 1)/ 2;
                    printf("Current wait queue: ");
                    Q_print(wait_queue);
                    printf("Dropping %d requests\n", req_to_drop);
                    while(req_to_drop > 0){
                        int index_to_drop = rand() % wait_queue->size;
                        Request request_to_drop = Q_FindByIndex(wait_queue, index_to_drop);
                        Q_PopVal(wait_queue, request_to_drop.connfd);
                        Close(request_to_drop.connfd);
                        req_to_drop--;
                        printf("Dropped request %d, in index %d\n", request_to_drop.connfd, index_to_drop);
                        Q_print(wait_queue);
                    }
                }
                break;
            }
        }
        Q_Insert(wait_queue, new_request);
        pthread_cond_signal(&work_cond);
        pthread_mutex_unlock(&mtx);
    }

    // Add destroy function to all Queses and Arrays
    Q_DestroyQueue(wait_queue);
    Q_DestroyQueue(exec_queue);
    free(threads_arr);
}   

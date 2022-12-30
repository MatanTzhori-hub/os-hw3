#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define EMPTY -1
#define SEGFAULT -2

typedef struct Queue {
    int head, tail, size;
    int max_size;
    Request* array;
} *Queue;

Queue Q_CreateQueue(int max_size)
{
    Queue queue = (Queue)malloc(sizeof(*queue));
    queue->size = 0;
    queue->head = 0;
    queue->max_size = max_size;
 
    queue->tail = max_size - 1;
    queue->array = (Request*)malloc(max_size * sizeof(Request));
    return queue;
}

void Q_DestroyQueue(Queue queue)
{
    free(queue->array);
    free(queue);
}
 
int Q_isFull(Queue queue)
{
    return (queue->size == queue->max_size);
}
 
int Q_isEmpty(Queue queue)
{
    return (queue->size == 0);
}
 
int Q_Insert(Queue queue, Request request)
{
    if (Q_isFull(queue))
        return EMPTY;
    queue->tail = (queue->tail + 1) % queue->max_size;
    queue->array[queue->tail] = request;
    queue->size = queue->size + 1;
    return request.connfd;
}
 
Request Q_Pop(Queue queue)
{
    if (Q_isEmpty(queue)){
        Request empty;
        empty.connfd = EMPTY;
        return empty;
    }
    Request request = queue->array[queue->head];
    queue->head = (queue->head + 1) % queue->max_size;
    queue->size = queue->size - 1;
    return request;
}

Request Q_PopVal(Queue queue, int connfd){
    if (Q_isEmpty(queue)){
        Request empty;
        empty.connfd = EMPTY;
        return empty;
    }
    int cur_size = queue->size;
    Request temp_request;
    for(int i=0; i<cur_size; i++){
        temp_request = Q_Pop(queue);
        if (temp_request.connfd != connfd)
            Q_Insert(queue, temp_request);
    }
    return temp_request;
}
 
Request Q_Head(Queue queue)
{
    if (Q_isEmpty(queue)){
        Request empty;
        empty.connfd = EMPTY;
        return empty;
    }
    return queue->array[queue->head];
}
 
Request Q_Tail(Queue queue)
{
    if (Q_isEmpty(queue)){
        Request empty;
        empty.connfd = EMPTY;
        return empty;
    }
    return queue->array[queue->tail];
}

Request Q_FindByIndex(Queue queue, int index){
    if(index >= queue->size){
        Request empty;
        empty.connfd = SEGFAULT;
        return empty;
    }
    return queue->array[(queue->head + index) % queue->max_size];
}

void Q_print(Queue queue){
    printf("(");
    for(int i=0; i<queue->size; i++){
        printf("%d ", queue->array[(queue->head + i) % queue->max_size].connfd);
    }
    printf(")\n");
}

#endif /* __QUEUE_H__ */
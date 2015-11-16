#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>   //for bool
#include "tfrc.h"

struct logEntry {
    struct data_t packet;
    uint64_t timeArrived;
};

typedef struct Queue
{
    struct logEntry *qBase;
    int front;
    int rear;
}QUEUE;

void initQueue(QUEUE *pq);
void enQueue(QUEUE *pq , struct logEntry value);
bool isemptyQueue(QUEUE *pq);
bool is_fullQueue(QUEUE *pq);
void deQueue(QUEUE *pq , struct logEntry *value);
void traverseQueue( QUEUE *pq);

/***********************************************
 *
 *     defination of the function
 *
 *     ************************************************/
int main()
{
    int val;
    QUEUE queue = {NULL,0,0} ;
    initQueue(&queue);
    enQueue(&queue,4);
    enQueue(&queue,5);
    enQueue(&queue,6);
    enQueue(&queue,7);
    enQueue(&queue,72);
    enQueue(&queue,42);


    traverseQueue(&queue);
    deQueue(&queue , &val);
    deQueue(&queue , &val);

    traverseQueue(&queue);
    enQueue(&queue,55);
    enQueue(&queue,65);
    traverseQueue(&queue);


    return 0;
}
/************************************
 *     init a empty queue
 *     ************************************/
void initQueue(QUEUE *pq)
{
    pq->qBase = (struct logEntry *)malloc(sizeof(struct logEntry));
    if(pq->qBase == NULL)
    {
        printf("??????!\n");
        exit(-1);
    }
    pq->front = pq->rear = 0;

}
/************************************************************
 *
 *     ???????
 *
 *         ?:???????????????,?????????
 *
 *         ************************************************************/

void enQueue(QUEUE *pq , struct logEntry value)
{

    if(is_fullQueue(pq))
        deQueue();

    pq->qBase[pq->rear] = value;
    pq->rear = (pq->rear + 1)%6 ;
    //printf("\n %d ?? \n" , value);

}

/*************************************
 *
 *     ??????,?????????
 *
 *         ?:??????????????
 *
 *         *************************************/

void deQueue(QUEUE *pq , struct logEntry *value)
{

    if(isemptyQueue(pq))
    {
        printf("??????!");
    }else
    {
        *value = pq->qBase[pq->front];
        //printf("\n %d ?? \n",*value);
        pq->front = (pq->front + 1)%6 ;

    }

}

/************************************
 *
 *     ??????????
 *
 *     ************************************/
bool isemptyQueue(QUEUE *pq)
{
    if(pq->front == pq->rear)
    {
        return true;
    }else
        return false;
}

/************************************
 *     ??????????
 *     ************************************/
bool is_fullQueue(QUEUE *pq)
{
    if((pq->rear +1)%6 == pq->front)
    {
        return true;
    }else
        return false;
}

/*************************************
 *
 *     ???????????
 *     *************************************/
void traverseQueue( QUEUE *pq)
{
    if(isemptyQueue(pq))
    {
        printf("??????!\n");
        exit(0);
    }
    printf("?????? :\n");
    printf("front?%d,rear?%d :\n",pq->front,pq->rear);


    int tail = pq->front ;
    while(tail != pq->rear)
    {
        //printf(" %d ",pq->qBase[tail]);
        tail = (tail + 1)%6;

    }
}

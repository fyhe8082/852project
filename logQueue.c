#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>   //for bool
#include "tfrc.h"
#include "tfrc-server.h"

//struct logEntry {
//    struct data_t *packet;
//    uint64_t timeArrived;
//};
//
//typedef struct Queue
//{
//    struct logEntry **qBase;
//    int front;
//    int rear;
//}QUEUE;

//void initQueue(QUEUE *pq);
//void enQueue(QUEUE *pq , struct logEntry *value);
//bool isemptyQueue(QUEUE *pq);
//bool is_fullQueue(QUEUE *pq);
//void deQueue(QUEUE *pq , struct logEntry *value);
//void traverseQueue( QUEUE *pq);

/************************************
 *     init a empty queue
 *     ************************************/
void initQueue(QUEUE *pq)
{
    pq->qBase = (struct logEntry **)malloc(sizeof(struct logEntry *));
    if(pq->qBase == NULL)
    {
        printf("can't init!\n");
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

void enQueue(QUEUE *pq , struct logEntry *value)
{

    if(is_fullQueue(pq))
        deQueue(pq, value);

    pq->qBase[pq->rear] = value;
    pq->rear = (pq->rear + 1)%MAXN ;
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
        printf("Is empty!");
    }else
    {
        value = pq->qBase[pq->front];
        //printf("\n %d ?? \n",*value);
        pq->front = (pq->front + 1)%MAXN ;

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
    if((pq->rear +1)%MAXN == pq->front)
    {
        return true;
    }else
        return false;
}


uint32_t getMax3SeqNum(QUEUE *pq)
{
    uint32_t m[3];
    int i = 0;
    for(i=0;i<3;i++)
    {
        m[i] = 0;
    }
    
    if(isemptyQueue(pq))
        return 0;

    int tail = pq->front;
    while(tail != pq->rear)
    {   
        if((pq->qBase[tail])->packet->seqNum > m[0])
            m[0] = pq->qBase[tail]->packet->seqNum;
        else if(pq->qBase[tail]->packet->seqNum > m[1])
            m[1] = pq->qBase[tail]->packet->seqNum;
        else if(pq->qBase[tail]->packet->seqNum > m[2])
            m[2] = pq->qBase[tail]->packet->seqNum;
        
        tail = (tail + 1)%MAXN;
    }
    return m[2];
}

int existSeqNum(QUEUE *pq, uint32_t seqNum)
{
    if(isemptyQueue(pq))
        return -1;

    int tail = pq->front;
    while(tail != pq->rear)
    {
        if(pq->qBase[tail]->packet->seqNum == seqNum)
            return tail;
        tail = (tail + 1)%MAXN ;
    }
    return -1;
}

int getIndexBefore(QUEUE *pq, uint32_t S_loss)
{
    int tail = pq->front;
    int index = tail;

    while(tail != pq->rear)
    {
        if(pq->qBase[tail]->packet->seqNum < S_loss 
                && pq->qBase[tail]->packet->seqNum 
                > pq->qBase[index]->packet->seqNum)
            index = tail;

        tail = (tail + 1)%MAXN ;
    }
}

int getIndexAfter(QUEUE *pq, uint32_t S_loss)
{
    int tail = pq->front;
    int index = tail;

    while(tail != pq->rear)
    {
        if(pq->qBase[tail]->packet->seqNum > S_loss 
                && pq->qBase[tail]->packet->seqNum 
                < pq->qBase[index]->packet->seqNum)
            index = tail;

        tail = (tail + 1)%MAXN;
    }
}

int getRecvCount(QUEUE *pq, uint64_t timeLine)
{
   int count = 0;
   int tail = pq->front;
   
   while(tail != pq->rear)
   {
       if (pq->qBase[tail]->timeArrived > timeLine)
           count++;
   } 
   return count;
}


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>   //for bool

/*struct for receive history*/
struct logEntry {
    struct data_t *packet;
    uint64_t timeArrived;
};

typedef struct Queue
{
    struct logEntry **qBase;
    int front;
    int rear;
}QUEUE;

/*the node defination of the loss record*/
typedef struct node{
    uint32_t seqNum;
    uint64_t timeArrived;
    bool isNewLoss;
    struct node * next;
}node_t;

/*External receive history log operate function*/
void initQueue(QUEUE *pq);
void enQueue(QUEUE *pq , struct logEntry *value);
bool isemptyQueue(QUEUE *pq);
bool is_fullQueue(QUEUE *pq);
void deQueue(QUEUE *pq , struct logEntry *value);
uint32_t getMax3SeqNum(QUEUE *pq)
int existSeqNum(QUEUE *pq, uint32_t seqNum)
int getIndexBefore(QUEUE *pq, uint32_t S_loss)
int getIndexAfter(QUEUE *pq, uint32_t S_loss)
int getRecvCount(QUEUE *pq, uint64_t timeLine)

/*External loss record operate function*/
int pop(node_t ** head); 
int remove_by_seqNum(node_t ** head, int seqNum); 
void append(node_t ** head, int seqNum, uint64_t timeArrived); 

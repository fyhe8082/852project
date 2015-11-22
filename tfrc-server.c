/*********************************************************
 *
 * Module Name: tfrc server 
 *
 * File Name:    tfrc-server.c	
 *
 * Summary:
 *  This file contains the echo server code
 *
 * Revisions:
 *  Created by Fangyu He for CPSC 8520, Fall 2015
 *   School of Computing,  Clemson University
 *
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include "tfrc.h"
#include "tfrc-server.h"

static struct control_t *buffer = NULL;
static struct control_t *ok = NULL;
static struct data_t *data = NULL;
static struct ACK_t *dataAck = NULL;
static uint32_t CxID;
static uint32_t lossRate = 0;
static uint32_t recvRate = 0;

/*var for output*/
static int countRecv = 0;
static int countRecvBytes = 0;
static int countAck = 0;
static double countDroped = 0;
static double accuLossrate = 0;
static uint32_t seqMax = 0;
static uint32_t seqMin = 0;

/*the newest RTT*/
static uint32_t RTT;
/*the struct for store the receive history*/
QUEUE *mylog;
struct logEntry *entry;

/*the struct for storing the loss packets*/
node_t *lossRecord = NULL;

struct timeval tv;

void sigHandler(int);
char Version[] = "1.1";

void sendOk(int sock, struct sockaddr_in *server, uint32_t seqNum, uint16_t msgSize);
void sendDataAck(int sock,struct sockaddr_in *server);
int isNewLoss(struct data_t *data);
void updateLoss();
uint64_t T_lossCompute(uint32_t S_loss);
float getWeight(int i, int I_num);
void compute();
uint64_t max(uint64_t i1, uint64_t i2);
void display();

/**
 *  bin     :   echo the BIN from client  
 *  seq     :   echo the seq from client  
 *  bsize   :   echo the Bisze from client
 *
 * */
void sendOk(int sock, struct sockaddr_in *server, uint32_t seqNum, uint16_t msgSize)
{
    
    ok->msgLength = CONT_LEN;
    ok->msgType = CONTROL;
    ok->code = OK;
    ok->CxID = CxID;
    ok->seqNum = seqNum;
    ok->msgSize = msgSize;
    /* start to send.. */
    if (sendto(sock, ok, sizeof(struct control_t), 0, (struct sockaddr *)server, sizeof(*server)) != sizeof(struct control_t))
    {
        DieWithError("sendto() sent a different number of bytes than expected");
    }

}

/**
 *  bin     :   echo the BIN from client  
 *  seq     :   echo the seq from client  
 *  bsize   :   echo the Bisze from client
 *
 * */
void sendDataAck(int sock,struct sockaddr_in *server)
{   
    dataAck->msgLength = ACK_LEN;
    dataAck->msgType = ACK;
    dataAck->code = OK;
    dataAck->CxID = CxID;
    dataAck->ackNum = lossRecord->seqNum + 1; 
    gettimeofday(&tv, NULL);
    dataAck->timeStamp = 1000000 * tv.tv_sec + tv.tv_usec;
    dataAck->T_delay = data->timeStamp - mylog->qBase[mylog->front]->packet->timeStamp;
    dataAck->lossRate = lossRate;
    //multi 1000 then take the floor for recvRate
    dataAck->recvRate = (uint32_t)(getRecvBits(mylog, (data->timeStamp - RTT))*1000000/RTT);
    /* start to send.. */
    if (sendto(sock, ok, sizeof(struct control_t), 0, (struct sockaddr *)server, sizeof(*server) ) != sizeof(struct control_t))
    {
        DieWithError("sendto() sent a different number of bytes than expected");
    }
    countAck++;
    accuLossrate += dataAck->recvRate;
}

int isNewLoss(struct data_t *data)
{   
    entry->packet = data;
    gettimeofday(&tv, NULL);
    entry->timeArrived = 1000000 * tv.tv_sec + tv.tv_usec; 
    enQueue(mylog, entry);
    /*if it is a packet used to be a loss one at receiver*/
    if (remove_by_seqNum(&lossRecord, data->seqNum)!=-1)
    {
        /*remove this loss in the loss record and recompute the lossrate*/
        countDroped--;
        compute();
        return 0;
    }else{
        /*else to figure if any new packet need to be looked as loss ones and recompute the lossrate*/
        updateLoss();
        compute();
    }
}

/*add any new loss to the lossRecord*/
void updateLoss ()
{
    uint64_t T_loss;

    //if (higher==3)        add to lossRecord;
    if (mylog->rear-3 <= mylog->front)
        exit(0);

    /*get the third biggest seqNum*/
    uint32_t num = getMax3SeqNum(mylog);
    uint32_t i = 1;
    int index;

    /*add all packets(not received) that have exactly 3 higher packet seqNum*/
    for (i=1;i<=MAXN-3;i++)
    {
        index = existSeqNum(mylog, num-i);
        if (index == -1)
        {
            T_loss = T_lossCompute(num-i);
            append(&lossRecord, num-i, T_loss);
            countDroped++;
        }
        else
            break;
    }
}

/*compute the T_loss*/
uint64_t T_lossCompute(uint32_t S_loss)
{
    uint64_t T_loss;

    int index_before = getIndexBefore(mylog, S_loss);
    int index_after = getIndexAfter(mylog, S_loss);

    uint32_t S_before = mylog->qBase[index_before]->packet->seqNum;
    uint32_t S_after = mylog->qBase[index_after]->packet->seqNum;
    uint64_t T_before = mylog->qBase[index_before]->timeArrived;
    uint64_t T_after = mylog->qBase[index_after]->timeArrived;

    T_loss = T_before + ((T_after - T_before) * (S_loss - S_before) / (S_after - S_before));

    return T_loss;
    
}

/*compute the weight of loss Interval*/
float getWeight(int i, int I_num)
{
    if(I_num > 8)
        I_num = 8;

    float w_i = 0;
    if(i < I_num/2)
        w_i = 1;
    else
        w_i = 1-(i-(I_num/2-1))/(I_num/2+1);
}

/*correct the start loss event mark and compute the lossrate*/
void compute()
{
   /*correct the start loss parket mark*/ 
   node_t * p = lossRecord;
   p->isNewLoss = true;
   uint64_t lossStart = p->timeArrived;
   uint64_t interval = 0;

   /*compute the number of loss event*/
   int I_count = 0;

   while(p->next != NULL)
   {
       interval = p->next->timeArrived - p->timeArrived;
       if(interval > RTT)
       {
           lossStart = p->next->timeArrived;
           p->next->isNewLoss = true;
           I_count++;
       }
       p=p->next;
   }

   /*contruct the array to store the Interval of loss event, the order is the reveal as the RFC 3448 description*/

   uint64_t *array;
   array = (uint64_t *)malloc(I_count*sizeof(uint64_t));
   int i = 0;
   uint64_t preTime = 0;
   p = lossRecord;

   while(p != NULL)
   {
       if (p->isNewLoss == true)
       {
           if (preTime != 0)
           {
               array[i] = p->timeArrived - preTime;
               i++;
           }
           preTime = p->timeArrived;
       }
       p=p->next;
   }

   gettimeofday(&tv, NULL);
   array[i] = (1000000 * tv.tv_sec + tv.tv_usec) - preTime;

   /*compute the loss event rate*/
   int n = I_count -1;
   uint64_t I_tot0 = 0;
   uint64_t I_tot1 = 0;
   uint64_t I_tot = 0;
   float I_mean = 0;
   float W_tot = 0;
   for (i=n;i>0;i--)
   {
       I_tot0 = I_tot0 + (array[n-i]*getWeight(i-1,n));
       W_tot = W_tot + getWeight(i,n);
   }
   for (i=n-1;i>=0;i--)
       I_tot1 = I_tot1 + (array[n-i]*getWeight(i,n));
   I_tot = max(I_tot0, I_tot1);
   I_mean = I_tot/W_tot;
   lossRate = (uint32_t)(1/I_mean)*1000;
}

uint64_t max(uint64_t i1, uint64_t i2)
{
    if (i1>i2)
        return i1;
    else
        return i2;
}

int main(int argc, char *argv[])
{
    /* server var */
    int sock;                    /* Socket */
    unsigned short servPort;     /* Server port */
    struct sockaddr_in servAddr; /* Local address */

    int bindFlag = 0;            /* server bind with any client or not*/
    uint8_t bindMsgSize;

    /* addr&port bind with */
    unsigned long bindIP;
    unsigned short bindPort;


    /* client var */
    struct sockaddr_in clntAddr; /* Client address */
    unsigned int cliAddrLen;     /* Length of incoming message */
    int recvMsgSize;             /* Size of received message */

    /* packets var */
    uint8_t msgType;
    uint8_t code;

    /*init the Queue used for receive history*/
    mylog = (QUEUE *)malloc(sizeof(QUEUE));
    initQueue(mylog);
    //mylog->qBase = (struct logEntry **)malloc(sizeof(struct logEntry *)*MAXN);
    //mylog->front = mylog->rear = 0;

    /*init loss Record*/
    lossRecord = (node_t *)malloc(sizeof(node_t));

    /* Check for correct number of parameters */ 
    if (argc >= 2)
    {
        servPort = atoi(argv[1]); /* local port */
        printf("%s",argv[1] );
    }
    else
    {
        fprintf(stderr,"Usage:  %s <TFRC SERVER PORT>\n", argv[0]);
        exit(1);
    }

    /* bind SIGINT function */
    signal(SIGINT, sigHandler);

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("Failure on socket call, errno:%d\n", errno);
    }

    /* Construct local address structure */
    memset(&servAddr, 0, sizeof(servAddr));     /* Zero out structure */
    servAddr.sin_family = AF_INET;              /* Internet address family */
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);/* Any incoming interface */
    servAddr.sin_port = htons(servPort);        /* Local port */

    /* create buffer to store packets. 1600 maximum of packet size */
    buffer = (struct control_t*)calloc((size_t)MAX_BUFFER, 1);
    ok = (struct control_t*)calloc((size_t)MAX_BUFFER, 1);
    dataAck = (struct ACK_t*)calloc((size_t)MAX_BUFFER, 1);


    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    {
        printf("Failure on bind, errno:%d\n", errno);
    }

    /* Forever Loop */
    for (;;) 
    {
        cliAddrLen = sizeof(clntAddr);
        printf(" success!!\n");
        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, buffer, sizeof(struct control_t), 0, (struct sockaddr *) &clntAddr, &cliAddrLen)) < 0)
        {
            printf("Failure on recvfrom, client: %s, errno:%d\n", inet_ntoa(clntAddr.sin_addr), errno);
            continue;
        }

        printf("received success!!\n");
        countRecv++;
        countRecvBytes += recvMsgSize;
        if(buffer->seqNum > seqMax)
            seqMax = buffer->seqNum;
        
        /* Parsing the packet */
        msgType = buffer -> msgType;
        code = buffer -> code;
        switch (msgType) 
        {
            case CONTROL : 
                {
                    switch (code)
                    {
                        case START :
                            {
                                if (bindFlag == 0)
                                {
                                    bindFlag = 1;
                                    //init the record var
                                    countRecv = 1;
                                    countRecvBytes = recvMsgSize;
                                    seqMax = buffer->seqNum;
                                    seqMin = buffer->seqNum;

                                    bindPort = clntAddr.sin_port;
                                    bindIP = clntAddr.sin_addr.s_addr;
                                    CxID = buffer->CxID;
                                    bindMsgSize = ntohl(buffer->msgSize);
                                    printf("start:\n");
                                    printf("length:%d\n", ntohs(buffer->msgLength));
                                    printf("type:%d\n", (int)buffer->msgType);
                                    printf("code:%d\n", (int)buffer->code);
                                    printf("Cxid:%d\n", ntohl(buffer->CxID));
                                    printf("Seq#:%d\n", ntohl(buffer->seqNum));
                                    printf("size:%d\n", ntohs(buffer->msgSize));
                                    sendOk(sock, &clntAddr, 
                                            buffer->seqNum,
                                            buffer->msgSize
                                          );
                                }
                                break;
                            }
                        case STOP :
                            {
                                if (bindFlag == 1 
                                        && bindIP == clntAddr.sin_addr.s_addr
                                        && bindPort == clntAddr.sin_port)
                                {
                                    printf("STOP msg received!\n\n");
                                    sendOk(sock, &clntAddr, 
                                            buffer->seqNum,
                                            buffer->msgSize
                                          );
                                    //display the output information
                                    display();
                                }
                                break;
                            }
                        default : break;
                    }

                    break;
                }
            case DATA :
                {
                    if (bindFlag == 1 
                            && bindIP == clntAddr.sin_addr.s_addr
                            && bindPort == clntAddr.sin_port)
                    {
                        printf("data recv\n");
                        
                        data = (struct data_t *)buffer;
                        RTT = data->RTT;
                        if(isNewLoss(data) == 1)
                        {
                            sendDataAck(sock, &clntAddr);
                        }
                        //now send each receive
                        else sendDataAck(sock, &clntAddr);
                    }

                    break;
                }

            default : break;
        }
    }
    close(sock);
    printf("\nServer terminates.\n\n");
    return 0;
}   

void display()
{
    printf("\nAmount of data received: %d packets and %d bytes\n", countRecv,countRecvBytes);
    printf("Number of ACKs sent: %d packets\n", countAck);
    printf("The total packet loss rate: %.3f\n",countDroped/(seqMax-seqMin+1));
    printf("Average of loss event rates sent to the send: %.3f\n", countAck==0 ? 0 : accuLossrate/1000/countAck);
}

void sigHandler(int sig)
{
    //printf("result\n");
    display();
    exit(1);
}

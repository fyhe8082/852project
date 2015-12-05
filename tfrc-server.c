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
#include <inttypes.h> // for print uint64
#include <sys/time.h>
#include <stdbool.h>
#include "tfrc.h"
#include "tfrc-server.h"

static struct control_t *buffer = NULL;
static struct control_t *ok = NULL;
static struct data_t *data = NULL;
static struct ACK_t *dataAck = NULL;
int sock;                    /* Socket */
struct sockaddr_in clntAddr; /* Client address */
static uint32_t CxID;
static uint32_t lossRate = 0;
static uint32_t preLossRate = 0;
int bindFlag = 0;            /* server bind with any client or not*/
//static uint32_t recvRate = 0;

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
void enQueueAndCheck(struct data_t *data);
void updateLoss();
uint64_t T_lossCompute(uint32_t S_loss);
float getWeight(int i, int I_num);
void compute();
uint64_t max(uint64_t i1, uint64_t i2);
void display();
void handle_alarm(int ignored);

/**
 *  bin     :   echo the BIN from client  
 *  seq     :   echo the seq from client  
 *  bsize   :   echo the Bisze from client
 *
 * */
void sendOk(int sock, struct sockaddr_in *server, uint32_t seqNum, uint16_t msgSize)
{

    ok->msgLength = htons(CONT_LEN);
    ok->msgType = CONTROL;
    ok->code = OK;
    ok->CxID = htonl(CxID);
    ok->seqNum = htonl(seqNum);
    ok->msgSize = htons(msgSize);
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
    dataAck->msgLength = htons(ACK_LEN);
    dataAck->msgType = ACK;
    dataAck->code = OK;
    dataAck->CxID = htonl(CxID);
    dataAck->ackNum = htonl(lossRecord->seqNum + 1); 
    gettimeofday(&tv, NULL);
    dataAck->timeStamp = 1000000 * tv.tv_sec + tv.tv_usec;
    dataAck->T_delay = htonl(data->timeStamp - mylog->qBase[mylog->front]->packet->timeStamp);
    dataAck->lossRate = htonl(lossRate);
    //multi 1000 then take the floor for recvRate
    if (RTT == 0){
        printf("\nRTT equals 0. exited\n");
        exit(0);
    }
    //printf("timeStamp recv%" PRIu64 "\n",data->timeStamp);
    dataAck->recvRate = htonl((uint32_t)(getRecvBits(mylog, (data->timeStamp - RTT))*1000000/RTT));
    //printf("start to send ack\n");
    /* start to send.. */
    if (sendto(sock, ok, sizeof(struct control_t), 0, (struct sockaddr *)server, sizeof(*server) ) != sizeof(struct control_t))
    {
        DieWithError("sendto() sent a different number of bytes than expected");
    }
    countAck++;
    accuLossrate += dataAck->recvRate;
}

void enQueueAndCheck(struct data_t *data)
{   
    entry->packet = data;
    gettimeofday(&tv, NULL);
    entry->timeArrived = 1000000 * tv.tv_sec + tv.tv_usec; 
    //printf("%" PRIu64 "\n",entry->timeArrived);
    //printf("%" PRIu64 "\n",tv.tv_sec);
    //printf("%" PRIu64 "\n",tv.tv_usec);
    enQueue(mylog, entry);
    /*if it is a packet used to be a loss one at receiver*/
    if (remove_by_seqNum(&lossRecord, data->seqNum)!=-1)
    {
        /*remove this loss in the loss record and recompute the lossrate*/
        countDroped--;
        compute();
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
        return;

    /*get the third biggest seqNum*/
    uint32_t num = getMax3SeqNum(mylog);
    uint32_t i = 1;
    int index;

    /*add all packets(not received) that have exactly 3 higher packet seqNum*/
    for (i=1;i<=(mylog->rear-mylog->front)-3;i++)
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

    if ((S_after - S_before)==0){
        printf("(S_after - S_before)==0 exited\n%d %d\n", S_after, S_before);
        exit(0);
    }
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

    return w_i;
}

/*correct the start loss event mark and compute the lossrate*/
void compute()
{
    /*correct the start loss parket mark*/ 
    node_t * p = lossRecord;
    p->isNewLoss = true;
    uint64_t lossStart;
    lossStart = p->timeArrived;
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
    unsigned short servPort;     /* Server port */
    struct sockaddr_in servAddr; /* Local address */

    uint8_t bindMsgSize;

    /* addr&port bind with */
    unsigned long bindIP;
    unsigned short bindPort;


    /* client var */
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

    /*init entry*/
    entry = (struct logEntry *)malloc(sizeof(struct logEntry));
    entry->packet = (struct data_t*)malloc(sizeof(struct data_t));

    /* Check for correct number of parameters */ 
    if (argc >= 2)
    {
        servPort = atoi(argv[1]); /* local port */
        //printf("%s",argv[1] );
    }
    else
    {
        fprintf(stderr,"Usage:  %s <TFRC SERVER PORT>\n", argv[0]);
        exit(1);
    }

    /* bind SIGINT function */
    signal(SIGINT, sigHandler);
    signal(SIGALRM, handle_alarm);

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
        //printf(" success!!\n");
        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, buffer, sizeof(struct control_t), 0, (struct sockaddr *) &clntAddr, &cliAddrLen)) < 0)
        {
            printf("Failure on recvfrom, client: %s, errno:%d\n", inet_ntoa(clntAddr.sin_addr), errno);
            continue;
        }

        buffer->msgLength = ntohs(buffer->msgLength);
        buffer->CxID = ntohl(buffer->CxID);
        buffer->seqNum = ntohl(buffer->seqNum);
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
                                printf("\nreceived START!!\n\n");
                                if (bindFlag == 0)
                                {
                                    alarm(1);
                                    bindFlag = 1;
                                    //init the record var
                                    countRecv = 1;
                                    countRecvBytes = recvMsgSize;
                                    seqMax = buffer->seqNum;
                                    seqMin = buffer->seqNum;

                                    bindPort = clntAddr.sin_port;
                                    bindIP = clntAddr.sin_addr.s_addr;
                                    CxID = buffer->CxID;
                                    bindMsgSize = ntohs(buffer->msgSize);
                                    printf("start packet:\n");
                                    printf("length:%d\n", buffer->msgLength);
                                    printf("type:%d\n", (int)buffer->msgType);
                                    printf("code:%d\n", (int)buffer->code);
                                    printf("Cxid:%d\n", buffer->CxID);
                                    printf("Seq#:%d\n", buffer->seqNum);
                                    printf("size:%d\n\n", buffer->msgSize);
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
                                    printf("\nSTOP msg received!\n\n");
                                    sendOk(sock, &clntAddr, 
                                            buffer->seqNum,
                                            buffer->msgSize
                                          );
                                    //display the output information
                                    display();
                                    bindFlag = 0;
                                }
                                break;
                            }
                        default : break;
                    }

                    break;
                }
            case DATA :
                {
                    printf("received DATA!!\n");
                    if (bindFlag == 1 
                            && bindIP == clntAddr.sin_addr.s_addr
                            && bindPort == clntAddr.sin_port)
                    {
                        data = (struct data_t *)buffer;
                        data->RTT = ntohl(data->RTT);
                       // printf("timeStamp recv%" PRIu64 "\n",data->timeStamp);
                        printf("timeStamp %lu, %d, %d, %d\n",data->timeStamp, data->code, ntohl(data->RTT), data->msgLength);
                       
						RTT = data->RTT;
                        printf("data %" PRIu32 " received\n", data->seqNum);
                        RTT = 1000000;//for test
                        preLossRate = lossRate;
                        enQueueAndCheck(data);
                        if(lossRate > preLossRate)
                        {
                            sendDataAck(sock, &clntAddr);
                            alarm(RTT/1000000);
                        }
                        //send each receive
                        //else sendDataAck(sock, &clntAddr);
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
    printf("%lf\n",accuLossrate);
}

void sigHandler(int sig)
{
    //printf("result\n");
    display();
    exit(1);
}

void handle_alarm(int ignored)
{
    //printf("enter alarm handler\n");
    if(bindFlag == 0)
        return;
    sendDataAck(sock, &clntAddr);
    alarm(RTT/1000000);
    return;
}

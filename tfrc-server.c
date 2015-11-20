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
#include "tfrc.h"

static struct control_t *buffer = NULL;
static struct control_t *ok = NULL;
static struct data_t *data = NULL;
static struct ACK_t *dataAck = NULL;
static uint32_t CxID;
static uint32_t lossRate;
static uint32_t recvRate;
/*the struct for store the receive history*/
struct QUEUE log;
struct logEntry entry;

/*the struct for storing the loss packets*/
struct node_t lossRecord;

struct timeval tv;

/*the function for receive history */
void initQueue(QUEUE *pq);
void enQueue(QUEUE *pq , struct logEntry value);
bool isemptyQueue(QUEUE *pq);
bool is_fullQueue(QUEUE *pq);
void deQueue(QUEUE *pq , struct logEntry *value);
void traverseQueue( QUEUE *pq);

void sigHandler(int);
char Version[] = "1.1";

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
    if (sendto(sock, ok, sizeof(struct control_t), 0, (struct sockaddr *)server, sizeof(*server) ) != sizeof(struct control_t))
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
    dataAck->ackNum = log->qBase[log->front]->packet->seqNum + 1; //need to fix
    gettimeofday(&tv, NULL);
    dataAck->timeStamp = 1000000 * tv.tv_sec + tv.tv_usec;
    dataAck->T_delay = T_delay;
    dataAck->lossRate = lossRate;
    dataAck->recvRate = recvRate;
    /* start to send.. */
    if (sendto(sock, ok, sizeof(struct control_t), 0, (struct sockaddr *)server, sizeof(*server) ) != sizeof(struct control_t))
    {
        DieWithError("sendto() sent a different number of bytes than expected");
    }
}

int isNewLoss(struct data_t data)
{   
    entry->packet = &data;
    gettimeofday(&tv, NULL);
    entry->timeArrived = 1000000 * tv.tv_sec + tv.tv_usec; 
    enQueue(log, entry);
    if (remove_by_seqNum(lossRecord, data->seqNum)!=-1)
    {
        compute();
        return 0;
    }else{
        updateLoss();
        compute();
    }
}

void updateLoss ()
{
    //if (higher==3)        add to lossRecord;
    if (log->rear-3 <= log->front)
        exit(0);

    /*get the third biggest seqNum*/
    int num = getMax3SeqNum()
    int i = 1;
    int index;

    /*add all packets(not received) that have exactly 3 higher packet seqNum*/
    for (i=1;i<=MAXN-3;i++)
    {
        index = existSeqNum(log, num-i);
        if (index == -1)
            push(lossRecord, num-i, log[index]->timeArrived)
        else
            break;
    }
}

void compute()
{
    
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
    int isNewLoss = 0;           /* flag for if new Loss event occur*/


    /* Check for correct number of parameters */ 
    if (argc >= 2)
    {
        servPort = atoi(argv[1]); /* local port */
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
    servAddr.sin_port = htonl(servPort);        /* Local port */

    /* create buffer to store packets. 1600 maximum of packet size */
    buffer = (struct control_t*)calloc((size_t)MAX_BUFFER, 1);


    /* Bind to the local address */
    if (bind(sock, (struct sockadr *) &servAddr, sizeof(servAddr)) < 0)
    {
        printf("Failure on bind, errno:%d\n", errno);
    }

    /* Forever Loop */
    for (;;) 
    {
        cliAddrLen = sizeof(clntAddr);

        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, buffer, sizeof(struct control_t) + 1600, 0, (struct sockaddr *) &clntAddr, &cliAddrLen)) < 0)
        {
            printf("Failure on recvfrom, client: %s, errno:%d\n", inet_ntoa(clntAddr.sin_addr), errno);
            continue;
        }

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
                                    bindPort = (struct sockaddr *)&clntAddr.sin_port;
                                    bindIP = (struct sockaddr *)&clntAddr.sin_addr.s_addr;
                                    CxID = buffer->CxID;
                                    bindMsgSize = ntohl(buffer->msgSize);
                                    sendOk(sock, &clntAddr, 
                                            buffer->seqNum,
                                            buffer->msgSize
                                          );
                                    printf("start:\n");
                                    printf("length:%d\n", ntohs(buffer->msgLength));
                                    printf("type:%d\n", (int)buffer->msgType);
                                    printf("code:%d\n", (int)buffer->code);
                                    printf("Cxid:%d\n", ntohl(buffer->CxID));
                                    printf("Seq#:%d\n", ntohl(buffer->seqNum));
                                    printf("size:%d\n", ntohs(buffer->msgSize));
                                }
                                break;
                            }
                        case STOP :
                            {
                                if (bindFlag == 1 
                                        && bindIP == (struct sockaddr *)&clntAddr.sin_addr.s_addr
                                        && bindPort == (struct sockaddr *)&clntAddr.sin_port)
                                {
                                    printf("STOP msg received!\n\n");
                                    sendOk(sock, &clntAddr, 
                                            buffer->seqNum,
                                            buffer->msgSize
                                          );
                                    //display();
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
                            && bindIP == (struct sockaddr *)&clntAddr.sin_addr.s_addr
                            && bindPort == (struct sockaddr *)&clntAddr.sin_port)
                    {
                        printf("data recv\n");
                        
                        data = (data_t *)buffer;
                        if(isNewLoss(data) == 1)
                        {

                            sendDataAck(sock, &clntAddr);
                        }
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

void sigHandler(int sig)
{
    printf("result\n");
    exit(1);
}

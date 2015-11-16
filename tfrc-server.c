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
    dataAck->ackNum = ackNum;
    dataAck->timeStamp = timeStamp;
    dataAck->T_delay = T_delay;
    dataAck->lossRate = lossRate;
    dataAck->recvRate = recvRate;
    /* start to send.. */
    if (sendto(sock, ok, sizeof(struct control_t), 0, (struct sockaddr *)server, sizeof(*server) ) != sizeof(struct control_t))
    {
        DieWithError("sendto() sent a different number of bytes than expected");
    }
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
                        if(isNewLoss == 1)
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

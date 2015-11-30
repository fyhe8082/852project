/*********************************************************
*
* Module Name: tfrc client/server header file
*
* File Name:    tfrc.h	
*
* Summary:
*  This file contains common stuff for the client and server
*
* Revisions:
*
*********************************************************/
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>     /* for memset() */
#include <netinet/in.h> /* for in_addr */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <unistd.h>     /* for close() */


/*largest amount of data on a socket read*/
#define MAX_BUFFER 65000
/* largest queue number */
#define MAXN 50 


/* Message type */
#define CONTROL (0x01)
#define DATA (0x02)
#define ACK (0x03)

/* Code */
#define START (0x01)
#define STOP (0x02)
#define OK (0x03)
#define REDO (0x04)

/* Message length */
#define MSGMAX 4096
#define CONT_LEN 14
#define ACK_LEN 32
#define DATA_HEADER_LEN 24

/* Flow control params */
#define N_MAX 8
/* Considered as Control Message */
struct control_t {
    uint16_t msgLength; /* Msg Length */
    uint8_t msgType;   /* Msg Type */
    uint8_t code;      /* Code */
    uint32_t CxID;      /* connection ID */
    uint32_t seqNum;    /* Sequence Number*/
    uint16_t msgSize;    /* Send Msg Size */
};   

/* Considered as Data Message Format */
struct data_t {
    uint16_t msgLength; /* Msg Length */
    uint8_t msgType;   /* Msg Type */
    uint8_t code;      /* Code */
    uint32_t CxID;      /* connection ID */
    uint32_t seqNum;    /* Sequence Number */
    uint64_t timeStamp; /* TimeStamp */
    uint32_t RTT;       /* Sender's RTT Estimate (in microsecond) */
    uint8_t*  X;      /* Holds Msg Data */
};

/* Considered as ACK Message Format */
struct ACK_t {
    uint16_t msgLength; /* Msg Length */
    uint8_t msgType;   /* Msg Type */
    uint8_t code;      /* Code */
    uint32_t CxID;      /* connection ID */
    uint32_t ackNum;    /* ACK Number */
    uint64_t timeStamp; /* TimeStamp */
    uint32_t T_delay;   /* T_delay (in microsecond) */
    uint32_t lossRate;  /* Loss Event Rate */
    uint32_t recvRate;  /* Receive Rate (in bits per second) */
};

void DieWithError(char *errorMessage);  /* External error handling function */


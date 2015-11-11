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
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

/*largest amount of data on a socket read*/
#define MAX_BUFFER 65000

#ifndef LINUX
#define INADDR_NONE 0xffffffff
#endif

/* Message type */
#define CONTROL (0x01)
#define DATA (0x02)
#define ACK (0x03)

/* Code */
#define START (0x01)
#define STOP (0x02)
#define OK (0x03)

/* Considered as Control Message */
struct control_t {
    uint32_t msgLength; /* Msg Length */
    uint16_t msgType;   /* Msg Type */
    uint16_t code;      /* Code */
    uint32_t CxID;      /* connection ID */
    uint32_t seqNum;    /* Sequence Number*/
    uint32_t msgSize    /* Send Msg Size */
}   

/* Considered as Data Message Format */
struct data_t {
    uint32_t msgLength; /* Msg Length */
    uint16_t msgType;   /* Msg Type */
    uint16_t code;      /* Code */
    uint32_t CxID;      /* connection ID */
    uint32_t seqNum;    /* Sequence Number */
    uint32_t timeStamp; /* TimeStamp */
    uint32_t RTT;       /* Sender's RTT Estimate (in microsecond) */
    uint8_t  X[0];      /* Holds Msg Data */
}

/* Considered as ACK Message Format */
struct ACK_t {
    uint32_t msgLength; /* Msg Length */
    uint16_t msgType;   /* Msg Type */
    uint16_t code;      /* Code */
    uint32_t CxID;      /* connection ID */
    uint32_t ackNum;    /* ACK Number */
    uint32_t timeStamp; /* TimeStamp */
    uint32_t T_delay;   /* T_delay (in microsecond) */
    uint32_t lossRate;  /* Loss Event Rate */
    uint32_t recvRate;  /* Receive Rate (in bits per second) */
}

void DieWithError(char *errorMessage);  /* External error handling function */

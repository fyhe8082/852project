/*************************************************************************
    > File Name: tfrc_client.h
    > Author: Xubin Zhuge
    > Mail: xzhuge@clemson.edu 
    > Created Time: Sat 7 Nov 2015 06:45:17 PM EST
 ************************************************************************/
#include "tfrc.h"
#include "clientresources.h"
#include <sys/time.h>
#include <signal.h>

#define CNTRLMSGSIZE 14
#define ACKMSGSIZE 36
#define DATAHEADERSIZE 24
#define MAXSEQ 10000
#define DATAMAX 1500
#define MAXINITTRY 10
#define MEG 1000000.0
#define TIMESTAMPWINDOW 10
#define t_mbi 64.0 // t_mbi  = 64 seconds in usec

#define true 1
#define false 0
typedef enum {
	CLIENT_START,
	CLIENT_SENDING,
	CLIENT_STOP,
} ClientStatus;

struct ClientPrms {
	/*********Socket Things***********/
	int sock;  /* socket descriptor */
	struct sockaddr_in servAddr; /* echo server address */
	struct sockaddr_in clientAddr; 
	uint32_t clientAddrLen;
	uint32_t servAddrLen;
	uint16_t servPort;       /* echo server port */
	uint16_t clientPort;
	char *servIP;            /* ip address of server */
	struct hostent *thehost; /* hostent from gethostbyname() */
	uint32_t connectionID;

	/********TRFC Params**************/
	uint32_t msgSize;        /* message size in bytes */
	float simulatedLossRate;
	double maxAllowedThroughput;
	double timebetnPackets; // packet scheduling time duration
	double X_calc;
	double X_trans;  // allowed transmit rate in bytes/s
	double X_bps;    // average tramsmit rate in bytes/s
	double X_recv;   // rate seen by receiver
	double t_RTO;    // tcp retransmission timeout value in secs, default = 4*R
	double tld;     //time since last doubled during slow-start
	double t_recvdata; // timestamp contained in ACK
	double t_delay;    // t_delay contained in ACK
	double t_now;
	float R;       // Round trip time in seconds
	double R_sample;
	float p;       // loss event rate
	uint16_t b;     // max number of packets acknowledged by a single TCP ACK, default=1
	
	int alarmtimeout;
	double sessionTime;
	struct sigaction displaytimer;
	double numDropped;
	double latestPktTimestamp;
	double lastAckreceived;
	uint32_t lossEventCounter;

	double numSent;
	double numReceived;
	uint32_t cntrlTimeout; // timeout value for control message, default = 10 sec
	uint32_t cntrlTimeoutCounter; //count at most 10 times for control msg
	double noFeedbackTimer; // start of no feedback interval
	int feedbackRecvd;
	uint32_t sequencenum;
	uint32_t expectedACK;
	double timestore[TIMESTAMPWINDOW]; // for storing last 10 Time stamps
};


/****** Global variables *****/
struct control_t control;
struct data_t data;
struct ACK_t ack;
struct ClientPrms tfrc_client;
ClientStatus cStatus;
int CNTCStop;

double usec1, usec2,usec3,usec4;

void initializeparams();

/*************************************************************************
    > File Name: tfrc_client.h
    > Author: Xubin Zhuge
    > Mail: xzhuge@clemson.edu 
    > Created Time: Sat 7 Nov 2015 06:45:17 PM EST
 ************************************************************************/
#include "tfrc.h"

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
	uint32_t msgSize;        /* message size in bytes */

	float simulatedLossRate;
	double maxAllowedThroughput;

};


/****** Global variables *****/
struct control_t control;
struct data_t data;
struct ACK_t ack;
struct ClientPrms tfrc_client;
ClientStatus cStatus;

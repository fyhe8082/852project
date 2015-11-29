/*************************************************************************
    > File Name: tfrc_client.c
    > Author: Xubin Zhuge
    > Mail: xzhuge@clemson.edu 
    > Created Time: Sun 07 Nov 2015 06:46:18 PM EST
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h> /* for in_addr */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h> /* for sockaddr_in, inet_addr() */
#include <netdb.h>
#include <signal.h>
#include "tfrc_client.h"

char startBuffer[CNTRLMSGSIZE+1];
char dataBuffer[DATAHEADERSIZE+DATAMAX];
char ackBuffer[ACKMSGSIZE+1];

void catchCntrlTimeout(int ignored) {
	tfrc_client.alarmtimeout = true;
	if(tfrc_client.cntrlTimeoutCounter++ > MAXINITTRY) 
		DieWithError(" Client: Server Initialize Failed, server not responding");
	alarm(0);
}
void CNTCCatch(int ignored) {
	sleep(1);
	cStatus = CLIENT_STOP;
	tfrc_client.feedbackRecvd = false;
}

void setupTcpConnection() {
	memset(&(tfrc_client.servAddr), 0, sizeof(tfrc_client.servAddr)); 
	tfrc_client.servAddr.sin_family = AF_INET;
	tfrc_client.servAddr.sin_addr.s_addr = inet_addr(tfrc_client.servIP);

	// resolve dotted decimal address
	if(tfrc_client.servAddr.sin_addr.s_addr == -1) {
		tfrc_client.thehost = gethostbyname(tfrc_client.servIP);
		tfrc_client.servAddr.sin_addr.s_addr = *((unsigned long *) tfrc_client.thehost->h_addr_list[0]);
	}
	
	tfrc_client.servAddr.sin_port = htons(tfrc_client.servPort);

	if((tfrc_client.sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");
}

void setupCntrlMsg(char* buffer, uint16_t msgSize) {
	struct control_t *startPtr = (struct control_t *) buffer;
	startPtr->msgLength = CNTRLMSGSIZE;
	startPtr->msgType = CONTROL;
	startPtr->code = START;
	startPtr->CxID = htonl(OK);
	startPtr->seqNum = htonl(rand()%MAXSEQ);  // limit the max to avoid overflow
	startPtr->msgSize = htons(msgSize);
}
void setupDataMsg() {

}

void setupAckMsg() {

}
int main(int argc, char *argv[]) {
    printf("client start!\n");
	struct sigaction initialtimer;

	if(argc != 7) {
		fprintf(stderr, "Usage: [<tfrc-client> <destinationAddress> <destinationPort> <messageSize> <connectionID> <simulatedLossRate> <maxAllowedThroughput>]\n");
		exit(1);
	}

	tfrc_client.servIP = argv[1];
	tfrc_client.servPort = atoi(argv[2]);
	tfrc_client.msgSize = atoi(argv[3]);
	tfrc_client.connectionID = atoi(argv[4]);
	tfrc_client.simulatedLossRate = atoi(argv[5]);
	tfrc_client.maxAllowedThroughput = atoi(argv[6]);

	//Sender Initialize Parameters
	initializeparams();
	setupTcpConnection();
	setupCntrlMsg(startBuffer, tfrc_client.msgSize);

	setupDataMsg();
	setupAckMsg();

	initialtimer.sa_handler = catchCntrlTimeout;
	if(sigfillset(&initialtimer.sa_mask) < 0)
		DieWithError("sigfillset() failed");
	
	initialtimer.sa_flags = 0;
	if(sigaction(SIGALRM, &initialtimer, 0) < 0)
		DieWithError("sigaction failed for sigalarm");

	// CTRL+C interrupt setup
	signal(SIGINT, CNTCCatch);

   tfrc_client.alarmtimeout = true;
   tfrc_client.feedbackRecvd = false;

	while(true) {
		switch(cStatus) {
		case CLIENT_START:
			if(tfrc_client.alarmtimeout) {
				printf("send control----\n");
				alarm(0);
				if(sendto(tfrc_client.sock, startBuffer, CNTRLMSGSIZE, 0, (struct sockaddr *)
						&(tfrc_client.servAddr), sizeof(tfrc_client.servAddr)) != CNTRLMSGSIZE) {
					printf("%ld\n", sizeof(tfrc_client.servAddr));
					DieWithError("sendto() send a different number of bytes than expected.\n");
				}
				tfrc_client.alarmtimeout = false;
				alarm(tfrc_client.cntrlTimeout); // start the timeout
			}
		case CLIENT_SENDING:

			break;
		case CLIENT_STOP:

			break;
		}
	}
}



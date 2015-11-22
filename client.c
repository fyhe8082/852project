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
#include "tfrc_client.h"


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

	if(tfrc_client.sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) < 0)
		DieWithError("socket() failed");
}

void setupCntrlMsg(uint16_t msgSize) {
	cntrl.msgLength = htons(CNTRLMSGSIZE);
	cntrl.msgType = CONTROL;
	cntrl.code = START;
	cntrl.CxID = htonl(OK);
	cntrl.seqNum = htonl(rand()%MAXSEQ);  // limit the max to avoid overflow
	cntrl.msgSize = htons(msgSize);
}
void setupDataMsg() {

}

void setupAckMsg() {

}
int main(int argc, char *argv[]) {


    printf("client start!\n");
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
	setupCntrlMsg(tfrc_client.msgSize);
	setupDataMsg();
	setupAckMsg();

	while(true) {
		switch(clientStatus) {
		case CLIENT_START:
			if(tfrc_client.alarmtimeout) {
				alarm(0);
				if(sendto(tfrc_client.sock, (char *)cntrl, CNTRLMSGSIZE, 0, (struct sockaddr *)
						&(tfrc_client.servAddr), sizeof(tfrc_client.servAddrLen) != CNTRLMSGSIZE))
					DieWithError("sendto() send a different number of bytes than expected");
				tfrc_client.alarmtimeout = false;
				alarm(tfrc_client.cntrlTimeout); // start the timeout
		case CLIENT_SENDING:


		case CLIENT_STOP:
		}
	}
}



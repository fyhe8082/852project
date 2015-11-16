/*************************************************************************
    > File Name: client.c
    > Author: Xubin Zhuge
    > Mail: xzhuge@clemson.edu 
    > Created Time: Sun 07 Nov 2015 06:46:18 PM EST
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tfrc_client.h"

int main(int argc, char *argv[]) {
	
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

	while(1) {
		switch(cStatus) {
		case CLIENT_START:
			
			break;
		case CLIENT_SENDING:

		break;
		case CLIENT_STOP:
			break;
		}
	}
}



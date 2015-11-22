/*************************************************************************
    > File Name: clientresources.c
    > Author: Xubin Zhuge
    > Mail: xzhuge@g.clemson.edu 
    > Created Time: Sat 21 Nov 2015 04:11:41 PM EST
 ************************************************************************/

#include<stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "tfrc_client.h"
#include <math.h>
#include <sys/time.h>

/* Returns time in seconds with precision down to micresecond */
double get_time() {
	struct timeval curTime;
	(void) gettimeofday(&curTime, (struct timezone *) NULL);
	return (((((double)curTime.tv_sec)*1000000.0) + (double) curTime.tv_usec)/1000000.0);
}


void initializeparams() {
	tfrc_client.cntrlTimeout = 10; // cntrl pkts timeout = 10 sec
	tfrc_client.cntrlTimeoutCounter = 0; 
	tfrc_client.X_trans = tfrc_client.msgSize;  //
	tfrc_client.tld = 0;
	tfrc_client.t_RTO = 2; // 2 secs 
	tfrc_client.b = 1;  // one ACK per packet
	tfrc_client.t_now = get_time();
	tfrc_client.R = 0;

}

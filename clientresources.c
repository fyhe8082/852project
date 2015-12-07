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
#include "clientresources.h"

/* Returns time in seconds with precision down to micresecond *
double get_time() {
	struct timeval curTime;
	(void) gettimeofday(&curTime, (struct timezone *) NULL);
	return (((((double)curTime.tv_sec)*1000000.0) + (double) curTime.tv_usec)/1000000.0);
}*/

uint64_t get_time() {
	struct timeval curTime;
	(void) gettimeofday(&curTime, (struct timezone *) NULL);
	return (((curTime.tv_sec)*1000000.0) + curTime.tv_usec);
}


void initializeparams() {
	tfrc_client.cntrlTimeout = 10; // cntrl pkts timeout = 10 sec
	tfrc_client.cntrlTimeoutCounter = 0;
	tfrc_client.lossEventCounter = 0;
	tfrc_client.X_trans = tfrc_client.msgSize;  //
	tfrc_client.tld = 0;
	tfrc_client.t_RTO = 2; // 2 secs 
	tfrc_client.b = 1;  // one ACK per packet
	tfrc_client.t_now = get_time();
	tfrc_client.R = DATAMAX; //Initial RTT: MSS

}

void newsendingrate()
{
    double fq;
    
    if(tfrc_client.p>0) // congestion avoidance phase
	{
		fq = (tfrc_client.R/MEG)*sqrt(2*tfrc_client.b*tfrc_client.p/3) + 
			(tfrc_client.t_RTO*(3*sqrt(3*tfrc_client.b*tfrc_client.p/8)*tfrc_client.p*(1+32*tfrc_client.p*tfrc_client.p)));	
			
        tfrc_client.X_calc = tfrc_client.msgSize *8.0 * MEG / fq; // X_calc should be in [ bps]
        tfrc_client.X_trans = fmax(fmin(fmin(tfrc_client.X_calc,2*tfrc_client.X_recv),tfrc_client.maxAllowedThroughput),tfrc_client.msgSize*8.0/t_mbi);
    }
    else
        if(tfrc_client.t_now-tfrc_client.tld >= tfrc_client.R) // initial slow start
        {
            tfrc_client.X_trans = fmax(fmin(fmin(2*tfrc_client.X_trans,2*tfrc_client.X_recv),tfrc_client.maxAllowedThroughput),tfrc_client.msgSize*8.0/tfrc_client.R/MEG);
            tfrc_client.tld = tfrc_client.t_now;
        }
}


//Converts 8 bytes from host byte order to network byte order
double htond(double host)
{
	static int is_big = -1;			//1 if Big Endian 0 if Little Endian
	double network;							//Holds network byte order representation
	double host1 = host;
	char *n = (char *)&network + sizeof(double)-1;
	char *h = (char *)&host1;
	unsigned int x = 1;
	char *y = (char *)&x;
	char z = 1;
	int i;

	if(is_big < 0)
	{
		if(*y == z)
		{
			is_big = 0;
		}
		else
		{
			is_big = 1;
		}
	}

	if(is_big)
	{
		return host;
	}

	for(i=0;i<sizeof(double);i++)
	{
		*n = *h;
		n--;
		h++;
	}
	return network;
}

//Converts 8 bytes from network byte order to host byte order
double ntohd(double network)
{
	static int is_big = -1;			//1 if Big Endian 0 if Little Endian
	double host;								//Holds host byte order representation
	char *n = (char *)&network + sizeof(double) -1;
	char *h = (char *)&host;
	unsigned int x = 1;
	char *y = (char *)&x;
	char z = 1;
	int i;

	if(is_big < 0)
	{
		if(*y == z)
		{
			is_big = 0;
		}
		else
		{
			is_big = 1;
		}
	}

	if(is_big)
	{
		return network;
	}

	for(i=0;i<sizeof(double);i++)
	{
		*h = *n;
		n--;
		h++;
	}
	return host;
}

/*Shifts loss interval values*/
void shift_s_values(int s[])
{
	int i;
	for(i = N_MAX - 1; i>0; i--)
	{
		s[i] = s[i-1];
	}
	s[0] = 0;
	return;
}

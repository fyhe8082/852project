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
#include <signal.h>
#include <string.h>
#include <sys/time.h>

static struct control_t *start = NULL;
static struct control_t *ok = NULL;
static struct data_t *data = NULL;
static struct ACK_t *dataAck = NULL;

char Version[] = "1.1";

int main(int argc, char *argv[])
{
   /* Check for correct number of parameters */ 
    if (argc > 2)
    {
        servPort = atoi(argv[1]); /* local port */
    }
    else
    {
        fprintf(stderr,"Usage:  %s <TFRC SERVER PORT>\n", argv[0]);
        exit(1);
    }
}

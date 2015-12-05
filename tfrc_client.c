/*************************************************************************
    > File Name: tfrc_client.c
    > Author: Xubin Zhuge
    > Mail: xzhuge@clemson.edu 
    > Created Time: Sun 07 Nov 2015 06:46:18 PM EST	
    > Description: Main program send datagram to server and a thread receive 
      data from server simultaneously.
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h> /* for in_addr */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h> /* for sockaddr_in, inet_addr() */
#include <netdb.h>
#include <unistd.h> 
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "tfrc_client.h"

#define PACKETDROP(p) ((double)rand()*1.0/RAND_MAX) < p ? 0 : 1  //  p=0 would be taken for no packet drop

sem_t lock;

char startBuffer[CNTRLMSGSIZE+1];
char dataBuffer[DATAHEADERSIZE+DATAMAX];
char ackBuffer[ACKMSGSIZE+1];

char startBufferR[CNTRLMSGSIZE+1];
char dataBufferR[DATAHEADERSIZE+DATAMAX];
char ackBufferR[ACKMSGSIZE+1];


void printruntime(int ignored)
{	
    printf("%-11.2f %-8.2f %-11.2f %-8.2f %-8.2f %-1.2f\n",tfrc_client.X_trans,tfrc_client.X_calc,tfrc_client.X_recv,tfrc_client.R/MEG,tfrc_client.t_RTO,tfrc_client.p);
    
    ualarm(500000,0); // reset for next 0.5 second
}

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

/***************** thread for receiving data*********************/ 
void *thread_receive()
{
    long receivedStrLen;
    tfrc_client.servAddrLen = sizeof(tfrc_client.servAddr);

	printf("%d", tfrc_client.servAddrLen);
    while(1)
    {
        switch(cStatus)
        {
        case CLIENT_START:
           // if((receivedStrLen = recvfrom(tfrc_client.sock, startBuffer, MSGMAX, 0,
                //                          (struct sockaddr *) &(tfrc_client.servAddr), &(tfrc_client.servAddrLen))) != CNTRLMSGSIZE){
			if((receivedStrLen = recvfrom(tfrc_client.sock, startBufferR, MSGMAX, 0,
                                         (struct sockaddr *) &(tfrc_client.servAddr), &(tfrc_client.servAddrLen))) < 0){
				printf(" Receive Error from Server at CLIENT_START !!\n");
			}
            else
            {
				struct control_t *startPtr = (struct control_t *) startBufferR;
                // check for correctness of the received ACK.
                printf("Control ACK: %d--%d\n", startPtr->msgType, startPtr->code);   
	
                if(startPtr->msgType == CONTROL && startPtr->code==OK) //  server responded
                {	
                    cStatus = CLIENT_SENDING; // state change
                    tfrc_client.sessionTime = get_time();
                    
                    tfrc_client.feedbackRecvd = true; // to start packet transfer
                    
                    usec1 = 0; // start the transmission timer

                    /*** Recurring results Display Alarm timer setup *****/

                    tfrc_client.displaytimer.sa_handler = printruntime;
                    if (sigfillset(&tfrc_client.displaytimer.sa_mask) < 0)
                        DieWithError("sigfillset() failed");

                    tfrc_client.displaytimer.sa_flags = 0;

                    if (sigaction(SIGALRM, &tfrc_client.displaytimer, 0) < 0)
                        DieWithError("sigaction failed for sigalarm");
                    ualarm(500000,0); //  start the alarm with 2 seconds
                    
               
                }
            }
            break;
        case CLIENT_SENDING:
            if((receivedStrLen = recvfrom(tfrc_client.sock, ackBufferR, MSGMAX, 0,
                                          (struct sockaddr *) &(tfrc_client.servAddr), &(tfrc_client.servAddrLen))) < 0)
                    printf(" Receive Error from Server at CLIENT_START !!\n");
            else
            {
				struct ACK_t *ackPtr = (struct ACK_t*) ackBufferR;

				tfrc_client.numReceived++;
			
				printf("----msgType: %d code: %d, ackNum: %d, timestamp: %lu ---\n", ackPtr->msgType, ackPtr->code, ntohl(ackPtr->ackNum), ackPtr->timeStamp);

                if(ackPtr->msgType == ACK && ackPtr->code == OK)
                {
				//	printf("enter if --------------\n");
                    sem_wait(&lock);
					printf("sfdggd\n");
                    tfrc_client.lastAckreceived = ntohl(ackPtr->ackNum); // assuming receiver responds to most recent ACK
                    
                    
                    if(tfrc_client.lastAckreceived >= tfrc_client.expectedACK){// delayed ack !!! so a new expectedACK
						tfrc_client.feedbackRecvd = true; 
						tfrc_client.expectedACK=tfrc_client.lastAckreceived+1;
						
					}
					else
					{
						
					}
                    sem_post(&lock);
						 
                    tfrc_client.t_now = get_time();

                   // tfrc_client.t_recvdata = tfrc_client.timestore[ntohl(ackPtr->/eqnumrecvd)%TIMESTAMPWINDOW];
                    tfrc_client.t_delay = (double)ntohl(ackPtr->T_delay); //  CHECK is t_delay in microseconds
                    tfrc_client.X_recv = (double)(ntohl(ackPtr->recvRate)/1000.0);
                    tfrc_client.p = (float)ntohl(ackPtr->lossRate)/1000.0; // server sets p in int
                    tfrc_client.R_sample = (tfrc_client.t_now-tfrc_client.t_recvdata-tfrc_client.t_delay) ; //  CHANGES :: twice then in theory
				
					tfrc_client.lossEventCounter +=tfrc_client.p;
						
						
                    if(tfrc_client.R == DATAMAX) {
						printf("First Feedback has been received!!\n");  
                        tfrc_client.R= tfrc_client.R_sample;
					}//  usually the case for the first feedback
                    else {
                        tfrc_client.R = 0.9 * tfrc_client.R + 0.1 * tfrc_client.R_sample; // averaging funtion
					//	printf("++++++++++++++++++++Other ack received!!\n");
					}
 
                    tfrc_client.t_RTO = fmax(4 * tfrc_client.R/MEG,2*tfrc_client.msgSize*8.0/tfrc_client.X_trans);

                    newsendingrate(); //calculate new sending rate
                    
                   // tfrc_client.t_RTO = fmax(4 * tfrc_client.R,2*tfrc_client.msgSize*8.0/tfrc_client.X_trans); //  recalculate
                   
					tfrc_client.noFeedbackTimer = fmax(4 * tfrc_client.R/MEG,2*tfrc_client.msgSize*8.0/tfrc_client.X_trans); // reset the timer

                    tfrc_client.timebetnPackets = tfrc_client.msgSize * 8.0 / tfrc_client.X_trans;
                    
                }
            }
            break;
        case CLIENT_STOP:
          /*  if((receivedStrLen = recvfrom(tfrc_client.sock, cntrl.controlmessage, MSGMAX, 0,
                                          (struct sockaddr *) &(tfrc_client.servAddr), &(tfrc_client.servAddrLen))) != CNTRLMSGSIZE)
                    printf(" Receive Error from Server from CLIENT_STOP !!\n");

            else
            {
                // check for correctness of the received ACK.
                if(*cntrl.msgType == CONTROL && *cntrl.msgCode==OK) //  server responded
                {
                    
                    tfrc_client.feedbackRecvd = true; // to start packet transfer

                    CNTCStop=true;
                    pthread_exit(NULL);

                    break;// break the while loop


                }
            }
	*/
		break;
        }
    }


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

int setupCntrlMsg(char* buffer, uint16_t msgSize, uint32_t cxid) {
	struct control_t *startPtr = (struct control_t *) buffer;
	startPtr->msgLength = CNTRLMSGSIZE;
	startPtr->msgType = CONTROL;
	startPtr->code = START;
	startPtr->CxID = htonl(cxid);
	startPtr->seqNum = htonl(rand()%MAXSEQ);  // limit the max to avoid overflow
	startPtr->msgSize = htons(msgSize);
	return ntohl(startPtr->seqNum);
}
void setupDataMsg(char* buffer, uint16_t msgSize, uint32_t seqnum, uint32_t cxid) {
	struct data_t *dataPtr = (struct data_t *) buffer;
	dataPtr->msgLength = htons(msgSize+DATA_HEADER_LEN);
	dataPtr->msgType = DATA;
	dataPtr->code = OK;
	dataPtr->CxID = htonl(cxid);
	dataPtr->seqNum = htonl(seqnum);  // limit the max to avoid overflow
	
	//dataPtr->timeStamp = ;
//	dataPtr->RTT ; //define in initialization
	dataPtr->X = calloc(msgSize, sizeof(char));
}

void setupAckMsg(char* buffer) {
	//struct ack_t *ackPtr = (struct ack_t *) buffer;
}
int main(int argc, char *argv[]) {
    printf("client start!\n");
	struct sigaction initialtimer;

	pthread_t *thread1;

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
	
	tfrc_client.sequencenum = setupCntrlMsg(startBuffer, tfrc_client.msgSize, tfrc_client.connectionID);
	tfrc_client.expectedACK = tfrc_client.sequencenum+1;

	setupDataMsg(dataBuffer, tfrc_client.msgSize, tfrc_client.sequencenum,tfrc_client.connectionID);
	setupAckMsg(ackBuffer);
 
	//Initialize the semaphore
    	sem_init(&lock, 0, 0);

	initialtimer.sa_handler = catchCntrlTimeout;
	if(sigfillset(&initialtimer.sa_mask) < 0)
		DieWithError("sigfillset() failed");
	
	initialtimer.sa_flags = 0;
	if(sigaction(SIGALRM, &initialtimer, 0) < 0)
		DieWithError("sigaction failed for sigalarm");

	// CTRL+C interrupt setup
	signal(SIGINT, CNTCCatch);

	// thread out the receive process..

    	thread1 = (pthread_t*) calloc(1,sizeof(pthread_t));

   	if(pthread_create(thread1,NULL,thread_receive,NULL))
       		DieWithError("receive thread creation error!");

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
			break;
		case CLIENT_SENDING:
				usec2 = get_time(); // returns double in seconds so times MEG

            	if((usec2>=tfrc_client.noFeedbackTimer) || (usec2-usec1 >= tfrc_client.timebetnPackets*MEG)) {
                
            	if(usec2-usec1>= tfrc_client.timebetnPackets*MEG) {	// ready to send
                
					//if(tfrc_client.expectedACK+9 == tfrc_client.sequencenum)
					//continue;
					sem_wait(&lock);
					struct data_t *dataPtr = (struct data_t*)dataBuffer;
					dataPtr->seqNum = htonl(++tfrc_client.sequencenum); // increments seqnum before attaching
					printf("seq: %d\n", tfrc_client.sequencenum);
				
					tfrc_client.latestPktTimestamp = get_time(); // when the packet is sent
					dataPtr->timeStamp = tfrc_client.latestPktTimestamp; //  time now in usec
					
					dataPtr->RTT = htonl(tfrc_client.R); //  add senders RTT estimate
					
					tfrc_client.timestore[tfrc_client.sequencenum%TIMESTAMPWINDOW] = ntohl(dataPtr->timeStamp);

                    if ( tfrc_client.feedbackRecvd == true) {
						tfrc_client.noFeedbackTimer = get_time() + tfrc_client.t_RTO; // reset the timer
						tfrc_client.feedbackRecvd = false;
					}
				

                    if(PACKETDROP(tfrc_client.simulatedLossRate)==1)
                    {
						uint16_t sendsize = ntohs(dataPtr->msgLength);
                        if (sendto(tfrc_client.sock, dataBuffer, sendsize, 0, (struct sockaddr *)
                           &(tfrc_client.servAddr), sizeof(tfrc_client.servAddr)) != sendsize){
							printf("failed");
						//	printf("%s", tfrc_client.servAddr);
							DieWithError("sendto() sent a different number of bytes than expected.");

						}
                    }
                    else 
                    { 
						tfrc_client.numDropped++;
					}
					tfrc_client.numSent++;
					usec1 = get_time();
					sem_post(&lock);
				}
				else if(usec2>=tfrc_client.noFeedbackTimer && tfrc_client.feedbackRecvd ==false) // no feed back timer interrupts
                {
					printf("no feed back timer interrupts\n");
					sem_wait(&lock);
					
                    if(tfrc_client.R>0.0) // if there has been feedback beforehand
                    {
                        if(tfrc_client.X_calc>=tfrc_client.X_recv*2)
                            tfrc_client.X_recv = fmax(tfrc_client.X_recv/2,tfrc_client.msgSize*8.0/(2*t_mbi));
                        else
                            tfrc_client.X_recv = tfrc_client.X_calc/4;

                        newsendingrate();
                    }
                    else
                    {
                        tfrc_client.X_trans = fmax(tfrc_client.X_trans/2,tfrc_client.msgSize*8.0/t_mbi);
                    }
                    
                    tfrc_client.sequencenum = tfrc_client.expectedACK-1; // look for the last ack received 

                    tfrc_client.timebetnPackets = tfrc_client.msgSize * 8.0 / tfrc_client.X_trans;
                    
                    tfrc_client.t_RTO = fmax(4 * tfrc_client.R,2*tfrc_client.msgSize*8.0/tfrc_client.X_trans);
                    tfrc_client.noFeedbackTimer = get_time()
						+ tfrc_client.t_RTO; // update the nofeedbacktimer
                    tfrc_client.feedbackRecvd = true;
                    sem_post(&lock);
                }
            }
 
			break;
		case CLIENT_STOP:
			tfrc_client.sessionTime = get_time() - tfrc_client.sessionTime;
            // send out a CLIENT_STOP packet

            usec4 = get_time();


            if(usec4-usec3  > tfrc_client.t_RTO)// || tfrc_client.sendSTOP == false) //  repeat the stop packet
            {
				struct control_t *cntrl = (struct control_t*)ackBuffer; 
                tfrc_client.feedbackRecvd =false;
                cntrl->code=STOP;
                cntrl->msgType=CONTROL;
                cntrl->seqNum = htonl(tfrc_client.sequencenum);

                if (sendto(tfrc_client.sock, ackBuffer, CNTRLMSGSIZE, 0, (struct sockaddr *)
                           &(tfrc_client.servAddr), sizeof(tfrc_client.servAddr)) != CNTRLMSGSIZE)
                    DieWithError("sendto() sent a different number of bytes than expected");


                usec3 = get_time();
                

                tfrc_client.sendSTOP = true;
                
                tfrc_client.avgThroughput = tfrc_client.numSent*tfrc_client.msgSize*8*1000000.0/tfrc_client.sessionTime;
                tfrc_client.avgLossEvents = tfrc_client.lossEventCounter/tfrc_client.numReceived;
                
               // printf("\n Total time of session: %g uSec \n Total Data Sent = %g Packets (%g Bytes)\n Total Acks Received = %g \n Total Average Throughput = %g \n Average Loss Event = %g \n Total Pkt Droppped (dropped rate) = %g (%g)\n",tfrc_client.sessionTime,tfrc_client.numSent,tfrc_client.numSent*tfrc_client.msgSize*8,tfrc_client.numReceived,tfrc_client.avgThroughput,tfrc_client.avgLossEvents,tfrc_client.numDropped,tfrc_client.numDropped/tfrc_client.numSent); 
               printf("Total time of session: %.2lf uSec\n", tfrc_client.sessionTime);
			   printf("Total amount of data sent: %.0lf Packets (%.0lf Bytes)\n", tfrc_client.numSent, tfrc_client.numSent*tfrc_client.msgSize*8);
			   printf("Total number of ACKs recvd: %.0lf\n", tfrc_client.numReceived);
			   printf("Total average throughput: %.2lf\n", tfrc_client.avgThroughput);
			   printf("Average loss event rates: %.0lf\n", tfrc_client.avgLossEvents);
			   printf("Total Pkt Dropped (dropped rate): %.0f (%.3f)\n", tfrc_client.numDropped, tfrc_client.numDropped/tfrc_client.numSent);
                exit(1) ; //  hard stop 
            }
            else if(tfrc_client.feedbackRecvd)
            {
                if(CNTCStop)
                    exit(1);

                break;
            }

		//	break;
		}
	}
	pthread_exit(NULL);
}



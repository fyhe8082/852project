#ifndef _CLIENTRESOURCES__H
#define _CLIENTRESOURCES__H

typedef struct client_t{
	char ip[INET6_ADDRSTRLEN];
	unsigned short port;
	int id;
	int seq;
	short int length;
} client;


void initializeparams();
void newsendingrate();
client* tcp_accept(int sock);
char *NET_get_ip(struct sockaddr_in *addr);
unsigned short NET_get_port(struct sockaddr_in *addr);
double htond(double host);
double ntohd(double network);
void pack_ack(char buf[], int id, int seq, double drop, double rx, int seq_recvd);
void shift_s_values(int s[]);
double calc_s_hat(int s[]);
double get_time();

#endif

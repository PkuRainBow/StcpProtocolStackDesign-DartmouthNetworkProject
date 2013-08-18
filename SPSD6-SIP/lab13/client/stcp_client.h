#ifndef STCPCLIENT_H
#define STCPCLIENT_H
#include <pthread.h>
#include "../common/seg.h"

#define	CLOSED 1
#define	SYNSENT 2
#define	CONNECTED 3
#define	FINWAIT 4
#define TCB_MAX 20

typedef struct segBuf {
        seg_t seg;
        struct timeval sentTime;
        struct segBuf* next;
} segBuf_t;

typedef struct client_tcb {
	unsigned int server_nodeID;    
	unsigned int server_portNum;   
	unsigned int client_nodeID;     
	unsigned int client_portNum;   
	unsigned int state;     
	unsigned int next_seqNum;     
	pthread_mutex_t* bufMutex;   
	segBuf_t* sendBufHead;     
	segBuf_t* sendBufunSent;      
	segBuf_t* sendBufTail;      
	unsigned int unAck_segNum;   
} client_tcb_t;

typedef struct tcb_node{
	client_tcb_t* tcb;
	//0 is false # 1 is true
	char tag;
} client_tcb_node;

void stcp_client_init(int conn);
int stcp_client_sock(unsigned int client_port);
int stcp_client_connect(int socked, int nodeID, unsigned int server_port);
int stcp_client_send(int sockfd, void* data, unsigned int length);
int stcp_client_disconnect(int sockfd);
int stcp_client_close(int sockfd);
void *seghandler(void* arg);
void* sendBuf_timer(void* clienttcb);
#endif

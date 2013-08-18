#ifndef STCPSERVER_H
#define STCPSERVER_H

#include <pthread.h>
#include "../common/seg.h"
#include "../common/constants.h"

#define	CLOSED 1
#define	LISTENING 2
#define	CONNECTED 3
#define	CLOSEWAIT 4

typedef struct server_tcb {
	unsigned int server_nodeID;        //服务器节点ID, 类似IP地址
	unsigned int server_portNum;       //服务器端口号
	unsigned int client_nodeID;     //客户端节点ID, 类似IP地址
	unsigned int client_portNum;    //客户端端口号
	unsigned int state;         	//服务器状态
	unsigned int expect_seqNum;     //服务器期待的数据序号	
	char* recvBuf;                  //指向接收缓冲区的指针
	unsigned int  usedBufLen;       //接收缓冲区中已接收数据的大小
	pthread_mutex_t* bufMutex;      //指向一个互斥量的指针, 该互斥量用于对接收缓冲区的访问
} server_tcb_t;

typedef struct tcb_node{
	server_tcb_t* tcb;
	//0 is false # 1 is true
	char tag;
} server_tcb_node;


void stcp_server_init(int conn);

int stcp_server_sock(unsigned int server_port);

int stcp_server_accept(int sockfd);

int stcp_server_recv(int sockfd, void* buf, unsigned int length);

int stcp_server_close(int sockfd);

void* seghandler(void* arg);
#endif

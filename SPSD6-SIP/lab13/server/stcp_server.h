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
	unsigned int server_nodeID;        //�������ڵ�ID, ����IP��ַ
	unsigned int server_portNum;       //�������˿ں�
	unsigned int client_nodeID;     //�ͻ��˽ڵ�ID, ����IP��ַ
	unsigned int client_portNum;    //�ͻ��˶˿ں�
	unsigned int state;         	//������״̬
	unsigned int expect_seqNum;     //�������ڴ����������	
	char* recvBuf;                  //ָ����ջ�������ָ��
	unsigned int  usedBufLen;       //���ջ��������ѽ������ݵĴ�С
	pthread_mutex_t* bufMutex;      //ָ��һ����������ָ��, �û��������ڶԽ��ջ������ķ���
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

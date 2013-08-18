//文件名: sip/sip.c
//
//描述: 这个文件实现SIP进程  
//
//创建日期: 2013年1月

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "../topology/topology.h"
#include "sip.h"

#define Server_IP "127.0.0.1"
/**************************************************************/
//声明全局变量
/**************************************************************/
int son_conn; 		//到重叠网络的连接

/**************************************************************/
//实现SIP的函数
/**************************************************************/

//SIP进程使用这个函数连接到本地SON进程的端口SON_PORT
//成功时返回连接描述符, 否则返回-1
int connectToSON() { 
	int sockfd;
	struct sockaddr_in servaddr;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Problem in creating the socket ...\n");
		exit(2);
	}
	//in_addr_t hostaddr = get
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(Server_IP);
	servaddr.sin_port = htons(SON_PORT);
	
	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		perror("Problem in connecting to the server ...\n");
		exit(3);
	}	
	printf("connect to the local server successfully ... \n");
	return sockfd;	
}

void* routeupdate_daemon(void* arg) {
			int hostID = topology_getMyNodeID();
			sip_pkt_t temp;
	 	 	temp.header.src_nodeID = hostID;
	   	temp.header.dest_nodeID = BROADCAST_NODEID;
	    temp.header.length = 0;
	    temp.header.type = ROUTE_UPDATE;
	    memset(temp.data, 0, MAX_PKT_LEN);
			while(1){			
				son_sendpkt(BROADCAST_NODEID, &temp, son_conn);
				sleep(5);
			}
}

//这个线程处理来自SON进程的进入报文
//它通过调用son_recvpkt()接收来自SON进程的报文
//在本实验中, 这个线程在接收到报文后, 只是显示报文已接收到的信息, 并不处理报文 
void* pkthandler(void* arg) {
	sip_pkt_t pkt;
	while(son_recvpkt(&pkt,son_conn)>0) {
		printf("************** Route Update Info **********\n");
		printf("Routing srcnode_ID: %d\n", pkt.header.src_nodeID);
	  printf("Routing destnode_ID: %d\n", pkt.header.dest_nodeID);
	  printf("Routing type : %d\n\n", pkt.header.type);
	}
	close(son_conn);
	son_conn = -1;
	pthread_exit(NULL);
}

//这个函数终止SIP进程, 当SIP进程收到信号SIGINT时会调用这个函数. 
//它关闭所有连接, 释放所有动态分配的内存.
void sip_stop() {
	close(son_conn);
	exit(-1);	
}

int main(int argc, char *argv[]) {
	printf("SIP layer is starting, please wait...\n");

	//初始化全局变量
	son_conn = -1;

	//注册用于终止进程的信号句柄
	signal(SIGINT, sip_stop);

	//连接到重叠网络层SON
	son_conn = connectToSON();
	if(son_conn<0) {
		printf("can't connect to SON process\n");
		exit(1);		
	}
	//getchar();
	//启动线程处理来自SON进程的进入报文
	pthread_t pkt_handler_thread; 
	pthread_create(&pkt_handler_thread,NULL,pkthandler,(void*)0);

	//启动路由更新线程 
	pthread_t routeupdate_thread;
	pthread_create(&routeupdate_thread,NULL,routeupdate_daemon,(void*)0);	

	printf("SIP layer is started...\n");

	//永久睡眠
	while(1) {
		sleep(60);
	}
}



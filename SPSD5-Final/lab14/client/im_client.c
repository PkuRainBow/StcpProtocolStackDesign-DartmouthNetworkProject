#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "../common/constants.h"
#include "../topology/topology.h"
#include "stcp_client.h"

//创建一个连接, 使用客户端端口号87和服务器端口号88. 
#define CLIENTPORT 87
#define SERVERPORT 103

//在连接到SIP进程后, 等待1秒, 让服务器启动.
#define STARTDELAY 1
//在发送文件后, 等待5秒, 然后关闭连接.
#define WAITTIME 15
#define MAXLINE 200 //==max_seg_t data len

//这个函数连接到本地SIP进程的端口SIP_PORT. 如果TCP连接失败, 返回-1. 连接成功, 返回TCP套接字描述符, STCP将使用该描述符发送段.
int connectToSIP() {
	int sockfd;
	struct sockaddr_in servaddr;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Problem in creating the socket ...\n");
		exit(2);
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(SIP_PORT);
	
	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		perror("Problem in connecting to the server ...\n");
		exit(3);
	}
	return sockfd;
}

//这个函数断开到本地SIP进程的TCP连接. 
void disconnectToSIP(int sip_conn) {
	close(sip_conn);
}

//im function call ...
void *Recv(void *arg)
{
    int sockfd = (int)arg;
    char recvline[MAXLINE];
    memset(recvline, 0, MAXLINE);
    while(recv(sockfd, recvline, MAXLINE, 0) > 0){
		printf("%s", recvline);
		fflush(stdout);
        memset(recvline, 0, MAXLINE);
    }
}

int main() {
	//用于丢包率的随机数种子
	srand(time(NULL));

	//连接到SIP进程并获得TCP套接字描述符
	pthread_t tid;
	char sendline[MAXLINE];
	memset(sendline, 0, MAXLINE);
	
	int sip_conn = connectToSIP();
	if(sip_conn<0) {
		printf("fail to connect to the local SIP process\n");
		exit(1);
	}

	//初始化stcp客户端
	stcp_client_init(sip_conn);
	sleep(STARTDELAY);

	char hostname[50];
	/*
	printf("Enter server name to connect:");
	scanf("%s",hostname);
	int server_nodeID = topology_getNodeIDfromname(hostname);
	if(server_nodeID == -1) {
		printf("host name error!\n");
		exit(1);
	} else {
		printf("connecting to node %d\n",server_nodeID);
	}
	*/
	int server_nodeID = 188;

	//在端口87上创建STCP客户端套接字, 并连接到STCP服务器端口88.
	int sockfd = stcp_client_sock(CLIENTPORT);
	if(sockfd<0) {
		printf("fail to create stcp client sock");
		exit(1);
	}
	
	/*
     * fix a bug...
	 */
	signal(SIGPIPE, SIG_IGN);
	
	if(stcp_client_connect(sockfd, server_nodeID, SERVERPORT)<0) {
		printf("fail to connect to stcp server\n");
		exit(1);
	}
	printf("client connected to server, client port:%d, server port %d\n",CLIENTPORT,SERVERPORT);
	
	system("clear");
	printf("\nWelcome to the YYHCHATING HOME!\n");
	printf("***********************************************\n");
	printf("*               INSTRUCTOR INFO               *\n");
	printf("***********************************************\n");
	printf("LOGON:username:passwd\n");
	printf("LOGOFF\n");
	printf("SHOW\n");
	printf("ALL:******\n");
	printf("TO:username:*******\n");
	printf("***********************************************\n");
	printf("***********************************************\n");
	
	pthread_create(&tid, NULL, Recv, (void *)sockfd);
	while(fgets(sendline, MAXLINE, stdin) != NULL){
		stcp_client_send(sockfd, sendline, MAXLINE);
		memset(sendline, 0, MAXLINE);
	}
	/*
	if(stcp_client_disconnect(sockfd)<0) {
		printf("fail to disconnect from stcp server\n");
		exit(1);
	}	
	if(stcp_client_close(sockfd)<0) {
		printf("fail to close stcp client\n");
		exit(1);
	}
	*/
	//断开与SIP进程之间的连接
	//disconnectToSIP(sip_conn);
	//exit(0);
}

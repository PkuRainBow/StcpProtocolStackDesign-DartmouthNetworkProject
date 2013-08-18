#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "../common/constants.h"
#include "stcp_server.h"

#define CLIENTPORT1 87
#define SERVERPORT1 88
#define CLIENTPORT2 89
#define SERVERPORT2 90

#define WAITTIME 5

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

void disconnectToSIP(int sip_conn) {
	close(sip_conn);
}

int main() {
	srand(time(NULL));
	
	int sip_conn = connectToSIP();
	if(sip_conn<0) {
		printf("can not connect to the local SIP process\n");
	}

	stcp_server_init(sip_conn);

	int sockfd= stcp_server_sock(SERVERPORT1);
	if(sockfd<0) {
		printf("can't create stcp server\n");
		exit(1);
	}

	stcp_server_accept(sockfd);

	int sockfd2= stcp_server_sock(SERVERPORT2);
	if(sockfd2<0) {
		printf("can't create stcp server\n");
		exit(1);
	}

	stcp_server_accept(sockfd2);
	
	int i;
	/*
	 * both communication
	 */ 
	 /* 
	char buf1[6];
	char buf2[7];
	for(i=0;i<5;i++) {
		stcp_server_recv(sockfd,buf1,6);
		printf("recv string: %s from connection 1\n",buf1);
	}
	for(i=0;i<5;i++) {
		stcp_server_recv(sockfd2,buf2,7);
		printf("recv string: %s from connection 2\n",buf2);
	}

  sleep(WAITTIME);
  */
  char mydata[6] = "hello";

	for(i=0;i<5;i++){
      	stcp_server_send(sockfd, mydata, 6);
		    printf("send string:%s to connection 1\n",mydata);
  }
  char mydata2[7] = "byebye";
	for(i=0;i<5;i++){
      	stcp_server_send(sockfd2, mydata2, 7);
		    printf("send string:%s to connection 2\n",mydata2);
  }
    
  sleep(WAITTIME);
	

    
	if(stcp_server_close(sockfd)<0) {
		printf("can't destroy stcp server\n");
		exit(1);
	}				
	if(stcp_server_close(sockfd2)<0) {
		printf("can't destroy stcp server\n");
		exit(1);
	}				

	disconnectToSIP(sip_conn);
}

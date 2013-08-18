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

#define WAITTIME 20

int son_start() {
	socklen_t clilen;
	int listenfd, connfd;
	struct sockaddr_in cliaddr, servaddr;
		
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SON_PORT);
	
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(listenfd, 10);
	
	printf("TCP: Server running ... wait for connections ... \n");
	
	clilen = sizeof(cliaddr);
	connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
	printf("TCP: Received request ... \n");	
		
	return connfd;	
}

void son_stop(int son_conn) {
	close(son_conn);
}
int main() {
	srand(time(NULL));

	int son_conn = son_start();
	if(son_conn<0) {
		printf("can not start overlay network\n");
	}

	stcp_server_init(son_conn);

	int sockfd= stcp_server_sock(SERVERPORT1);
	if(sockfd<0) {
		printf("can't create stcp server\n");
		exit(1);
	}

	stcp_server_accept(sockfd);

	int fileLen;
	stcp_server_recv(sockfd,&fileLen,sizeof(int));
	char* buf = (char*) malloc(fileLen);
	stcp_server_recv(sockfd,buf,fileLen);

	FILE* f;
	f = fopen("receivedtext.txt","a");
	fwrite(buf,fileLen,1,f);
	fclose(f);
	free(buf);

	sleep(WAITTIME);

	if(stcp_server_close(sockfd)<0) {
		printf("can't destroy stcp server\n");
		exit(1);
	}				

	son_stop(son_conn);
}

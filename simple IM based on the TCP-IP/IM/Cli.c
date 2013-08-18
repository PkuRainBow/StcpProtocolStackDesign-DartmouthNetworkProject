#include "Protocol.h"


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

int
main(int argc, char** argv)
{
 int sockfd;
 struct sockaddr_in servaddr;
 pthread_t tid;
 char sendline[MAXLINE], recvline[MAXLINE];
 memset(sendline, 0, MAXLINE);
 memset(recvline, 0, MAXLINE);   
 if(argc != 2){
    perror("Usage: TCPclient <IP address of the server>");
    exit(1);
 }


 if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("Problem in creating the socket ");
    exit(2);
 }
 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr = inet_addr(argv[1]);
 servaddr.sin_port = htons(SERV_PORT);
 
 if(connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
  perror("Problem in connecting to the server ");
  exit(3);
 }
 
 //using info
 system("clear");
 printf("Welcome to the YYHCHATING HOME!\n");
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
    send(sockfd,sendline,strlen(sendline),0);
    memset(sendline, 0, MAXLINE);
 }
 exit(0);
}


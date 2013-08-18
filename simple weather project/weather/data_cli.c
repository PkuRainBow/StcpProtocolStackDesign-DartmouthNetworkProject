#include "tcp.h"

int
main(int argc, char** argv)
{
 int sockfd;
 struct sockaddr_in servaddr;

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

 system("clear");
 printf("Welcome to Wether Forecast Demo Program !\n");
 printf("Please input city name in chinese pinyin <e.g. nanjing or beijing>\n");
 printf("<q> cls, <#> exit\n");

 city(sockfd);
 query(sockfd);
 
}


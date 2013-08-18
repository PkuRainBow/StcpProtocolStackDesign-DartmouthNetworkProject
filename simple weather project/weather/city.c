#include"tcp.h"

int city(int sockfd)
{ 
  char sendline[MAXLINE], recvline[MAXLINE], buf[MAXLINE];
  memset(sendline, 0, MAXLINE);
  memset(recvline, 0, MAXLINE);
  memset(buf,0, MAXLINE);

  while(fgets(buf, MAXLINE, stdin) != NULL){  
      if(strncmp(buf,"q",1)==0)  {
              system("clear");
              printf("Welcome to Wether Forecast Demo Program !\n");
              printf("Please input city name in chinese pinyin <e.g. nanjing or beijing>\n");
              printf("<q> cls, <#> exit\n");
              //printf("%s  %d",buf,strlen(buf));
              continue;   
      }
      else if(strncmp(buf,"#",1)==0)  exit(0);
      else{
        //fill in the data
        //printf("%s  %d",buf,strlen(buf));
        sendline[0]=0;
        sendline[1]=0; 
        int i;
        for(i=0; i<strlen(buf)-1; i++)  sendline[i+2]=buf[i];
        //send the data  
        send(sockfd, sendline, 33, 0);
        if(recv(sockfd, recvline, MAXLINE, 0) == 0){
           perror("The server terminated permaturely");
           exit(4);
        }

       if(recvline[0]=='B')  {
              printf("sorry, Server does not have weather info for the input city!\n");
              printf("Welcome to Wether Forecast Demo Program !\n");
              printf("Please input city name in chinese pinyin <e.g. nanjing or beijing>\n");
              printf("<q> cls, <#> exit\n");
              continue;//City Wrong!
       }

       if(recvline[0]=='A') {
            system("clear");
            printf("Please enter the given number to query\n");
            printf("1. today\n");
            printf("2. three days from today\n");
            printf("3. custom day by your self\n");
            printf("<r> back, <q> cls, <#> exit\n");
            printf("===========================================\n");
            memset(cname, 0, 35);
            strcpy(cname,sendline+2);//store the cname
            break;   //City OK!
       }
      }
       memset(sendline, 0, MAXLINE);
       memset(recvline, 0, MAXLINE);
       memset(buf, 0, MAXLINE);
   }
   return 1;
}

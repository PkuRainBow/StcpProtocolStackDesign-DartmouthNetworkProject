#include"tcp.h"

int 
query(int sockfd)
{
  char sendline[MAXLINE], recvline[MAXLINE], buf[MAXLINE];
  memset(sendline, 0, MAXLINE);
  memset(recvline, 0, MAXLINE);
  memset(buf, 0, MAXLINE);
  short year;
  int i;
  //printf("111\n");
  while(fgets(buf, MAXLINE, stdin) != NULL){//out while
      
      if(strncmp(buf,"q",1)==0)  {
            system("clear");
            printf("Please enter the given number to query\n");
            printf("1. today\n");
            printf("2. three days from today\n");
            printf("3. custom day by your self\n");
            printf("<r> back, <q> cls, <#> exit\n");
            printf("===========================================\n");
            continue;    
      }
      else if(strncmp(buf,"#",1)==0) exit(0);
      else if(strncmp(buf,"r",1)==0) {
              system("clear");
              printf("Welcome to Wether Forecast Demo Program !\n");
              printf("Please input city name in chinese pinyin <e.g. nanjing or beijing>\n");
              printf("<q> cls, <#> exit\n");
              //
              city(sockfd);
       }

    //this is hard for me to think ! beautiful...
    //short year = ntohs(*( (short*)recvline[32] ));    
    else if(strncmp(buf,"1",1)==0) {
      sendline[0]=1;
      sendline[1]='A';
      for(i=2; i<30; i++)  sendline[i] = cname[i-2];      
      send(sockfd, sendline, 33, 0);     
      if(recv(sockfd, recvline, MAXLINE, 0) == 0){
          perror("The server terminated permaturely");
          exit(4);
       }
       year = ntohs(*( (short*)(recvline+32)));
       //printf("year   %02x\n",recvline[32]);
       printf("City: %s",cname);       
       printf("  Today is: %d - %d - %d ", year, recvline[34], recvline[35]);
       printf("  Weather info is as follows:\n");
       printf("Today's weather is: ");
       switch(recvline[37]){
                  case 0: printf(" shower ");  break;
                  case 1: printf(" clear ");  break;
                  case 2: printf(" cloudy ");  break;
                  case 3: printf(" rain ");  break;
                  case 4: printf(" fog ");  break;
                  default:  printf("You are kidding! NO such weather! ");  break;
       }
       printf("Wind-level: %d Temp: %d \n",recvline[38], recvline[39]);
    }//case 1

    else if(strncmp(buf,"2",1)==0) {
      sendline[0]=1;
      sendline[1]='B';
      for(i=2; i<30; i++)  sendline[i] = cname[i-2];     
      
      send(sockfd, sendline, 33, 0);
      if(recv(sockfd, recvline, MAXLINE, 0) == 0){
          perror("The server terminated permaturely");
          exit(4);
       }

       printf("City: %s",cname); 
       printf("  Today is: %d - %d - %d ", year, recvline[34], recvline[35]);
       printf("  Weather info is as follows:\n");

       printf("The 1th day's weather is: ");
       switch(recvline[37]){
                  case 0: printf(" shower ");  break;
                  case 1: printf(" clear ");  break;
                  case 2: printf(" cloudy ");  break;
                  case 3: printf(" rain ");  break;
                  case 4: printf(" fog ");  break;
                  default:  printf("You are kidding! NO such weather! ");  break;
       }
       printf("Wind-level: %d Temp: %d \n",recvline[38], recvline[39]);

       printf("The 2th day's weather is: ");
       switch(recvline[40]){
                  case 0: printf(" shower ");  break;
                  case 1: printf(" clear ");  break;
                  case 2: printf(" cloudy ");  break;
                  case 3: printf(" rain ");  break;
                  case 4: printf(" fog ");  break;
                  default:  printf("You are kidding! NO such weather! ");  break;
       }
       printf("Wind-level: %d Temp: %d \n",recvline[41], recvline[42]);

       printf("The 3rd day's weather is: ");
       switch(recvline[43]){
                  case 0: printf(" shower ");  break;
                  case 1: printf(" clear ");  break;
                  case 2: printf(" cloudy ");  break;
                  case 3: printf(" rain ");  break;
                  case 4: printf(" fog ");  break;
                  default:  printf("You are kidding! NO such weather! ");  break;
       }
       printf("Wind-level: %d Temp: %d \n",recvline[44], recvline[45]);
    }//case 2

    else if(strncmp(buf,"3",1)==0) {
       //
      memset(buf, 0, MAXLINE);
      if(fgets(buf, MAXLINE, stdin) != NULL){

         if(strncmp(buf,"q",1)==0)  {
            system("clear");
            printf("Please enter the given number to query\n");
            printf("1. today\n");
            printf("2. three days from today\n");
            printf("3. custom day by your self\n");
            printf("<r> back, <q> cls, <#> exit\n");
            printf("===========================================\n");
            continue;    
         }
         if(strncmp(buf,"#",1)==0) exit(0);
         if(strncmp(buf,"r",1)==0) {
              system("clear");
              printf("Welcome to Wether Forecast Demo Program !\n");
              printf("Please input city name in chinese pinyin <e.g. nanjing or beijing>\n");
              printf("<q> cls, <#> exit\n");
              //
              city(sockfd);
          }

          if(strncmp(buf,"9",1)>0){
                printf("Please enter the day number<below 10, e.g. 1 means today>:\n");
                continue;
          }
          if(strncmp(buf,"6",1)>0){
                printf("Sorry, no given day's weather info for city %s !\n", cname);
          }
          else if(strncmp(buf,"0",1)>0 && strncmp(buf,"6",1)<=0){
            sendline[0]=1;
            sendline[1]='A';
            for(i=2; i<30; i++)  sendline[i] = cname[i-2];     
            sendline[32]=buf[0];
            send(sockfd, sendline, 33, 0);
            if(recv(sockfd, recvline, MAXLINE, 0) == 0){
              perror("The server terminated permaturely");
              exit(4);
            }
            printf("City: %s",cname);                    
            printf("  Today is: %d - %d - %d ", year, recvline[34], recvline[35]);
            printf("  Weather info is as follows:\n");
            printf("The %d day's weather is: ",sendline[32]-'0');

            switch(recvline[37]){
                  case 0: printf(" shower ");  break;
                  case 1: printf(" clear ");  break;
                  case 2: printf(" cloudy ");  break;
                  case 3: printf(" rain ");  break;
                  case 4: printf(" fog ");  break;
                  default:  printf("You are kidding! NO such weather! ");  break;
             }
             printf("Wind-level: %d Temp: %d \n",recvline[38], recvline[39]);  

          }
       }//while
     }//case 3
     else{
          printf("input error!\n");  continue;
     }

     memset(sendline, 0, MAXLINE);
     memset(recvline, 0, MAXLINE);
     memset(buf, 0, MAXLINE);
   }//out while
   return 1;
}

#include "im_server.h"

#define WAITTIME 15
#define SERVERPORT1 101
#define SERVERPORT2 102
#define SERVERPORT3 103

//serv_type
#define LOGON 0x01
#define LOGOFF 0x02
#define TO 0x03
#define ALL 0x04
#define SHOW 0x05

#define MAXLINE 200 //==max_seg_t data len
#define LISTENQ 20
#define MAXONLINE 100

//im function ...
void init_list()
{
   int i;
   for(i=0; i<MAXONLINE; i++)
   {
  		printf("%d ****\n", i);
      List[i].port_id = -1;
      //struct's char * chengyuanbianliang xuyao fenpei kongjian !!!!!!!!!!!!
      memset(List[i].username, 0, 10);
      memset(List[i].passwd, 0, 10);
      List[i].tag = false;//connect to the server?
      List[i].logon = false;//log on the server?
   }
}

void add_list(int id)
{
   int i=0;
   if(Online_num > MAXONLINE){
        printf("The chatting room is full, please connect later!\n");
        return;
   }
   while(List[i].tag == true) i++;
   List[i].tag = true;
   List[i].port_id = id;
   //lock and unlock the mutex
   pthread_mutex_lock(&mutexsum);
   Online_num++;
   pthread_mutex_unlock(&mutexsum);
}

void del_list(int id)
{
   int i;
   for(i=0; i<MAXONLINE; i++)
   {
      if(List[i].port_id == id){
         List[i].tag = false;
         pthread_mutex_lock(&mutexsum);
         Online_num--;
         pthread_mutex_unlock(&mutexsum);
      }
   } 
}

void logon(int id, char *name, char *wd)
{
   int i;
   for(i=0; i<MAXONLINE; i++)
   {
      if(List[i].port_id == id && List[i].tag){
         List[i].logon = true;
         strcpy(List[i].username, name);
         strcpy(List[i].passwd, wd);
      }
   }
}

bool check_log(char *name)
{
   int i;
   for(i=0; i<MAXONLINE; i++)
   {  
      if(!List[i].logon) continue;
      else if(strcmp(List[i].username, name) == 0) return false;  
   }
   return true;
}

void logoff(int id)
{
   int i;
   for(i=0; i<MAXONLINE; i++)
   {
      if(List[i].port_id == id){
         List[i].logon = false;
         break;
      }
   }
}


int get_port(char *name)
{
    int i;
    for(i=0; i<MAXONLINE; i++){
       if(strncmp(List[i].username,name,10)==0){       
            return List[i].port_id;          
       }
    }
    return -100;
}

char* get_username(int id)
{
   int i;
   for(i=0; i<MAXONLINE; i++)
   {
      if(List[i].port_id == id){
         return List[i].username;
      }     
   }
   //illegal
   return List[0].username;
}

//sip <---> stcp level ...
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

void* doit(void *arg)
{
  int n;
  int connfd;
  connfd = (int)arg; 
  char buf[MAXLINE];
  memset(buf, 0, MAXLINE);
  while(1){
       //recv(connfd, buf, MAXLINE, 0);
       stcp_server_recv(connfd, buf, MAXLINE);  
       printf("1......\n");
       char *SER_TYPE;
       int serv_type;
       SER_TYPE = strtok(buf, ":");      
       printf("2......\n");
       //change the serv_type to int as switch can only recognize int
       if(strncmp(SER_TYPE,"LOGON",5)==0)  serv_type = 1;
       if(strncmp(SER_TYPE,"LOGOFF",6)==0)  serv_type = 2;
       if(strncmp(SER_TYPE,"TO",2)==0)  serv_type = 3;
       if(strncmp(SER_TYPE,"ALL",3)==0)  serv_type = 4;
       if(strncmp(SER_TYPE,"SHOW",4)==0)  serv_type = 5;
       switch(serv_type){
            case 1:{//LOGON
                 char cname[10];
                 strcpy(cname,strtok(NULL, ":"));
                 if(cname==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The correct format should be LOGON:username:passwd !\n");
                      stcp_server_send(connfd, buf, MAXLINE);
                      memset(buf, 0, MAXLINE);
                      break;
                 }          
                 char password[10];
                 strcpy(password,strtok(NULL, ":"));
                 if(password==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The correct format should be LOGON:username:passwd !\n");
                      stcp_server_send(connfd, buf, MAXLINE);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 //check the username is not used yet            
                 printf("3......\n");            
                 if(!check_log(cname)){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The user name has been used !\n");
                      stcp_server_send(connfd, buf, MAXLINE);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 printf("4......\n");          
                 //logon OK! And notice the log user
                 logon(connfd,cname,password);   
                 printf("5......\n");  
                 int i;
                 char *temp = "Welcome log on the server successful !\n";   
                 memset(buf, 0, MAXLINE);         
                 strcpy(buf,temp);
                 stcp_server_send(connfd, buf, MAXLINE);
                 
                 printf("6......\n");
                 memset(buf, 0, MAXLINE);
                 //notify all the online user
                 strcpy(buf,cname);
                 char *message = "  has log on the server\n";
                 strcat(buf,message);                
                 for(i=0; i<MAXONLINE; i++){
                     if(List[i].logon && List[i].port_id != connfd) 
												stcp_server_send(List[i].port_id, buf, MAXLINE);
                 }
                 memset(buf, 0, MAXLINE);
                 break;
            }
            case 2:{//LOGOFF 
            		 printf("LOGOFF...\n");
                 logoff(connfd);              
                 int i;
                 //notice the logoff user
                 char cname[10];
                 char *leaveinfo = "Leave the server successful !\n";
                 memset(buf, 0, MAXLINE);              
                 strcpy(buf,leaveinfo);               
                 stcp_server_send(connfd, buf, MAXLINE);
                 memset(buf, 0, MAXLINE);
                 strcpy(cname,get_username(connfd));
                 //notice all the online user
                 strcpy(buf,cname);
                 char *message = "  has left the server\n";
                 strcat(buf,message);            
                 for(i=0; i<MAXONLINE; i++){
                     if(List[i].logon && List[i].port_id != connfd)
							stcp_server_send(List[i].port_id, buf, MAXLINE);
                 }
                 memset(buf, 0, MAXLINE);
                 break;
            }
            case 3:{//TO 
            	   printf("TO...\n");
                 char dest_user[10];
                 char message[100];
                 //check the input format is OK!
                 char *t1 = strtok(NULL, ":");
                 if(t1==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The correct format should be: TO:username:message !\n");
                      stcp_server_send(connfd, buf, MAXLINE);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 char *t2 = strtok(NULL, ":");
                 if(t2==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The correct format should be: TO:username:message !\n");
                      stcp_server_send(connfd, buf, MAXLINE);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 //
                 strcpy(dest_user,t1 );               
                 strcpy(message,t2 );
                 memset(buf, 0, MAXLINE);
                 char cname[10];
                 strcpy(cname,get_username(connfd) );
                 char *split = ": ";
                 strcpy(buf,cname);
                 strcat(buf,split);
                 strcat(buf,message);
                 int port = get_port(dest_user);
                 if(port==-100){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The user input is not online!\n");
                      stcp_server_send(connfd, buf, MAXLINE);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 //send(port,buf,100,0);
                 stcp_server_send(port, buf, MAXLINE);
                 memset(buf, 0, MAXLINE);                 
                 break;
            }
            case 4:{//ALL
            		 printf("ALL...\n");
                 char message[100];
                 strcpy(message,strtok(NULL, ":") );
                 if(message==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The message can not be NULL !\n");
                      stcp_server_send(connfd, buf, MAXLINE);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 int i;
                 char *cname = get_username(connfd);
                 char *split = ": ";
                 char *temp = "[ALL]";
                 memset(buf, 0, MAXLINE); 
                 strcpy(buf,temp);
                 strcat(buf,cname);
                 strcat(buf,split);
                 strcat(buf,message);
                 //send the message to all the online user
                 for(i=0; i<MAXONLINE; i++){
                     if(List[i].logon && List[i].port_id != connfd)
							stcp_server_send(List[i].port_id, buf, MAXLINE);
                 }
                 memset(buf, 0, MAXLINE);                 
                 break;
            } 
            case 5:{//SHOW
            		 printf("SHOW...\n");
                 int i;
                 char *split = " # ";
                 memset(buf, 0, MAXLINE); 
                 char *temp = "Online list is as list :\n";
                 strcpy(buf,temp);
                 for(i=0; i<MAXONLINE; i++)
                 {
                    if(List[i].logon)
                    {
                        strcat(buf,List[i].username);
                        strcat(buf,split);
                    }
                 }
                 strcat(buf,"\n");
                 stcp_server_send(connfd, buf, MAXLINE);
                 memset(buf, 0, MAXLINE);                 
                 break;
            }
            default:{
                 memset(buf, 0, MAXLINE);
                 strcpy(buf,"Please input according to the instructor info...\n");
                 stcp_server_send(connfd, buf, MAXLINE);
                 memset(buf, 0, MAXLINE);
                 break;
            }
         }
  }
  if(n<0){
     perror("Read error");
     exit(1);
  }
  close(connfd);
}

int main() {
	printf("yy 0...\n");
	srand(time(NULL));
	pthread_t tid;
	printf("yy 1...\n");
	int sip_conn = connectToSIP();
	if(sip_conn<0) {
		printf("can not connect to the local SIP process\n");
	}
	stcp_server_init(sip_conn);
	
	printf("yy 2...\n");
	init_list();
	int i;
	for(i=1; i<4; i++){
		int sockfd;
		if(i==1)
			sockfd = stcp_server_sock(SERVERPORT1);
		if(i==2)
			sockfd=stcp_server_sock(SERVERPORT2);
		if(i==3)
			sockfd=stcp_server_sock(SERVERPORT3);
			
 		if(sockfd < 0) {
				printf("can't create stcp server\n");
				exit(1);
		}
		stcp_server_accept(sockfd);
		printf("New client join in ...\n");
		add_list(sockfd);
		pthread_create(&tid, NULL, &doit, (void *)sockfd);
		printf("sockfd is %d ......\n", sockfd);	
	}
	while(1){
		sleep(20);
	}
  //will not excute ...
  /*
	if(stcp_server_close(sockfd)<0) {
		printf("can't destroy stcp server\n");
		exit(1);  
	}	
	*/			
	//disconnectToSIP(sip_conn);
}

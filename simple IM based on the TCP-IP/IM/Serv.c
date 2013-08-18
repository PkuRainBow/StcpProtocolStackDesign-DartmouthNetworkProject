#include"Protocol.h"
#define LISTENQ 8
#define MAXONLINE 20

//online not means we have log on the server,only means that we connect to
//server successfully
//connfd is the key ele to identify the unique client

static struct Online_list{
    int port_id;
    char username[10];
    char passwd[10];    
    bool tag;
    bool logon;
}List[MAXONLINE];

pthread_mutex_t  mutexsum;

static int Online_num;

void 
init_list()
{
   int i;
   for(i=0; i<MAXONLINE; i++)
   {
      List[i].port_id = -1;
      //struct's char * chengyuanbianliang xuyao fenpei kongjian !!!!!!!!!!!!
      memset(List[i].username, 0, 10);
      memset(List[i].passwd, 0, 10);
      List[i].tag = false;//connect to the server?
      List[i].logon = false;//log on the server?
   }
}

void 
add_list(int id)
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

void
del_list(int id)
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

void 
logon(int id, char *name, char *wd)
{
   int i;
   for(i=0; i<MAXONLINE; i++)
   {
      if(List[i].port_id == id && List[i].tag){
         List[i].logon = true;
         strcpy(List[i].username, name);
         strcpy(List[i].passwd, wd);
         //printf("log on name:%s#\n",List[i].username);
      }
   }
}

bool
check_log(char *name)
{
   int i;
   for(i=0; i<MAXONLINE; i++)
   {  
      if(!List[i].logon) continue;
      else if(strcmp(List[i].username, name) == 0) return false;  
   }
   return true;
}

void 
logoff(int id)
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


int
get_port(char *name)
{
    int i;
    for(i=0; i<MAXONLINE; i++){
       if(strncmp(List[i].username,name,10)==0){
       //printf("username:%s#\n",List[i].username);
       //printf("name:%s#\n",name);         
            return List[i].port_id;          
       }
    }
    return -100;
}

char*
get_username(int id)
{
   int i;
   //char name[20];
   for(i=0; i<MAXONLINE; i++)
   {
      if(List[i].port_id == id){
         //printf("get_username:   %s\n", List[i].username);
         return List[i].username;
      }     
   }
}

void *
doit(void *arg)
{
  int n;
  int connfd;
  connfd = (int)arg; 
  char buf[MAXLINE];
  memset(buf, 0, MAXLINE);
  while(1){
       recv(connfd, buf, MAXLINE, 0);
       char *SER_TYPE;
       int serv_type;
       SER_TYPE = strtok(buf, ":");
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
                 //check the input format is OK!
                 if(cname==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The correct format should be LOGON:username:passwd !\n");
                      send(connfd,buf,100,0);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 char password[10];
                 strcpy(password,strtok(NULL, ":"));
                 if(password==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The correct format should be LOGON:username:passwd !\n");
                      send(connfd,buf,100,0);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 //check the username is not used yet
                 if(!check_log(cname)){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The user name has been used !\n");
                      send(connfd,buf,100,0);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 //logon OK! And notice the log user
                 logon(connfd,cname,password);
                 int i;
                 char *temp = "Welcome log on the server successful !\n";   
                 memset(buf, 0, MAXLINE);         
                 strcpy(buf,temp);
                 send(connfd,buf,100,0);
                 memset(buf, 0, MAXLINE);
                 //notify all the online user
                 strcpy(buf,cname);
                 char *message = "  has log on the server\n";
                 strcat(buf,message);                
                 for(i=0; i<MAXONLINE; i++){
                     if(List[i].logon && List[i].port_id != connfd)   send(List[i].port_id,buf,100,0);
                 }
                 memset(buf, 0, MAXLINE);
                 break;
            }
            case 2:{//LOGOFF 
                 logoff(connfd);              
                 int i;
                 //notice the logoff user
                 char cname[10];
                 char *leaveinfo = "Leave the server successful !\n";
                 memset(buf, 0, MAXLINE);              
                 strcpy(buf,leaveinfo);               
                 send(connfd,buf,100,0);
                 memset(buf, 0, MAXLINE);
                 strcpy(cname,get_username(connfd));
                 //notice all the online user
                 strcpy(buf,cname);
                 char *message = "  has left the server\n";
                 strcat(buf,message);            
                 for(i=0; i<MAXONLINE; i++){
                     if(List[i].logon && List[i].port_id != connfd)   send(List[i].port_id,buf,100,0);
                 }
                 memset(buf, 0, MAXLINE);
                 break;
            }
            case 3:{//TO 
                 char dest_user[10];
                 char message[100];
                 //check the input format is OK!
                 char *t1 = strtok(NULL, ":");
                 if(t1==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The correct format should be: TO:username:message !\n");
                      send(connfd,buf,100,0);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 char *t2 = strtok(NULL, ":");
                 if(t2==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The correct format should be: TO:username:message !\n");
                      send(connfd,buf,100,0);
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
                      send(connfd,buf,100,0);
                      memset(buf, 0, MAXLINE);
                      break;
                 }
                 send(port,buf,100,0);
                 memset(buf, 0, MAXLINE);                 
                 break;
            }
            case 4:{//ALL
                 char message[100];
                 strcpy(message,strtok(NULL, ":") );
                 if(message==NULL){
                      memset(buf, 0, MAXLINE);
                      strcpy(buf,"The message can not be NULL !\n");
                      send(connfd,buf,100,0);
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
                     if(List[i].logon && List[i].port_id != connfd)   send(List[i].port_id,buf,100,0);
                 }
                 memset(buf, 0, MAXLINE);                 
                 break;
            } 
            case 5:{//SHOW
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
                 send(connfd,buf,100,0);
                 memset(buf, 0, MAXLINE);                 
                 break;
            }
            default:{
                 memset(buf, 0, MAXLINE);
                 strcpy(buf,"Please input according to the instrutor info...\n");
                 send(connfd,buf,100,0);
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

int 
main(int argc, char **argv)
{
  int listenfd, connfd, n;
  pid_t childpid;
  socklen_t clilen;
  pthread_t tid;
  struct sockaddr_in cliaddr, servaddr;

  if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
       perror("Problem in creating the socket");
       exit(2);
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);

  bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  listen(listenfd,LISTENQ);

  init_list();
  
  for( ; ; ){
     static int name_count=1;
     clilen = sizeof(cliaddr);
     printf("Waiting for new cli...\n");
     connfd = accept( listenfd, (struct sockaddr*) &cliaddr, &clilen); 
     printf("New client join in \n"); 
     add_list(name_count);
     name_count++;
     pthread_create(&tid, NULL, &doit, (void *)connfd);
   }
}

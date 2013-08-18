#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "stcp_server.h"
#include "../common/constants.h"

/*面向应用层的接口*/

//
//
//  我们在下面提供了每个函数调用的原型定义和细节说明, 但这些只是指导性的, 你完全可以根据自己的想法来设计代码.
//
//  注意: 当实现这些函数时, 你需要考虑FSM中所有可能的状态, 这可以使用switch语句来实现.
//
//  目标: 你的任务就是设计并实现下面的函数原型.
//

// stcp服务器初始化
//
// 这个函数初始化TCB表, 将所有条目标记为NULL. 它还针对重叠网络TCP套接字描述符conn初始化一个STCP层的全局变量,
// 该变量作为sip_sendseg和sip_recvseg的输入参数. 最后, 这个函数启动seghandler线程来处理进入的STCP段.
// 服务器只有一个seghandler.
//

void stcp_server_init(int conn) {
	//printf("Server: Init STCP server successfully ...\n");
	int i;
    for(i=0; i<MAX_TRANSPORT_CONNECTIONS; i++){
		if(tcblist[i].tcb != NULL) free(tcblist[i].tcb);
		tcblist[i].tcb = NULL;
		tcblist[i].tag = 0;
	}
	//初始一个全局变量？？
	//client_tcb_t *tp = new client_tcb_t;
	STCP_server = conn;
	pthread_t tid;
    pthread_create(&tid, NULL, seghandler, (void *)conn);
}

// 创建服务器套接字
//
// 这个函数查找服务器TCB表以找到第一个NULL条目, 然后使用malloc()为该条目创建一个新的TCB条目.
// 该TCB中的所有字段都被初始化, 例如, TCB state被设置为CLOSED, 服务器端口被设置为函数调用参数server_port. 
// TCB表中条目的索引应作为服务器的新套接字ID被这个函数返回, 它用于标识服务器的连接. 
// 如果TCB表中没有条目可用, 这个函数返回-1.

int stcp_server_sock(unsigned int server_port) {
	int i;
	for(i=0; i<MAX_TRANSPORT_CONNECTIONS; i++){
		if(tcblist[i].tag == 0) {
		   tcblist[i].tcb = (server_tcb_t *)malloc(sizeof(server_tcb_t));
		   tcblist[i].tcb->state = CLOSED;
		   tcblist[i].tcb->server_portNum = server_port;
		   tcblist[i].tcb->server_nodeID = 0;
		   tcblist[i].tcb->client_portNum = 0;
		   tcblist[i].tcb->client_nodeID = 0;
		   tcblist[i].tcb->expect_seqNum = 0;
		   tcblist[i].tcb->recvBuf = NULL;
		   tcblist[i].tcb->usedBufLen = 0;
		   tcblist[i].tcb->bufMutex = NULL;
		   tcblist[i].tag = 1;
		   break;
	   }
	}
	if(i==MAX_TRANSPORT_CONNECTIONS)  return -1;
    return i;
}

// 接受来自STCP客户端的连接
//
// 这个函数使用sockfd获得TCB指针, 并将连接的state转换为LISTENING. 它然后进入忙等待(busy wait)直到TCB状态转换为CONNECTED 
// (当收到SYN时, seghandler会进行状态的转换). 该函数在一个无穷循环中等待TCB的state转换为CONNECTED,  
// 当发生了转换时, 该函数返回1. 你可以使用不同的方法来实现这种阻塞等待.
//

int stcp_server_accept(int sockfd) {
	//printf("Server: Waiting for the  STCP client connect to the server ...\n");
	printf("\nServer: +++  CLOSED ------>  LISTENING\n");
	tcblist[sockfd].tcb->state = LISTENING;
	//
	while(1){
	   usleep(ACCEPT_POLLING_INTERVAL / 1000);
	   if(tcblist[sockfd].tcb->state==CONNECTED){
	       break;	       
	   }
	}
	return 1;
}

// 接收来自STCP客户端的数据
//
// 这个函数接收来自STCP客户端的数据. 你不需要在本实验中实现它.
//
int stcp_server_recv(int sockfd, void* buf, unsigned int length) {
	return 1;
}

// 关闭STCP服务器
//
// 这个函数调用free()释放TCB条目. 它将该条目标记为NULL, 成功时(即位于正确的状态)返回1,
// 失败时(即位于错误的状态)返回-1.
//

int stcp_server_close(int sockfd) {
	sleep(CLOSEWAIT_TIMEOUT);
	printf("Server: +++  CLOSEWAIT ------>  CLOSED\n");
    tcblist[sockfd].tcb->state = CLOSED;
	int state = tcblist[sockfd].tcb->state;
	if(tcblist[sockfd].tcb != NULL)
	     free(tcblist[sockfd].tcb);
	tcblist[sockfd].tag = 0;
	return 1;
}

// 处理进入段的线程
//
// 这是由stcp_server_init()启动的线程. 它处理所有来自客户端的进入数据. seghandler被设计为一个调用sip_recvseg()的无穷循环, 
// 如果sip_recvseg()失败, 则说明重叠网络连接已关闭, 线程将终止. 根据STCP段到达时连接所处的状态, 可以采取不同的动作.
// 请查看服务端FSM以了解更多细节.
//

void *seghandler(void* arg) {
	char buffer[sizeof(seg_t)];
	
	while(1){
		memset(buffer, 0, sizeof(seg_t));
		int t = sip_recvseg(STCP_server, (seg_t*)buffer);		
		if(t == -1){
		    pthread_exit(NULL);
		}
		
	    if(t == 1){
			//printf("Server: The seg info is partly lost ... \n");
			//drop out the packet ...
		    continue;
		}
		int client_port = ((seg_t*)buffer)->header.src_port;
		int server_port = ((seg_t*)buffer)->header.dest_port;
		int client_type = ((seg_t*)buffer)->header.type;
		int state;
		int i;
		int index;
		for(i=0; i<10; i++){
		    if(tcblist[i].tag==1 && tcblist[i].tcb->server_portNum == server_port){
				state = tcblist[i].tcb->state;
				index = i;
			 }
		}
			 
		switch(state){
		    case CLOSED:{
				break;		
			}
			case LISTENING:{
				if(client_type==SYN){
					printf("Server: +++  LISTENING ------>  CONNECTED\n");
			        tcblist[index].tcb->state = CONNECTED;
			        tcblist[index].tcb->client_portNum = client_port;	
				    seg_t* temp = (seg_t *)malloc(sizeof(seg_t));
					temp->header.type = SYNACK;
					temp->header.src_port = server_port;
					temp->header.dest_port = client_port;
					sip_sendseg(STCP_server, temp);
					printf("\n***********************\n");
					printf("srcport: %d  \n", server_port);
				    printf("destport: %d \n", client_port);
				    printf("type:  SYNACK\n");
					printf("***********************\n");
				}
				break;
			}
			
			case CONNECTED:{				
				if(client_type==FIN){
					printf("Server: +++  CONNECTED ------>  CLOSEWAIT\n");
					tcblist[index].tcb->state = CLOSEWAIT;
				    seg_t* temp = (seg_t *)malloc(sizeof(seg_t));
					temp->header.type = FINACK;
					temp->header.src_port = server_port;
					temp->header.dest_port = client_port;
					sip_sendseg(STCP_server, temp);
					printf("\n***********************\n");
					printf("srcport: %d  \n", server_port);
				    printf("destport: %d \n", client_port);
				    printf("type:  FINACK\n");
					printf("***********************\n");
			    }
				//send data
					
				if(client_type==SYN){
					printf("Server: +++  CONNECTED ------>  CONNECTED\n");
					tcblist[index].tcb->state = CONNECTED;
					seg_t* temp = (seg_t *)malloc(sizeof(seg_t));
					temp->header.type = SYNACK;
					temp->header.src_port = server_port;
					temp->header.dest_port = client_port;
					sip_sendseg(STCP_server, temp);	
					printf("\n***********************\n");				
					printf("srcport: %d  \n", server_port);
				    printf("destport: %d \n", client_port);
				    printf("type:  SYNACK\n");
					printf("***********************\n");
				}
				
				break;		 
			}
			
			case CLOSEWAIT:{
				if(client_type==FIN){
				   tcblist[index].tcb->state = CLOSEWAIT;
				   printf("Server: +++  CLOSEWAIT ------>  CLOSEWAIT\n");
				   seg_t* temp = (seg_t *)malloc(sizeof(seg_t));
				   temp->header.type = FINACK;
				   temp->header.src_port = server_port;
				   temp->header.dest_port = client_port;
				   sip_sendseg(STCP_server, temp);
				   printf("\n***********************\n");
				   printf("srcport: %d  \n", server_port);
				   printf("destport: %d \n", client_port);
				   printf("type:  FINACK\n");
				   printf("***********************\n");
			    }
			    break;
			}
			
		   default:{
		       printf("Server: The state is wrong ...\n");
		       break;
		   }							
		}		
	}
}

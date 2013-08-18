#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "stcp_client.h"

/*面向应用层的接口*/

//
//  我们在下面提供了每个函数调用的原型定义和细节说明, 但这些只是指导性的, 你完全可以根据自己的想法来设计代码.
//
//  注意: 当实现这些函数时, 你需要考虑FSM中所有可能的状态, 这可以使用switch语句来实现.
//
//  目标: 你的任务就是设计并实现下面的函数原型.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// stcp客户端初始化
//
// 这个函数初始化TCB表, 将所有条目标记为NULL.  
// 它还针对重叠网络TCP套接字描述符conn初始化一个STCP层的全局变量, 该变量作为sip_sendseg和sip_recvseg的输入参数.
// 最后, 这个函数启动seghandler线程来处理进入的STCP段. 客户端只有一个seghandler.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

void stcp_client_init(int conn) {
	//printf("Client: Init STCP client successfully ...\n");
	int i;
    for(i=0; i<TCB_MAX; i++){
		if(tcblist[i].tcb != NULL) free(tcblist[i].tcb);
		tcblist[i].tcb = NULL;
		tcblist[i].tag = 0;
	}
	
	STCP_client = conn;
	
	pthread_t tid;
    pthread_create(&tid, NULL, seghandler, (void *)conn);
    return;
}

// 创建一个客户端TCB条目, 返回套接字描述符
//
// 这个函数查找客户端TCB表以找到第一个NULL条目, 然后使用malloc()为该条目创建一个新的TCB条目.
// 该TCB中的所有字段都被初始化. 例如, TCB state被设置为CLOSED，客户端端口被设置为函数调用参数client_port. 
// TCB表中条目的索引号应作为客户端的新套接字ID被这个函数返回, 它用于标识客户端的连接. 
// 如果TCB表中没有条目可用, 这个函数返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_client_sock(unsigned int client_port) {
	int i;
	for(i=0; i<TCB_MAX; i++){
		if(tcblist[i].tag == 0) {
		   tcblist[i].tcb = malloc(sizeof(client_tcb_t));
		   tcblist[i].tcb->state = CLOSED;
		   tcblist[i].tcb->client_portNum = client_port;
		   tcblist[i].tcb->server_nodeID = 0;
		   tcblist[i].tcb->server_portNum = 0;
		   tcblist[i].tcb->client_nodeID = 0;
		   tcblist[i].tcb->next_seqNum = 0;
		   tcblist[i].tcb->bufMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
		   pthread_mutex_init(tcblist[i].tcb->bufMutex, NULL);
		   tcblist[i].tcb->sendBufHead = NULL;
		   tcblist[i].tcb->sendBufunSent = NULL;
		   tcblist[i].tcb->sendBufTail = NULL;
		   tcblist[i].tcb->unAck_segNum = 0;
		   tcblist[i].tag = 1;
		   break;
	   }
	}
	if(i==TCB_MAX)  return -1;
    return i;
}

// 连接STCP服务器
//
// 这个函数用于连接服务器. 它以套接字ID和服务器的端口号作为输入参数. 套接字ID用于找到TCB条目.  
// 这个函数设置TCB的服务器端口号,  然后使用sip_sendseg()发送一个SYN段给服务器.  
// 在发送了SYN段之后, 一个定时器被启动. 如果在SYNSEG_TIMEOUT时间之内没有收到SYNACK, SYN 段将被重传. 
// 如果收到了, 就返回1. 否则, 如果重传SYN的次数大于SYN_MAX_RETRY, 就将state转换到CLOSED, 并返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_client_connect(int sockfd, unsigned int server_port) {
	tcblist[sockfd].tcb->server_portNum = server_port;
	seg_t *send_seg = malloc(sizeof(seg_t));
	send_seg->header.type = SYN;
	send_seg->header.src_port = tcblist[sockfd].tcb->client_portNum;
	send_seg->header.dest_port = server_port;
	send_seg->header.length = 0;

	sip_sendseg(STCP_client, send_seg);
	
	printf("\nClient: +++  CLOSED ------>  SYNSENT\n");
	
	tcblist[sockfd].tcb->state = SYNSENT;
	printf("\n***********************\n");
	printf("srcport: %d  \n", send_seg->header.src_port);
	printf("destport: %d \n", send_seg->header.dest_port);
	printf("type: SYN\n"); 	
	printf("***********************\n");
	//printf("Client: Now state is SYNSENT , waiting for the SYNACK ...\n");
	
	int count;
	for(count=0; count < SYN_MAX_RETRY; count++){
		usleep(SYN_TIMEOUT/1000);
	    if(tcblist[sockfd].tcb->state == CONNECTED){
			printf("Client: Receive the SYNACK successfully ...\n");
	        return 1;
		}
	    else{
			printf("Resend the SYN ...\n");
	        sip_sendseg(STCP_client, send_seg);
		}
	}
	
	if(count == SYN_MAX_RETRY){
		printf("Client: Having tried to connect for many times, but failed ...\n");
		tcblist[sockfd].tcb->state = CLOSED;
	    return -1;
	}
}

// 发送数据给STCP服务器. 这个函数使用套接字ID找到TCB表中的条目. 
// 然后它使用提供的数据创建segBuf, 将它附加到发送缓冲区链表中. 
// 如果发送缓冲区在插入数据之前为空, 一个名为sendbuf_timer的线程就会启动. 
// 每隔SENDBUF_ROLLING_INTERVAL时间查询发送缓冲区以检查是否有超时事件发生.
// 这个函数在成功时返回1，否则返回-1. 
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int stcp_client_send(int sockfd, void* data, unsigned int length){	
	int count = length/MAX_SEG_LEN;
	int remain = length%MAX_SEG_LEN;	
	if(data==NULL){
		printf("Warning: send NULL data ...\n");
	    return -1;
	}
	char *char_data = (char*)data;
	pthread_mutex_lock(tcblist[sockfd].tcb->bufMutex);
	if(tcblist[sockfd].tcb->sendBufHead == NULL){
		   // 如果发送缓冲区在插入数据之前为空, 一个名为sendbuf_timer的线程就会启动.
		   pthread_t tid;
           pthread_create(&tid, NULL, sendBuf_timer, (void *)tcblist[sockfd].tcb);
	}
	//store the data to the seg_buf
	while(count > 0){
		if(tcblist[sockfd].tcb->sendBufHead == NULL){           
		   segBuf_t *temp = malloc(sizeof(segBuf_t));		   
		   temp->seg.header.type = DATA;
		   temp->seg.header.length = MAX_SEG_LEN;
		   temp->seg.header.seq_num = tcblist[sockfd].tcb->next_seqNum;		   
		   temp->seg.header.src_port = tcblist[sockfd].tcb->client_portNum;
		   temp->seg.header.dest_port = tcblist[sockfd].tcb->server_portNum;
		  		   
		   tcblist[sockfd].tcb->next_seqNum += temp->seg.header.length;
		   
		   memcpy(temp->seg.data, char_data, MAX_SEG_LEN);
		   char_data = char_data+MAX_SEG_LEN;	   
		   tcblist[sockfd].tcb->sendBufHead = temp;
		   tcblist[sockfd].tcb->sendBufTail = temp;	
		   tcblist[sockfd].tcb->sendBufunSent = temp;	   
		}
	    else{
			segBuf_t *p = malloc(sizeof(segBuf_t));
			p->seg.header.type = DATA;
			p->seg.header.length = MAX_SEG_LEN;
			p->seg.header.seq_num = tcblist[sockfd].tcb->next_seqNum;
		    p->seg.header.src_port = tcblist[sockfd].tcb->client_portNum;
		    p->seg.header.dest_port = tcblist[sockfd].tcb->server_portNum;
		   
			tcblist[sockfd].tcb->next_seqNum += p->seg.header.length;
		    memcpy(p->seg.data, char_data, MAX_SEG_LEN);
		    char_data = char_data+MAX_SEG_LEN;
		    tcblist[sockfd].tcb->sendBufTail->next = p;
		    tcblist[sockfd].tcb->sendBufTail = p;
		    if(tcblist[sockfd].tcb->sendBufunSent == NULL)
		        tcblist[sockfd].tcb->sendBufunSent = p; 
		}	
		count--;   
	}
	
	if(remain > 0){
		if(tcblist[sockfd].tcb->sendBufHead == NULL){           
		   segBuf_t *temp = malloc(sizeof(segBuf_t));		   
		   temp->seg.header.type = DATA;
		   temp->seg.header.length = remain;
		   temp->seg.header.seq_num = tcblist[sockfd].tcb->next_seqNum;
		   temp->seg.header.src_port = tcblist[sockfd].tcb->client_portNum;
		   temp->seg.header.dest_port = tcblist[sockfd].tcb->server_portNum;
		   
		   temp->next = NULL;
		   tcblist[sockfd].tcb->next_seqNum += remain;
		   
		   memcpy(temp->seg.data, char_data, remain);
		   tcblist[sockfd].tcb->sendBufHead = temp;
		   tcblist[sockfd].tcb->sendBufTail = temp;	
		   tcblist[sockfd].tcb->sendBufunSent = temp;	   
		}
	    else{
			segBuf_t *p = malloc(sizeof(segBuf_t));
			p->seg.header.type = DATA;
			p->seg.header.length = remain;
			p->seg.header.seq_num = tcblist[sockfd].tcb->next_seqNum;
			p->seg.header.src_port = tcblist[sockfd].tcb->client_portNum;
		    p->seg.header.dest_port = tcblist[sockfd].tcb->server_portNum;
		    
			p->next = NULL;
			tcblist[sockfd].tcb->next_seqNum += remain;
		    memcpy(p->seg.data, char_data, remain);
		    tcblist[sockfd].tcb->sendBufTail->next = p;
		    tcblist[sockfd].tcb->sendBufTail = p;
		    if(tcblist[sockfd].tcb->sendBufunSent == NULL)
		        tcblist[sockfd].tcb->sendBufunSent = p; 
		}	
	}
	
	if(tcblist[sockfd].tcb->unAck_segNum < GBN_WINDOW){
		segBuf_t *tt = tcblist[sockfd].tcb->sendBufunSent;
		
		while(tt != NULL && tcblist[sockfd].tcb->unAck_segNum < GBN_WINDOW){
			gettimeofday(&tt->sentTime, NULL);
			sip_sendseg(STCP_client, &tt->seg);
		    tcblist[sockfd].tcb->unAck_segNum++;		    
		    tt = tt->next;
		}
		tcblist[sockfd].tcb->sendBufunSent = tt;
	}	
	pthread_mutex_unlock(tcblist[sockfd].tcb->bufMutex);
	return 1;
}

// 断开到STCP服务器的连接
//
// 这个函数用于断开到服务器的连接. 它以套接字ID作为输入参数. 套接字ID用于找到TCB表中的条目.  
// 这个函数发送FIN segment给服务器. 在发送FIN之后, state将转换到FINWAIT, 并启动一个定时器.
// 如果在最终超时之前state转换到CLOSED, 则表明FINACK已被成功接收. 否则, 如果在经过FIN_MAX_RETRY次尝试之后,
// state仍然为FINWAIT, state将转换到CLOSED, 并返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_client_disconnect(int sockfd) {	
    seg_t *send_seg = malloc(sizeof(seg_t));
    send_seg->header.src_port = tcblist[sockfd].tcb->client_portNum;
    send_seg->header.dest_port = tcblist[sockfd].tcb->server_portNum;
    send_seg->header.type = FIN;
    send_seg->header.length = 0;
    sip_sendseg(STCP_client, send_seg);
    printf("Client: +++  CONNECTED ------>  FINWAIT\n");
    tcblist[sockfd].tcb->state = FINWAIT;
    printf("\n***********************\n");
    printf("srcport: %d  \n", send_seg->header.src_port);
	printf("destport: %d \n", send_seg->header.dest_port);
	printf("type: FINWAIT \n");    
    printf("***********************\n");
    int count;
	for(count=0; count < FIN_MAX_RETRY; count++){
		usleep(FIN_TIMEOUT/1000);
		
	    if(tcblist[sockfd].tcb->state == CLOSED){
			printf("Client: Receive the FINACK successfully ...\n");
		    segBuf_t *temp = tcblist[sockfd].tcb->sendBufHead;
	        while(temp != NULL){
		          segBuf_t *p = temp;
		          temp = temp->next;
		          free(p);
	        }
	        return 1;
		}
	    else{
	        sip_sendseg(STCP_client, send_seg);
            printf("Resend the FIN ...\n");
		}
	}

	if(count == FIN_MAX_RETRY){
		tcblist[sockfd].tcb->state = CLOSED;
		printf("Client: Having tried to close connect for many times, but failed ...\n");
	    return -1;
	}
}

// 关闭STCP客户
//
// 这个函数调用free()释放TCB条目. 它将该条目标记为NULL, 成功时(即位于正确的状态)返回1,
// 失败时(即位于错误的状态)返回-1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int stcp_client_close(int sockfd) {
//printf(	"THE CURRENT STATE IS %d\n", sockfd);
	int state = tcblist[sockfd].tcb->state;
    free(tcblist[sockfd].tcb->bufMutex);
    	
	if(tcblist[sockfd].tcb != NULL)
	    free(tcblist[sockfd].tcb);
	tcblist[sockfd].tcb = NULL;
	tcblist[sockfd].tag = 0;
	if(state==1) {
		printf("Client: Free the client tcb item successfully ...\n");
	    return 1;
	}
	else {
		printf("Client: Failed to free the client tcb item successfully ...\n");
	    return -1;
	}
}



// 这个线程持续轮询发送缓冲区以触发超时事件. 如果发送缓冲区非空, 它应一直运行.
// 如果(当前时间 - 第一个已发送但未被确认段的发送时间) > DATA_TIMEOUT, 就发生一次超时事件.
// 当超时事件发生时, 重新发送所有已发送但未被确认段. 当发送缓冲区为空时, 这个线程将终止.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//clienttcb is the   segBuf_t
void* sendBuf_timer(void* clienttcb)
{
	while(1){			
		client_tcb_t* temp = (client_tcb_t*)((long)clienttcb);
		usleep(SENDBUF_POLLING_INTERVAL/1000);
		pthread_mutex_lock(temp->bufMutex);
					
		if(temp->sendBufHead == NULL){
			pthread_mutex_unlock(temp->bufMutex);
			printf("sendBuf_timer exit...\n");
			pthread_exit(NULL);
		}
	
		struct timeval curtime;
		gettimeofday(&curtime, NULL);	
		long long timeuse = 1000000*(curtime.tv_sec - temp->sendBufHead->sentTime.tv_sec)
		             + curtime.tv_usec - temp->sendBufHead->sentTime.tv_usec;		 
		if(timeuse > DATA_TIMEOUT/1000 ){
			printf("the unacked data will be resend ...\n");
			segBuf_t *start = temp->sendBufHead;
			segBuf_t *end  = temp->sendBufunSent;
            while(start != end){
			    gettimeofday(&start->sentTime, NULL);
			    sip_sendseg(STCP_client, &start->seg);
		        //tcblist[sockfd].tcb->unAck_segNum++;
		        start = start->next;
		    }
		}
		pthread_mutex_unlock(temp->bufMutex);
		
	}
}

// 处理进入段的线程
//
// 这是由stcp_client_init()启动的线程. 它处理所有来自服务器的进入段. 
// seghandler被设计为一个调用sip_recvseg()的无穷循环. 如果sip_recvseg()失败, 则说明重叠网络连接已关闭,
// 线程将终止. 根据STCP段到达时连接所处的状态, 可以采取不同的动作. 请查看客户端FSM以了解更多细节.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

void *seghandler(void* arg) {
	char buffer[sizeof(seg_t)];

	while(1){
		//printf("on going\n");
		memset(buffer, 0, sizeof(seg_t));
		int test = sip_recvseg(STCP_client, (seg_t*)buffer);
		if(test == -1){
		      pthread_exit(NULL);
		}
		//
	    if(test == 1){		
		    continue;
		}
//~ printf("after continue ...\n");
		int server_port = ((seg_t*)buffer)->header.src_port;
		int client_port = ((seg_t*)buffer)->header.dest_port;
		int server_type = ((seg_t*)buffer)->header.type;
		int seq = ((seg_t*)buffer)->header.ack_num;
		int state;
		int i;
		int index;
	
		for(i=0; i<TCB_MAX; i++){
		    if( (tcblist[i].tag==1) && (tcblist[i].tcb->server_portNum==server_port) ){
				state = tcblist[i].tcb->state;
				index = i;
			}
		}

		switch(state){
		    case CLOSED:{
				break;		
			}
			case SYNSENT:{
				if(server_type==SYNACK){
				    tcblist[index].tcb->state = CONNECTED;
				    printf("\n***********************\n");
				    printf("srcport: %d  \n", server_port);
				    printf("destport: %d \n", client_port);
				    printf("type: SYNACK\n");
				    printf("***********************\n");
				    printf("Client: +++  SYNSENT ------>  CONNECTED\n");
				}
				break;
			}
			case CONNECTED:{	
				if(server_type==DATAACK){
					pthread_mutex_lock(tcblist[index].tcb->bufMutex);
					
					printf("\n***********************\n");
				    printf("srcport: %d  \n", server_port);
				    printf("destport: %d \n", client_port);
				    printf("type: DATAACK\n");
				    printf("ack_num: %d \n", seq);
				    printf("***********************\n");
					
					int ack = seq;
					segBuf_t *temp = tcblist[index].tcb->sendBufHead;
					/*	
					while(temp != NULL){
						printf(" *******   the seq num is %d\n", temp->seg.header.seq_num);	
						temp = temp->next;
					}				
						
					temp = tcblist[index].tcb->sendBufHead;		
					*/
					
					if(temp==NULL) printf("the head is null\n");
					while(temp != NULL){
						printf(" *******   the Head seq num is %d\n", temp->seg.header.seq_num);
						if(temp->seg.header.seq_num < ack){
							printf("free the acked data --- the seq is %d...\n", temp->seg.header.seq_num);
							//segBuf_t *q = temp;
							//temp = temp->next;					
							tcblist[index].tcb->sendBufHead = temp->next;
							tcblist[index].tcb->unAck_segNum--;
							free(temp);
							temp = tcblist[index].tcb->sendBufHead;
						}
						else{
							break;
						}						
					}
										
					// all is acked 
					if(tcblist[index].tcb->sendBufHead == NULL){
					    tcblist[index].tcb->sendBufTail = NULL;
					    tcblist[index].tcb->sendBufunSent = NULL;
					    tcblist[index].tcb->unAck_segNum = 0;
					}
					
					//< GBN so we can send the new seg...
					if(tcblist[index].tcb->sendBufunSent != NULL){
						segBuf_t *tt = tcblist[index].tcb->sendBufunSent;
						while(tcblist[index].tcb->unAck_segNum < GBN_WINDOW && tt != NULL){						    
						    gettimeofday(&(tt->sentTime), NULL);
			                sip_sendseg(STCP_client, &tt->seg);
			                tcblist[index].tcb->unAck_segNum++;
			                tt = tt->next;
						}
					    tcblist[index].tcb->sendBufunSent = tt;		            
					}
					
					pthread_mutex_unlock(tcblist[index].tcb->bufMutex);
				}				
				break;			 
			}
			
			case FINWAIT:{
				if(server_type==FINACK){
				   tcblist[index].tcb->state = CLOSED;
				   printf("\n***********************\n");				   
				   printf("srcport: %d  \n", server_port);
				   printf("destport: %d \n", client_port);
				   printf("type: FINACK\n");
				   printf("***********************\n");
				   printf("Client: +++  FINWAIT ------>  CLOSED\n");
			    }
			    break;			   
			}			
		    default:{
		        printf("Client: The state is wrong ...\n");
		        break;
		    }		
		}		
	}
}




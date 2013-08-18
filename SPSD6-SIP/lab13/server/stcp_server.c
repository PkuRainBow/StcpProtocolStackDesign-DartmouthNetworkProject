#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/select.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include "stcp_server.h"
#include "../topology/topology.h"
#include "../common/constants.h"

int sip_conn;
server_tcb_node tcblist[MAX_TRANSPORT_CONNECTIONS];

void stcp_server_init(int conn) {
	int i;
    for(i=0; i<MAX_TRANSPORT_CONNECTIONS; i++){
		if(tcblist[i].tcb != NULL) free(tcblist[i].tcb);
		tcblist[i].tcb = NULL;
		tcblist[i].tag = 0;
	}
	sip_conn = conn;
	pthread_t tid;
  pthread_create(&tid, NULL, seghandler, (void *)conn);
}

int stcp_server_sock(unsigned int server_port) {
	int i;
	for(i=0; i<MAX_TRANSPORT_CONNECTIONS; i++){
		if(tcblist[i].tag == 0) {
		   tcblist[i].tcb = (server_tcb_t *)malloc(sizeof(server_tcb_t));
		   tcblist[i].tcb->state = CLOSED;
		   tcblist[i].tcb->server_portNum = server_port;
		   //added 
		   tcblist[i].tcb->server_nodeID = topology_getMyNodeID();
		   tcblist[i].tcb->client_portNum = 0;
		   tcblist[i].tcb->client_nodeID = 0;
		   tcblist[i].tcb->expect_seqNum = 0;
		   tcblist[i].tcb->recvBuf = malloc(RECEIVE_BUF_SIZE*sizeof(char));
		   tcblist[i].tcb->usedBufLen = 0;
		   tcblist[i].tcb->bufMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
		   pthread_mutex_init(tcblist[i].tcb->bufMutex, NULL);
		   tcblist[i].tag = 1;
		   break;
	   }
	}
	if(i==MAX_TRANSPORT_CONNECTIONS)  return -1;
  return i;
}

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

int stcp_server_recv(int sockfd, void* buf, unsigned int length){
	while(1){
		pthread_mutex_lock(tcblist[sockfd].tcb->bufMutex);	
		unsigned int recv_count = tcblist[sockfd].tcb->usedBufLen;	
        if(recv_count >= length){
printf("Bingo ---  at sockfd %d --- recv data  length is %d...\n", sockfd, length);
			memcpy((char*)buf, tcblist[sockfd].tcb->recvBuf, length);
			char temp[recv_count-length];
			memcpy(temp, tcblist[sockfd].tcb->recvBuf+length, recv_count-length);
			memset(tcblist[sockfd].tcb->recvBuf, 0, RECEIVE_BUF_SIZE);
			memcpy(tcblist[sockfd].tcb->recvBuf, temp, recv_count-length);
			tcblist[sockfd].tcb->usedBufLen -= length;		
			pthread_mutex_unlock(tcblist[sockfd].tcb->bufMutex);	
			break;
		}
		pthread_mutex_unlock(tcblist[sockfd].tcb->bufMutex);
		sleep(RECVBUF_POLLING_INTERVAL);
        printf("sleep 1 sec...\n");			
	}	
    return 1;
}

int stcp_server_close(int sockfd) {
	sleep(CLOSEWAIT_TIMEOUT);
	tcblist[sockfd].tcb->usedBufLen = 0;
	printf("Server: +++  CLOSEWAIT ------>  CLOSED\n");
    tcblist[sockfd].tcb->state = CLOSED;
	//int state = tcblist[sockfd].tcb->state;
	free(tcblist[sockfd].tcb->bufMutex);
	free(tcblist[sockfd].tcb->recvBuf);
	if(tcblist[sockfd].tcb != NULL)
	     free(tcblist[sockfd].tcb);
	tcblist[sockfd].tag = 0;
	return 1;
}

void *seghandler(void* arg) {
	char buffer[sizeof(seg_t)];
	int srcnodeID;
	while(1){
		memset(buffer, 0, sizeof(seg_t));
		//while(sip < ){}
		printf("~~~~~~~1  \n");
		int t = sip_recvseg(sip_conn, &srcnodeID, (seg_t*)buffer);
		printf("~~~~~~~2  \n");
			
		if(t == -1){
				printf("~~~~~~~ 999  \n");
		    pthread_exit(NULL);
		}
		
	  if(t == 1){
	  	  printf("~~~~~~~3  \n");
		    continue;
		}
		int client_port = ((seg_t*)buffer)->header.src_port;
		int server_port = ((seg_t*)buffer)->header.dest_port;
		int client_type = ((seg_t*)buffer)->header.type;
		int data_len = ((seg_t*)buffer)->header.length;
		int client_nodeID = srcnodeID;
		
		int seq = ((seg_t*)buffer)->header.seq_num;
		
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
					//added
					tcblist[index].tcb->client_nodeID = client_nodeID;
			        tcblist[index].tcb->state = CONNECTED;
			        tcblist[index].tcb->client_portNum = client_port;
			        //**************
			        tcblist[index].tcb->expect_seqNum = seq;	
			        
				    seg_t* temp = (seg_t *)malloc(sizeof(seg_t));
					temp->header.type = SYNACK;
					temp->header.src_port = server_port;
					temp->header.dest_port = client_port;
					temp->header.length = 0;
					sip_sendseg(sip_conn, tcblist[index].tcb->client_nodeID, temp);
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
					temp->header.length = 0;
					sip_sendseg(sip_conn, tcblist[index].tcb->client_nodeID, temp);
					
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
					temp->header.length = 0;
					
					//tcblist[index].tcb->expect_seqNum = seq;
					
					sip_sendseg(sip_conn, tcblist[index].tcb->client_nodeID, temp);	
					printf("\n***********************\n");				
					printf("srcport: %d  \n", server_port);
				    printf("destport: %d \n", client_port);
				    printf("expect data seq: %d \n", seq);
				    printf("type:  SYNACK\n");
					printf("***********************\n");
				}
				
				if(client_type==DATA){
					pthread_mutex_lock(tcblist[index].tcb->bufMutex);	
												
					if(tcblist[index].tcb->expect_seqNum == seq){
						//
						if(tcblist[index].tcb->usedBufLen+data_len <= RECEIVE_BUF_SIZE){
printf("Server: receive the data ok ...\n");
							memcpy(tcblist[index].tcb->recvBuf+tcblist[index].tcb->usedBufLen, ((seg_t*)buffer)->data, data_len);
							tcblist[index].tcb->usedBufLen += data_len;
							tcblist[index].tcb->expect_seqNum += data_len;
						    seg_t* temp = (seg_t *)malloc(sizeof(seg_t));
					        temp->header.type = DATAACK;
					        temp->header.ack_num = tcblist[index].tcb->expect_seqNum;
					        temp->header.src_port = server_port;
					        temp->header.dest_port = client_port;
					        temp->header.length = 0;
					        
					        sip_sendseg(sip_conn, tcblist[index].tcb->client_nodeID, temp);
					        printf("\n***********************\n");
					        printf("type:  DATAACK\n");
					        printf("The expect seq is equal to the recv seq ...\n");				
					        printf("srcport: %d  \n", server_port);
				            printf("destport: %d \n", client_port);
				            printf("receive the data seq: %d \n", seq);
				            printf("data seq length : %d \n", data_len);
				            printf("expect the data seq:  %d\n", tcblist[index].tcb->expect_seqNum);
					        printf("***********************\n");	
						}
					}
					else{
						seg_t* temp = (seg_t *)malloc(sizeof(seg_t));
					    temp->header.type = DATAACK;
					    temp->header.ack_num = tcblist[index].tcb->expect_seqNum;
					    temp->header.src_port = server_port;
					    temp->header.dest_port = client_port;
					    temp->header.length = 0;
					    
					    sip_sendseg(sip_conn, tcblist[index].tcb->client_nodeID, temp);
					    printf("\n***********************\n");
					    printf("type:  DATAACK\n");
					    printf("The expect seq No equal to the recv seq ...\n");				
					    printf("srcport: %d  \n", server_port);
				        printf("destport: %d \n", client_port);
				        printf("expect the data seq:  %d\n", tcblist[index].tcb->expect_seqNum);
					    printf("***********************\n");
					}
					
					pthread_mutex_unlock(tcblist[index].tcb->bufMutex);
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
				   temp->header.length = 0;
				   sip_sendseg(sip_conn, tcblist[index].tcb->client_nodeID, temp);
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


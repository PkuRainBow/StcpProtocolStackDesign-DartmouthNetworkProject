//文件名: client/stcp_client.c
//
//描述: 这个文件包含STCP客户端接口实现
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <assert.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "../topology/topology.h"
#include "stcp_client.h"
#include "../common/seg.h"

//client_tcb_t* tcbtable[MAX_TRANSPORT_CONNECTIONS];
static tcb_node tcblist[TCB_MAX];
int sip_conn;

void stcp_client_init(int conn) {
	int i;
    for(i=0; i<TCB_MAX; i++){
		if(tcblist[i].tcb != NULL) free(tcblist[i].tcb);
		tcblist[i].tcb = NULL;
		tcblist[i].tag = 0;
	}
	sip_conn = conn;
	pthread_t tid;
	pthread_create(&tid, NULL, seghandler, (void *)conn);
	return;
}


int stcp_client_sock(unsigned int client_port) {
	int i;
	for(i=0; i<TCB_MAX; i++){
		if(tcblist[i].tag == 0) {
		   tcblist[i].tcb = (tcb_t *)malloc(sizeof(tcb_t));
		   tcblist[i].tcb->client_portNum = client_port;
		   tcblist[i].tcb->server_nodeID = 0;
		   tcblist[i].tcb->server_portNum = 0;
		   tcblist[i].tcb->client_nodeID = topology_getMyNodeID();
		   tcblist[i].tcb->state = CLOSED;
		   //send manage ...
		   tcblist[i].tcb->next_seqNum = 0;
		   tcblist[i].tcb->sendBufHead = NULL;
		   tcblist[i].tcb->sendBufunSent = NULL;
		   tcblist[i].tcb->sendBufTail = NULL;
		   tcblist[i].tcb->unAck_segNum = 0;
		   //recv manage ...
		   tcblist[i].tcb->expect_seqNum = 0;
		   tcblist[i].tcb->recvBuf = malloc(RECEIVE_BUF_SIZE*sizeof(char));
		   tcblist[i].tcb->usedBufLen = 0;
		   //mutex manage ...
		   tcblist[i].tcb->bufMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
		   pthread_mutex_init(tcblist[i].tcb->bufMutex, NULL);
		   tcblist[i].tag = 1;
		   break;
	   }
	}
	if(i==TCB_MAX)  return -1;
    return i;
}


int stcp_client_connect(int sockfd, int nodeID, unsigned int server_port) {
	tcblist[sockfd].tcb->server_portNum = server_port;
	tcblist[sockfd].tcb->server_nodeID = nodeID;
	//initialize the seg_t to send ...
	seg_t *send_seg = malloc(sizeof(seg_t));
	send_seg->header.type = SYN;
	send_seg->header.src_port = tcblist[sockfd].tcb->client_portNum;
	send_seg->header.dest_port = server_port;
	send_seg->header.length = 200;
	sip_sendseg(sip_conn, nodeID, send_seg);

	printf("\nClient: +++  CLOSED ------>  SYNSENT\n");

	tcblist[sockfd].tcb->state = SYNSENT;
	printf("\n***********************\n");
	printf("srcport: %d  \n", send_seg->header.src_port);
	printf("destport: %d \n", send_seg->header.dest_port);
	printf("type: SYN\n");
	printf("***********************\n");

	int count;
	for(count=0; count < SYN_MAX_RETRY; count++){
		usleep(SYN_TIMEOUT/1000);
	    if(tcblist[sockfd].tcb->state == CONNECTED){
			printf("Client: Receive the SYNACK successfully ...\n");
	        return 1;
		}
	    else{
			    printf("Resend the SYN ...\n");
	        sip_sendseg(sip_conn, nodeID, send_seg);
		}
	}

	if(count == SYN_MAX_RETRY){
		printf("Client: Having tried to connect for many times, but failed ...\n");
		tcblist[sockfd].tcb->state = CLOSED;
	    return -1;
	}
	return 0;
}


int stcp_client_send(int sockfd, void* data, unsigned int length){
	int nodeID = tcblist[sockfd].tcb->server_nodeID;
	int count = length/MAX_SEG_LEN;
	int remain = length%MAX_SEG_LEN;
	if(data==NULL){
		printf("Warning: send NULL data ...\n");
	    return -1;
	}
	char *char_data = (char*)data;
	pthread_mutex_lock(tcblist[sockfd].tcb->bufMutex);
	if(tcblist[sockfd].tcb->sendBufHead == NULL){
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
			sip_sendseg(sip_conn, nodeID, &tt->seg);
		    tcblist[sockfd].tcb->unAck_segNum++;
		    tt = tt->next;
		}
		tcblist[sockfd].tcb->sendBufunSent = tt;
	}
	pthread_mutex_unlock(tcblist[sockfd].tcb->bufMutex);
	return 1;
}


int stcp_client_recv(int sockfd, void* buf, unsigned int length){
	while(1){
		pthread_mutex_lock(tcblist[sockfd].tcb->bufMutex);
		unsigned int recv_count = tcblist[sockfd].tcb->usedBufLen;
        if(recv_count >= length){
		   printf("Bingo ---  at sockfd %d --- recv data  length is %d...\n", sockfd, length);
			memcpy((char*)buf, tcblist[sockfd].tcb->recvBuf, length);
			if(recv_count > length){
				char temp[recv_count-length];
				memcpy(temp, tcblist[sockfd].tcb->recvBuf+length, recv_count-length);
				memset(tcblist[sockfd].tcb->recvBuf, 0, RECEIVE_BUF_SIZE);
			 	memcpy(tcblist[sockfd].tcb->recvBuf, temp, recv_count-length);				
			}
			else
				memset(tcblist[sockfd].tcb->recvBuf, 0, RECEIVE_BUF_SIZE);
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


int stcp_client_disconnect(int sockfd) {
	int nodeID = tcblist[sockfd].tcb->server_nodeID;
    seg_t *send_seg = malloc(sizeof(seg_t));
    send_seg->header.src_port = tcblist[sockfd].tcb->client_portNum;
    send_seg->header.dest_port = tcblist[sockfd].tcb->server_portNum;
    send_seg->header.type = FIN;
    send_seg->header.length = 0;
    sip_sendseg(sip_conn, nodeID, send_seg);
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
	    sip_sendseg(sip_conn, nodeID, send_seg);
      printf("Resend the FIN ...\n");
		}
	}
	if(count == FIN_MAX_RETRY){
			tcblist[sockfd].tcb->state = CLOSED;
			printf("Client: Having tried to close connect for many times, but failed ...\n");
	  	return -1;
	}
	return 0;
}


int stcp_client_close(int sockfd) {
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


void* sendBuf_timer(void* clienttcb){
	while(1){
		tcb_t* temp = (tcb_t*)((long)clienttcb);
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
			int nodeID = temp->server_nodeID;
            while(start != end){
			    gettimeofday(&start->sentTime, NULL);
			    sip_sendseg(sip_conn, nodeID, &start->seg);
		        start = start->next;
		    }
		}
		pthread_mutex_unlock(temp->bufMutex);
	}
}

void *seghandler(void* arg) {
	char buffer[sizeof(seg_t)];
	int src_nodeID;
	while(1){
		memset(buffer, 0, sizeof(seg_t));
		int test = sip_recvseg(sip_conn, &src_nodeID, (seg_t*)buffer);
		if(test == -1){
		      pthread_exit(NULL);
		}
		//
	    if(test == 1){
		    continue;
		}

		int server_port = ((seg_t*)buffer)->header.src_port;
		int client_port = ((seg_t*)buffer)->header.dest_port;
		int server_type = ((seg_t*)buffer)->header.type;
		int seq = ((seg_t*)buffer)->header.seq_num;
		int ack = ((seg_t*)buffer)->header.ack_num;
		int data_len = ((seg_t*)buffer)->header.length;

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
        if(server_type==DATA){
        	/*
        	 * debug ...
        	 */
        	printf(" \n\n\n*******   client recv data seq is %d-----expectnum is %d **********\n", seq, tcblist[index].tcb->expect_seqNum);
        	char c;
        	fgets(&c, 1, stdin);
        	
					pthread_mutex_lock(tcblist[index].tcb->bufMutex);
					if(tcblist[index].tcb->expect_seqNum == seq){
						if(tcblist[index].tcb->usedBufLen+data_len <= RECEIVE_BUF_SIZE){
							printf("Server: receive the data ok ...\n");
							memcpy(tcblist[index].tcb->recvBuf+tcblist[index].tcb->usedBufLen, ((seg_t*)buffer)->data, data_len);
							tcblist[index].tcb->usedBufLen += data_len;
							tcblist[index].tcb->expect_seqNum += data_len;
						    	seg_t* temp = (seg_t *)malloc(sizeof(seg_t));
					        temp->header.type = DATAACK;
					        temp->header.ack_num = tcblist[index].tcb->expect_seqNum;
					        temp->header.src_port = client_port;
					        temp->header.dest_port = server_port;
					        temp->header.length = 0;
					        sip_sendseg(sip_conn, tcblist[index].tcb->server_nodeID, temp);
					        
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
					    temp->header.src_port = client_port;
					    temp->header.dest_port = server_port;
					    temp->header.length = 0;

					    sip_sendseg(sip_conn, tcblist[index].tcb->server_nodeID, temp);
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

				if(server_type==DATAACK){
					pthread_mutex_lock(tcblist[index].tcb->bufMutex);

					printf("\n***********************\n");
				    printf("srcport: %d  \n", server_port);
				    printf("destport: %d \n", client_port);
				    printf("type: DATAACK\n");
				    printf("ack_num: %d \n", ack);
				    printf("***********************\n");
					segBuf_t *temp = tcblist[index].tcb->sendBufHead;

					if(temp==NULL) printf("the head is null\n");
					while(temp != NULL){
						printf(" *******   the Head seq num is %d\n", temp->seg.header.seq_num);
						if(temp->seg.header.seq_num < ack){
							printf("free the acked data --- the seq is %d...\n", temp->seg.header.seq_num);
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
						int nodeID = tcblist[index].tcb->server_nodeID;
						while(tcblist[index].tcb->unAck_segNum < GBN_WINDOW && tt != NULL){
						    gettimeofday(&(tt->sentTime), NULL);
			                sip_sendseg(sip_conn, nodeID, &tt->seg);
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




#include "seg.h"
#include "stdio.h"
#include "stdlib.h"

int sip_sendseg(int connection, int destnodeID, seg_t* segPtr)
{	
	if( send(connection, "!", 1, 0) == -1)   {  printf("errno=%d\n",errno); return -1; }
	if( send(connection, "&", 1, 0) == -1)   {  printf("errno=%d\n",errno); return -1; }
	//发送方是确定了数据的大小的
	
	segPtr->header.checksum = 0;
	segPtr->header.checksum = checksum(segPtr);	
	sendseg_arg_t temp;
	memcpy(&(temp.seg), segPtr, sizeof(seg_t));
	temp.nodeID = destnodeID;//DEST NODE ID
	if( send(connection, &temp, sizeof(sendseg_arg_t), 0) == -1)  return -1;
	if( send(connection, "!", 1, 0) == -1)    return -1;
	if( send(connection, "#", 1, 0) == -1)    return -1;
  return 1;
}

int sip_recvseg(int connection, int* src_nodeID, seg_t* segPtr)
{
	int state = SEGSTART1;
	int test = 0;
	int index = 0;
	char buffer[sizeof(sendseg_arg_t)];	
	char c;
	memset(buffer, 0, sizeof(sendseg_arg_t));
	//printf("recv 1 ....\n");
	
	while(1){
	  if(test==1) break;	  
    switch(state){
		case SEGSTART1:{
			if(recv(connection, &c, 1, 0)==-1) return -1;			
			if(c=='!')
			   state = SEGSTART2;
			break;
		}
	       
	    case SEGSTART2:{
			if(recv(connection, &c, 1, 0)==-1) return -1;
			if(c=='&')
			   state = SEGRECV;
			else
			   state = SEGSTART1;
			break;		
		}
		
	    case SEGRECV:{
			while(1){
				if(recv(connection, &c, 1, 0)==-1) return -1;
				if(c=='!'){
					state = SEGSTOP1;
				    break;
				}
				else
				    buffer[index++] = c;		       
		   }
		   break;   
		}
	
	    case SEGSTOP1:{
			recv(connection, &c, 1, 0);
			if(c == '#'){
			   test = 1;		   			   
		    }
		    if(c != '#'){
			   state = SEGRECV;
			   buffer[index++] = '!';
			   buffer[index++] = c;
			}
			break;
		}
	    default:{}
	  }
	 // printf("%c..", c);
    }
    
    int tt = seglost(segPtr);
    if(tt == 0){
		printf("the seg is not lost ...\n");
		int result = checkchecksum(&(((sendseg_arg_t *)buffer)->seg));
		
		if(result == 1){
			memcpy(segPtr, &(((sendseg_arg_t *)buffer)->seg), sizeof(seg_t));
			memcpy(src_nodeID, &(((sendseg_arg_t *)buffer)->nodeID), sizeof(int));
		  return 0;
		}
		else{
			printf("the seg is damaged ...\n");
			return 1;
		}
	}
	else{
		printf("the seg is partly lost ...\n");
	    return 1;    
	}	
}

//SIP进程使用这个函数接收来自STCP进程的包含段及其目的节点ID的sendseg_arg_t结构.
//参数stcp_conn是在STCP进程和SIP进程之间连接的TCP描述符.
//如果成功接收到sendseg_arg_t就返回1, 否则返回-1.
int getsegToSend(int stcp_conn, int* dest_nodeID, seg_t* segPtr)
{
	int state = SEGSTART1;
	int test = 0;
	int index = 0;
	char buffer[sizeof(sendseg_arg_t)];	
	char c;
	memset(buffer, 0, sizeof(sendseg_arg_t));
	while(1){
	  if(test==1) break;	  
      switch(state){
		case SEGSTART1:{
			if(recv(stcp_conn, &c, 1, 0)==-1) return -1;
			if(c=='!')
			   state = SEGSTART2;
			break;
		}
	       
	    case SEGSTART2:{
			if(recv(stcp_conn, &c, 1, 0)==-1) return -1;
			if(c=='&')
			   state = SEGRECV;
			else
			   state = SEGSTART1;
			break;		
		}
		
	    case SEGRECV:{
			while(1){
				if(recv(stcp_conn, &c, 1, 0)==-1) return -1;
				if(c=='!'){
					state = SEGSTOP1;
				    break;
				}
				else
				    buffer[index++] = c;		       
		   }
		   break;   
		}
	
	    case SEGSTOP1:{
			recv(stcp_conn, &c, 1, 0);
			if(c == '#'){
			   test = 1;		   			   
		    }
		    if(c != '#'){
			   state = SEGRECV;
			   buffer[index++] = '!';
			   buffer[index++] = c;
			}
			break;
		}
	    default:{}
	  }
    }
    memcpy(segPtr, &(((sendseg_arg_t *)buffer)->seg), sizeof(seg_t));
	memcpy(dest_nodeID, &(((sendseg_arg_t *)buffer)->nodeID), sizeof(int));
    return 1;
}

//SIP进程使用这个函数发送包含段及其源节点ID的sendseg_arg_t结构给STCP进程.
//参数stcp_conn是STCP进程和SIP进程之间连接的TCP描述符.
//如果sendseg_arg_t被成功发送就返回1, 否则返回-1.
int forwardsegToSTCP(int stcp_conn, int src_nodeID, seg_t* segPtr)
{
	sendseg_arg_t temp;
	segPtr->header.checksum = 0;
	segPtr->header.checksum = checksum(segPtr);
	temp.nodeID = src_nodeID;
	memcpy(&(temp.seg), segPtr, sizeof(seg_t));
  if( send(stcp_conn, "!", 1, 0) == -1)    return -1;
	if( send(stcp_conn, "&", 1, 0) == -1)    return -1;
	if( send(stcp_conn, &temp, sizeof(sendseg_arg_t), 0) == -1)  return -1;
	if( send(stcp_conn, "!", 1, 0) == -1)    return -1;
	if( send(stcp_conn, "#", 1, 0) == -1)    return -1;
  return 1;
}

int seglost(seg_t* segPtr) {
	int random = rand()%100;
	if(random<PKT_LOSS_RATE*100) {
		//50%可能性丢失段
		if(rand()%2==0) {
			printf("seg lost!!!\n");
                        return 1;
		}
		//50%可能性是错误的校验和
		else {
			//获取数据长度
			int len = sizeof(stcp_hdr_t)+segPtr->header.length;
			//获取要反转的随机位
			int errorbit = rand()%(len*8);
			//反转该比特
			char* temp = (char*)segPtr;
			temp = temp + errorbit/8;
			*temp = *temp^(1<<(errorbit%8));
			return 0;
		}
	}
	return 0;
}

unsigned short checksum(seg_t* segment){
	unsigned short *buf = (unsigned short *)segment;
	unsigned short count;
	count = segment->header.length + 24;
    long sum=0;
    int size_s = sizeof(unsigned short int);
	while(count > 1){
		sum += *buf++;
		count -= size_s;
	}

    if(count > 0){
    	sum += *(unsigned char*)buf;
    }

    while(sum>>16)
    	sum = (sum & 0xFFFF) + (sum >> 16);

    return (unsigned short) (~sum);
}



int checkchecksum(seg_t* segment){
	if(checksum(segment))
		return -1;
	else 
		return 1;
}

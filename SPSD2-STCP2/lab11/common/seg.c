//
// 文件名: seg.c

// 描述: 这个文件包含用于发送和接收STCP段的接口sip_sendseg() and sip_rcvseg(), 及其支持函数的实现. 
//
// 创建日期: 2013年1月
//

#include "seg.h"
#include "stdio.h"

//
//
//  用于客户端和服务器的SIP API 
//  =======================================
//
//  我们在下面提供了每个函数调用的原型定义和细节说明, 但这些只是指导性的, 你完全可以根据自己的想法来设计代码.
//
//  注意: sip_sendseg()和sip_recvseg()是由网络层提供的服务, 即SIP提供给STCP.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

// 通过重叠网络(在本实验中，是一个TCP连接)发送STCP段. 因为TCP以字节流形式发送数据, 
// 为了通过重叠网络TCP连接发送STCP段, 你需要在传输STCP段时，在它的开头和结尾加上分隔符. 
// 即首先发送表明一个段开始的特殊字符"!&"; 然后发送seg_t; 最后发送表明一个段结束的特殊字符"!#".  
// 成功时返回1, 失败时返回-1. sip_sendseg()首先使用send()发送两个字符, 然后使用send()发送seg_t,
// 最后使用send()发送表明段结束的两个字符.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int sip_sendseg(int connection, seg_t* segPtr)
{
	if( send(connection, "!", 1, 0) == -1)    return -1;
	if( send(connection, "&", 1, 0) == -1)    return -1;
	//发送方是确定了数据的大小的
	segPtr->header.checksum = 0;
	segPtr->header.checksum = checksum(segPtr);
	if( send(connection, segPtr, segPtr->header.length+24, 0) != (segPtr->header.length+24) )  return -1;
	if( send(connection, "!", 1, 0) == -1)    return -1;
	if( send(connection, "#", 1, 0) == -1)    return -1;
    return 1;
}

// 通过重叠网络(在本实验中，是一个TCP连接)接收STCP段. 我们建议你使用recv()一次接收一个字节.
// 你需要查找"!&", 然后是seg_t, 最后是"!#". 这实际上需要你实现一个搜索的FSM, 可以考虑使用如下所示的FSM.
// SEGSTART1 -- 起点 
// SEGSTART2 -- 接收到'!', 期待'&' 
// SEGRECV -- 接收到'&', 开始接收数据
// SEGSTOP1 -- 接收到'!', 期待'#'以结束数据的接收
// 这里的假设是"!&"和"!#"不会出现在段的数据部分(虽然相当受限, 但实现会简单很多).
// 你应该以字符的方式一次读取一个字节, 将数据部分拷贝到缓冲区中返回给调用者.
//
// 注意: 在你剖析了一个STCP段之后,  你需要调用seglost()来模拟网络中数据包的丢失. 
// 在sip_recvseg()的下面是seglost()的代码.
// 
// 如果段丢失了, 就返回1, 否则返回0.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
int sip_recvseg(int connection, seg_t* segPtr)
{
	int state = SEGSTART1;
	int test = 0;
	int index = 0;
	char buffer[sizeof(seg_t)];	
	char c;
	memset(buffer, 0, sizeof(seg_t));
	while(1){
	  if(test==1) break;	  
      switch(state){
		case SEGSTART1:{
			//printf("a");	
			if(recv(connection, &c, 1, 0)==-1) return -1;
			if(c=='!')
			   state = SEGSTART2;
			break;
		}
	       
	    case SEGSTART2:{
			//printf("sip_recvseg state 2\n");
			if(recv(connection, &c, 1, 0)==-1) return -1;
			if(c=='&')
			   state = SEGRECV;
			else
			   state = SEGSTART1;
			break;		
		}
		
	    case SEGRECV:{
			//printf("sip_recvseg state 3\n");
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
			   //printf("receive all the message \n");
			   test = 1;		   			   
		    }
		    if(c != '#'){
			   state = SEGRECV;
			   buffer[index++] = '!';
			   buffer[index++] = c;
			}
			break;
		}
	    default:{
		    //printf("The state is wrong ...\n");
		}
	  }
    }
    //printf("%x\n", ((seg_t *)buffer)->header.checksum);
    int tt = seglost(segPtr);
    if(tt == 0){
		printf("the seg is complete ...\n");
		int result = checkchecksum((seg_t *)buffer);
		
		if(result == 1){
		    memcpy(segPtr, buffer, sizeof(seg_t));
		    return 0;
		}
		else{
			printf("the seg is damaged ...\n");
			return 1;
		}
	}
	else if(tt ==1){
		printf("the seg is partly lost ...\n");
	    return 1;    
	}
	
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



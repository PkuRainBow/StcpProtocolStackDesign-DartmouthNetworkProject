/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"

//typedef unsigned char byte
//丢弃分组函数
extern void ip_DiscardPkt(char* pBuffer,int type);
//发送分组函数
extern void ip_SendtoLower(char*pBuffer,int length);
//上层接收函数
extern void ip_SendtoUp(char *pBuffer,int length);
//获取本机IPV4地址函数
extern unsigned int getIpv4Address();

// implemented by students

typedef struct IPv4{
	byte Version;
	byte IHL;
	unsigned short len;
	byte ttl;
	byte protocol;
	unsigned short checksum;
	unsigned int Saddr;
	unsigned int Daddr;
};

//RFC的实现
//不需要字节序转换？？？？
unsigned short checksum(unsigned short * buf, unsigned short count){
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
//接收函数
int stud_ip_recv(char *pBuffer,unsigned short length)
{
    struct IPv4 header;
    //C语言的移位运算设计 char*可以当做char数组进行处理
    header.Version = (pBuffer[0]>>4) & 0x0f;
    header.IHL = pBuffer[0] & 0x0f;
    header.len = ntohs(*((unsigned short *)(pBuffer+2)));
    header.ttl = pBuffer[8];
    header.protocol = pBuffer[9];
    header.checksum = ntohs(*(unsigned short *)(pBuffer+10));
    header.Saddr = ntohl(*(unsigned int *)(pBuffer+12));
    header.Daddr = ntohl(*(unsigned int *)(pBuffer+16));

    printf("*****************  Packet  *****************\n");
    printf("Version is %d\n", header.Version);
    printf("IHL is %d\n", header.IHL);
    printf("len is %d\n", header.len);
    printf("ttl is %d\n", header.ttl);
    printf("protocol is %d\n", header.protocol);
    printf("test check is %d\n", checksum((unsigned short *)pBuffer, 4*header.IHL));
    //printf("Saddr is %d\n", header.Saddr);
    // printf("Daddr is %d\n", header.Daddr);

    if(header.Version != 4){
    	ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
      printf("version error\n\n");
    	return 1;
    }

    if (header.IHL < 5 || header.IHL > 15){
    	ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
      printf("IHL error\n\n");
    	return 1;
    }

    if(header.ttl <= 0){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
            printf("ttl error\n\n");
		return 1;
    }

    // if (header.len < 20 || header.len > 65535){
    	//	ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
    	//	return 1;
    //}
    if(checksum((unsigned short *)pBuffer, 4*header.IHL) != 0){
    	ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
      printf("check error\n\n");
		return 1;
    }
    //目标地址正确分为两种情况，一是是本机ip地址，二是是广播地址oxFFFF
	if (header.Daddr != getIpv4Address() && header.Daddr != 0xffffffff)
	{
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
            printf("Daddr error\n\n");
		return 1;
	}

	ip_SendtoUp(pBuffer+4*header.IHL, length-4*header.IHL);
      return 0;
}

//发送函数
int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
				   unsigned int dstAddr,byte protocol,byte ttl)
{
    char *send = new char[20+len];
    memset(send, 0, len+20);
    send[0] = 0x45;
    *((unsigned short*)(send+2)) = htons(len+20);
	*((unsigned int*)(send+12)) = htonl(srcAddr);
	*((unsigned int*)(send+16)) = htonl(dstAddr);
	send[8] = ttl;
	send[9] = protocol;
	*((unsigned short*)(send+10)) = checksum((unsigned short *)send, 20);
	memcpy(send+20, pBuffer, len);
	ip_SendtoLower(send, len+20);
	return 0;
}

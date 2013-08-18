/*
 * 101220151_lab11.cpp
 *
 *  Created on: 2013-5-6
 *      Author: njucsyyh
 */
/*
* THIS FILE IS FOR TCP TEST
*/

/*
struct sockaddr_in {
        short   sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
};
*/

#include "sysInclude.h"
//define all the state
#define CLOSED 1
#define SYN_SENT 2
#define ESTABLISHED 3
#define FIN_WAIT1 4
#define FIN_WAIT2 5
#define TIME_WAIT 6
//user defined var
int gSrcPort = 2008;
int gDstPort = 2009;
int gSeqNum = 1234;
int gAckNum = 0;
int socknum = 80;

extern void tcp_DiscardPkt(char *pBuffer, int type);
extern void tcp_sendReport(int type);
extern void tcp_sendIpPkt(unsigned char *pData, UINT16 len, unsigned int  srcAddr, unsigned int dstAddr, UINT8	ttl);
extern int waitIpPacket(char *pBuffer, int timeout);
extern unsigned int getIpv4Address();
extern unsigned int getServerIpv4Address();

typedef struct TCPHEAD tcphead, *p_tcphead;
typedef struct PSEUDO_HEAD pseudo_head;
typedef struct TCP_CHECK_BUF tcp_check_buf;
typedef struct TCB tcb, *p_tcb;
typedef struct TCBNODE tcbnode, *p_tcbnode;

struct TCPHEAD{
	unsigned short src_port;
	unsigned short dst_port;
	unsigned int seq;
	unsigned int ack;
	//THL covers the field of reserved field
	unsigned char THL;
	unsigned char flags;
	unsigned short window_size;
    unsigned short check_sum;
    unsigned short urgent_pointer;
    unsigned char data[256];
};

struct PSEUDO_HEAD{
	unsigned long src_addr;
	unsigned long dst_addr;
	unsigned char mbz;
	unsigned char protocol;
	unsigned short tcplen;
};

struct TCP_CHECK_BUF{
	pseudo_head pheader;
	tcphead theader;
};
/*
 * TCB�ṹ
 */
struct TCB{
	unsigned short sockfd;  //�׽���������
	unsigned char state;    //״̬��
	unsigned int seq;       //���к�
	unsigned int ack;       //ȷ�Ϻ�
	unsigned int window_size;    //�������ڴ�С
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned short src_port;
	unsigned short dst_port;
};


struct TCBNODE{
	p_tcb tcb;
	p_tcbnode next;
};

p_tcb tcb_now = NULL;
p_tcbnode tcb_list = NULL;

extern unsigned short checksum(unsigned short * buf, unsigned short count);
//extern unsigned short tcp_v4_check(unsigned char* ...);
extern p_tcb get_tcb_port(unsigned short src_port, unsigned short dst_port);
extern p_tcb get_tcb_sockfd(unsigned int sockfd);
extern int FSM(unsigned char flags, p_tcphead tcp, p_tcb tcb);
//extra function
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

p_tcb get_tcb_port(unsigned short src_port, unsigned short dst_port){
	p_tcbnode head = tcb_list;
	while(head != NULL){
		if(head->tcb->src_port == src_port && head->tcb->dst_port == dst_port)
			return  head->tcb;
		head = head->next;
	}
	return NULL;
}

p_tcb get_tcb_sockfd(unsigned int sockfd){
	p_tcbnode head = tcb_list;
	while(head != NULL){
		if(head->tcb->sockfd == sockfd)
			return  head->tcb;
		head = head->next;
	}
	return NULL;
}

int FSM(unsigned char flag, p_tcphead tcp, p_tcb tcb){
      unsigned char flags;
	if(tcp != NULL)   flags = tcp->flags;
	char state = tcb->state;
	printf("The FSM state is %d...\n ", tcb->state);
	switch(state)
	{
	case CLOSED:{
		if( (flag & 0x02) == 2)   // SYN
		{
			tcb->state = SYN_SENT;
			printf("   CLOSED  -------->  SYN_SENT ...\n");
			return 0;
	    }
	    return -1;
	}

	case SYN_SENT:{
		if( (flags & 0x12) == 18 ){  //SYN ACK
			if( ntohl(tcp->ack) != tcb->seq+1){
				printf("SEQ number error ...\n");
				tcp_DiscardPkt((char *)tcp, STUD_TCP_TEST_SEQNO_ERROR);
				return -1;
			}
			tcb->state = ESTABLISHED;
			tcb->ack = ntohl(tcp->seq)+1;
			tcb->seq = ntohl(tcp->ack);
			printf("   SYN_SENT  ------->  ESTABLISHED ...\n");
			stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, tcb->src_port, tcb->dst_port, getIpv4Address(), getServerIpv4Address());
			return 0;
	     }
		 return -1;
	}

	case ESTABLISHED:{
		if((flag & 0x01) == 1)   //FIN
		{
			tcb->state = FIN_WAIT1;
			printf("   ESTABLISHED  ------>  FIN-WAIT1 ...\n");
			return 0;
		}
		return -1;
	}

	case FIN_WAIT1:{
		if( (flags & 0x10) == 16){  // ACK
			if( ntohl(tcp->ack) != tcb->seq+1){
				printf("SEQ number error ...\n");
				tcp_DiscardPkt((char *)tcp, STUD_TCP_TEST_SEQNO_ERROR);
				return -1;
			}
			tcb->state = FIN_WAIT2;
			tcb->ack = ntohl(tcp->seq)+1;
			printf("   FIN_WAIT1  ------>  FIN_WAIT2 ...\n");
			return 0;
		}
		return -1;
	}

	case FIN_WAIT2:{
		if ((flags & 0x01) == 1)		//FIN
		{
			if (ntohl(tcp->ack) != tcb->seq+1)
			{
				printf("SEQ number error.\n");
				tcp_DiscardPkt((char *)tcp, STUD_TCP_TEST_SEQNO_ERROR);
				return -1;
			}
			tcb->seq = ntohl(tcp->ack);
			tcb->ack = ntohl(tcp->seq) + 1;
			printf("   FIN_WAIT2  ------>  TIME_WAIT ...\n");
			tcb->state = TIME_WAIT;
			stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, tcb->src_port, tcb->dst_port, getIpv4Address(), getServerIpv4Address());
			return 0;
		}
		return -1;
	}

	default: return -1;
	}
}

/*
 * *********************************** ��Ի�������TCP���ĶεĽ��պʹ��� ���� **********************************************
 */
/*
 * TCPЭ��Ľ��մ���
 */
int stud_tcp_input(char *pBuffer, unsigned short len, unsigned int srcAddr, unsigned int dstAddr){
    //tcp head
	p_tcphead temp_tcp = (p_tcphead)(pBuffer);
    memset((temp_tcp->data + (len-20)), 0, 256-(len-20));

    pseudo_head *p_header = new pseudo_head;
    p_header->src_addr = srcAddr;
    p_header->dst_addr = dstAddr;
    p_header->mbz = 0;
    p_header->protocol = IPPROTO_TCP;
    p_header->tcplen = htons(len);
    tcp_check_buf *buffer = new tcp_check_buf;
    buffer->pheader = *p_header;
    buffer->theader = *temp_tcp;

    if(checksum((unsigned short*)buffer, sizeof(*buffer)) != 0){
    	printf("warning: checksum is wrong ...\n");
    	return -1;
    }

    tcb_now = get_tcb_port(ntohs(temp_tcp->dst_port), ntohs(temp_tcp->src_port));

	return FSM(0, temp_tcp, tcb_now);
}
/*
 * TCPЭ��ķ�װ����
 */
void stud_tcp_output(char *pData, unsigned short len, unsigned char flag, unsigned short srcPort, unsigned short dstPort, unsigned int srcAddr, unsigned int dstAddr)
{
	if( (tcb_now = get_tcb_port(srcPort, dstPort)) == NULL){
		int sockfd = stud_tcp_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		tcb_now = get_tcb_sockfd(sockfd);
		tcb_now->src_addr = srcAddr;
		tcb_now->dst_addr = dstAddr;
		tcb_now->src_port = srcPort;
		tcb_now->dst_port = dstPort;
	}

	if(tcb_now->window_size <= 0) return;
	else{
		//pseudo TCP header
		pseudo_head* p_head = new pseudo_head;
		p_head->src_addr=ntohl(srcAddr);
		p_head->dst_addr=ntohl(dstAddr);
		p_head->mbz=0;
		p_head->protocol = IPPROTO_TCP;
		p_head->tcplen = ntohs(20+len);
		//TCP
		p_tcphead tcphdr= new tcphead;
		memset(tcphdr, 0, sizeof(tcphead));
		for(int i=0; i<len; i++)
			tcphdr->data[i] = pData[i];

		tcphdr->src_port = ntohs(srcPort);
		tcphdr->dst_port = ntohs(dstPort);
		tcphdr->seq = ntohl(tcb_now->seq);
	    tcphdr->ack = ntohl(tcb_now->ack);
		tcphdr->THL = 0x50;
		tcphdr->flags = flag;
		tcphdr->window_size = ntohs(30);
		tcphdr->urgent_pointer = ntohs(0);

		tcp_check_buf* check_buf = new tcp_check_buf;
		check_buf->pheader = *p_head;
		check_buf->theader = *tcphdr;

        tcphdr->check_sum = checksum((unsigned short*)check_buf, sizeof(tcp_check_buf));
        //ת��״̬�ֶΣ��޸�TCB�еļ�¼��Ϣ
        FSM(flag, NULL, tcb_now);
        //����tcp����
        tcp_sendIpPkt((unsigned char*)tcphdr, 20+len, srcAddr, dstAddr, 50);//
	}
}

/*
 * *************************************  socket  API  *************************************
 */

/*
 * domain:  AF_INET
 * type: SOCK_STREAM
 * protocol: IPPROTO_TCP
 */
int stud_tcp_socket(int domain, int type, int protocol)
{
	tcb_now = new tcb;
	//��ֵ�õ�Ψһ��socket������
	tcb_now->sockfd = socknum++;
	//��ʼ״̬�� CLOSED
	tcb_now->state = CLOSED;
	//����Ψһ�Ķ˿ں�
	tcb_now->src_port = gSrcPort++;
	tcb_now->dst_port = gDstPort++;
	//��ȡ���͵�Դ��ַ��Ŀ���ַ
	tcb_now->src_addr = getIpv4Address();
	tcb_now->dst_addr = getServerIpv4Address();
	//
	tcb_now->seq = gSeqNum++;
	tcb_now->ack = gAckNum;
	//ͣ��Э�鴰�ڴ�СΪ1
	tcb_now->window_size = 1;
	//��ͷ������tcb�ڵ�
	tcbnode* temp = new tcbnode;
	temp->tcb = tcb_now;
	temp->next = tcb_list;
	tcb_list = temp;
	//���ش����ɹ���TCP socket������
    return tcb_now->sockfd;
}
/*
 * TCP�������Ӻ���
 * ����SYN���Ĳ�����wait_IpPacket()�������SYN_ACK���ģ�������
 * ACK�Σ�����TCP����
 */
int stud_tcp_connect(int sockfd, struct sockaddr_in *addr, int addrlen)
{

	if( (tcb_now = get_tcb_sockfd(sockfd)) != NULL){
		//�趨Ŀ��IP�Ͷ˿ں�
		tcb_now->dst_addr = ntohl(addr->sin_addr.s_addr);
		tcb_now->dst_port = ntohs(addr->sin_port);
        //��һ������
		stud_tcp_output(NULL,0,PACKET_TYPE_SYN, tcb_now->src_port,
				tcb_now->dst_port, tcb_now->src_addr, tcb_now->dst_addr);
		int len = 0;
		p_tcphead tcphdr = new tcphead;
        //���յ���TCPͷ��������ϢΪ20��������Ҫ�ٴ��������ܷ��ص����ݶ�
		//�ڶ�������
		while(len < 20){
			len = waitIpPacket((char*)tcphdr, 500);
		}
        //�յ�ȷ����Ϣ֮��
		//�ڵ���FSM()�����л���е���������
		return stud_tcp_input((char*)tcphdr, len,
				addr->sin_addr.s_addr,htonl(getIpv4Address()));
		//�����3�����֣�����
	}
	else{
		printf("NO EXIST ...\n");
		return -1;
	}
	return 0;
}

/*
 * TCP �η��ͺ���
 * ���ȴ��������˵�ACK
 */
int stud_tcp_send(int sockfd, const unsigned char *pData, unsigned short datalen, int flags)
{
	if( (tcb_now = get_tcb_sockfd(sockfd)) == NULL){
		printf("NO EXIST ...\n");
		return -1;
	}
	if(tcb_now->state == ESTABLISHED){
		//�������ݱ��Ķ�
		stud_tcp_output((char *)pData, datalen, PACKET_TYPE_DATA, tcb_now->src_port,
				tcb_now->dst_port, tcb_now->src_addr, tcb_now->dst_addr);
		p_tcphead tcphdr = new tcphead;
		//ÿ�η��ͱ��Ķ�Ҫ�ȴ����յ�ACK�ſ��Է�����һ�����Ķ�
		//��������ͣ��Э���Ҫ��
		int len = 0;
		while(len < 20){
			len = waitIpPacket((char*)tcphdr, 500);
		}
        //�ж��յ������ݱ�����ACK����RST����������Ϣ
		if(tcphdr->flags == PACKET_TYPE_ACK){
			if(ntohl(tcphdr->ack) != (tcb_now->seq+datalen)){
				printf("SEQ NUMBER ERROR ...\n");
				tcp_DiscardPkt((char *)tcphdr, STUD_TCP_TEST_SEQNO_ERROR);
				return -1;
			}
			tcb_now->seq = ntohl(tcphdr->ack);
			tcb_now->ack = ntohl(tcphdr->seq) + len-20;
		}
		return 0;
	}

	return -1;
}

/*
 * TCP�ν��պ���
 * �������Է�����������
 * ���ظ�ȷ�ϵ�ACK
 */
int stud_tcp_recv(int sockfd, unsigned char *pData, unsigned short datalen, int flags)
{
	if( (tcb_now = get_tcb_sockfd(sockfd)) == NULL){
		printf("warning:��NO EXIST ...\n");
		return -1;
	}
	if(tcb_now->state == ESTABLISHED){
		//�ȴ����շ��͵ı���
		int len = 0;
		p_tcphead tcphdr = new tcphead;
		while(len < 20){
			len = waitIpPacket((char*)tcphdr, 500);
		}
		tcb_now->ack = ntohl(tcphdr->seq)+len-20;
		memcpy(pData, (unsigned char*)tcphdr+20, len-20);
		//����ACK����������ȷ���յ��˷���������������
		stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, tcb_now->src_port,
				tcb_now->dst_port, tcb_now->src_addr, tcb_now->dst_addr);

		return 0;
	}
	return -1;
}

/*
 * TCP�ر����ӵĺ���
 * 4������
 */

int stud_tcp_close(int sockfd)
{
	if( (tcb_now = get_tcb_sockfd(sockfd)) == NULL ){
		printf("NO EXIST ...\n");
		return -1;
	}
    //ֻ����ESTABLISHED ״̬�²ſ��Թر�����
	if(tcb_now->state == ESTABLISHED){
		//��һ�����֣����͹ر����ӵı���,״̬��Ϊ  FIN_WAIT1
		stud_tcp_output(NULL, 0, PACKET_TYPE_FIN_ACK, tcb_now->src_port,
				tcb_now->dst_port, tcb_now->src_addr, tcb_now->dst_addr);
		//�ڶ������֣��ȴ���ȥ�䷢��ACK
		int len;
		p_tcphead tcphdr = new tcphead;
		while(len < 20){
			len = waitIpPacket((char*)tcphdr, 500);
		}
        //���մ���ACK���ģ�����״̬���л��� FIN_WAIT2
		stud_tcp_input((char *)tcphdr, len, htonl(tcb_now->dst_addr),
				htonl(tcb_now->src_addr));
        //�ȴ����շ������˵�FIN����
        len = 0;
		while(len < 20){
			len = waitIpPacket((char*)tcphdr, 500);
		}
        //������յ���FIN���ģ�״̬��Ϊ  CLOSED ������ACK����
		stud_tcp_input((char *)tcphdr, len, htonl(tcb_now->dst_addr),
				htonl(tcb_now->src_addr));
        //���ɾ����Ӧsockfd��TCB
		tcbnode *p = tcb_list;
		tcbnode *q = tcb_list->next;
		while(q != NULL){
			if(q->tcb->sockfd == sockfd){
				p->next = q->next;
				delete q;
				break;
			}
			p = q;
			q = q->next;
		}
		return 0;
	}
	//��ǰ״̬����ESTABLISHED��������ǲ����Ե��� close()�����ģ����򽫵�ǰTCB�ṹɾ��
	else{
		tcbnode *p = tcb_list;
		tcbnode *q = tcb_list->next;
		while(q != NULL){
			if(q->tcb->sockfd == sockfd){
				p->next = q->next;
				delete q;
				break;
			}
			p = q;
			q = q->next;
		}
	}
	return -1;
}

#include "sysinclude.h"

extern unsigned int getMNHaddress();
extern unsigned int getHAHaddress();
extern unsigned int getFAHaddress();
extern void submitCareofadd(unsigned int careofadd);
extern bool isMNaddressRegister(unsigned int address);
extern int icmp_sendIpPkt(unsigned char* pData, unsigned short len);
extern int MN_regi_sendIpPkt(unsigned char* pData, unsigned short len);
extern int FA_sendregi_rep(char* pData,unsigned short len,unsigned int destadd,unsigned int srcadd);
extern int FA_sendregi_req(char* pData,unsigned short len,unsigned int destadd,unsigned int srcadd);
extern int HA_regi_sendIpPkt(unsigned char type,unsigned char code,unsigned int destadd,unsigned int srcadd,unsigned int Mn_addr,unsigned int Ha_addr);
extern int HA_send_Encap_packet(char* pbuffer,unsigned short len,unsigned int dstAddr,unsigned int srcAddr);
extern int FA_send_ipv4_toMN(char *pBuffer, int length);
extern int FA_forward_ipv4packet(char *pBuffer, int length);
extern int HA_forward_ipv4packet(char *pBuffer, int length);
extern void HA_DiscardPkt(char * pBuffer, int length);
extern void FA_DiscardPkt(char * pBuffer, int length);

extern struct  stud_MN_route_node *g_MN_route_table;
extern struct  stud_FA_route_node *g_FA_route_table;
extern struct  stud_HA_route_node *g_HA_route_table;

extern unsigned short checksum(unsigned short * buf, unsigned short count);
extern void MN_add(unsigned int dest, unsigned int mask, unsigned int nexthop, unsigned int if_no);
extern void FA_add(unsigned int dest, unsigned int mask, unsigned int nexthop, unsigned int if_no);
extern void HA_add(unsigned int dest, unsigned int mask, unsigned int nexthop, unsigned int if_no);
extern unsigned short checksum(unsigned short * buf, unsigned short count);

//8字节的ICMP请求消息
typedef struct ICMP_req{
	unsigned char type;
	unsigned char code;
	unsigned short checksum;
	unsigned int reserved;
}* icmpheader;

//注册请求消息
typedef struct Regi_req{
	unsigned char type;
	//特殊标志字段（略去）
	unsigned char TAG;
	unsigned short ttl;
	//家乡地址
	unsigned int MN_addr;
	//家乡代理
	unsigned int HA_addr;
	//外地代理
	unsigned int FA_addr;
	//标识
	unsigned int iden_low;
	unsigned int iden_high;
}* request;

//注册应答消息
typedef struct Regi_rep{
	unsigned char type;
	unsigned char code;
	unsigned short ttl;
	unsigned int MN_addr;
	unsigned int HA_addr;
	unsigned int iden_low;
	unsigned int iden_high;
}* reply;

//记录移动节点的网络掩码，接收该报文的接口号
static int mask_global;
static int port_global;

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

//移动节点路由表添加函数
void MN_add(unsigned int dest, unsigned int mask, unsigned int nexthop, unsigned int if_no){
	stud_MN_route_node *route = new stud_MN_route_node;
	route->dest = dest;
	route->mask = mask;
	route->nexthop = nexthop;
	route->if_no = if_no;
	route->next = NULL;
	//插入节点
	if(NULL == g_MN_route_table)  g_MN_route_table = route;
	else{
		stud_MN_route_node *head = g_MN_route_table;
		while(head->next != NULL)   head = head->next;
		head->next = route;
	}
}
//家乡代理路由表项添加函数
void HA_add(unsigned int dest, unsigned int mask, unsigned int nexthop, unsigned int if_no){
	stud_HA_route_node *route = new stud_HA_route_node;
	route->dest = dest;
	route->mask = mask;
	route->nexthop = nexthop;
	route->if_no = if_no;
	route->next = NULL;
	//插入节点
	if(NULL == g_HA_route_table)  g_HA_route_table = route;
	else{
		stud_HA_route_node *head = g_HA_route_table;
		while(head->next != NULL)   head = head->next;
		head->next = route;
	}
}
//外地代理路由表项添加函数
void FA_add(unsigned int dest, unsigned int mask, unsigned int nexthop, unsigned int if_no){
	stud_FA_route_node *route = new stud_FA_route_node;
	route->dest = dest;
	route->mask = mask;
	route->nexthop = nexthop;
	route->if_no = if_no;
	route->next = NULL;
	//插入节点
	if(NULL == g_FA_route_table)  g_FA_route_table = route;
	else{
		stud_FA_route_node *head = g_FA_route_table;
		while(head->next != NULL)   head = head->next;
		head->next = route;
	}
}
/**********************************************/
//移动节点发送代理请求报文函数
int stud_MN_icmp_send()
{
	icmpheader buf = new ICMP_req;
	memset(buf, 0, sizeof(ICMP_req));
	buf->type = 10;
	buf->code = 0;
	//结构体指针类型转换限制
	//ICMP报文的校验和字段计算方法是将高16位和低16位相加即可，需要交换字节序
	buf->checksum = 0xFFF5;
	buf->reserved = htonl(0);
	//发送ICMP请求报文，从ICMP首部开始，字节序是网络字节序。
	icmp_sendIpPkt((unsigned char*)buf, sizeof(ICMP_req));
	return 0;
}

//移动节点接收代理应答报文函数
int stud_MN_icmp_recv(char *buffer,unsigned short  len)
{
	//ipheader buf = (ipheader)buffer;
	unsigned int host_addr = getHAHaddress();
	unsigned int src_addr = ntohl(*( (int*)(buffer+12) ));
	unsigned int dest_addr = ntohl(*( (int*)(buffer+16) ));
	//收到家乡代理的应答
	if(src_addr == host_addr)
		return 2;
	//收到外地代理的应答
	else{
        submitCareofadd(src_addr);
		return 1;
	}
	return 0;
}

//移动节点发送注册请求报文函数
 int stud_MN_send_Regi_req(char*pdata,unsigned char len,unsigned short ttl,unsigned int HA_addr,
		 unsigned int FA_addr,unsigned int MN_Haddr,unsigned int iden_low,unsigned int iden_high)
{
	request buf = new struct Regi_req;
	memset(buf, 0, 24);
	buf->type = 1;
	buf->TAG = 0;
	buf->ttl = htons(ttl);
	buf->HA_addr = htonl(HA_addr);
	buf->FA_addr = htonl(FA_addr);
	buf->MN_addr = htonl(MN_Haddr);
	buf->iden_high = htonl(iden_high);
	buf->iden_low = htonl(iden_low);
	char *buffer = new char[24+len];
	memcpy(buffer, buf, 24);
	memcpy(buffer+24, pdata, len);
	//发送移动节点注册请求消息的系统函数
	MN_regi_sendIpPkt( (unsigned char*)buffer, len+24);
	return 0;
}

//移动节点接收注册应答报文函数
int stud_MN_recv_Regi_rep(char *pbuffer,unsigned short len,unsigned int if_no)
{
	reply buf = (reply)(pbuffer+28);
	if(buf->code != 1 && buf->code != 0)
		return 1;
	//注册成功后，默认路由  只需要指定下一跳就可以了
	//在哪里使用了默认网关？？？
    MN_add(0, 0, getFAHaddress(), if_no);
	return 0;
}

//外地代理接收注册请求报文函数
int stud_FA_recv_Regi_req(char *pbuffer,unsigned short len, unsigned int mask,unsigned int if_no)
{
	request buf = (request)(pbuffer+28);
	int HA_addr = ntohl(buf->HA_addr);
	int FA_addr = ntohl(buf->FA_addr);
	int MN_addr = ntohl(buf->MN_addr);
	mask_global = mask;
	port_global = if_no;
	FA_sendregi_req(pbuffer+28, len-28, HA_addr, FA_addr);
	return 0;
}

//外地代理接收注册应答报文函数
int stud_FA_recv_Regi_rep(char *pbuffer,unsigned short len)
{
    reply buf = (reply)(pbuffer+28);
    //ipheader head = (ipheader)pbuffer;
    unsigned int dest_addr = ntohl(*( (int*)(pbuffer+16) ));
    int MNaddr = ntohl(buf->MN_addr);
    int FAaddr = getFAHaddress();
    if(buf->code==0 || buf->code==1){
    	FA_add(MNaddr, mask_global, MNaddr, port_global);
    	FA_sendregi_rep((pbuffer+28), len-28, MNaddr, FAaddr);
    	return 0;
    }
    else{
    	FA_sendregi_rep((pbuffer+28), len-28, MNaddr, FAaddr);
    	return 1;
    }
}
//家乡代理注册请求报文函数
int stud_HA_recv_Regi_req(char *pbuffer,unsigned short len,unsigned int mask,unsigned int if_no)
{
	request buf = (request)(pbuffer+28);
	int MN_addr = ntohl(buf->MN_addr);
	int HA_addr = ntohl(buf->HA_addr);
	int FA_addr = ntohl(buf->FA_addr);
	unsigned int src = ntohl(*( (int*)(pbuffer+12) ));
	unsigned int dest = ntohl(*( (int*)(pbuffer+16) ));

	if(MN_addr == getMNHaddress()){
		HA_add(MN_addr, mask, src, if_no);
		HA_regi_sendIpPkt(3, 0, src, dest, MN_addr, HA_addr);
		return 0;
	}
	else{
		HA_regi_sendIpPkt(3, 131, src, dest, MN_addr, HA_addr);
		return 1;
	}
}

int stud_packertrans_HA_recv(char *pbuffer,unsigned short len)
{
	unsigned int dest = ntohl(*( (int*)(pbuffer+16) ));
	unsigned int src = ntohl(*( (int*)(pbuffer+12) ));

    int HA_addr = getHAHaddress();
	int MN_addr = getMNHaddress();
	int FA_addr = getFAHaddress();
	unsigned char protocol = pbuffer[9];

	if(dest == MN_addr){
		//此处没有执行查找绑定表，所谓绑定表就是路由表，因此完全没有必要进行查找操作，只需要根据具体情况做转发处理和封装处理
		if(protocol==4){
		  unsigned int dest_in = ntohl(*( (int*)(pbuffer+36) ));
		  if(dest == dest_in){
			  if(src == FA_addr)  { HA_DiscardPkt(pbuffer,len); return 1; }
			  else{
			    *( (int*)(pbuffer+16) ) = htonl(FA_addr);
			    HA_forward_ipv4packet(pbuffer,len);
		        }
		  }
		  else
			 HA_send_Encap_packet(pbuffer,len,FA_addr,HA_addr);
		}
		else
			HA_send_Encap_packet(pbuffer,len,FA_addr,HA_addr);
	}
	else{
		 stud_HA_route_node* p = g_HA_route_table;
		 while(p != NULL){
			if(dest == p->dest){
				break;
			}
			p=p->next;
		 }
		 if (p == NULL)  {
                  HA_DiscardPkt(pbuffer,len);
                  return 1;
             }
		 else{
			 pbuffer[8]--;
			 *((short *)(pbuffer+10)) = 0;
			 *((short *)(pbuffer+10)) = checksum((unsigned short *)(pbuffer), 20);
			 HA_forward_ipv4packet(pbuffer,len);
		 }
	}
    return 0;
}

int stud_packertrans_FA_recv(char *pbuffer,unsigned short len)
{
	unsigned char protocol = (pbuffer[9]);
	unsigned int HA_addr = getHAHaddress();
	unsigned int MN_addr = getMNHaddress();
	unsigned int dest = ntohl(*( (int*)(pbuffer+16) ));
	unsigned int src = ntohl(*( (int*)(pbuffer+12) ));

    //printf("\n Protocol: %d\n", protocol);
	if(src == HA_addr){
	    if(protocol==4){
		   FA_send_ipv4_toMN(pbuffer+20, len-20);
	    }
	    else
		   FA_DiscardPkt(pbuffer, len);
	}
	else if(src == MN_addr){
		unsigned short check = checksum((unsigned short *)(pbuffer), 20);
		if(check == 0){
			pbuffer[8]--;
			*((short *)(pbuffer+10)) = 0;
			*((short *)(pbuffer+10)) = checksum((unsigned short *)(pbuffer), 20);
			FA_forward_ipv4packet(pbuffer,len);
		}
		else  FA_DiscardPkt(pbuffer, len);
	}
	return 0;
}

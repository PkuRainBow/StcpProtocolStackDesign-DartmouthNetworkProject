/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"
#include <set>
#include <iostream>
#include <string>
using namespace std;
// system support
extern void fwd_LocalRcv(char *pBuffer, int length);

extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char *pBuffer, int type);

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
}IPv4;

typedef struct Route_node{
	//stud_route_msg msg;
	//重载比较函数
	unsigned int dest;
	unsigned int masklen;
	unsigned int nexthop;
}Route;

//指定排序的标准
class RouteSortCriterion {
public:
	bool operator() (const Route_node &a, const Route_node &b) const {
		unsigned int sub_a = a.dest &  (0xffffffff << (a.masklen));
		unsigned int sub_b = b.dest &  (0xffffffff << (b.masklen));
		if(sub_a < sub_b) return true;
		else return false;
	}
};
//定义一个set集合
set<Route, RouteSortCriterion> s;

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



void stud_Route_Init()
{
    s.clear();
	return;
}

void stud_route_add(stud_route_msg *proute)
{
    cout<<"******************************"<<endl;
    Route* t = new Route;
	t->dest = ntohl(proute->dest);
	t->masklen = ntohl(proute->masklen);
	t->nexthop = ntohl(proute->nexthop);
	unsigned char *p1 = (unsigned char *) &(t->dest);
	unsigned char *p2 = (unsigned char *) &(t->nexthop);
	cout<<"Dest: "<<(int)p1[3]<<":"<<(int)p1[2]<<":"<<(int)p1[1]<<":"<<(int)p1[0]<<endl;
	cout<<"Masklen: "<<t->masklen<<endl;
	cout<<"Nexthop: "<<(int)p2[3]<<":"<<(int)p2[2]<<":"<<(int)p2[1]<<":"<<(int)p2[0]<<endl;
	cout<<"************ ADD INFO ************"<<endl;

    s.insert(*t);
    return;
}


int stud_fwd_deal(char *pBuffer, int length)
{
    struct IPv4 header;
    //C语言的移位运算设计 char*可以当做char数组进行处理
    header.Version = (pBuffer[0]>>4) & 0x0f;
    header.IHL = pBuffer[0] & 0x0f;
    header.len = ntohs(*((unsigned short *)(pBuffer+2)));
    header.ttl = pBuffer[8];
    header.protocol = pBuffer[9];
    header.checksum = ntohs(*((unsigned short *)(pBuffer+10)));
    header.Saddr = ntohl(*((unsigned int *)(pBuffer+12)));
    header.Daddr = ntohl(*((unsigned int *)(pBuffer+16)));

    if(header.Daddr == getIpv4Address()){
    	fwd_LocalRcv(pBuffer, length);
    	return 0;
    }
    //unsigned int dst_dest = (header.Daddr) & (0xffffffff << (32-(p->msg.masklen)));
    //unsigned int sub_net[32];
    Route temp[33];
    //0的时候，没有子网划分，32的时候没有子网内部的主机编号的划分
    for(int i=1; i<33; i++){
    	temp[i].dest = header.Daddr;
    	temp[i].masklen = i;
        //sub_net[i] = (header.Daddr) & (0xffffffff << (32-i));
    }

    //去 set 集合中查找路由信息
    //Route temp;
    //temp.dest = header.Daddr;
    //temp.
    set<Route, RouteSortCriterion>::iterator iter;
    //如何超找目的IP地址的位置
    //如何在set集合中查找目的IP的节点
    int j;
    for(j=32; j>0; j--){
    	iter = s.find(temp[j]);
    	if(iter != s.end()) break;
    }
    //没有超找到路由信息
    if(iter == s.end()) {
    	fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
        return -1;
    }
    //找到路由信息，但是TTL信息出错
	if (header.ttl <= 1)
	{
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR);
		return -1;
	}

	pBuffer[8]--;
	(*((unsigned short*) (pBuffer+10))) = 0;
	(*((unsigned short*) (pBuffer+10))) = checksum((unsigned short*)pBuffer, header.IHL*4);

	if ((header.Daddr ^ getIpv4Address()) & (0xffffffff << (32-j)) == 0x00000000 )
			fwd_SendtoLower(pBuffer, length, header.Daddr);
      else
			fwd_SendtoLower(pBuffer, length, (*iter).nexthop);
      return 0;
}

#include "sysinclude.h"

extern void rip_sendIpPkt(unsigned char *pData, UINT16 len,unsigned short dstPort,UINT8 iNo);
extern void Get_Route(UINT8 iNo, int num);
extern struct stud_rip_route_node *g_rip_route_table;

typedef struct RIP{
	unsigned char command;
	unsigned char version;
	unsigned short TAG;
}* head;

typedef struct Route{
    unsigned short addr_family_id;
    unsigned short route_id;
    unsigned int ip_add;
    unsigned int mask;
    unsigned int nexthop;
    unsigned int metric;
}* Route_item;
//������Ļ�ȡ·����Ϣ�������͵ĺ���
void Get_Route(UINT8 iNo, int num){
	int count = num/25;
	int remain = num%25;
	stud_rip_route_node* p = g_rip_route_table;
	//����25�Ļ�����Ҫ�Ƚ���װ�õ����ݰ�����
	if(count > 0){
       for(int i=0; i<count; i++){
	    		unsigned char* buffer = new unsigned char[504];
	    		memset(buffer, 0, 504);
	    		buffer[0] = 2;
	    		buffer[1] = 2;
	    		Route_item temp = (Route_item)(buffer+4);
	    		for(int j=0; p!=NULL && j<25; p=p->next){
	    			//ˮƽ�ָ��
	    			if(p->if_no != iNo){
	    				temp[j].addr_family_id = htons(2);
	    				temp[j].route_id = htons(0);
	    				temp[j].ip_add = htonl(p->dest);
	    				temp[j].mask = htonl(p->mask);
	    				temp[j].nexthop = htonl(p->nexthop);
	    				temp[j].metric = htonl(p->metric);
	    				j++;
	    			}
	    		}
	    		rip_sendIpPkt(buffer, 504, 520, iNo);
	   }
	}
    unsigned char* buffer = new unsigned char[20*remain+4];
    memset(buffer, 0, 20*remain+4);
    buffer[0] = 2;
    buffer[1] = 2;
    Route_item temp = (Route_item)(buffer+4);
    for(int j=0; p!=NULL && j<remain; p=p->next){
    	if(p->if_no != iNo){
    		temp[j].addr_family_id = htons(2);
    	    temp[j].route_id = htons(0);
    	    temp[j].ip_add = htonl(p->dest);
    	    temp[j].mask = htonl(p->mask);
    	    temp[j].nexthop = htonl(p->nexthop);
    	    temp[j].metric = htonl(p->metric);
    	    j++;
        }
    }
    rip_sendIpPkt(buffer, 20*remain+4, 520, iNo);
}


int stud_rip_packet_recv(char *pBuffer,int bufferSize,UINT8 iNo,UINT32 srcAdd)
{
    head buf = (head)pBuffer;
    if(buf->version != 2){
    	ip_DiscardPkt(pBuffer, STUD_RIP_TEST_VERSION_ERROR);
    	return -1;
    }
    if(buf->command != 1 && buf->command != 2){
    	ip_DiscardPkt(pBuffer, STUD_RIP_TEST_COMMAND_ERROR);
    	return -1;
    }
    //������
    if(buf->command == 1){
    	printf("\nRequest\n");
    	int num = 0;
    	//���˵����Ը�Դ�ӿڵ�·�ɣ���ȡ��Ч·�ɱ������Ŀ
    	stud_rip_route_node* p = g_rip_route_table;
    	while(p != NULL){
    		if(p->if_no != iNo)
    			num++;
    		p = p->next;
    	}
    	Get_Route(iNo, num);
    }

    //Ӧ����
    else if(buf->command == 2){
    	Route_item temp = (Route_item)(pBuffer+4);
    	int Route_count = (bufferSize-4)/20;
    	for(int i=0; i<Route_count; i++){
    		int metric = ntohl(temp[i].metric);
    		stud_rip_route_node *p = g_rip_route_table;

    		//����·�ɱ����Ƿ����·�ɱ���
    		while(p!=NULL){
    			if (ntohl(temp[i].ip_add)==p->dest && ntohl(temp[i].mask)==p->mask){
    				//����������16����˵����·��ͨ·�Ѿ��Ͽ�����Ҫ���⴦��
    				if(metric >= 16){
    					p->metric = 16;
    				}
    				//����·�ɱ���
    				else if(metric+1 < p->metric){
    					p->nexthop = srcAdd;
    		    	    p->metric = metric+1;
    		    	    p->if_no = iNo;
    		        }
    		        break;
    			}
    			p = p->next;
    	    }
            //������ı������µ�·�ɱ�������·�ɱ���
    	   if(p==NULL){
    			stud_rip_route_node* node = new stud_rip_route_node;
    			node->dest = ntohl(temp[i].ip_add);
    			node->mask = ntohl(temp[i].mask);
    			node->nexthop = srcAdd;
    		    node->metric = (metric+1 <= 16)? metric+1 : 16;
    			node->if_no = iNo;
    			//������ͷ������·�ɱ���
    			node->next = g_rip_route_table;
    			g_rip_route_table = node;
    		}
    	}
    }
	return 0;
}

void stud_rip_route_timeout(UINT32 destAdd, UINT32 mask, unsigned char msgType)
{
	stud_rip_route_node *p = g_rip_route_table;
	//��ʱ����������Ϊ16����ʾɾ��·�������Ϣ
	if(RIP_MSG_DELE_ROUTE == msgType){
		while(p != NULL){
			if(p->dest == destAdd && p->mask == mask)
				p->metric = 16;
			p = p->next;
		}
	}
	//��ʱ���ã�����·�ɱ������Ϣ
	else if (RIP_MSG_SEND_ROUTE == msgType){
    	int num1 = 0, num2 = 0;
    	p = g_rip_route_table;
    	while(p != NULL){
    		if(p->if_no != 1)
    			num1++;
    		if(p->if_no != 2)
    			num2++;
    		p = p->next;
    	}
        Get_Route(1, num1);
        Get_Route(2, num2);
	}
}

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "../common/constants.h"
#include "stcp_server.h"

typedef int bool;
#define true 1
#define false 0

typedef struct Online_list{
    int port_id;
    char username[10];
    char passwd[10];    
    bool tag;
    bool logon;
}online_list;

pthread_mutex_t  mutexsum;
static int Online_num;
static online_list List[100];
//im level function ...
void init_list();
void add_list(int id);
void del_list(int id);
void logon(int id, char *name, char *wd);
bool check_log(char *name);
void logoff(int id);
int get_port(char *name);
char* get_username(int id);

//sip <---> stcp level ...
int connectToSIP();
void disconnectToSIP(int sip_conn);

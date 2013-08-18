#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<pthread.h>

typedef int bool;
#define true 1
#define false 0

//serv_type
#define LOGON 0x01
#define LOGOFF 0x02
#define TO 0x03
#define ALL 0x04
#define SHOW 0x05

#define MAXLINE 500
#define SERV_PORT 17733


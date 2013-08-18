#ifndef NETWORK_H
#define NETWORK_H

int connectToSON();
void* routeupdate_daemon(void* arg);
void* pkthandler(void* arg); 
void sip_stop();
#endif

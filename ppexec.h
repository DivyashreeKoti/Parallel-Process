#ifndef ppexec_H_
#define ppexec_H_

#include<iostream>
#include<string.h>
#include<string>
#include<stdlib.h>
#include<vector>
#include <fstream>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <algorithm>
#define RECV_OK    		0
#define RECV_EOF  		-1
#define DEFAULT_BACKLOG		5
#define NON_RESERVED_PORT 	5001
#define BUFFER_LEN	 	1024	
//#define SERVER_ACK		"ACK_FROM_SERVER"

#define PPEXEC_PORT_NUM	5777


using namespace std;


const int ports[] = {5705,5706,5708,5709,5710,5711,5712,5713,
					 5714,5715,5716,5717,5718,5719,5720,5721,
					 5722,5723,5724,5725,5726,5727,5728,5729,
					};
char MyName[20];
int pthreadCount, numranks,Exitingrankscount,BarrierCount;
pthread_t id[200];
bool allRanksFlag;
char SERVER_ACK[16] = {"ACK_FROM_SERVER"};
string respectiveHostPortNames[100];
//functions
void ppexec_server();
void* toServeConnection(void* arg);
int setup_to_accept(int port);
int accept_connection(int accept_socket);
void error_check(int val,string str);
vector<int> startedranks;
#endif
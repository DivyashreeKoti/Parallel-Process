#include<stdio.h>
#include "mpi.h"

void error_check(int val, char *str)	
{
    if (val < 0)
    {
	printf(" from mpi.c %s :%d: %s\n", str, val, strerror(errno));
	exit(1);
    }
}
int recv_msg(int fd, char *buf)
{
    int bytes_read;
    bytes_read = read(fd, buf, BUFFER_LEN);
    error_check( bytes_read, "recv_msg read");
    if (bytes_read == 0)
	return(RECV_EOF);
    return( bytes_read );
}
void send_msg(int fd, char *buf, int size)	
{
    int n;
    n = write(fd, buf, size);
    error_check(n, "send_msg write");
}
int connect_to_server(char *hostname, int port)	
{
    int rc, to_server_socket;
    int optval = 1;
    struct sockaddr_in listener;
    struct hostent *hp;
    hp = gethostbyname(hostname);
    if (hp == NULL)
    {
	printf("connect_to_server: gethostbyname %s: %s -- exiting\n",
		hostname, strerror(errno));
	exit(99);
    }

    bzero((void *)&listener, sizeof(listener));
    bcopy((void *)hp->h_addr, (void *)&listener.sin_addr, hp->h_length);
    listener.sin_family = hp->h_addrtype;
    listener.sin_port = htons(port);

    to_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    error_check(to_server_socket, "net_connect_to_server socket");

    setsockopt(to_server_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));

    rc = connect(to_server_socket,(struct sockaddr *) &listener, sizeof(listener));
    error_check(rc, "net_connect_to_server connect");

    return(to_server_socket);
}
int accept_connection(int accept_socket)	
{
    struct sockaddr_in from;
    int fromlen, to_client_socket, gotit;
    int optval = 1;

    fromlen = sizeof(from);
    gotit = 0;
    while (!gotit)
    {
	to_client_socket = accept(accept_socket, (struct sockaddr *)&from, &fromlen);
	if (to_client_socket == -1)
	{
	    /* Did we get interrupted? If so, try again */
	    if (errno == EINTR)
		continue;
	    else
		error_check(to_client_socket, "accept_connection accept");
	}
	else
	    gotit = 1;
    }

    setsockopt(to_client_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));
    return(to_client_socket);
}
int setup_to_accept(int port)	
{
    int rc, accept_socket;
    int optval = 1;
    struct sockaddr_in sin, from;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    accept_socket = socket(AF_INET, SOCK_STREAM, 0);
    error_check(accept_socket,"setup_to_accept socket");

    setsockopt(accept_socket,SOL_SOCKET,SO_REUSEADDR,(char *)&optval,sizeof(optval));

    rc = bind(accept_socket, (struct sockaddr *)&sin ,sizeof(sin));
    error_check(rc,"setup_to_accept bind");

    rc = listen(accept_socket, DEFAULT_BACKLOG);
    error_check(rc,"setup_to_accept listen");

    return(accept_socket);
}
void serve_one_connection(int to_client_socket)
{
	//printf("My rank %d my srever called ",PP_MPI_RANK);
    int rc, ack_length;
    char buf[BUFFER_LEN];
	int to_server_socket;
	
    ack_length = strlen(SERVER_ACK)+1;
    rc = recv_msg(to_client_socket, buf);	
	//printf("%s---\n",buf);
	if(strcmp(buf,"MYEND")==0){
	//	printf("ENDDDDD %d",PP_MPI_RANK);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
	}
	else if(strcmp(buf,"BARRIER")==0){
		allbarrierrflag=0;
		send_msg(to_client_socket, SERVER_ACK, ack_length);
	}
	else if(strcmp(buf,"TOSENDDATA")==0){
//		printf("tosend data\n");
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		rc = recv_msg(to_client_socket, buf);
		pthread_mutex_lock(&RecvDatarankindex_mutex);
		RecvDatarank[RecvDatarankindex].fromrank=atoi(buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		rc = recv_msg(to_client_socket, buf);
		RecvDatarank[RecvDatarankindex].tag=atoi(buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		rc = recv_msg(to_client_socket, buf);
		
		RecvDatarank[RecvDatarankindex].data=atoi(buf);
//		printf("======%s\n",buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		rc = recv_msg(to_client_socket, buf);
		RecvDatarank[RecvDatarankindex].mpiworld = atoi(buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		pthread_mutex_unlock(&RecvDatarankindex_mutex);
		pthread_mutex_lock(&RecvDatarankindex_mutex);
		RecvDatarank[RecvDatarankindex].rcount=0;
		RecvDatarankindex++;
		pthread_mutex_unlock(&RecvDatarankindex_mutex);
//		printf("%d######\n",RecvDatarankindex);
	}
	else if(strcmp(buf,"TOIRECV")==0){
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		int flag=1,src,tag,i;
		//src
		rc = recv_msg(to_client_socket, buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		src = atoi(buf);
		//tag
		rc = recv_msg(to_client_socket, buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		tag = atoi(buf);
		//printf("Recieved at irecv %d  %d\n",src,tag);
		//while(flag){
		rc = recv_msg(to_client_socket, buf);
		//for send - irecv
		if(RecvDatarankindex>0){
			int j;
			for(j=0;j<irecvcount;j++){
				int k;
				for(k=0;k<RecvDatarankindex;k++){
					if(RecvDatarank[k].fromrank == irecvdata[j].indest &&
						RecvDatarank[k].tag == irecvdata[j].intag){
							*irecvdata[j].inbuf=RecvDatarank[k].data;
							irecvdata[j].indest=-1;
							irecvdata[j].intag=-1;
							irecvdata[j].req->request=-1;
							irecvdata[j].req->flag = 0;
							increirecv++;
							RecvDatarankindex--;
							break;
						}
				}
			}
		}
		//
		for(i=0;i<irecvcount;i++){
			if(	(src == irecvdata[i].indest &&
				tag == irecvdata[i].intag)){
				buf[0] = '0';
				send_msg(to_client_socket, buf, 2);
				increirecv++;
				flag=0;break;
			}
			else if(irecvdata[i].intag==1000 && irecvtotalflag){
				if(increirecv==irecvtagscount){
//					printf("yes for tag %d\n",tag);
					buf[0] = '0';
					send_msg(to_client_socket, buf, 2);
					increirecv++;
					flag=0;break;
				}
			}
		}
		if(flag){
			buf[0] = '1';
			send_msg(to_client_socket, buf, 2);
			return;
		}
		//}
//		printf("i am ready\n");
		//for sync
		rc = recv_msg(to_client_socket, buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		//recv data
		rc = recv_msg(to_client_socket, buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
//		printf("%d\n----\n",atoi(buf));
		*irecvdata[i].inbuf = atoi(buf);
		irecvdata[i].indest=-1;
		irecvdata[i].intag=-1;
		irecvdata[i].req->request=-1;
		irecvdata[i].req->flag = 0;
		if(RecvDatarankindex>0){
			int j;
			for(j=0;j<irecvcount;j++){
				int k;
				for(k=0;k<RecvDatarankindex;k++){
					if(increirecv==irecvtagscount &&
						RecvDatarank[k].tag == 1000){
							*irecvdata[j].inbuf=RecvDatarank[k].data;
							irecvdata[j].indest=-1;
							irecvdata[j].intag=-1;
							irecvdata[j].req->request=-1;
							irecvdata[j].req->flag = 0;
							increirecv++;
							RecvDatarankindex--;
							break;
						}
				}
			}
		}
	//	printf("CAAAAAAAAme %d\n",PP_MPI_RANK);
	}
	else if(strcmp(buf,"REDUCE")==0){
		int rankyy;
//		printf("HERE reduce\n");
		//send_msg(to_client_socket, SERVER_ACK, ack_length);
		if(reduce_count==0){
			buf[0]='1';
			send_msg(to_client_socket,buf,2);
			rc = recv_msg(to_client_socket, buf);
			close(to_client_socket);
			return;
		}
		else buf[0] = '0';
		send_msg(to_client_socket,buf,2);
		rc = recv_msg(to_client_socket, buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		
//		return;
		//rank
		rc = recv_msg(to_client_socket, buf);
		rankyy=atoi(buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
//		return;
		
		//count
		rc = recv_msg(to_client_socket, buf);
		int count = atoi(buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		int i=0;
		for(i=0;i<count;i++){
			rc = recv_msg(to_client_socket, buf);
			reduce_recvbuf[rankyy][i]=atoi(buf);
			send_msg(to_client_socket, SERVER_ACK, ack_length);
		}
		
		reduce_count++;
		/*if(reduce_count>=PP_MPI_SIZE){
			reducelag=0;
		}*/
	}
	close(to_client_socket);
}
void* toServeConnection(void * a){
	//printf("ServerCalled %d \n",PP_MPI_RANK);
    int rc, accept_socket, to_client_socket;
    accept_socket = setup_to_accept(MYPORT);
//printf("Entered while waiting for connection %d \n",toServeConnectionflag);	
    while(toServeConnectionflag==1)
    {		
		
		to_client_socket = accept_connection(accept_socket);
	
		serve_one_connection(to_client_socket);
		
	}
    	
//		printf("out of server mpic %d \n ",PP_MPI_RANK);
}
void get_Port_Number(){
	int to_server_socket;
    char buf[BUFFER_LEN];

    to_server_socket = connect_to_server(PP_MPI_HOST_PORT_Name,PP_MPI_HOST_PORT_Port);
    strcpy(buf,"MYPORT");
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	memset(buf,'\0',sizeof(buf));
	sprintf(buf,"%d",PP_MPI_RANK);
	send_msg(to_server_socket, buf, 3);
	memset(buf,'\0',sizeof(buf));
	recv_msg(to_server_socket, buf);
	MYPORT = atoi(buf);
	memset(buf,'\0',sizeof(buf));
	send_msg(to_server_socket, MYNAME, sizeof(MYNAME));
	recv_msg(to_server_socket, buf);
	close(to_server_socket);
}
int MPI_Init( int *argc, char ***argv){
	//printf("Init Called\n ");
	char *ptr_PP_MPI_HOST_PORT = getenv("PP_MPI_HOST_PORT");
	char *ptr_PP_MPI_RANK = getenv("PP_MPI_RANK");
	char *ptr_PP_MPI_SIZE = getenv("PP_MPI_SIZE");
	
	PP_MPI_RANK = atoi(ptr_PP_MPI_RANK);
	PP_MPI_SIZE = atoi(ptr_PP_MPI_SIZE);
	
	char temp_ptr_PP_MPI_HOST_PORT[100];
	strcpy(temp_ptr_PP_MPI_HOST_PORT,ptr_PP_MPI_HOST_PORT);
	char *word[10];
	word[0]=strtok(temp_ptr_PP_MPI_HOST_PORT,":");
	strcpy(PP_MPI_HOST_PORT_Name,word[0]);
	
	char *str= strchr(ptr_PP_MPI_HOST_PORT,':');
	int from_to = (int)(str - ptr_PP_MPI_HOST_PORT);
	char temp_port[10];
	memcpy( temp_port, &ptr_PP_MPI_HOST_PORT[from_to+1], 4 );
	PP_MPI_HOST_PORT_Port = atoi(temp_port);
	// Save my own hostname
	gethostname(MYNAME, sizeof(MYNAME));

	// getting my server port number to start server on
	toServeConnectionflag=1;
	allbarrierrflag=1;
	RecvDatarankindex=0;
	isendscount=0;
	irecvcount=0;
	irecvtagscount=0;
	irecvtotalflag=0;
	increirecv=0;
	resetall=0;
	
	reduce_count=0;
	int i;
	/*for(i=0;i<PP_MPI_SIZE;i++){
		irecvdata[i].req->flag=-1;
		irecvdata[i].req->request = -1;
	}
	*/
	get_Port_Number();
	//printf("Got back from ppexec %d \n",PP_MPI_RANK);
	pthread_create(&id,NULL,toServeConnection,NULL);
//	printf("Init Exit name %s  my server port numebr %d\n ",MYNAME,MYPORT);
}

int MPI_Comm_rank(int mpiworld,int* myrank){
	*myrank = PP_MPI_RANK;
	return 1;
}

int MPI_Comm_size(int mpiworld,int* numranks){
	*numranks = PP_MPI_SIZE;
	return 1;
}

int MPI_Barrier(int mpiworld){
	
	int i;
	int to_server_socket;
    char buf[BUFFER_LEN];
	//printf("BEFOREEEEEEEEEE\n");
	to_server_socket = connect_to_server(PP_MPI_HOST_PORT_Name,PP_MPI_HOST_PORT_Port);
	strcpy(buf,"BARRIER");
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	close(to_server_socket);	
	while(allbarrierrflag);	
}

void MPI_Finalize(){
//	printf("Called finalize %d\n",PP_MPI_RANK);
	int to_server_socket;
    char buf[BUFFER_LEN];
	to_server_socket = connect_to_server(PP_MPI_HOST_PORT_Name,PP_MPI_HOST_PORT_Port);
	strcpy(buf,"EXIT");
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	close(to_server_socket);
	toServeConnectionflag=0;
	
	// stop my own server
	to_server_socket = connect_to_server(MYNAME,MYPORT);
	strcpy(buf,"MYEND");
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	close(to_server_socket);
	
	//pthread_join(comm2,NULL);
	//sleep(1);
	//printf("Done with finalize \n %d ",PP_MPI_RANK);
}
void gettheserverdetailsofhost(int destrank,int * destport, char desthostname[50])
{
    int to_server_socket;
    char buf[BUFFER_LEN];
	to_server_socket = connect_to_server(PP_MPI_HOST_PORT_Name,PP_MPI_HOST_PORT_Port);
		
	strcpy(buf,"TOPORTSEND");
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	snprintf(buf, sizeof(buf), "%d", destrank);
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	*destport=atoi(buf);
	strcpy(buf,"ACK");
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, desthostname);
	close(to_server_socket);
}
void Senddatatoranks(char *server_host, int serverport,int tag,int count,int * data,int mpiworld)
{
//	printf("1\n");
    int to_server_socket;
    char buf[BUFFER_LEN];
	to_server_socket = connect_to_server(server_host,serverport);
	strcpy(buf,"TOSENDDATA");
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
//	printf("2\n");
	sprintf(buf, "%d", PP_MPI_RANK);
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
//	printf("3\n");
	sprintf(buf,"%d", tag);
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	sprintf(buf,"%d", *data);
	send_msg(to_server_socket, buf, strlen(buf)+1);
	
	recv_msg(to_server_socket, buf);
	sprintf(buf,"%d",mpiworld);
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	close(to_server_socket);
	
}
int MPI_Send(int * buf,int count,char type[50],int dest,int tag,int mpiworld)
{
//	printf("Send Called %d----%d \n",buf[0],mpiworld);
	int destport=-1;
	char desthostname[50];
	
	int flag=1;
	while(flag)
	{
		gettheserverdetailsofhost(dest,&destport,desthostname);
		if(destport!=-1){
			//printf("HIIIIIIIIIIIIIIIII\n");
			flag=0;
			break;
		}
		else
			sleep(1);
	}
	//sleep(1);
	//printf("%d Got hostname %s port %d \n ",PP_MPI_RANK,desthostname,destport);
	//printf("bat man\n");
	Senddatatoranks(desthostname,destport,tag,count,buf,mpiworld);
//	printf("DONEeeee\n");
	return 1;
}
int MPI_Ssend(int * buf,int count,char type[50],int dest,int tag,int mpiworld)
{
	MPI_Send( buf, count, type, dest, tag, mpiworld);
	//printf("Send Called \n");
	return 1;
}
int MPI_Recv(int* buf,int count,char type[50],int src,int tag,int mpiworld,MPI_Status * status){
	sleep(1);
//printf("Received %d--%d \n",PP_MPI_RANK,mpiworld);
while(1){
	int i;
		for(i=0;i<RecvDatarankindex;i++){	
		//printf("Cam src --%d-- tag --%d-- comapring src --%d-- tag --%d-- \n",
		//RecvDatarank[i].fromrank,
		//RecvDatarank[i].tag,src,tag);		
			if((RecvDatarank[i].fromrank==src&&RecvDatarank[i].tag==tag&&
			RecvDatarank[i].mpiworld == mpiworld)||
			src==MPI_ANY_SOURCE||tag==MPI_ANY_TAG){
				if(RecvDatarank[i].mpiworld == mpiworld&&RecvDatarank[i].rcount == 0){
					RecvDatarank[i].rcount=1;
					status->MPI_SOURCE=RecvDatarank[i].fromrank;
				//	printf("DIVYAAAAAAAAA\n");
					status->MPI_TAG=RecvDatarank[i].tag;
					
					*buf=RecvDatarank[i].data;
				//	printf("-- rank %d -- tag %d -- data %d\n",RecvDatarank[i].fromrank,RecvDatarank[i].tag,RecvDatarank[i].data);
					return 1;
				}
			}
		}
	}
	return 1;
}

//----------------------------------------------------------------------------
//project 3 implementation
double MPI_Wtime(void){
	struct timeval tv;

    gettimeofday( &tv, ( struct timezone * ) 0 );
    return ( tv.tv_sec + (tv.tv_usec / 1000000.0) );
}

int MPI_Comm_dup(int mpiworld, int *COMM_WORLD2){
	//pthread_create(&comm2,NULL,forcommworld2,void);
	*COMM_WORLD2 = mpiworld + 1;
	return 1;
}
void toaskNumOfRanks(){
	int to_server_socket;
    char buf[BUFFER_LEN];

    to_server_socket = connect_to_server(PP_MPI_HOST_PORT_Name,PP_MPI_HOST_PORT_Port);
    strcpy(buf,"NUMRANKS");
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	TOTALRANKS = atoi(buf);
	close(to_server_socket);
	return;
}
int MPI_Bcast(void* buf, int count, char type[50], int root, int mpiworld){
	int i,tag = 8888;
	MPI_Status * status;
	if(root == PP_MPI_RANK){
		//ask for number of ranks
		toaskNumOfRanks();
		for(i=0;i<TOTALRANKS;i++){
			if(PP_MPI_RANK != i){
				MPI_Send(buf,count,type,i,tag, mpiworld);
				sleep(1);
			}
		}
	}
	else{
		MPI_Recv(buf,count,type,root,tag,mpiworld,status);
	}
}

int MPI_Gather(void* buf, int count, char type[50], 
				void* recvbuf, int recvcount, char recvtype[50], 
				int root, int mpiworld){
	int i,tag = 9999,stat,rc;
	MPI_Status * status;
//	int tempbuf[PP_MPI_SIZE * sizeof(int)];
	//printf("[[[[[[[[ %d  %d\n",PP_MPI_RANK,*(int*)buf);
	//strcpy(recvbuf,"");
	int tempbuf[PP_MPI_SIZE*sizeof(int)];
	//char *chararr = malloc(100*sizeof(char));
	if(root == PP_MPI_RANK){
		MPI_Send(buf,count,type,root,tag, mpiworld);
		while(RecvDatarankindex<PP_MPI_SIZE);
		//printf("%d RecvDatarankindex : %d \n", PP_MPI_RANK,RecvDatarankindex);
		for(i=0;i<PP_MPI_SIZE;i++){
			tempbuf[i]=0;
		}
		for(i=0;i<PP_MPI_SIZE;i++){
			//MPI_Recv(tempbuf,recvcount,recvtype,i,tag,mpiworld,status);
			//chararr = tempbuf;
			//(recvbuf[RecvDatarank[i].fromrank]) = RecvDatarank[i].data;
			//sprintf(tempbuf+RecvDatarank[i].fromrank,"%d",RecvDatarank[i].data);
			tempbuf[RecvDatarank[i].fromrank] = RecvDatarank[i].data;
			//memcpy(recvbuf+RecvDatarank[i].fromrank,&RecvDatarank[i].data,sizeof(RecvDatarank[i].data));
			//int a = recvbuf+RecvDatarank[i].fromrank;
		//	printf("rrrr %d %d %d\n",i,RecvDatarank[i].fromrank,tempbuf[RecvDatarank[i].fromrank]);
		}
		memcpy(recvbuf,(void*)tempbuf,sizeof(tempbuf));
	//	recvbuf+0 = (void*)tempbuf;
	//	printf("#######   %d\n",*(recvbuf+0));
	}
	else{
		MPI_Send(buf,count,type,root,tag, mpiworld);
	}
//	printf("DONEEEEEEEEEEEEE\n\n");
	
}

//Project 4 implementation
void isenddatatoranks(int destport,char desthostname[50],int send_count){
	int to_server_socket;
	int rc, ack_length;
	char flag='1';
    char buf[BUFFER_LEN];
	ack_length = strlen(SERVER_ACK)+1;
	//printf("isend data\n");
	while(flag=='1'){
		to_server_socket = connect_to_server(desthostname,destport);
	//	printf("SER %d\n",to_server_socket);
		strcpy(buf,"TOIRECV");
		send_msg(to_server_socket, buf, strlen(buf)+1);
		recv_msg(to_server_socket, buf);
		//src 
		sprintf(buf,"%d",PP_MPI_RANK);
		send_msg(to_server_socket, buf, strlen(buf)+1);
		recv_msg(to_server_socket, buf);	//ack

		//tag
	//	printf("$$$$$$$ %d\n",isenddata[send_count].intag);
		sprintf(buf,"%d",isenddata[send_count].intag);
		send_msg(to_server_socket, buf, strlen(buf)+1);
		recv_msg(to_server_socket, buf);
	
	
		send_msg(to_server_socket, SERVER_ACK, ack_length);
		recv_msg(to_server_socket, buf);
		flag = buf[0];
	}
	//sleep(1);
//	printf("Reciver is ready\n");
	//for sync
	send_msg(to_server_socket, SERVER_ACK, ack_length);
	recv_msg(to_server_socket, buf);
	
	//send buf
	sprintf(buf,"%d",*isenddata[send_count].inbuf);
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	isenddata[send_count].req->flag = 0;
	
	//printf("came to isenddata %d\n",PP_MPI_RANK);
}
void *Isendthread(void *arg){
	int destport=-1;
	char desthostname[50];
	int flag=1;
	int send_count = *(int*)arg;
	//printf("thread\n");
	while(flag)
	{
		gettheserverdetailsofhost(isenddata[send_count].indest,&destport,desthostname);
		if(destport!=-1){
			flag=0;
			break;
		}
		else
			sleep(1);
	}
	//printf("divya\n");
	isenddatatoranks(destport,desthostname,send_count);
}
int MPI_Isend(int *buf, int count, char type[50], int dest, int tag,int mpiworld , MPI_Request *req){
	isenddata[isendscount].inbuf = buf;
	isenddata[isendscount].count = count;
	strcpy(isenddata[isendscount].intype,type);
	isenddata[isendscount].indest = dest;
	isenddata[isendscount].intag = tag;
	isenddata[isendscount].inmpiworld = mpiworld;
	isenddata[isendscount].req = req;
	isenddata[isendscount].req->request = isendscount;
	isenddata[isendscount].req->flag=1;
	isenddata[isendscount].req->sendrecv = 's';
	int arg = isendscount;
	pthread_create(&isend_id[isendscount],NULL,Isendthread,(void*)&arg);
	isendscount++;
	sleep(1);
	
	return 1;
}
int MPI_Irecv(int *buf, int count, char type[50], int src, int tag, int mpiworld, MPI_Request *req){
	
	irecvdata[irecvcount].inbuf = buf;
	irecvdata[irecvcount].count = count;
	strcpy(irecvdata[irecvcount].intype,type);
	irecvdata[irecvcount].indest = src;
	irecvdata[irecvcount].intag = tag;
	irecvdata[irecvcount].inmpiworld = mpiworld;
	irecvdata[irecvcount].req = req;
	irecvdata[irecvcount].req->request = irecvcount;
	irecvdata[irecvcount].req->flag=1;
	irecvdata[irecvcount].req->sendrecv = 'r';
	if(tag!=1000){
		irecvtagscount++;
	}
//	pthread_create(&irecv_id[irecvcount],NULL,Irecvthread,(void*)&irecvcount);
//	irecvflag[irecvcount]=0;
	irecvcount++;
	return 1;
}
int MPI_Test(MPI_Request *req, int *flag, MPI_Status *status){
	*flag = !req->flag;
	return 0;
}
int MPI_Wait(MPI_Request *req, MPI_Status *status){
	int i;
	if(req->sendrecv == 'r'){
		if(irecvtotalflag==0){
			irecvtotal=irecvcount;
			irecvtotalflag=1;
		}
		for(i=0;i<irecvcount;i++){
			if(req->request == irecvdata[i].req->request){
//				printf("WAITTTTTT %d\n",irecvdata[i].req->request);
				while(irecvdata[i].req->flag==1);
	//			printf("OUT of wait\n");
				irecvdata[i].req->flag=-1;
				resetall++;
				if(resetall==irecvcount){
					isendscount=0;
					irecvcount=0;
					irecvtagscount=0;
					irecvtotalflag=0;
					increirecv=0;
					resetall=0;
				}
				break;
			}
		}
	}
	else{
	//	printf("send");
		for(i=0;i<isendscount;i++){
			if(req->request == isenddata[i].req->request){
			//	printf("WAITTTTTT %d\n",irecvdata[i].req->request);
				while(isenddata[i].req->flag==1);
	//			printf("OUT of wait\n");
				isenddata[i].req->flag=-1;
				break;
			}
		}
	}
	return 0;
}

//project 5 implementation
void reduceddatasend (char *desthostname, int destport,const int *sendbuf,int count){
	int to_server_socket;
	int rc, ack_length;
	char flag='1';
    char buf[BUFFER_LEN];
	ack_length = strlen(SERVER_ACK)+1;
//	printf("Redduce to send\n");
	//printf("isend data\n");
	while(flag=='1')
	{
		to_server_socket = connect_to_server(desthostname,destport);
	//	printf("SER %d\n",to_server_socket);
		strcpy(buf,"REDUCE");
		send_msg(to_server_socket, buf, strlen(buf)+1);
		recv_msg(to_server_socket, buf);
		flag=buf[0];
		send_msg(to_server_socket, SERVER_ACK, ack_length);
		if(flag == '1') { close(to_server_socket); sleep(1);}
	}
	recv_msg(to_server_socket, buf);
//	printf("OUT\n");
	//rank
	sprintf(buf,"%d",PP_MPI_RANK);
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	//sum
	sprintf(buf,"%d",count);
	send_msg(to_server_socket, buf, strlen(buf)+1);
	recv_msg(to_server_socket, buf);
	
	//send values
	int i=0;
	for(i=0;i<count;i++){
		sprintf(buf,"%d",sendbuf[i]);
		send_msg(to_server_socket, buf, strlen(buf)+1);
		recv_msg(to_server_socket, buf);
	}
	
	close(to_server_socket);
}
void for_root_reduce_helper(int *sendbuf,int count,int *recvbuf,int root){
	int i,sum=0,j;
	for(i=0;i<count;i++){
		reduce_recvbuf[root][i] = sendbuf[i];
	}
	reduce_count++;
	
	while(reduce_count<PP_MPI_SIZE);
	for(i=0;i<count;i++){
		sum=0;
		for(j=0;j<PP_MPI_SIZE;j++){
			sum+=reduce_recvbuf[j][i];
		}
		recvbuf[i]=sum;
	}
}
void for_other_reduce_helper(int *sendbuf,int count,int root){
	int destport=-1;
	char desthostname[50];
	int flag=1;
	while(flag)
	{
		gettheserverdetailsofhost(root,&destport,desthostname);
		if(destport!=-1){
			flag=0;
			break;
		}
		else
			sleep(1);
	}
	reduceddatasend(desthostname,destport,sendbuf,count);
}
int MPI_Reduce(int *sendbuf, int *recvbuf, int count, char datatype[50],
                char op[60], int root, int comm){
//	printf("op--:%s\n",op);
	if(strcmp(op,"sum")==0){
		int sum=0;
		if(PP_MPI_RANK == root){
			for_root_reduce_helper(sendbuf,count,recvbuf,root);
		}
		else{
			for_other_reduce_helper(sendbuf,count,root);
		}
	}
	else{
	//	printf("in else\n");
		int *buf = malloc(1000*sizeof(int)),i;
	//	mpiopvar.fp(sendbuf,buf,&count,datatype);
		
		if(PP_MPI_RANK == root){
			reduce_count=0;
			//for_root_reduce_helper(buf,count,recvbuf,root);
			int i,sum=0,j;
			for(i=0;i<count;i++){
				reduce_recvbuf[root][i] = sendbuf[i];
			}
			reduce_count++;
			
			while(reduce_count<PP_MPI_SIZE);
			int *tempbuf = malloc(1000*sizeof(int));
	//		printf("%d\n",PP_MPI_SIZE);
			if(PP_MPI_SIZE>1){
				for(i=0;i<count;i++){
					buf[i]=reduce_recvbuf[0][i];
				}
				for(j=1;j<PP_MPI_SIZE;j++){
					for(i=0;i<count;i++){
						tempbuf[i]=buf[i];
					}
					for(i=0;i<count;i++){
						buf[i] = reduce_recvbuf[j][i];
					}
					mpiopvar.fp(tempbuf,buf,&count,datatype);
				}
				for(i=0;i<count;i++){
					recvbuf[i] = buf[i];
				}
				
			}
			else if(PP_MPI_SIZE==1){
				for(i=0;i<count;i++){
					recvbuf[i] = sendbuf[i];
				}
			}
		}
		else{
			for_other_reduce_helper(sendbuf,count,root);
		}
		free(buf);
	}
	return 0;
}

void MPI_Op_create(void (*fp)(void *inp, 
					void *inoutp, int *len, 
					MPI_Datatype *dptr),int commute,
					MPI_Op *op){

	mpiopvar.fp = fp;
	strcpy(*op,"other");
}
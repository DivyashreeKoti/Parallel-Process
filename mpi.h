#ifndef MPI_H_
#define MPI_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#define RECV_OK    		0
#define RECV_EOF  		-1
#define DEFAULT_BACKLOG		5
#define NON_RESERVED_PORT 	5001
#define BUFFER_LEN	 	1024	
#define SERVER_ACK		"ACK_FROM_SERVER"
#define MPI_COMM_WORLD 	1
#define  MPI_INT "int"
#define MPI_ANY_TAG 1000
#define MPI_ANY_SOURCE 2000
#define MPI_COMM 10
#define MPI_SUM "sum"

typedef char MPI_Datatype;
typedef struct _MPI_Status {
  int count;
  int cancelled;
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
}MPI_Status;

typedef struct _MPI_Request{
	int request;
	int flag;
	char sendrecv;
}MPI_Request;

typedef char MPI_Op[60];

struct SendRecInfo{
	int data;
	int count;
	int fromrank;
	int tag;
	int mpiworld;
	int rcount;
}RecvDatarank[100];

struct isendinfo{
	int *inbuf;
	int count;
	char intype[100];
	int indest;
	int intag;
	int inmpiworld;
	MPI_Request *req;
}isenddata[100],irecvdata[100];
struct mpiopcreate{
	void (*fp)(void *inp, 
					void *inoutp, int *len, 
					MPI_Datatype *dptr);
}mpiopvar;
int PP_MPI_HOST_PORT_Port,PP_MPI_RANK,PP_MPI_SIZE,MYPORT,TOTALRANKS;
char PP_MPI_HOST_PORT_Name[20],MYNAME[20];
int toServeConnectionflag,allbarrierrflag,RecvDatarankindex,recvblockfalg;
pthread_t id,isend_id[100],irecv_id[100];
pthread_mutex_t RecvDatarankindex_mutex;
int isendscount,irecvcount,irecvtags[100],waitflag,irecvtagscount,proptagscount,reduce_count;
int irecvtotal,irecvtotalflag,increirecv;

int resetall;
//int reducelag;
int reduce_recvbuf[100][100];



int MPI_Init( int *argc, char ***argv );
int MPI_Comm_rank(int mpiworld,int* myrank);
int MPI_Comm_size(int mpiworld,int* numranks);
void MPI_Finalize(); 
int MPI_Barrier(int mpiworld);
int MPI_Send(int * buf,int count,char type[50],int dest,int tag,int mpiworld);
int MPI_Ssend(int * buf,int count,char type[50],int dest,int tag,int mpiworld);
int MPI_Recv(int * buf,int count,char type[50],int src,int tag,int mpiworld,MPI_Status * status);


//project3
double MPI_Wtime(void);
int MPI_Comm_dup(int mpiworld, int *COMM_WORLD2);
int MPI_Bcast(void* buf, int count, char type[50], int root, int mpiworld);
int MPI_Gather(void* buf, int count, char type[50], void* recvbuf, int recvcount, char recvtype[50], int root, int mpiworld); 

//project4
int MPI_Isend(int *buf, int count, char type[50], int dest, int tag,int mpiworld , MPI_Request *req);
int MPI_Irecv(int *buf, int count, char type[50], int src, int tag, int mpiworld, MPI_Request *req);
int MPI_Test(MPI_Request *req, int *flag, MPI_Status *status);
int MPI_Wait(MPI_Request *req, MPI_Status *status);

//project 5
int MPI_Reduce(int *sendbuf, int *recvbuf, int count, char datatype[50],
                char op[60], int root, int comm);
//int MPI_Op_create(void *user_fn, int commute, char *op);
void MPI_Op_create(void (*fp)(void *inp, void *inoutp, int *len, MPI_Datatype *dptr),int commute,MPI_Op *op);
#endif
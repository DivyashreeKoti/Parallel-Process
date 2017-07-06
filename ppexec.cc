#include "ppexec.h"

int main(int argc, char** argv){
	char file_name[100];
	int i;
	vector<string> execute_cmd;
	
	pthreadCount=0;
	allRanksFlag=true;
	numranks=1;
	Exitingrankscount=0;
	BarrierCount=0;
	strcpy(file_name,"hostnames");
	
	for(i=1;i<argc;i++){
		string arg=argv[i];
		if(arg=="-f"){
			strcpy(file_name,argv[i+1]);
			i++;
		}
		else if(arg=="-n"){
			numranks=atoi(argv[i+1]);
			i++;
		}
		else
			execute_cmd.push_back(argv[i]);
	}
	
	char pwd[100];
	getcwd(pwd,sizeof(pwd));
	string st="/";
	if(execute_cmd[0][0]=='.')
		execute_cmd[0] = pwd+execute_cmd[0].substr(1,execute_cmd[0].size()-1);
	else
		execute_cmd[0] = pwd+st+execute_cmd[0].substr(0,execute_cmd[0].size());
	//cout<<execute_cmd[0]<<endl;
	gethostname(MyName, sizeof(MyName));
	if(fork()){
		while(wait(NULL)>0);
		return 0;
	}
	else
	{
		vector<string> hosts;
		string localhost;
		ifstream ifp;
		bool flag;
		ifp.open (file_name);
		if (ifp.is_open()) {
			flag=true;
			while(ifp>>localhost){			
				hosts.push_back(localhost);		
			}
		}
		for(i=0;i<numranks;i++){
			char *execute_cmd_args[execute_cmd.size()+2];
			int status=0,j;
			if(flag){
				
				int index=i%hosts.size();
				char cmd[20],inner_cmd[100];
				string bash_cmd="bash -c  \' ";
				strcpy(cmd,"ssh");
				execute_cmd_args[0]=strdup(cmd);
				char PP_MPI_RANK[50],PP_MPI_SIZE[50],PP_MPI_HOST_PORT[50],t[50];
				strcpy(PP_MPI_RANK," PP_MPI_RANK=");
				sprintf(t,"%d",i);
				strcat(PP_MPI_RANK,t);
				
				strcpy(PP_MPI_SIZE," PP_MPI_SIZE=");
				sprintf(t,"%d",numranks);
				strcat(PP_MPI_SIZE,t);
				
				strcpy(PP_MPI_HOST_PORT," PP_MPI_HOST_PORT=");
				strcat(PP_MPI_HOST_PORT,MyName);
				strcat(PP_MPI_HOST_PORT,":");
				char temp_PP_MPI_HOST_PORT[20];
				sprintf(temp_PP_MPI_HOST_PORT,"%d",PPEXEC_PORT_NUM);
				strcat(PP_MPI_HOST_PORT,temp_PP_MPI_HOST_PORT);
				
				execute_cmd_args[1]=strdup(hosts[index].c_str());	
				execute_cmd_args[2]=strdup(PP_MPI_RANK);
				execute_cmd_args[3]=strdup(PP_MPI_SIZE);
				execute_cmd_args[4]=strdup(PP_MPI_HOST_PORT);
				
				for(j=0;j<execute_cmd.size();j++){
					bash_cmd+= " " + execute_cmd[j];
				}
				bash_cmd+=" \' ";
				strcpy(inner_cmd,bash_cmd.c_str());
				execute_cmd_args[5]=strdup(inner_cmd);
				execute_cmd_args[6]=NULL;	
			}
			else{
				
				for(j=0;j<execute_cmd.size();j++){
					char temp_execute_cmd[50];
					strcpy(temp_execute_cmd,execute_cmd[j].c_str());
					execute_cmd_args[j]=strdup(temp_execute_cmd);
				}
				execute_cmd_args[j]=NULL;
			}
			
			//forking to execute process on other hosts
			int rc=fork();			
			if(rc==0){
				// child 
				if(!flag){
					string ppmpirank=to_string(i);
					string sz=to_string(numranks);
					char temp_PP_MPI_HOST_PORT[20];
					sprintf(temp_PP_MPI_HOST_PORT,"%d",PPEXEC_PORT_NUM);
					char ppmpihostnameport[128];
					gethostname(ppmpihostnameport, sizeof(ppmpihostnameport));
					strcat(ppmpihostnameport,":");
					strcat(ppmpihostnameport,temp_PP_MPI_HOST_PORT);
					setenv("PP_MPI_RANK",ppmpirank.c_str(),1);
					setenv("PP_MPI_SIZE",sz.c_str(),1);
					setenv("PP_MPI_HOST_PORT",ppmpihostnameport,1);
				}
				execvp(execute_cmd_args[0],execute_cmd_args);
				exit(0);
			}else{
				//parent
				//waitpid(rc,&status,WNOHANG);
			}
		}
		
		ppexec_server();
		
		while(wait(NULL)>0);
	}
	exit(0);
}

void ppexec_server(){
	Exitingrankscount=0;
	int rc, accept_socket, to_client_socket;
	
	accept_socket = setup_to_accept(PPEXEC_PORT_NUM);
	
	while(allRanksFlag){
		to_client_socket = accept_connection(accept_socket);
		pthread_create(&id[pthreadCount++],NULL,toServeConnection,(void*)&to_client_socket);
		pthread_join(id[pthreadCount-1],NULL);
	}
	// to check  for proper port numbers
	
//	printf("ppexec about to exit \n");
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

    setsockopt(accept_socket,SOL_SOCKET,SO_REUSEPORT,(char *)&optval,sizeof(optval));

    rc = bind(accept_socket, (struct sockaddr *)&sin ,sizeof(sin));
    error_check(rc,"setup_to_accept bind");

    rc = listen(accept_socket, DEFAULT_BACKLOG);
    error_check(rc,"setup_to_accept listen");

    return(accept_socket);
}

int accept_connection(int accept_socket)	
{
    struct sockaddr_in from;
    int to_client_socket, gotit;
	socklen_t fromlen;
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

void error_check(int val,string str)	
{
    if (val < 0)
    {
	//printf("%s :%d: %s\n", str, val, strerror(errno));
	cout<<str<<" :" << val << " :" << strerror(errno)<<endl;;
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
void sendBarrierDoneToAll(){
	//cout<<"CALLED"<<endl;
	int to_server_socket;
	char buf[BUFFER_LEN];
	
	//	cout<<respectiveHostPortNames[i]<<":"<<ports[i];
	int i;
	for(i=0;i<numranks;i++){
	//	cout<<ports[i]<<":"<<respectiveHostPortNames[i]<<endl;
		char temphostname[10];
		strcpy(temphostname,respectiveHostPortNames[i].c_str());
		to_server_socket = connect_to_server(temphostname,ports[i]);
		strcpy(buf,"BARRIER");
		send_msg(to_server_socket, buf, strlen(buf)+1);
		recv_msg(to_server_socket, buf);
		close(to_server_socket);
		sleep(1);
	}	
	
}
void serve_one_connection(int to_client_socket)
{
    int rc, ack_length;
    char buf[BUFFER_LEN];
	int to_server_socket;
	
    ack_length = strlen(SERVER_ACK)+1;
    rc = recv_msg(to_client_socket, buf);	
    
//	cout<<"-------"<<buf<<endl;
	if(strcmp(buf,"MYPORT")==0){
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		rc = recv_msg(to_client_socket, buf);
//		cout<<buf<<endl;
		int indexRank = atoi(buf);
		int toSendRankPort = ports[indexRank];
		memset(buf,'\0',sizeof(buf));
		sprintf(buf,"%d",toSendRankPort);
		send_msg(to_client_socket, buf, 4);
		memset(buf,'\0',sizeof(buf));
		rc = recv_msg(to_client_socket, buf);
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		string str(buf);
		respectiveHostPortNames[indexRank] = str;
		startedranks.push_back(indexRank);
	}else if(strcmp(buf,"EXIT")==0){
		Exitingrankscount++;
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		if(Exitingrankscount>=numranks){
			allRanksFlag=false;
			to_server_socket = connect_to_server(MyName,PPEXEC_PORT_NUM);
			send_msg(to_server_socket, buf, strlen(buf)+1);
			close(to_server_socket);
		}
	}else if(strcmp(buf,"BARRIER")==0){
		//cout<<"BARRIER"<<endl;
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		BarrierCount++;
		if(BarrierCount>=numranks){
	//		cout<<"ALL process barrier\n";
			sendBarrierDoneToAll();
		}
	}else if(strcmp(buf,"TOPORTSEND")==0){
		
		char msg[50];
		send_msg(to_client_socket, SERVER_ACK, ack_length);
		rc = recv_msg(to_client_socket, buf);	
		int reqport;
		int requestrank=atoi(buf);
		if(find(startedranks.begin(),startedranks.end(),requestrank)!=startedranks.end()){
			sleep(1);
			reqport=ports[requestrank];
		}
		else
			reqport=-1;
		
		snprintf(msg, sizeof(msg), "%d", reqport);
	
		send_msg(to_client_socket, msg, strlen(msg)+1);
		rc = recv_msg(to_client_socket, buf);	
		strcpy(msg,respectiveHostPortNames[requestrank].c_str());
		
		send_msg(to_client_socket, msg, strlen(msg)+1);
	}else if(strcmp(buf,"NUMRANKS")==0){
		char msg[50];
		sprintf(msg,"%d",(numranks));
		send_msg(to_client_socket, msg, strlen(msg)+1);
	}
		
	close(to_client_socket);
}
void* toServeConnection(void* arg){
	int to_client_socket = *(int*)arg;
	serve_one_connection(to_client_socket);
//	cout<<"Served" <<endl;
}
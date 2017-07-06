Description:
Overall project course work aims at bulding a prototype of MPI library, supporting different funtions of mpi. 

Project 1:
Write a version of mpiexec and any supporting programs that you
need to run MPI programs which use your own implementation of MPI.
Your mpiexec should be able to run non-MPI programs as well.
It should compile and link as p1.

Project 2:
To implement our own MPI that supports the following functions:

    
MPI_Comm_rank 	
    
MPI_Comm_size 	
    
MPI_Finalize 	
    
MPI_Init 	
    
MPI_Send (just wrapper for Ssend)
    
MPI_Ssend
    
MPI_Recv
    
MPI_Barrier


only need to support MPI_COMM_WORLD for this project.

Project 3:
Enhance your MPI library to support the following functions:

    double MPI_Wtime(void);					//timestamp.c
    int MPI_Comm_dup(MPI_Comm, MPI_Comm *);		//duplicates the communicator
    int MPI_Bcast(void*, int, MPI_Datatype, int, MPI_Comm );	//one will send and everybody will recieve, this is blocking b_cast
    int MPI_Gather(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);		//one will recv frm all others, just opposite of b_cast 

Project 4:
Enhance your MPI library to support the following functions:

        
int MPI_Isend(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
        
int MPI_Irecv(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
        
int MPI_Test(MPI_Request *, int *, MPI_Status *);
       
 int MPI_Wait(MPI_Request *, MPI_Status *);

Project 5:
Enhance your MPI library to support the following:

    
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
 MPI_Op op, int root, MPI_Comm comm);
    
int MPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op);

Files
-----------------
mpi.h and mpi.c ----> has library used by the professor's test programs
ppexec.cc and ppexec.h -----> acts as mpiexec 

./ppexec -n 2 -f somefile ./test_program is the command used to execute.
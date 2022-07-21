#include "mpi.h"
#include <iostream>
#include <cstring>
#include <signal.h>
#include <chrono>
#include <thread>
#include "ConcurrentManager.h"

/**
   Create a concurrent manager by calling the Manager constructor.   
   @param[in] _instance, the benchmark path
   @param[in] _solvers, the set of solvers that will be ran
 */
ConcurrentManager::ConcurrentManager(string _instance, vector<Solver> &_solvers) : 
  Manager(_instance,_solvers),
  nb_threads_per_worker(NULL),
  nb_threads_workers(0)
{
  launch(); //Manager.cc class 
  nb_threads_per_worker = (int*)malloc(sizeof(int)*sizeComm);
}
ConcurrentManager::~ConcurrentManager(){}


/**
 * It's the listener
 */
void ConcurrentManager::listener()
{
  int messageReceived = 0;
  int flag = 0;
  MPI_Status status;
  MPI_Request request;
  for(;;){
    flag = 0;
    MPI_Irecv(&messageReceived, 1, MPI_INT, MPI_ANY_SOURCE, TAG_MANAGER, intercomm, &request);
    while(!flag){
      MPI_Test(&request, &flag, &status);
      if(flag){
        switch (messageReceived) 
          {
          case SOLUTION_FOUND:solutionFound(status.MPI_SOURCE);break;
          case STOP_COM_ULS:messageStopComUls(status.MPI_SOURCE);break;
          case STOP_COM_CLAUSES:messageStopComClauses(status.MPI_SOURCE);break;
          case JOB_FINISHED:return;
          default:
            printf("Listener manager received a message %d from [Worker %d] problem\n"
                   ,messageReceived,status.MPI_SOURCE);
            exit(0);
            break;
          }
        break;
      }else
        std::this_thread::sleep_for(std::chrono::milliseconds(100));//It's not a busy sleep
    } 
  }
}

void ConcurrentManager::messageStopComUls(const int rankOfSender){
  unsigned long nbSendULs = 0;
  MPI_Recv(&nbSendULs, 1, MPI_UNSIGNED_LONG, rankOfSender, TAG_MANAGER, intercomm, MPI_STATUS_IGNORE);
  for(int j = 0 ; j<sizeComm ; j++){ 
    if(j != rankOfSender)nbULsToReceive[j]+=nbSendULs;
  }
  nbWorkerULs++;
  //printf("[MANAGER] [WORKER %d] Barrier ULs : %d/%d\n",rankOfSender,nbWorkerULs,sizeComm);        
  if(nbWorkerULs == sizeComm){// Ok, everybody are contact the manager for ULs : Good !!
    //Now, send the number of ULs that each worker has receive
    for(int j = 0 ; j<sizeComm ; j++)
      MPI_Send(nbULsToReceive+j, 1, MPI_UNSIGNED_LONG, j, TAG_MANAGER, intercomm);
  }
}

void ConcurrentManager::messageStopComClauses(const int rankOfSender){
  unsigned long nbSendClauses = 0;
  MPI_Recv(&nbSendClauses, 1, MPI_UNSIGNED_LONG, rankOfSender
           ,TAG_MANAGER, intercomm, MPI_STATUS_IGNORE);
  for(int j = 0 ; j<sizeComm ; j++)
    if(j != rankOfSender)nbClausesToReceive[j]+=nbSendClauses;
  nbWorkerClauses++;
  //printf("[MANAGER] [WORKER %d] Barrier Clauses : %d/%d\n",rankOfSender,nbWorkerClauses,sizeComm);  
  if(nbWorkerClauses == sizeComm){// Ok, everybody are contact the manager for ULs : Good !!
    //Now, send the number of Clauses that each worker has receive
    for(int j = 0 ; j<sizeComm ; j++)
      MPI_Send(nbClausesToReceive+j, 1, MPI_UNSIGNED_LONG, j, TAG_MANAGER, intercomm);
  }
}
/**
   Run the solver.
 */
void ConcurrentManager::run()
{ 
  int msg = CONCU_MODE, i = 0;// We work in concurrent mode
    
  for (; i < sizeComm ;++i) MPI_Send(&msg, 1, MPI_INT, i, TAG_MSG, intercomm);//Launch worker    
  for (i = 0; i < sizeComm ;++i){
    MPI_Recv(&msg, 1, MPI_INT, i, TAG_MSG, intercomm, MPI_STATUS_IGNORE);
    MPI_Recv(nb_threads_per_worker+i, 1, MPI_INT, i, TAG_MSG, intercomm, MPI_STATUS_IGNORE);
    nb_threads_workers+=nb_threads_per_worker[i];
  }  
  listener();
}// run

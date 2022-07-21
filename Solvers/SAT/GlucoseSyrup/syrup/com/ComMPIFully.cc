
#include "com/ComMPIFully.h"
#include "parallel/SharedCompanion.h"

#include <sched.h>

using namespace Glucose;

ComMPIFully::ComMPIFully
(OptConcurrent* PoptConcurrent,OptAmpharos* PoptAmpharos):
  ComMPI(PoptConcurrent,PoptAmpharos),
  listenerClauseThread(NULL),
  sizeClauseToReceive(0)
{
  for(int i = 0; i < MAX_SIZE_CLAUSE;i++)tableClauseToReceive[i]=0;
}

void ComMPIFully::setBarrier(){
  startingBarrier = new Barrier(solvers->nbThreads+2);
}

void ComMPIFully::start(){
  listenerClauseThread = new std::thread(&ComMPIFully::listenerClause,this);
  startingBarrier->wait();
  bool DoBreak = false;
  int  nbBreak = 0;
  
  int ManagerMessageReceived = 0;
  int ManagerFlag = 1;
  MPI_Status ManagerStatus;
  MPI_Request ManagerRequest;

  int UlMessageReceived = 0;
  int UlFlag = 1;
  MPI_Status UlStatus;
  MPI_Request UlRequest;
  for(;;){
    DoBreak = true;
    if(solvers->getAllEndSearchIsFinished())
      {
        //To stop this method => not this thread !
        //I have to stop my Irecv !
      
        if(!ManagerFlag){
          MPI_Test(&ManagerRequest, &ManagerFlag, &ManagerStatus);
          if(ManagerFlag){
            if(ManagerMessageReceived == STOP_AND_KILL){
              stopAndKillReceived = true;
              printf("[WORKER %d] STOP_AND_KILL OK (During Solution IRECV)\n",dataMPI->mpiRank); 
            }
          }else{
            MPI_Cancel(&ManagerRequest);
            MPI_Test(&ManagerRequest, &ManagerFlag, &ManagerStatus);
          }
        }
        if(!UlFlag){
          MPI_Test(&UlRequest, &UlFlag, &UlStatus);
          if(UlFlag){
            infoImportedUL++;
          }else{
            MPI_Cancel(&UlRequest);
            MPI_Test(&UlRequest, &UlFlag, &UlStatus);
          }
        }
        return;
      }
    
    if(ManagerFlag)
      {//Init or Reinit
        ManagerFlag = 0;
        MPI_Irecv(&ManagerMessageReceived, 1, MPI_INT, 0, TAG_MANAGER, mpiParentComm, &ManagerRequest);
      }

    if(UlFlag)
      {//Init or Reinit
        UlFlag = 0;
        MPI_Irecv(&UlMessageReceived, 1, MPI_INT, MPI_ANY_SOURCE, TAG_UL, MPI_COMM_WORLD, &UlRequest);
      }

    MPI_Test(&ManagerRequest, &ManagerFlag, &ManagerStatus);
    MPI_Test(&UlRequest, &UlFlag, &UlStatus);

    if(ManagerFlag)
      {
        if(ManagerMessageReceived == STOP_AND_KILL){
          solvers->setAllEndSearch(SOLUTION_FOUND_BY_ANOTHER_STATE);
          stopAndKillReceived = true;
          printf("[WORKER %d] STOP_AND_KILL OK (During Search)\n",dataMPI->mpiRank); 
          //Have to verify if UlFlag is to 1 : don't forget a unit clause
          if(UlFlag)infoImportedUL++;
          continue;
        }else{
          printf("Worker listener manager received a message %d from the manager(%d) problem\n"
                 ,ManagerMessageReceived,ManagerStatus.MPI_SOURCE);
          exit(0);
           
        }
        DoBreak = false;
      }

    if(UlFlag)
      {
        solvers->addLearntHightLevel(UlMessageReceived);
        infoImportedUL++;
        DoBreak = false;
      }
    
    if(DoBreak)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(optConcurrent->optSleepingTimeBase));
        nbBreak++;  
      } 
  }
}

void ComMPIFully::listenerClause(){
  MPI_Status ClauseStatus;
  printf("[WORKER %d] Listener thread on the core %d\n",mpiRank,sched_getcpu()); 
  sizeClauseToReceive = 0;
  startingBarrier->wait();
  for(;;){
    MPI_Recv(tableClauseToReceive, MAX_SIZE_CLAUSE
             , MPI_INT, MPI_ANY_SOURCE, TAG_CLAUSE, MPI_COMM_WORLD, &ClauseStatus);
    MPI_Get_count(&ClauseStatus, MPI_INT, &sizeClauseToReceive);
    if(ClauseStatus.MPI_SOURCE == mpiRank){
      printf("[WORKER %d] Listener thread END\n",mpiRank); 
      return;
    }
    solvers->addLearntHightLevel(ClauseStatus.MPI_SOURCE,tableClauseToReceive,sizeClauseToReceive);
    infoImportedClauses++; 
    
  }
}

void ComMPIFully::sendUL(int ul){
  int target = 0;
  for (;target != mpiRank;target++)
    MPI_Send(&ul, 1, MPI_INT, target, TAG_UL, MPI_COMM_WORLD);
  for (target++;target < mpiParentCommSize;target++)
    MPI_Send(&ul, 1, MPI_INT, target, TAG_UL, MPI_COMM_WORLD);
  infoExportedUL+=(mpiParentCommSize-1);
  infoExportedULEach++;
}


void ComMPIFully::sendClause(Clause& c){
  const int sizeClauseToSend = (int)c.size();
  int targetClauseToSend = 0;
  int tableClauseToSend[sizeClauseToSend];
  for(int i=0;i<sizeClauseToSend;i++)tableClauseToSend[i]=toInt(c[i]);
  for (;targetClauseToSend < mpiRank;targetClauseToSend++)
    MPI_Send(tableClauseToSend, sizeClauseToSend, MPI_INT
             ,targetClauseToSend, TAG_CLAUSE, MPI_COMM_WORLD);
  for (targetClauseToSend++;targetClauseToSend < mpiParentCommSize;targetClauseToSend++)
    MPI_Send(tableClauseToSend, sizeClauseToSend, MPI_INT
             ,targetClauseToSend, TAG_CLAUSE, MPI_COMM_WORLD);  
  infoExportedClauses+=(mpiParentCommSize-1);
  infoExportedClausesEach++;
}



void ComMPIFully::stopCommunicationsULs(){
  int send = STOP_COM_ULS;
  unsigned long answer = 0;
  int UlMessageReceived = 0;
  MPI_Send(&send, 1, MPI_INT, 0, TAG_MANAGER, dataMPI->mpiParentComm);
  MPI_Send(&infoExportedULEach, 1, MPI_UNSIGNED_LONG, 0, TAG_MANAGER, dataMPI->mpiParentComm);
  
  //Receive a response from the Manager : The number Of ULs that I have to receive
  //Use to do a barrier !
  MPI_Recv(&answer, 1, MPI_UNSIGNED_LONG, 0, TAG_MANAGER, dataMPI->mpiParentComm, MPI_STATUS_IGNORE);
  //printf("[WORKER %d] Number of ULs to receive : %d/%d\n",mpiRank,infoImportedUL,answer);
  unsigned long left = answer-infoImportedUL;
  while(infoImportedUL < answer){
    MPI_Recv(&UlMessageReceived, 1, MPI_INT, MPI_ANY_SOURCE, TAG_UL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    infoImportedUL++;
  } 
  printf("[WORKER %d] Recuperate the left ULs : %lu/%lu (%lu left)\n",mpiRank,infoImportedUL,answer,left);
  fflush(stdout);      
}

void ComMPIFully::stopCommunicationsClauses(){
  int send = STOP_COM_CLAUSES;
  unsigned long answer = 0;
  MPI_Send(&send, 1, MPI_INT, 0, TAG_MANAGER, dataMPI->mpiParentComm);
  MPI_Send(&infoExportedClausesEach, 1, MPI_UNSIGNED_LONG, 0, TAG_MANAGER, dataMPI->mpiParentComm);
  //Receive a response from the Manager : The number Of ULs that I have to receive
  //Use to do a barrier !
  MPI_Recv(&answer, 1, MPI_UNSIGNED_LONG, 0, TAG_MANAGER, dataMPI->mpiParentComm, MPI_STATUS_IGNORE);
  unsigned long left = answer-infoImportedClauses;
  while(infoImportedClauses < answer){
    MPI_Recv(tableClauseToReceive, MAX_SIZE_CLAUSE
             , MPI_INT, MPI_ANY_SOURCE, TAG_CLAUSE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    infoImportedClauses++;
  } 
  printf("[WORKER %d] Recuperate the left Clauses : %lu/%lu (%lu left)\n",mpiRank,infoImportedClauses,answer,left);
}

void ComMPIFully::stopCommunicationsThreads(){
  int hazard = 0;
  MPI_Send(&hazard, 1, MPI_INT
           ,mpiRank, TAG_CLAUSE, MPI_COMM_WORLD);
  if(listenerClauseThread->joinable())
    listenerClauseThread->join();
}
     
void ComMPIFully::kill(){
  printf("[WORKER %d] End\n",mpiRank);
  MPI_Finalize();       
  exit(0);
}

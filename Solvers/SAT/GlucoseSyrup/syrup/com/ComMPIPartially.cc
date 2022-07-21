
#include "com/ComMPIPartially.h"
#include "parallel/SharedCompanion.h"
#include "com/ComEncoding.h"

using namespace Glucose;

ComMPIPartially::ComMPIPartially
(OptConcurrent* PoptConcurrent,OptAmpharos* PoptAmpharos):
  ComMPI(PoptConcurrent,PoptAmpharos),
  allGatherEnd(0),
  collectifThread(NULL),
  tableClausesToSend(NULL),
  posClausesToSend(0),
  nbClausesToSend(0),
  bufferMaxClauses(0),
  bufferClausesMaxBytes(0),
  bufferClausesMaxMb(0)
{
  for(int i = 0; i < MAX_SIZE_CLAUSE;i++)tableClauseToReceive[i]=0;
}

void ComMPIPartially::setBarrier(){
  startingBarrier = new Barrier(solvers->nbThreads+2);
}

void ComMPIPartially::start(){
  //Sender buffer
  bufferMaxClauses=1000*1000*2; // Buffer of the message of cluses to send
  bufferClausesMaxBytes=sizeof(int)*bufferMaxClauses;
  bufferClausesMaxMb=bufferClausesMaxBytes/1000.0/1000.0; 
  tableClausesToSend = (unsigned int*)malloc(sizeof(unsigned int)*bufferMaxClauses);
  printf("[WORKER %d] Buffer for collectif communication : %2.2fMb\n",mpiRank,bufferClausesMaxMb);
  printf("[WORKER %d] Collectif communication  %d Ms\n",mpiRank,optConcurrent->optSleepingTimeCollectif);  
  collectifThread = new std::thread(&ComMPIPartially::collectifClause,this);
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
        if(ManagerMessageReceived == STOP_AND_KILL)
          {
            solvers->setAllEndSearch(SOLUTION_FOUND_BY_ANOTHER_STATE);
            stopAndKillReceived = true;
            printf("[WORKER %d] STOP_AND_KILL OK (During Search)\n",dataMPI->mpiRank); 
            //Have to verify if UlFlag is to 1 : to don't forget a unit clause
            if(UlFlag)infoImportedUL++;
            continue;
          }
        else
          {
            printf("Worker listener manager received a message %d from the manager(%d) problem\n"
                   ,ManagerMessageReceived,ManagerStatus.MPI_SOURCE);
            exit(0);
          }
        DoBreak = false;
      }

    if(UlFlag)
      {
        solvers->addLearntHightLevel(UlMessageReceived);
        infoImportedUL++;;
        DoBreak = false;
      }
    
    if(DoBreak)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(optConcurrent->optSleepingTimeBase));
        nbBreak++;
      } 
  }
}

void ComMPIPartially::collectifClause(){
  int allEnd[mpiParentCommSize]; //Others workers are stopped ?
  int allSend[mpiParentCommSize]; //Others workers have sent messages ?
  int allPos[mpiParentCommSize]; //Position of sent messages
  int startPosCurrentBloc = 0; //To browse the received bloc of clauses
  int endPosCurrentBloc = 0; //To browse the received bloc of clauses
  int currentRank = 0; // Rank of this computers
  int end = 0; // To verify if a worker has stopped 
  int size = 0; // To browse a bloc of messages
  int maxAllSend = 0; 
  int sumAllSend = 0;
  unsigned int* tableClausesToSendTMP = NULL; // To copy the message to send
  unsigned int* tableClausesToReceive = NULL; // To recuperate messages to receive 
  int posClausesToSendTMP = 0; // To browse the message to send
  tableClausesToSendTMP = (unsigned int*)malloc(sizeof(unsigned int)*bufferMaxClauses);
  tableClausesToReceive = (unsigned int*)malloc (sizeof(unsigned int)*bufferMaxClauses*mpiParentCommSize);
  for(; currentRank < mpiParentCommSize; ++currentRank){
    //Initialization of static tables 
    allEnd[currentRank]=false;
    allSend[currentRank]=0;
    allPos[currentRank]=0;
  }
  startingBarrier->wait(); //Barrier
    
  for(;;){
    // Sleep
    std::this_thread::sleep_for(std::chrono::milliseconds(optConcurrent->optSleepingTimeCollectif));
    
    // First, share ends of solvers 
    end = solvers->getSolutionFound();
    MPI_Allgather(&end, 1, MPI_INT, allEnd, 1, MPI_INT, MPI_COMM_WORLD);
    for(currentRank = 0; currentRank <  mpiParentCommSize; ++currentRank)
      if(allEnd[currentRank])return;
    
    // Copy of the message to send
    mutexClausesToSend.lock();  
    posClausesToSendTMP = posClausesToSend;
    memcpy(tableClausesToSendTMP,tableClausesToSend,posClausesToSendTMP*sizeof(int));
    infoExportedClauses+=((nbClausesToSend)*(mpiParentCommSize-1));
    nbClausesToSend=0;
    posClausesToSend=0;
    mutexClausesToSend.unlock();
    
    // Share sizes of message buffers to send and Calculate some information
    MPI_Allgather(&posClausesToSendTMP, 1, MPI_INT, allSend, 1, MPI_INT, MPI_COMM_WORLD);
    maxAllSend = 0; 
    sumAllSend = 0;
    for(currentRank = 0; currentRank <  mpiParentCommSize; ++currentRank){
      // What is the maximum size of buffers to send 
      if(allSend[currentRank] > maxAllSend)maxAllSend=allSend[currentRank];
      // Calculating good positions
      allPos[currentRank]=sumAllSend;
      // Calculating the sum of sizes of buffers to send
      sumAllSend+=allSend[currentRank];
    }
    if(sumAllSend > bufferMaxClauses)
      printf("WARNING : Buffer for collective communications exceed (%2.2fMb)\n",bufferClausesMaxMb);
    
    
    if(maxAllSend){//If we have some clauses to share 
      //Exhange clauses
      infoExportedPaquetClauses++;    
      MPI_Allgatherv(tableClausesToSendTMP, posClausesToSendTMP, MPI_INT,
                     tableClausesToReceive, allSend, allPos,MPI_INT, MPI_COMM_WORLD);
      infoImportedPaquetClauses++;
      //To put received clauses in CDCL solvers 
      for(currentRank=0; currentRank < mpiParentCommSize;++currentRank){
        if(currentRank != mpiRank){//No received clauses for this worker
          startPosCurrentBloc= allPos[currentRank];
          endPosCurrentBloc= allPos[currentRank+1];
          for(int i = startPosCurrentBloc; i < endPosCurrentBloc;infoImportedClauses++){ 
            //Take the size of the clause
            size = tableClausesToReceive[i++];
            //Copy the clause 
            for(int j = 0; j < size; j++)tableClauseToReceive[j]=tableClausesToReceive[i++];
            // Add the clause to solver
            solvers->addLearntHightLevel(1,(int*)tableClauseToReceive,size);
          }
        }
      }
    } 
  }
}


void ComMPIPartially::sendUL(int ul){
  int target = 0;
  for (;target != mpiRank;target++)
    MPI_Send(&ul, 1, MPI_INT, target, TAG_UL, MPI_COMM_WORLD);
  for (target++;target < mpiParentCommSize;target++)
    MPI_Send(&ul, 1, MPI_INT, target, TAG_UL, MPI_COMM_WORLD);
  infoExportedUL+=(mpiParentCommSize-1);
  infoExportedULEach++;
}

void ComMPIPartially::sendClause(Clause& c){
  mutexClausesToSend.lock();  
  sizeClauseToSend = (int)c.size();
  tableClausesToSend[posClausesToSend++]=sizeClauseToSend;
  for(int i=0;i<sizeClauseToSend;i++)tableClausesToSend[posClausesToSend++]=toInt(c[i]);    
  nbClausesToSend++;
  mutexClausesToSend.unlock();
}

void ComMPIPartially::stopCommunicationsULs(){
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

//Here, it is the collective communications that properly stop communications of clauses
void ComMPIPartially::stopCommunicationsClauses(){}

void ComMPIPartially::stopCommunicationsThreads(){
  if(collectifThread->joinable())
    collectifThread->join();
}

void ComMPIPartially::kill(){
  printf("[WORKER %d] End\n",mpiRank);
  MPI_Finalize();       
  exit(0);
}

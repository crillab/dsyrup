
#include "com/ComMPI.h"
#include "parallel/SharedCompanion.h"
using namespace Glucose;

ComMPI::ComMPI(OptConcurrent* PoptConcurrent,OptAmpharos* PoptAmpharos):
  mode(0),
  optConcurrent(PoptConcurrent),
  optAmpharos(PoptAmpharos),
  dataMPI(new DataMPI()),
  solvers(NULL),
  myAmpharosManager(NULL),
  startingBarrier(NULL),
  mpiRank(0),
  mpiParentComm(MPI_COMM_WORLD),
  mpiParentCommSize(0),
  infoImportedClauses(0),
  infoImportedUL(0),
  infoExportedClauses(0),
  infoExportedUL(0),
  infoExportedClausesEach(0),
  infoExportedULEach(0),
  infoImportedPaquetClauses(0),
  infoExportedPaquetClauses(0),
  coreOfManagerThread(0),
  stopAndKillReceived(false)
{
  int provided,claimed;
  int rc = MPI_Init_thread(0,0,MPI_THREAD_MULTIPLE,&provided);
  MPI_Query_thread(&claimed);    
  if (rc != MPI_SUCCESS || claimed != 3 || provided != MPI_THREAD_MULTIPLE)
    {
      fprintf(stderr, "Error starting MPI program.\n MPI_THREAD_MULTIPLE must be activated\n");
      MPI_Abort(MPI_COMM_WORLD, rc);
    }
  init_wall_time();
  MPI_Get_processor_name(dataMPI->mpiHostname, &dataMPI->mpiHostnameLenght);
  MPI_Comm_get_parent(&dataMPI->mpiParentComm);
  MPI_Comm_rank(dataMPI->mpiParentComm,&dataMPI->mpiRank);
  MPI_Comm_size(dataMPI->mpiParentComm, &dataMPI->mpiParentCommSize); 
  mpiRank = dataMPI->mpiRank;
  mpiParentComm = dataMPI->mpiParentComm;
  mpiParentCommSize = dataMPI->mpiParentCommSize;
}

void ComMPI::linkingOfSolvers(void* Psolvers){
  solvers = (SharedCompanion*)Psolvers;
  unsigned int cores = std::thread::hardware_concurrency();
  printf("[WORKER %d] %d threads as solvers on %d cores detected\n"
         ,dataMPI->mpiRank         
         ,solvers->nbThreads,(int)cores);  
  solvers->mpiCommunicator = this;
  solvers->mpiParentCommSize = mpiParentCommSize;
  solvers->noClausesFromNetwork = optConcurrent->optComNoClausesFromNetwork;
}

void ComMPI::linkingOfManager(){
  MPI_Recv(&mode,1,MPI_INT,0,TAG_MSG,dataMPI->mpiParentComm,MPI_STATUS_IGNORE);//To receive the mode
  MPI_Send(&mode,1,MPI_INT,0,TAG_MSG,dataMPI->mpiParentComm);
  const int nbThreads = solvers->nbThreads;
  MPI_Send(&nbThreads,1,MPI_INT,0,TAG_MSG,dataMPI->mpiParentComm);
  if(mode == AMPHAROS_MODE)myAmpharosManager = new AmpharosManager(this);
}

void ComMPI::solutionDisplayed(int winnerThread){
  int jobFinished = JOB_FINISHED;
  printf("[WORKER %d] [THREAD %d] is the winner !\n",dataMPI->mpiRank,winnerThread);     
  MPI_Send(&jobFinished, 1, MPI_INT, 0, TAG_MANAGER, dataMPI->mpiParentComm);
}

bool ComMPI::solutionFound(int& res){
  //res : 20 = UNSAT, 10 = SAT, 0 = IND
  int sendRes = SOLUTION_FOUND, answer;
  //Contact the Manager to say that we have a solution !
  MPI_Send(&sendRes, 1, MPI_INT, 0, TAG_MANAGER, dataMPI->mpiParentComm);
  //Send SAT || UNSAT || IND
  MPI_Send(&res, 1, MPI_INT, 0, TAG_MANAGER, dataMPI->mpiParentComm);  
  
  //Before, we have to ensure that the message STOP_AND_KILL is received 
  if(!stopAndKillReceived){
    int msg = 0;
    MPI_Recv(&msg, 1, MPI_INT, 0, TAG_MANAGER, dataMPI->mpiParentComm, MPI_STATUS_IGNORE);
    assert(msg = STOP_AND_KILL);
    stopAndKillReceived = true;
    printf("[WORKER %d] STOP_AND_KILL OK (During Solution RECV)\n",dataMPI->mpiRank); 
  }
  printf("[WORKER %d] WAIT RESPONSE OF MANAGER\n",dataMPI->mpiRank); 
  
  //Receive a response from the Manager : Display the solution Or Not
  MPI_Recv(&answer, 1, MPI_INT, 0, TAG_MANAGER, dataMPI->mpiParentComm, MPI_STATUS_IGNORE);
  switch(answer)
    {
    case PRINT_SOLUTION :
      return true;
    case SEND_RESULT :          
      printf("TO DO\n");               
      break;
    case SOLUTION_IND :
      return false;
    default :
      printf("The message received from the master is unknown\n");
      exit(9);
    }
  return false;
}

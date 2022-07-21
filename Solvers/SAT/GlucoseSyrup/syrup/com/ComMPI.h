


#ifndef CO
#define CO
#include <mpi.h>
#include "com/Barrier.h"
#include "com/ComEncoding.h"
#include "com/ComSubsum.h"
#include "../../../../MessageCoding.h"
#include "../../../../OptionCoding.h"
#include "../../../../DataCoding.h"
#include <sys/time.h>
#include "AmpharosManager.h"

namespace Glucose {
// Derived classes
class Clause;  
class SharedCompanion;
class ComMPI
  {
  public:
    ComMPI(OptConcurrent* PoptConcurrent,OptAmpharos* PoptAmpharos);
    void linkingOfSolvers(void* Psolvers);//To link solvers to this class
    void linkingOfManager(); //First contact with the manager    
    bool solutionFound(int& res); //When a solver find the solution 
    void solutionDisplayed(int winnerThread); //When a solver has finiched to display the solution   

    //Principal variables
    int mode; //Cube And Conquer OR Concurential
    OptConcurrent* optConcurrent;
    OptAmpharos* optAmpharos;    
    DataMPI* dataMPI;
    SharedCompanion* solvers;
    AmpharosManager* myAmpharosManager;
    Barrier* startingBarrier;

    int mpiRank;
    MPI_Comm mpiParentComm; 
    int mpiParentCommSize;
    
    //Information on imported/exported ULs/Clauses
    unsigned long infoImportedClauses;
    unsigned long infoImportedUL;
    unsigned long infoExportedClauses;
    unsigned long infoExportedUL;
    unsigned long infoExportedClausesEach;
    unsigned long infoExportedULEach;
    int infoImportedPaquetClauses;
    int infoExportedPaquetClauses;

    //To save the numerus of core that execute the thread dedicated to manage principal communication (this class)
    int coreOfManagerThread; 
    inline void setCoreOfManagerThread(int tmp){coreOfManagerThread = tmp;}
    inline int getCoreOfManagerThread(){return coreOfManagerThread;}

    //To avoid deadlock and ensure to receive all communication  
    bool stopAndKillReceived; //To ensure to receive the message STOP_AND_KILL
    std::mutex mutexOneThreadInTurn; //To ensure that each thread communicate with the manager in turn  
    inline void startCommunications(){mutexOneThreadInTurn.lock();}
    inline void endCommunications(){mutexOneThreadInTurn.unlock();}
    
    
    //For the wall time
    double start_wall_time;
    double current_wall_time;
    struct timeval time;
    inline void init_wall_time(){
      gettimeofday(&time,NULL);
      start_wall_time = (double)time.tv_sec + (double)time.tv_usec * .000001;
    }

    inline double get_wall_time(){
      gettimeofday(&time,NULL);
      current_wall_time = (double)time.tv_sec + (double)time.tv_usec * .000001;
      double result = current_wall_time-start_wall_time;
      return result;
    }

    //Methods call into solvers 
    virtual void start()=0;
    virtual void setBarrier()=0;
    virtual void sendUL(int ul)=0;
    virtual void sendClause(Clause& c)=0;
    virtual void stopCommunicationsULs()=0;
    virtual void stopCommunicationsClauses()=0;
    virtual void stopCommunicationsThreads()=0;
    virtual void kill()=0;
  };
}
#endif

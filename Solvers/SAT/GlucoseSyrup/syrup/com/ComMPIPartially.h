#include "com/ComMPI.h"

namespace Glucose {

  class ComMPIPartially : public ComMPI {
  public:
    ComMPIPartially(OptConcurrent* PoptConcurrent,OptAmpharos* PoptAmpharos);
    //methods inherited from the root class ComMPI
    void start();
    void setBarrier();
    void sendUL(int ul);
    void sendClause(Clause& c);
    void stopCommunicationsULs();
    void stopCommunicationsClauses();
    void stopCommunicationsThreads();
    void kill();
    
    //To collect clauses
    bool allGatherEnd; //To verify the end of collective communications    
    void collectifClause();    
    std::thread* collectifThread;
    
    unsigned int* tableClausesToSend;//Table of clauses to send
    int posClausesToSend;//Lenght of tableClausesToSend
    int nbClausesToSend;//Number of clauses in tableClausesToSend 
    int sizeClauseToSend;//The lenght of one clause 
    std::mutex mutexClausesToSend;//To protect clauses to send
    
    unsigned int tableClauseToReceive[MAX_SIZE_CLAUSE];//Table to receive one clause
    int bufferMaxClauses; //Size of buffer for collective communications 
    int bufferClausesMaxBytes; //Size of buffer for collective communications in Bytes
    double bufferClausesMaxMb; //Size of buffer for collective communications in Mb
  };
}

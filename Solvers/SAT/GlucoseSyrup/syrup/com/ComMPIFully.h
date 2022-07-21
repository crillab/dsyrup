


#include "com/ComMPI.h"
#include "com/ComSubsum.h"
#include <vector>

namespace Glucose {

  class ComMPIFully : public ComMPI {
  public:
    ComMPIFully(OptConcurrent* PoptConcurrent,OptAmpharos* PoptAmpharos);
    
    //methods inherited from the root class ComMPI
    void start();
    void setBarrier();
    void sendUL(int ul);
    void sendClause(Clause& c);
    void stopCommunicationsULs();
    void stopCommunicationsClauses();
    void stopCommunicationsThreads();
    void kill();
    
    //To receive clauses
    void listenerClause();//Method that execute the listener thread
    std::thread* listenerClauseThread;//The thread to receive clauses
    int tableClauseToReceive[MAX_SIZE_CLAUSE];//Table to receive clauses 
    int sizeClauseToReceive;//Lenght of received clauses
  };
}

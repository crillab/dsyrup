#include "com/ComMPI.h"
namespace Glucose {

  class ComMPInoCom : public ComMPI {
  public:
    ComMPInoCom(OptConcurrent* PoptConcurrent,OptAmpharos* PoptAmpharos);
    void start();
    void setBarrier();
    void sendUL(int ul);
    void sendClause(Clause& c);
    void stopCommunicationsULs();
    void stopCommunicationsClauses();
    void stopCommunicationsThreads();
    void kill();
  };
}

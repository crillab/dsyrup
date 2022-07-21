
#include "com/ComMPInoCom.h"
#include "com/ComMPI.h"
#include "parallel/SharedCompanion.h"

using namespace Glucose;

ComMPInoCom::ComMPInoCom
(OptConcurrent* PoptConcurrent,OptAmpharos* PoptAmpharos):ComMPI(PoptConcurrent,PoptAmpharos)
{

}

void ComMPInoCom::setBarrier(){
  startingBarrier = new Barrier(solvers->nbThreads+1);
}


void ComMPInoCom::start(){startingBarrier->wait();}

void ComMPInoCom::sendUL(int ul){}

void ComMPInoCom::sendClause(Clause& c){}

void ComMPInoCom::stopCommunicationsULs(){}
void ComMPInoCom::stopCommunicationsClauses(){}
void ComMPInoCom::stopCommunicationsThreads(){}

void ComMPInoCom::kill(){ 
  MPI_Finalize();       
  exit(0);
}

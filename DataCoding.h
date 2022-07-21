#ifndef DATA
#define DATA

#include "mpi.h"
#include <thread>
#include <mutex>  

struct DataMPI {
  int mpiRank;
  int mpiParentCommSize;
  MPI_Comm mpiParentComm;
  char mpiHostname[256]; 
  int mpiHostnameLenght;
};

#endif


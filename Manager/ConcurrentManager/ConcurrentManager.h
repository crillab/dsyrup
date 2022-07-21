#ifndef ConcurrentManage_h
#define ConcurrentManage_h

#include "../Main/Manager.h"
#include "../../MessageCoding.h"
#include "../Main/Parser.h"

using namespace std;

#define SIZE_BUFFER 1024

/**
   Can run parallel solver in concurrence.
 */
class ConcurrentManager : public Manager {
 private:
  
 public:
  ~ConcurrentManager();
  ConcurrentManager(string instance, vector<Solver> &solvers);

  void run();  
  void listener();
  void messageStopComUls(const int rankOfSender);
  void messageStopComClauses(const int rankOfSender);
  
  //Data of threads 
  int* nb_threads_per_worker;//the number of threads for each worker
  int nb_threads_workers;//The total number of threads for all worker
};

#endif

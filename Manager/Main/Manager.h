#ifndef Manage_h
#define Manage_h

#include "../../MessageCoding.h"
#include "../Main/Parser.h"
#include <thread>
#include <mutex> 

using namespace std;

#define SIZE_BUFFER 1024

/**
   Abstract class including data in order to launch the solvers provided in constructor parameters.
   This class have to be implemented to work.
 */
class Manager
{
 public:

  Manager(string instance, vector<Solver> &solvers);
  ~Manager();

  void launch();
  void solutionFound(const int rankOfSender);
  

  //Data of solvers
  string instance; /* Instance of problem (cnf, ...) */ 
  vector<Solver> &solvers; /* The objet representing all information of solvers */
  int nbSolvers; /* The number of animals */  
  int *np; /* The number of threads for each worker */
  char **cmds; /* The command line for each worker */
  char ***arrayOfArgs; /* Parametre of each worker (option) */ 
  
  //Data of MPI
  MPI_Info *infos; /* To launch each command and put the hostname */
  MPI_Comm intercomm; /* The intercommunicator created by MPI_Comm_spawn_multiple */
  int sizeComm; /* Size of intercom : it's the number of workers */
  
  //Data of threads and to find the solution
  //std::mutex mutexSolution; /* Mutex usefull in order to find only one solution */
  //std::mutex mutexULs;
  
  bool solutionOk; /* True if the solution is found */ 
  int rankOfTheFirstSolution;
  int nbWorkerFinished; /* The number of workers that have closed theirs solvers */ 
  int nbWorkerULs; /* The number of workers that have finished to communicate ULs */ 
  int nbWorkerClauses; /* The number of workers that have finished to communicate clauses */
  
  unsigned long* nbClausesToReceive; /* Number of clauses that each computer has to receive */
  unsigned long* nbULsToReceive; /* Number of uls that each computer has to receive */

  //To get the wall time 
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

};

#endif

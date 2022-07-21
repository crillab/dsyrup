
#include "mpi.h"
#include <iostream>
#include <cstring>

#include <signal.h>

#include "Manager.h"

/**
   Create a manager : init the data structures to run
   the solvers.
   
   @param[in] _instance, the benchmark path
   @param[in] _solvers, the set of solvers that will be ran
*/
Manager::Manager(string _instance, vector<Solver> &_solvers) :
  instance(_instance), solvers(_solvers)
{
  setvbuf(stdout, NULL, _IONBF, 0);
  init_wall_time();
  nbSolvers = _solvers.size();
  np = new int[nbSolvers];
  cmds = new char*[nbSolvers];
  arrayOfArgs = new char **[nbSolvers];
  infos = new MPI_Info[nbSolvers];
  //mutexSolution.lock();
  solutionOk = false;
  nbWorkerFinished = 0;
  nbWorkerULs = 0;
  nbWorkerClauses = 0;
  rankOfTheFirstSolution = -1;
  //mutexSolution.unlock();
  for(int i = 0 ; i<nbSolvers ; i++)
    {
      Machine &m = solvers[i].machine;
      np[i] = m.npernode;

      cmds[i] = new char[(solvers[i].executablePath).size() + 1];
      strcpy(cmds[i], (solvers[i].executablePath).c_str());      

      arrayOfArgs[i] = new char *[(solvers[i].options).size() + 1];
      for(unsigned int j = 0 ; j<(solvers[i].options).size() ; j++)
        {
          arrayOfArgs[i][j] = new char[(solvers[i].options[j]).size() + 1];
          strcpy(arrayOfArgs[i][j], (solvers[i].options[j]).c_str());          
        }
      arrayOfArgs[i][(solvers[i].options).size()] = NULL;

      MPI_Info_create(&infos[i]);

      char hostname[m.hostname.size() + 1];
      strcpy(hostname, m.hostname.c_str());
      MPI_Info_set(infos[i], "host", hostname);
    }
  sizeComm = 0;
}// constructor

/**
   Destructor.
*/
Manager::~Manager()
{
  delete []np;

  for(int i = 0 ; i<nbSolvers ; i++) delete []cmds[i];
  delete []cmds;
  
  for(int i = 0 ; i<nbSolvers ; i++)
    {
      char **tmp = arrayOfArgs[i];
      while(*tmp)
        {
          delete [](*tmp);
          tmp++;
        }
      delete []arrayOfArgs[i];
    }
  
  delete []arrayOfArgs;

  delete []infos;
}// destructor


void Manager::launch()
{
  
  int errsize = 0;
  for(int i = 0 ; i<nbSolvers; i++)errsize+=np[i]; 
  int errcodes[errsize];
   
  /*for(int i = 0 ; i<nbSolvers; i++)
    {
      printf("[MANAGER] : np[%d] : %d - nbSolvers : %d\n",i,np[i],nbSolvers); 
      }*/
  MPI_Comm_spawn_multiple(nbSolvers, cmds, arrayOfArgs, np, infos, 0, MPI_COMM_WORLD, &intercomm, errcodes);
  MPI_Comm_remote_size(intercomm, &sizeComm);
   
  //cout << "\n[MANAGER] Workers are now running : " << sizeComm << endl;
  nbClausesToReceive = (unsigned long*)calloc(sizeComm,sizeof(unsigned long));
  nbULsToReceive = (unsigned long*)calloc(sizeComm,sizeof(unsigned long));
  printf("D-Syrup started ...\n"); 
}


void Manager::solutionFound(const int rankOfSender)
{
  int msg = 0;
  int resultat = 0;
  MPI_Recv(&resultat, 1, MPI_INT, rankOfSender, TAG_MANAGER, intercomm, MPI_STATUS_IGNORE);
  
  if(!solutionOk && resultat != 0){//If it's the first worker with a solution
    printf("[MANAGER] Solution : %f (%s)\n",get_wall_time(),resultat==20?"UNSAT":(resultat==10?"SAT":"PROBLEM"));
    solutionOk = true;
    rankOfTheFirstSolution = rankOfSender;
    msg = STOP_AND_KILL;
    for(int j = 0 ; j<sizeComm ; j++){// To stop all workers 
      MPI_Send(&msg, 1, MPI_INT, j, TAG_MANAGER, intercomm);
    }
  }
  nbWorkerFinished++;
  //printf("[MANAGER] [WORKER %d] Barrier Threads Solvers : %d/%d \n",rankOfSender,nbWorkerFinished,sizeComm);
  if(nbWorkerFinished == sizeComm){// Ok, everybody are contact the manager : Good !!
    //Free solvers
    for(int j = 0 ; j<sizeComm ; j++){
      if(j==rankOfTheFirstSolution)msg = PRINT_SOLUTION;
      else msg = SOLUTION_IND;
      MPI_Send(&msg, 1, MPI_INT, j, TAG_MANAGER, intercomm);
    }
  }
}

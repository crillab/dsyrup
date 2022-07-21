#include <iostream>
#include <sstream>
#include <assert.h>

using namespace std;

#include "InfoSystem.h"
#include "Assignment.h"
#include "../AmpharosManager/Tree.h"
#include "Parser.h"
#include "../ConcurrentManager/ConcurrentManager.h"
#include "../AmpharosManager/AmpharosManager.h"
#include "../Main/Manager.h"
/**
   Helping function.
 */
void help()
{
  cout << "[USAGE] ./treeManager [BENCHNAME] CONFIG_FILE" << endl;
  cout << " - BENCHNAME, the instance\n" << endl;
  cout << " - CONFIG_FILE, give all the information to run the solver\n" << endl;
}// help


/**
   Main function.
 */
int main(int argc, char **argv)
{   
  setbuf(stdout,NULL);
  if(argc < 2){
    help();
    printf("c [MANAGER] Erreur parameter\n");
    exit(0);
  }
  Parser *p = NULL;
  
  if(argc == 2) p = new Parser(argv[2]);
  if(argc >= 3) p = new Parser(argv[1], argv[2]);
  p->printInformation();
  int provided,claimed;
  int rc = MPI_Init_thread(&argc,&argv,MPI_THREAD_MULTIPLE,&provided);
  MPI_Query_thread(&claimed);    
  if (rc != MPI_SUCCESS || claimed != 3)
  {
    fprintf(stderr, "Error starting MPI program. Terminating.\nYou should check that ompi_info | grep -i thread displays MPI_THREAD_MULTIPLE: yes\nIf not, you must install Open MPI with this command line :\n ./configure --enable-mpi-thread-multiple\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  if(p->method == "concurrent")
    {      
      ConcurrentManager cc(p->instance, p->solvers);
      cc.run();
    }
  else if(p->method == "eps")
    {
      printf("EPS don't work in this version\n");
      exit(1);
    }
  else if(p->method == "ampharos")
    {
      assert(p->sizeAssignment);
      AmpharosManager a(p->instance, p->solvers, p->sizeAssignment);
      a.run();
    }
  else
    {
      cerr << "c The only available methods are: concurrent or ampharos" << endl;
      exit(1);
    }
  printf("[MANAGER] End\n");
  delete(p);
  MPI_Finalize();
  exit(0);
}// main


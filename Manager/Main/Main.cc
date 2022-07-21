// MIT License

// Copyright (c) 2022 Univ. Artois and CNRS
// Copyright (c) 2022 Centre de Recherche en Informatique de Lens

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


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


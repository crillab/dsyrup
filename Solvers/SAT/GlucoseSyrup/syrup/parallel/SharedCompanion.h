/***************************************************************************************[SharedCompanion.h]
 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                LRI  - Univ. Paris Sud, France (2009-2013)
                                Labri - Univ. Bordeaux, France

 Syrup (Glucose Parallel) -- Copyright (c) 2013-2014, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                Labri - Univ. Bordeaux, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it 
is based on. (see below).

Glucose-Syrup sources are based on another copyright. Permissions and copyrights for the parallel
version of Glucose-Syrup (the "Software") are granted, free of charge, to deal with the Software
without restriction, including the rights to use, copy, modify, merge, publish, distribute,
sublicence, and/or sell copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

- The above and below copyrights notices and this permission notice shall be included in all
copies or substantial portions of the Software;
- The parallel version of Glucose (all files modified since Glucose 3.0 releases, 2013) cannot
be used in any competitive event (sat competitions/evaluations) without the express permission of 
the authors (Gilles Audemard / Laurent Simon). This is also the case for any competitive event
using Glucose Parallel as an embedded SAT engine (single core or not).


--------------- Original Minisat Copyrights

Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

/* This class is responsible for protecting (by mutex) information exchange between threads.
 * It also allows each solver to send / receive clause / unary clauses.
 *
 * Only one sharedCompanion is created for all the solvers
 */


#ifndef SharedCompanion_h
#define SharedCompanion_h
#include "core/SolverTypes.h"
#include "parallel/ParallelSolver.h"
//#include "parallel/SolverCompanion.h"
#include "parallel/ClausesBuffer.h"

#include "com/ComMPI.h"
#include <mutex>  
namespace Glucose {

    
  class SharedCompanion {
    friend class MultiSolvers;
    friend class ParallelSolver;
  public:
    SharedCompanion(int nbThreads=0);
    int nbThreads; // Number of threads
    bool panicMode; // Keep more memory
    //Principal methods
    void setNbThreads(int _nbThreads); // Sets the number of threads (cannot by changed once the solver is running)
    void newVar(bool sign);            // Adds a var (used to keep track of unary variables)
    bool addSolver(ParallelSolver*);   // attach a solver to accompany 
    void addLearnt(ParallelSolver *s,Lit unary);   // Add a unary clause to share
    bool addLearnt(ParallelSolver *s, Clause & c); // Add a clause to the shared companion, as a database manager
    bool getNewClause(ParallelSolver *s, int &th, vec<Lit> & nc); // gets a new interesting clause for solver s 
    Lit getUnary(ParallelSolver *s);                              // Gets a new unary literal
    int nbVars(); //Number of variables
    
    //For hight level communication : MPI
    void addLearntHightLevel(int ul);
    void addLearntHightLevel(int worker, int* my_clause, int my_clause_size);
    
    ComMPI* mpiCommunicator; //Communicator, set by ComMPI.cc
    int mpiParentCommSize; //Number of computer, set by ComMPI.cc 
    bool noClausesFromNetwork; // Not to add clauses from MPI (network), set by ComMPI.cc
    
    //To obtain a lot of information 
    
    int infoImportedULusefull; //Number of unit clauses usefull (received and not known by this solver)
    inline int getNbUnitLit(){return unitLit.size();}
    inline int getIntegerSharedMemory(){return clausesBuffer.getIntegerSharedMemory();}
    inline int getNbForcedRemovedClauses(){return clausesBuffer.getNbForcedRemovedClauses();}
    inline int getNbForcedRemovedBinaryClauses(){return clausesBuffer.getNbForcedRemovedBinaryClauses();}
    inline int getNbBinaryClauses(){return clausesBuffer.getNbBinaryClauses();}
    inline int getNbClausesPassIntoBuffer(){return clausesBuffer.getNbClausesPassIntoBuffer();}
    inline int getSizeSharedMemory(){return clausesBuffer.getSizeSharedMemory();}
        
    inline int getInfoExportedClauses(){return mpiCommunicator->infoExportedClauses;}
    inline int getInfoImportedClauses(){return mpiCommunicator->infoImportedClauses;}
    inline int getInfoExportedPaquetClauses(){return mpiCommunicator->infoExportedPaquetClauses;}
    inline int getInfoImportedPaquetClauses(){return mpiCommunicator->infoImportedPaquetClauses;}
    inline int getInfoExportedUL(){return mpiCommunicator->infoExportedUL;}
    inline int getInfoImportedUL(){return mpiCommunicator->infoImportedUL;}
    inline int getInfoImportedULusefull(){return infoImportedULusefull;}
    void printStats(); // Printing statistics of all solvers

       
    //To stop the search of a solver or to kill a solver 

    bool solution; //True if at least one solver of this computer has the solution (SAT or UNSAT) 
    std::mutex mutexSolution; // Mutex to protect the varaible solution (write and read)
    ParallelSolver *solverSolution; //The first solver to have found the solution       
    inline ParallelSolver* winner(){return solverSolution;} // To get the first solver with a solution 
    bool solutionFound(ParallelSolver *s); // Returns true if you are the first solver with a solution
    bool getSolutionFound(); //Get for variable solution 
    void setSolutionFound(bool tmp); //Set for variable solution 
    
    /* Warning : Solvers can stop their searchs without to have found solutions ! 
     * For each solver, variable endSearch (Solver.cc) is fixed to stop the search or kill the solver,
     * setAllEndSearch() can be usefull to say to all solver to stop search or kill. 
     */
    void setAllEndSearch(const int tmp);
    /*
     *  getAllEndSearchIsFinished() verify if all solver are going to stop their search and kill yourself
     */ 
    bool getAllEndSearchIsFinished();

    //Operations to obtain the wall time
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

    //For Ampharos 
    void setPharusInitialization(unsigned int nbConflicts);

  protected:
    // A set of mutexs
    std::mutex mutexSharedCompanion; // mutex for any high level sync between all threads (like reportf)
    std::mutex mutexSharedClauseCompanion; // mutex for reading/writing clauses on the blackboard
    std::mutex mutexSharedUnitCompanion; // mutex for reading/writing unit clauses on the blackboard 
    
    vec<ParallelSolver*> solvers; // The set of solvers   
    ClausesBuffer clausesBuffer; // A big blackboard for all threads sharing non unary clauses
	
    vec<int> nextUnit; // indice of next unit clause to retrieve for solver number i 
    vec<Lit> unitLit;  // Set of unit literals found so far
    vec<lbool> isUnary; // sign of the unary var (if proved, or l_Undef if not)	
    
    // To randomize
    double    random_seed;

    // Returns a random float 0 <= x < 1. Seed must never be 0.
    static inline double drand(double& seed) {
      seed *= 1389796;
      int q = (int)(seed / 2147483647);
      seed -= (double)q * 2147483647;
      return seed / 2147483647; }

    // Returns a random integer 0 <= x < size. Seed must never be 0.
    static inline int irand(double& seed, int size) {
      return (int)(drand(seed) * size); }
    
    
    //I don't obtain good result with this class !! but it is a good idea, so I keep it
    ComSubsum* subsumR;
    inline void initSubsumReceive(){
      mutexSharedClauseCompanion.lock();
      subsumR = new ComSubsum((nbVars()+1)*2,false);
      mutexSharedClauseCompanion.unlock();
    }
    inline void clearSubsumReceive(){
      mutexSharedClauseCompanion.lock();  
      if(subsumR)
        subsumR->clear(false);
      mutexSharedClauseCompanion.unlock();
    }
  };
}
#endif

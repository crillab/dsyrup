/***************************************************************************************[MultiSolvers.cc]
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
#include "com/ComMPI.h"
#include "parallel/MultiSolvers.h"
#include "mtl/Sort.h"
#include "utils/System.h"
#include "simp/SimpSolver.h"
#include <errno.h>
#include <string.h>
#include "parallel/SolverConfiguration.h"
#include <chrono>

using namespace Glucose;

extern const char* _parallel ;
extern const char* _cunstable;
// Options at the parallel solver level
static IntOption opt_nbsolversmultithreads (_parallel, "nthreads", "Number of core threads for syrup (0 for automatic)", 0);
static IntOption opt_maxnbsolvers (_parallel, "maxnbthreads", "Maximum number of core threads to ask for (when nbthreads=0)", 4);
static IntOption opt_maxmemory    (_parallel, "maxmemory", "Maximum memory to use (in Mb, 0 for no software limit)", 20000);
static IntOption opt_statsInterval (_parallel, "statsinterval", "Seconds (real time) between two stats reports", 5);
//
// Shared with ClausesBuffer.cc
BoolOption opt_whenFullRemoveOlder (_parallel, "removeolder", "When the FIFO for exchanging clauses between threads is full, remove older clauses", false);
IntOption opt_fifoSizeByCore(_parallel, "fifosize", "Size of the FIFO structure for exchanging clauses between threads, by threads", 100000);
//
// Shared options with Solver.cc 
BoolOption    opt_dontExportDirectReusedClauses (_cunstable, "reusedClauses",    "Don't export directly reused clauses", false);
BoolOption    opt_plingeling (_cunstable, "plingeling",    "plingeling strategy for sharing clauses (exploratory feature)", false);

IntOption opt_exportLBDsearch(_parallel, "lbdSearch", "The limit to exchange clauses during search", 2);

//For MPI communications 
BoolOption    com_display(_parallel, "com-display","Display all information.", true);
BoolOption    com_ul_sharing(_parallel, "com-ul-sharing","Sharing unit literals between computers.", true);
BoolOption    com_clause_sharing(_parallel, "com-clause-sharing","Sharing clauses between computers.", true);
IntOption     com_how_clause_sharing(_parallel, "com-how-clause-sharing","Fully Hybrid mode (1) or Partially Hybrid mode (2)", 2);
IntOption     com_alea(_parallel, "com-alea","Alea", 0, IntRange(0,INT32_MAX));
IntOption     com_conflicts(_parallel, "com-conflicts","The solvers make only X conflicts and return unsatisfiable", 0, IntRange(0,INT32_MAX));

BoolOption    com_no_com(_parallel, "com-no-com","No message passing communication", false);
BoolOption    com_no_com1(_parallel, "com-no-com1","No imported clauses into solvers", false);
BoolOption    comNoClausesFromNetwork(_parallel, "com-no-clauses-from-network","No imported clauses from network into the shared memory", false);

BoolOption    com_encoding(_parallel, "com-encoding","Encode en Decode clauses to speed up network speed", false);

BoolOption    com_analyse_promoted(_parallel, "com-analyse-promoted","Display promoted variables (distributed usefull clauses)", false);

IntOption     com_sleeping_time_base(_parallel, "com-sleeping-time-base","Base", 100, IntRange(0,INT32_MAX));

IntOption     com_sleeping_time_listener(_parallel, "com-sleeping-time-listener","Listener", 100, IntRange(0,INT32_MAX));
IntOption     com_sleeping_time_sender(_parallel, "com-sleeping-time-sender","Sender", 1000, IntRange(0,INT32_MAX));
IntOption     com_sleeping_time_collectif(_parallel, "com-sleeping-time-collectif","Collectif", 1000, IntRange(0,INT32_MAX));

IntOption     com_ring_limit(_parallel, "com-ring-limit","Filter to exchange clauses", 7, IntRange(0,INT32_MAX));
IntOption     com_group(_parallel, "com-group","Topology following several groups", 0, IntRange(0,INT32_MAX));

IntOption     com_pharus_initialization_duration(_parallel, "com-pharus-initialization-duration","Number of conflicts for initialization phase of ampharos", 10000, IntRange(0,INT32_MAX));
IntOption     com_pharus_subproblem_work_duration(_parallel, "com-pharus-subproblem-work-duration","Number of conflicts for work on the same subproblem", 10000, IntRange(0,INT32_MAX));

IntOption     com_pharus_tree_heuristic(_parallel, "com-pharus-tree-heuristic","Variable choice heuristic to extend the tree of Ampharos", 0, IntRange(0,2));
IntOption     com_pharus_speed_extend(_parallel, "com-pharus-speed-extend","Speed of the extention of tree", 1, IntRange(0,INT32_MAX));

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

static inline double cpuTime(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000; }



/* First of all, this class generate the first solver to load the formula once, 
   thereafter, all others solvers are clone of this one !*/
MultiSolvers::MultiSolvers(ParallelSolver *s):
  mpiCommunicator(NULL)
  ,use_simplification(true)
  ,sharedcomp(new SharedCompanion())
  ,ok (true)
  ,maxnbthreads(4)
  ,nbthreads(opt_nbsolversmultithreads)
  ,nbsolvers(opt_nbsolversmultithreads)
  ,allClonesAreBuilt(0)
  ,showModel(false)
  ,winner(-1)
  ,var_decay(1 / 0.95)
  ,clause_decay(1 / 0.999)
  ,cla_inc(1)
  ,var_inc(1)
  ,random_var_freq(0.02)
  ,restart_first(100)
  ,restart_inc(1.5)
  ,learntsize_factor((double)1/(double)3)
  ,learntsize_inc(1.1)
  ,expensive_ccmin(true)
  ,polarity_mode(polarity_false)
  ,maxmemory(opt_maxmemory)
  ,maxnbsolvers(opt_maxnbsolvers)
  ,verb(0)
  ,verbEveryConflicts(10000)
  ,numvar(0)
  ,numclauses(0)
{
  // Add only the first solver 
  solvers.push(s);
  s->verbosity = 0; 
  s->setThreadNumber(0);
  s->setLimitLBDsearch(opt_exportLBDsearch);
  s->sharedcomp = sharedcomp;
  sharedcomp->addSolver(s);
  assert(solvers[0]->threadNumber() == 0);
}

/* Call the constructor with a new ParallelSolver in parameter */ 
MultiSolvers::MultiSolvers() : MultiSolvers(new ParallelSolver(-1)) {}

MultiSolvers::~MultiSolvers(){}

Var MultiSolvers::newVar(bool sign, bool dvar){
  assert(solvers[0] != NULL);
  numvar++;
  sharedcomp->newVar(sign);
  if(!allClonesAreBuilt) { // At the beginning we want to generate only solvers 0
    solvers[0]->newVar(sign,dvar);
  }else{
    for(int i=0;i<nbsolvers;i++) {
      solvers[i]->newVar(sign,dvar);
    }
  }
  return numvar;
}

bool MultiSolvers::addClause_(vec<Lit>&ps) {
  assert(solvers[0] != NULL); // There is at least one solver.
  // Check if clause is satisfied and remove false/duplicate literals:
  if (!okay())  return false;

  sort(ps);
  Lit p; int i, j;
  for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
    if (solvers[0]->value(ps[i]) == l_True || ps[i] == ~p)
      return true;
    else if (solvers[0]->value(ps[i]) != l_False && ps[i] != p)
      ps[j++] = p = ps[i];
  ps.shrink(i - j);
  
  
  if (ps.size() == 0) {
    return ok = false;
  }
  else if (ps.size() == 1){
    assert(solvers[0]->value(ps[0]) == l_Undef); // TODO : Passes values to all threads
    solvers[0]->uncheckedEnqueue(ps[0]);
    if(!allClonesAreBuilt) {
	return ok = ( (solvers[0]->propagate()) == CRef_Undef); // checks only main solver here for propagation constradiction
    } 

    // Here, all clones are built.
    // Gives the unit clause to everybody
    for(int i=0;i<nbsolvers;i++)
      solvers[i]->uncheckedEnqueue(ps[0]);
    return ok = ( (solvers[0]->propagate()) == CRef_Undef); // checks only main solver here for propagation constradiction
  }else{
    //		printf("Adding clause %0xd for solver %d.\n",(void*)c, thn);
    // At the beginning only solver 0 load the formula
    solvers[0]->addClause(ps);
    
    if(!allClonesAreBuilt) {
	numclauses++;
	return true;
    }
    // Clones are built, need to pass the clause to all the threads 
    for(int i=1;i<nbsolvers;i++) {
      solvers[i]->addClause(ps);
    }
    numclauses++;
  }
  return true;
}


bool MultiSolvers::simplify(){
  assert(solvers[0] != NULL); // There is at least one solver.
  if (!okay()) return false;
  return ok = solvers[0]->simplify(); 
}


bool MultiSolvers::eliminate(){
    // TODO allow variable elimination when all threads are built!
    assert(allClonesAreBuilt==false);
    SimpSolver * s = (SimpSolver*)getPrimarySolver();
    s->use_simplification = use_simplification;
    if(!use_simplification) return true;
    return s->eliminate(true);
}


/**
 * To launch a CDCL solver in a thread
 */
void localLaunch(ParallelSolver* s){s->solve();}

void MultiSolvers::printStats(){
  for(int i=0;i<solvers.size();i++)solvers[i]->reportProgress();
}

/**
 * Generate All solvers
 */
void MultiSolvers::generateAllSolvers() {
    assert(solvers[0] != NULL);
    assert(allClonesAreBuilt==false);
    for(int i=1;i<nbsolvers;i++) {
	ParallelSolver *s  = (ParallelSolver*)solvers[0]->clone();
	solvers.push(s);
	s->verbosity = 0; // No reportf in solvers... All is done in MultiSolver
	s->setThreadNumber(i);
        s->setLimitLBDsearch(opt_exportLBDsearch);
	s->sharedcomp =   this->sharedcomp;
	this->sharedcomp->addSolver(s);
	assert(solvers[i]->threadNumber() == i);
    }
    adjustParameters(); 
    allClonesAreBuilt = true;
}

// Well, all those parameteres are just naive guesses... No experimental evidences for this.
void MultiSolvers::adjustParameters() {
    SolverConfiguration::configure(this,nbsolvers);
}

void MultiSolvers::adjustNumberOfCores() {
  if (nbthreads==0) {
    nbsolvers = (int)std::thread::hardware_concurrency();
      nbthreads = nbsolvers;
  }
  assert(nbthreads == nbsolvers);
}

lbool MultiSolvers::solve() {
  int i = 0; 
  //Build others solvers
  adjustNumberOfCores();
  sharedcomp->setNbThreads(nbsolvers);
  generateAllSolvers();
  
  //For MPI, Options are given to constructor
  struct OptConcurrent* optConcurrent = (OptConcurrent*)malloc(sizeof(struct OptConcurrent));
  optConcurrent->optDisplay = com_display;
  optConcurrent->optClauseSharing = com_clause_sharing;
  optConcurrent->optUlSharing = com_ul_sharing;
  optConcurrent->optHowClauseSharing = com_how_clause_sharing;
  optConcurrent->optGroup = com_group; 
  optConcurrent->optAlea = com_alea;
  optConcurrent->optComNoCom = com_no_com;
  optConcurrent->optComNoCom1 = com_no_com1;
  optConcurrent->optComNoClausesFromNetwork = comNoClausesFromNetwork;
  optConcurrent->optConflicts = com_conflicts;
  optConcurrent->optSleepingTimeBase=com_sleeping_time_base;
  optConcurrent->optSleepingTimeListener=com_sleeping_time_listener;
  optConcurrent->optSleepingTimeSender=com_sleeping_time_sender;
  optConcurrent->optSleepingTimeCollectif=com_sleeping_time_collectif;
  optConcurrent->optRingLimit=com_ring_limit;
  optConcurrent->optEncoding=com_encoding;
  optConcurrent->optAnalysePromoted=com_analyse_promoted;
  struct OptAmpharos* optAmpharos = (OptAmpharos*)malloc(sizeof(struct OptAmpharos));
  optAmpharos->optInitializationDuration=com_pharus_initialization_duration;
  optAmpharos->optSubproblemWorkDuration=com_pharus_subproblem_work_duration;
  optAmpharos->optTreeHeuristic=com_pharus_tree_heuristic;
  optAmpharos->optSpeedExtend=com_pharus_speed_extend;
  if(com_no_com){
    printf("[WORKER %d] No Communications\n"
             ,mpiCommunicator->mpiRank);
    mpiCommunicator = new ComMPInoCom(optConcurrent,optAmpharos);
  }else
    if(optConcurrent->optHowClauseSharing == OptComMPIFully){
      mpiCommunicator = new ComMPIFully(optConcurrent,optAmpharos);
      printf("[WORKER %d] Fully Hybrid - Send do by solvers one clause at a time\n"
             ,mpiCommunicator->mpiRank);
      printf("[WORKER %d] Fully Hybrid - Receive by a busy waiting thread (Blocking receive)\n"
             ,mpiCommunicator->mpiRank);
    }else if (optConcurrent->optHowClauseSharing == OptComMPIPartially){
      mpiCommunicator = new ComMPIPartially(optConcurrent,optAmpharos);
      printf("[WORKER %d] Partially Hybrid - Collective Communications\n"
             ,mpiCommunicator->mpiRank);
    }else{
      printf("ERROR optConcurrent->optHowClauseSharing\n");
      exit(0);
    }
  
  printf("[WORKER %d] LBD limit during search : %d\n", mpiCommunicator->mpiRank, (int)opt_exportLBDsearch);
  mpiCommunicator->linkingOfSolvers(sharedcomp);//To link solvers and MPI
  mpiCommunicator->setBarrier();

  model.clear();

  mpiCommunicator->setCoreOfManagerThread(sched_getcpu());
  printf("[WORKER %d] Manager thread on the core %d (total cores : %d) (total solvers : %d)\n"
         ,mpiCommunicator->mpiRank
         ,mpiCommunicator->getCoreOfManagerThread()
         ,(int)std::thread::hardware_concurrency()
         ,nbsolvers
         );
  
  // Launch all solvers
  for (i = 0; i < nbsolvers; i++) {
    std::thread* pt = new std::thread(&localLaunch,solvers[i]);
    threads.push(pt);
  }
  mpiCommunicator->linkingOfManager();//First connection with the Manager
  mpiCommunicator->start();
  
  // Wait for all threads to finish
  for (i = 0; i < nbsolvers; i++) { 
    if(threads[i]->joinable())
      threads[i]->join();
  }
  printf("[WORKER %d] All solver threads closed\n"
         ,mpiCommunicator->mpiRank);
  assert(sharedcomp != NULL);
  // Recuperate the winner
  if(!sharedcomp->getSolutionFound()){
    return l_Undef; 
  }else{
    lbool result = sharedcomp->solverSolution->status; // The result of Multisolver
    winner = sharedcomp->solverSolution->threadNumber();
    //Extend the Model
    if (result == l_True) {
      sharedcomp->solverSolution->extendModel();
      int n = sharedcomp->solverSolution->nVars();
      model.growTo(n);
      for(int i = 0; i < n; i++) {
        model[i] = sharedcomp->solverSolution->model[i];
        assert(model[i]!=l_Undef);
      }
    }
    return result;
  }
}


// Still a ugly function... To be rewritten with some statistics class some day
void MultiSolvers::printFinalStats() {
    sharedcomp->printStats();
    printf("c\nc\n");
    printf("c\n");
    printf("c |---------------------------------------- FINAL STATS --------------------------------------------------|\n");
    printf("c\n");
    
    printf("c |---------------|-----------------");
    for(int i = 0;i<solvers.size();i++) 
        printf("|------------");
    printf("|\n");    

    printf("c | Threads       |      Total      ");
    for(int i = 0;i < solvers.size();i++) {
        printf("| %10d ",i);
    }
    printf("|\n");

    printf("c |---------------|-----------------");
    for(int i = 0;i<solvers.size();i++) 
        printf("|------------");
    printf("|\n");    

    
//--
    printf("c | Conflicts     ");
    long long int totalconf = 0;
    for(int i=0;i<solvers.size();i++)  
	totalconf +=  solvers[i]->conflicts;
    printf("| %15lld ",totalconf);
       
    for(int i=0;i<solvers.size();i++)  
	printf("| %10" PRIu64" ", solvers[i]->conflicts);
    printf("|\n");

    //--   
    printf("c | Decisions     ");
    long long int totaldecs = 0;
    for(int i=0;i<solvers.size();i++)  
	totaldecs +=  solvers[i]->decisions;
    printf("| %15lld ",totaldecs);
       
    for(int i=0;i<solvers.size();i++)  
	printf("| %10" PRIu64" ", solvers[i]->decisions);
    printf("|\n");

    //--
    printf("c | Propagations  ");
    long long int totalprops = 0;
    for(int i=0;i<solvers.size();i++)  
	totalprops +=  solvers[i]->propagations;
    printf("| %15lld ",totalprops);
       
    for(int i=0;i<solvers.size();i++)  
	printf("| %10" PRIu64" ", solvers[i]->propagations);
    printf("|\n");


    printf("c | Avg_Trail     ");
    printf("|                 ");        
    for(int i=0;i<solvers.size();i++)  
      printf("| %10" PRIu64" ", solvers[i]->conflicts?solvers[i]->stats[sumTrail]/solvers[i]->conflicts:0);
    printf("|\n");

    //--
    printf("c | Avg_DL        ");
    printf("|                 ");        
    for(int i=0;i<solvers.size();i++)  
      printf("| %10" PRIu64" ", solvers[i]->conflicts?solvers[i]->stats[sumDecisionLevels]/solvers[i]->conflicts:0);
    printf("|\n");

    //--
    printf("c | Avg_Res       ");
    printf("|                 ");        
    for(int i=0;i<solvers.size();i++)  
      printf("| %10" PRIu64" ", solvers[i]->conflicts?solvers[i]->stats[sumRes]/solvers[i]->conflicts:0);
    printf("|\n");

    //--
    printf("c | Avg_Res_Seen  ");
    printf("|                 ");        
    for(int i=0;i<solvers.size();i++)  
      printf("| %10" PRIu64" ", solvers[i]->conflicts?solvers[i]->stats[sumResSeen]/solvers[i]->conflicts:0);
    printf("|\n");

    //--

    printf("c |---------------|-----------------");
    for(int i = 0;i<solvers.size();i++) 
        printf("|------------");
    printf("|\n");    

    printf("c | Exported      ");
    uint64_t exported = 0;
    for(int i=0;i<solvers.size();i++) 
        exported += solvers[i]->stats[nbexported];
    printf("| %15" PRIu64" ", exported);

    for(int i=0;i<solvers.size();i++) 
	printf("| %10" PRIu64" ", solvers[i]->stats[nbexported]);
    printf("|\n");
//--    
    printf("c | Imported      ");
    uint64_t imported = 0;
    for(int i=0;i<solvers.size();i++) 
        imported += solvers[i]->stats[nbimported];
    printf("| %15" PRIu64" ", imported);
    for(int i=0;i<solvers.size();i++) 
	printf("| %10" PRIu64" ", solvers[i]->stats[nbimported]);
    printf("|\n");    
//--
    
    printf("c | Good          ");
    uint64_t importedGood = 0;
    for(int i=0;i<solvers.size();i++) 
        importedGood += solvers[i]->stats[nbImportedGoodClauses];
    printf("| %15" PRIu64" ", importedGood);
    for(int i=0;i<solvers.size();i++) 
	printf("| %10" PRIu64" ", solvers[i]->stats[nbImportedGoodClauses]);
    printf("|\n");    
//--
    
    printf("c | Purge         ");
    uint64_t importedPurg = 0;
    for(int i=0;i<solvers.size();i++) 
        importedPurg += solvers[i]->stats[nbimportedInPurgatory];
    printf("| %15" PRIu64" ", importedPurg);
    for(int i=0;i<solvers.size();i++) 
	printf("| %10" PRIu64" ", solvers[i]->stats[nbimportedInPurgatory]);
    printf("|\n");    
//-- 
    
    printf("c | Promoted      ");
    uint64_t promoted = 0;
    for(int i=0;i<solvers.size();i++) 
        promoted += solvers[i]->stats[nbPromoted];
    printf("| %15" PRIu64" ", promoted);
    for(int i=0;i<solvers.size();i++) 
	printf("| %10" PRIu64" ", solvers[i]->stats[nbPromoted]);
    printf("|\n");    
//--
    
    printf("c | Remove_Imp    ");
    uint64_t removedimported = 0;
    for(int i=0;i<solvers.size();i++) 
        removedimported += solvers[i]->stats[nbRemovedUnaryWatchedClauses];
    printf("| %15" PRIu64" ", removedimported);
    for(int i=0;i<solvers.size();i++) 
	printf("| %10" PRIu64" ", solvers[i]->stats[nbRemovedUnaryWatchedClauses]);
    printf("|\n");    
//--
    
    printf("c | Blocked_Reuse ");
    uint64_t blockedreused = 0;
    for(int i=0;i<solvers.size();i++) 
        blockedreused += solvers[i]->nbNotExportedBecauseDirectlyReused;
    printf("| %15" PRIu64" ",blockedreused);
    for(int i=0;i<solvers.size();i++) 
	printf("| %10" PRIu64" ", solvers[i]->nbNotExportedBecauseDirectlyReused);
    printf("|\n");    
//--
    printf("c |---------------|-----------------");
    for(int i = 0;i<solvers.size();i++) 
        printf("|------------");
    printf("|\n");    

    printf("c | Unaries       ");
    printf("|                 "); 
    for(int i=0;i<solvers.size();i++) {
	printf("| %10" PRIu64" ", solvers[i]->stats[nbUn]);
    }
    printf("|\n");    
//--
    
    printf("c | Binaries      ");
    printf("|                 "); 
    for(int i=0;i<solvers.size();i++) {
	printf("| %10" PRIu64" ", solvers[i]->stats[nbBin]);
    }
    printf("|\n");    
//--

    
    printf("c | Glues         ");
    printf("|                 "); 
    for(int i=0;i<solvers.size();i++) {
	printf("| %10" PRIu64" ", solvers[i]->stats[nbDL2]);
    }
    printf("|\n");    
//--

    printf("c |---------------|-----------------");
    for(int i = 0;i<solvers.size();i++) 
        printf("|------------");
    printf("|\n");    

    printf("c | Orig_Seen     ");
    uint64_t origseen = 0;
    
    for(int i=0;i<solvers.size();i++) {
        origseen+=solvers[i]->stats[originalClausesSeen];
    }
    printf("| %13" PRIu64" %% ",origseen*100/nClauses()/solvers.size());
    
    for(int i=0;i<solvers.size();i++) {
	printf("| %10" PRIu64" ", solvers[i]->stats[originalClausesSeen]);
    }
    
    printf("|\n");    


    int winner = -1;
   for(int i=0;i<solvers.size();i++) {
     if(sharedcomp->winner()==solvers[i])
       winner = i;
     }
    
//--
    if(winner!=-1) { 
        printf("c | Diff Orig seen");
        printf("|                 "); 
        
        for(int i=0;i<solvers.size();i++) {
            if(i==winner) {
                printf("|      X     ");
                continue;
            }
            if(solvers[i]->stats[originalClausesSeen]>solvers[winner]->stats[originalClausesSeen])
                printf("| %10" PRIu64" ", solvers[i]->stats[originalClausesSeen]-solvers[winner]->stats[originalClausesSeen]);
            else 
                printf("| -%9" PRIu64" ", solvers[winner]->stats[originalClausesSeen]-solvers[i]->stats[originalClausesSeen]);
                
        }
        
        printf("|\n");    
    }


//--
    
   if(winner!=-1) {
       int sum = 0;
       printf("c | Hamming       ");
       for(int i = 0;i<solvers.size();i++) {
           if(i==winner) 
               continue;
           int nb = 0;
           for(int j = 0;j<nVars();j++) {
               if(solvers[i]->valuePhase(j)!= solvers[winner]->valuePhase(j)) nb++;
           }
           sum+=nb;
           
       }
       sum = sum/(solvers.size()>1?solvers.size()-1:1);
       
       printf("| %13d %% ",sum*100/nVars());
       
       for(int i = 0;i<solvers.size();i++) {
           if(i==winner) {
               printf("|      X     ");
               continue;
           }
           int nb = 0;
           for(int j = 0;j<nVars();j++) {
               if(solvers[i]->valuePhase(j)!= solvers[winner]->valuePhase(j)) nb++;
           }
           printf("| %10d ",nb);
           sum+=nb;
           
       }
       printf("|\n");
   }
 
   printf("c |---------------|-----------------");
   for(int i = 0;i<solvers.size();i++) 
     printf("|------------");
   printf("|\n");    
}

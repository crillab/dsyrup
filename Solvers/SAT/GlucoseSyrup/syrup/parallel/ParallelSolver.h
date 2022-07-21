/**************************************************************************************[ParallelSolver.h]
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

#ifndef PARALLELSOLVER_H
#define	PARALLELSOLVER_H

#include "core/SolverTypes.h"
#include "core/Solver.h"
#include "simp/SimpSolver.h"
#include "parallel/SharedCompanion.h"
#include "com/ComSubsum.h"
#include <condition_variable>
#include <mutex>  

namespace Glucose {
    
  enum ParallelStats{
    nbexported=coreStatsSize,
    nbimported,
    nbexportedunit,
    nbimportedunit,
    nbimportedInPurgatory,
    nbImportedGoodClauses
  } ;
#define parallelStatsSize (coreStatsSize + 6)
 
  //=================================================================================================
    
  class ParallelSolver : public SimpSolver {
    friend class MultiSolvers;
    friend class SolverCompanion;
    friend class SharedCompanion;

  protected : 
    int rank;
    int	thn; // internal thread number
    SharedCompanion *sharedcomp;
    
  public:
    //Basic methods 
    ParallelSolver(int threadId);
    ParallelSolver(const ParallelSolver &s);
    ~ParallelSolver();
    virtual Clone* clone() const {return  new ParallelSolver(*this);}   

    // Methods of solvers (use in Solver.cc but implement here)
    virtual void reduceDB();
    virtual lbool solve_ (bool do_simp = true, bool turn_off_simp = false);
    virtual void parallelImportClauseDuringConflictAnalysis(Clause &c,CRef confl);
    virtual bool parallelImportClauses(); // true if the empty clause was received
    virtual void parallelImportUnaryClauses();
    virtual void parallelExportUnaryClause(Lit p);
    virtual void parallelExportClauseDuringSearch(Clause &c);
    virtual bool panicModeIsEnabled();
    virtual void display();
    bool shareClause(Clause & c); // true if the clause was succesfully sent

    //To display information
    void reportProgress();
    void reportProgressArrayImports(vec<unsigned int> &totalColumns);

    //Variables
    vec<Lit>    importedClause; // Temporary clause used to copy each imported clause
    unsigned int    goodlimitlbd; // LBD score of the "good" clauses, locally
    int goodlimitsize;
    bool purgatory; // mode of operation
    bool shareAfterProbation; // Share any none glue clause only after probation (seen 2 times in conflict analysis)
    bool plingeling; // plingeling strategy for sharing clauses (experimental)
    unsigned int nbTimesSeenBeforeExport;
    uint32_t  firstSharing, limitSharingByGoodLBD, limitSharingByFixedLimitLBD, limitSharingByFixedLimitSize;
    uint32_t  probationByFollowingRoads, probationByFriend;
    uint32_t  survivorLayers; // Number of layers for a common clause to survive
    bool dontExportDirectReusedClauses ; // When true, directly reused clauses are not exported
    uint64_t nbNotExportedBecauseDirectlyReused;
    vec<uint32_t> goodImportsFromThreads; // Stats of good importations from other threads
    unsigned int limitLBDsearch;
    bool com1;
    lbool status;

    //Inline methodes
    inline int getPsm(Clause & c){
      int psm = 0;
      for (int i = 0; i < c.size(); i++){
        Lit my_lit = c[i];
        if(polarity[var(my_lit)] == sign(my_lit))psm++;
      }
      return psm;
    }

    inline int get_conflicts(){return conflicts;}
    inline int get_best_VSIDS(){
      if(assumptions.size() != decisionLevel()){
        printf("Error assumptions.size() != decisionLevel()\n");
        exit(0);
      }
      return order_heap.recuperate();
    }
    inline int get_best_VSIDS(int* best_VSIDS, int nb){return order_heap.recuperate(best_VSIDS,nb);}
    
    inline int get_best_propagate(){
      if(assumptions.size() != decisionLevel()){
        printf("Error assumptions.size() != decisionLevel()\n");
        exit(0);
      }
      Var varSelected = var_Undef;
      double max = 0;
      double ratio = 0;
      for (Var v = 0; v < nVars(); v++)
        if (value(v) == l_Undef){
          ratio = (double)variablesPro[v]/(double)variablesDec[v];
          if(ratio > max){
            max = ratio;
            varSelected = v;
          }
        }
      return varSelected;
    }
    
    inline int get_best_promoted(){
      if(assumptions.size() != decisionLevel()){
        printf("Error assumptions.size() != decisionLevel()\n");
        exit(0);
      }
      Var varSelected = var_Undef;
      int max = 0;
      for (Var v = 0; v < nVars(); v++)
        if (value(v) == l_Undef){
          if(variablesPromoted[v] > max){
            max = variablesPromoted[v];
            varSelected = v;
            printf("ess : %d\n", max);
          }
        }
      return varSelected;
    }

    inline int threadNumber() const {return thn;}
    inline void setThreadNumber(int i){thn = i;}
    inline void setLimitLBDsearch(int i){limitLBDsearch = i;}

    //For pharus

    inline void restart(){cancelUntil(0);}
    inline void backtrack(){cancelUntil(decisionLevel()-1);}

    /**
     * \brief Checks if a variable is assigned or not by the solver.
     * \param[in] variable The variable to test.
     * \return 
     If this variable is assigned to false, she is positif : return 0 <=> VAR_POSITIVE_ASSIGN
     If this variable is assigned to true, she is negatif : return 1 <=> VAR_NEGATIVE_ASSIGN 
     If this variable is not assigned : return 2 <=> VAR_NO_ASSIGN 
     */
    inline char isAssigned(const int& variable){return toInt(assigns[variable]);}
    
    /*
     * \brief Create a new decision level, decide a litteral to assign and make the necessary propagations
     * \param[in] variable The decision variable.
     * \param[in] polarity The decision polarity. This variable and this polarity produce a literal
     * \return 
     If the initial problem is satisfiable return 0 <=> DECISION_PROPAGATES_SAT
     If the propagation produce a conflict return -1 <=> DECISION_PROPAGATES_CONFLICT
     Else return the number of propagation 
     */
    inline int decisionAndPropagates(const int& variable,const char& polarity)
    {
      Lit lit = mkLit(variable, polarity);
      decisions++;
      newDecisionLevel();
      uncheckedEnqueue(lit);
      CRef confl = propagate();
      int res = trail.size();
      return (confl != CRef_Undef)?DECISION_PROPAGATES_CONFLICT:((res == nVars())?DECISION_PROPAGATES_SAT:res);
    }

    inline char getPolarity(Var v){return polarity[v];}

    inline void displaySubproblem(int* subproblem, const int& sizeSubproblem){
      for(int i = 0;i < sizeSubproblem;i++){
        printf("%d ",subproblem[i]);
      }
      printf("\n");
    }
    inline void loadSubproblem(int* subproblem, const int& sizeSubproblem){
      assumptions.clear(); 
      for(int i = 0;i < sizeSubproblem;i++){
        assumptions.push(toLit(subproblem[i]));
      }
    }
    
    /*
     *Return the first level where the subproblem change. 0 is a new subproblem. 
     */
    inline int compareSubproblemWithPreviousSubproblem(){
      /* printf("Previous :");
      for(int j = 0; j < previousAssumptions.size();j++){
        printf("%d ",toInt(previousAssumptions[j]));
      }
      printf("\nCurrent :");
      for(int j = 0; j < assumptions.size();j++){
        printf("%d ",toInt(assumptions[j]));
      }
      printf("\n");
      */
      //If my previous is empty, the subproblem is a new
      if(previousAssumptions.size() == 0)return 0;
      //If the first litteral of my assumptions is not the same, return also 0 
      for(int i=0;i<previousAssumptions.size() && i<assumptions.size();i++)
        if(previousAssumptions[i]!=assumptions[i])return i;    
      //My previous assumptions are included in my new assumptions ! 
      return previousAssumptions.size();
    }

    /*
     * Put the good ULs in the table ULs 
     */
    inline void recuperateULs(){
      //printf("nbvar : %d\n",nVars());
//Reinit variables 
      nbULs = 0;
      nbULsLimit = 0;
      const int end = trail.size();
      int nextLevel = trail_lim[0];
      
      //We browse the trail
      for(int i = 0; i < end;i++){
        while(i == nextLevel && nbULsLimit < assumptions.size())
        {//In this case, we have to change the level
          ULsLimit[nbULsLimit++]=nbULs;
          nextLevel = trail_lim[nbULsLimit];
        }
        //Get the current literal in the trail
        Lit myLit = trail[i];
        Var myVar = var(myLit);
       
        if(checkULs[myVar] == NOT_SENT || (checkULs[myVar] != NOT_SENT && nbULsLimit < checkULs[myVar])){
          checkULs[myVar]=nbULsLimit;//We keep the good level
          ULs[nbULs++]=toInt(myLit);//We have to send this UL
        }
      }
      
      while(nbULsLimit < assumptions.size())
        ULsLimit[nbULsLimit++]=nbULs; 
    }

    inline bool addUL(const int& UL,const int& level,int* subproblem){
      Lit lit = toLit(UL);
      if(value(lit) == l_Undef){
        if(!level)uncheckedEnqueue(lit);
        else{
          vec<Lit> learnt;
          learnt.push(lit);
          for(int i=0;i<level;i++)learnt.push(~toLit(subproblem[i]));
          CRef cr = ca.alloc(learnt, true);
          permanentLearnts.push(cr);
          attachClause(cr);
          claBumpActivity(ca[cr]);
          uncheckedEnqueue(learnt[0], cr);
        }
      }else{
        if(assigns[var(lit)] != lbool(!sign(lit)))return true;
      }
      return false;
    }

    inline bool addULs(int level,int* subproblem){
      for(int i = 0,myVar=(*(ULsReceived+i))>>1;i < nbULsReceived;myVar=(*(ULsReceived+(++i)))>>1){
        if(checkULs[myVar] == NOT_SENT || level < checkULs[myVar]){//if I not this UL
          checkULs[myVar]=level;//I don't send
          if(addUL(ULsReceived[i],level,subproblem))return true;    
        }
      }
      return false;
    }

    inline void cleanULs(){
      int nbClean = 0;
      int levelToClean = compareSubproblemWithPreviousSubproblem();
      if(levelToClean <= previousAssumptions.size())
        return;
      for(int i = 0; i < nVars() + 1;i++){
        if(checkULs[i] != NOT_SENT && checkULs[i] != 0){
          if(checkULs[i] > levelToClean){
            checkULs[i] = NOT_SENT;
            nbClean++;
          }
        }
      }
    }

  };
}
#endif	/* PARALLELSOLVER_H */


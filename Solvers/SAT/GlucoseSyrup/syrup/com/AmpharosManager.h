
#include "../../../../MessageCoding.h"
#include "../../../../OptionCoding.h"
#include "../../../../DataCoding.h"
#include "ComMPI.h"

#ifndef amp_man
#define amp_man

namespace Glucose {
  class SharedCompanion;
  class ParallelSolver;
  class ComMPI;
  class AmpharosManager{
  public:
    AmpharosManager(ComMPI* n_comMPI);
    //Principal variable to avoid to pass with pointers
    ComMPI* comMPI;
    int mpiRank;
    MPI_Comm mpiParentComm;
    DataMPI* dataMPI;
    OptConcurrent* optConcurrent;
    OptAmpharos* optAmpharos;
    SharedCompanion* solvers;
    
    //Methode to do easier MPI communication 
    inline void MPI_Send_Manager(const int& msg, const int& tag){
      MPI_Send(&msg, 1, MPI_INT, 0, tag, mpiParentComm);
    }
    
    inline int MPI_Recv_Manager(const int& tag){
      int msg = 0;
      MPI_Recv(&msg, 1, MPI_INT, 0, tag, mpiParentComm, MPI_STATUS_IGNORE);
      return msg;
    }
    
   
    //For the pharus/ampharos mode
    int nbVariables;
    int nbSubproblems; //The number of subproblem
    int* subproblemCurrent; //A subproblem
    int lenghtSubproblemCurrent; //The lenght of the subproblem
    int* subproblemPrevious; //The previous subproblem
    int lenghtSubproblemPrevious; //The lenght of the previous subproblem

    //For the initialization
    void initialization(ParallelSolver &solver);
    
    //For prevent the manager that a subproblem is UNSAT
    void subproblemUNSAT(ParallelSolver &solver,int assertifLiteral);
    bool subproblemINDETERMINATE(ParallelSolver &solver);

    //For share ULs
    void transmissionULs(ParallelSolver &solver);
    bool receiveULs(const int& level,ParallelSolver &solver);
    int receiveULsUNSAT;

    //For the transmission of subproblems 
    bool transmissionFromRoot(ParallelSolver &solver);
    bool transmissionFromSubproblem(ParallelSolver &solver);
    
    bool transmissionGoDown(ParallelSolver &solver);
    
    void transmissionRecuperateChildren();
    bool transmissionVarNoAssign(ParallelSolver &solver);
    bool transmissionVarAssign(ParallelSolver &solver);
    int getBestVariable(ParallelSolver &solver);
   

    inline void transmissionGoToTheRight(){
      subproblemCurrent[lenghtSubproblemCurrent++]=currentVariable+currentVariable+1;
      currentVariable=rightChildVariable;
      MPI_Send_Manager(PHARUS_TRANSMISSION_GO_RIGHT,TAG_PHARUS);
      
    }
    inline void transmissionGoToTheLeft(){
      subproblemCurrent[lenghtSubproblemCurrent++]=currentVariable+currentVariable;
      currentVariable=leftChildVariable;
      MPI_Send_Manager(PHARUS_TRANSMISSION_GO_LEFT,TAG_PHARUS);

    }
    
    inline int transmissionHeuristicOfPolarity(const int phaseSaving){
      const bool leftOverload = (leftChildNbWorkers >= leftChildNbSubproblems)?true:false;
      const bool rightOverload = (rightChildNbWorkers >= rightChildNbSubproblems)?true:false;
      return phaseSaving;
      if(leftOverload && rightOverload)
        return phaseSaving;
      else if(leftOverload)
        return VAR_NEGATIVE_ASSIGN;
      else if(rightOverload)
        return VAR_POSITIVE_ASSIGN;
      else
        return phaseSaving;
    }

    inline void transmissionSubproblemSAT(){
      printf("[WORKER %d]Solution SAT found : TO DO\n",mpiRank);      
      exit(0);
    }

    

    int currentVariable;//Variable of last received node 
    int assignmentVariable;//State of the variable by the solver //VAR_NO_ASSIGN || VAR_POSITIVE_ASSIGN || VAR_NEGATIVE_ASSIGN
    int polarityVariable; //To store the polarity of a variable give by 
    int resultDecisionAndPropagates; //To store the result of solver.decisionAndPropagates
    int leftChildVariable;
    int leftChildNbSubproblems;
    int leftChildNbWorkers;    
    int rightChildVariable;
    int rightChildNbSubproblems;
    int rightChildNbWorkers;    
    

  };
}
#endif

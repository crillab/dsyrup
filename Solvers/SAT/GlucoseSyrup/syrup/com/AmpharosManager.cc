#include "com/AmpharosManager.h"
#include "parallel/SharedCompanion.h"
#include "parallel/ParallelSolver.h"

using namespace Glucose;

AmpharosManager::AmpharosManager(ComMPI* n_comMPI):
  comMPI(n_comMPI),
  mpiRank(n_comMPI->mpiRank),
  mpiParentComm(n_comMPI->mpiParentComm),
  dataMPI(n_comMPI->dataMPI),
  optConcurrent(n_comMPI->optConcurrent),
  optAmpharos(n_comMPI->optAmpharos),
  solvers(n_comMPI->solvers),
  nbVariables(n_comMPI->solvers->nbVars()),
  nbSubproblems(0),
  subproblemCurrent(NULL),
  lenghtSubproblemCurrent(0),
  subproblemPrevious(NULL),
  lenghtSubproblemPrevious(0),
  receiveULsUNSAT(false),
  currentVariable(0),
  assignmentVariable(0),
  polarityVariable(0),
  resultDecisionAndPropagates(0),
  leftChildVariable(0),
  leftChildNbSubproblems(0),
  leftChildNbWorkers(0),    
  rightChildVariable(0),
  rightChildNbSubproblems(0),
  rightChildNbWorkers(0)
{
  solvers->setPharusInitialization(optAmpharos->optInitializationDuration);//To enabled initialization
  subproblemCurrent = (int*)malloc(nbVariables*sizeof(int));
  subproblemPrevious = (int*)malloc(nbVariables*sizeof(int));
  if(!mpiRank){
    MPI_Send_Manager(nbVariables,TAG_MSG);
    MPI_Send_Manager(optAmpharos->optSpeedExtend,TAG_MSG);
  }
  
  printf("[WORKER %d][AMPHAROS] Initialisation duration : %d - Subproblem duration : %d - Tree heuristic : %s - Speed extention of the tree : %d\n"
         ,mpiRank
         ,optAmpharos->optInitializationDuration
         ,optAmpharos->optSubproblemWorkDuration
         ,!optAmpharos->optTreeHeuristic?"VSIDS":"GANESH"
         ,optAmpharos->optSpeedExtend);
}

void AmpharosManager::initialization(ParallelSolver &solver){
  comMPI->startCommunications();
  MPI_Send_Manager(PHARUS_INITIALIZATION,TAG_MANAGER);
  MPI_Send_Manager(getBestVariable(solver),TAG_PHARUS);
  MPI_Recv_Manager(TAG_PHARUS);//To finalyse the initialization
  printf("[COMPUTER %d][THREAD %d][PHARUS] Initialization finished\n",mpiRank,solver.threadNumber());
  comMPI->endCommunications();
}

void AmpharosManager::subproblemUNSAT(ParallelSolver &solver,int assertifLiteral){
  comMPI->startCommunications();
  MPI_Send_Manager(PHARUS_SUBPROBLEM_UNSAT,TAG_MANAGER);
  MPI_Send_Manager(solver.threadNumber(),TAG_PHARUS);
  MPI_Send_Manager(assertifLiteral,TAG_PHARUS);
  comMPI->endCommunications();
}

int AmpharosManager::getBestVariable(ParallelSolver &solver){
  switch(optAmpharos->optTreeHeuristic){
  case 0:
    return solver.get_best_VSIDS();
  case 1:
    return solver.get_best_propagate();
  case 2:
    return solver.get_best_promoted();
  default:
    printf("Error optAmpharos->optTreeHeuristic not implemented : %d\n",optAmpharos->optTreeHeuristic);
    exit(0);
    return 0;
  }
  return 0;
}

/*
 * return: true if I keep the same subproblem, false else
 */
bool AmpharosManager::subproblemINDETERMINATE(ParallelSolver &solver){
  comMPI->startCommunications();
  MPI_Send_Manager(PHARUS_SUBPROBLEM_INDETERMINATE,TAG_MANAGER);
  MPI_Send_Manager(solver.threadNumber(),TAG_PHARUS);
  MPI_Send_Manager(solver.getDistributedPromotedClausesSubproblem(),TAG_PHARUS);
  transmissionULs(solver);
  int msg = MPI_Recv_Manager(TAG_PHARUS);
  //the suproblem exit but it is not a leaf.
  if(msg == PHARUS_SUBPROBLEM_NOT_EXIST){
    //printf("case1\n");
    //the suproblem found UNSAT => not exist => so change 
    solver.pharusSameSubproblemConflicts = 0;
    comMPI->endCommunications();
    return false;
  }else if(msg == PHARUS_SUBPROBLEM_EXIST){
    //printf("case2\n");
    //The subproblem exists but is is not a leaf
    if(solver.pharusSameSubproblemConflicts >= optAmpharos->optSubproblemWorkDuration){
      solver.pharusSameSubproblemConflicts = 0;
      comMPI->endCommunications();
      return false; //Change subproblem 
    }
    //printf("case3\n");
    comMPI->endCommunications();
    return true;
  }else if(msg == PHARUS_SUBPROBLEM_EXIST_LEAF){
    //The subproblem exists steel and is is a leaf
    msg = MPI_Recv_Manager(TAG_PHARUS);
    if(msg == PHARUS_EXTEND){
      //printf("case4extend\n");
      MPI_Send_Manager(getBestVariable(solver),TAG_PHARUS);
      comMPI->endCommunications();
      return false;
    }
    if(solver.pharusSameSubproblemConflicts >= optAmpharos->optSubproblemWorkDuration){
      //printf("case5\n");
      solver.pharusSameSubproblemConflicts = 0;
      comMPI->endCommunications();
      return false; 
    }
    //printf("case6\n");
    comMPI->endCommunications();
    return true;
  }else{
    printf("Error message during subproblemINDETERMINATE\n");
    exit(0);
  }
  return false;
}


void AmpharosManager::transmissionULs(ParallelSolver &solver){
  /*MPI_Send_Manager(solver.nbULs,TAG_PHARUS);
  if(solver.nbULs){
    MPI_Send(solver.ULs,solver.nbULs,MPI_INT, 0, TAG_PHARUS, mpiParentComm);
    MPI_Send_Manager(solver.nbULsLimit,TAG_PHARUS);
    MPI_Send(solver.ULsLimit,solver.nbULsLimit,MPI_INT, 0, TAG_PHARUS, mpiParentComm);
    }*/
}
//True if UNSAT else False
bool AmpharosManager::receiveULs(const int& level,ParallelSolver &solver){
  return false;
  solver.nbULsReceived=MPI_Recv_Manager(TAG_PHARUS);
  if(solver.nbULsReceived){
    MPI_Recv(solver.ULsReceived,solver.nbULsReceived,MPI_INT,0,TAG_PHARUS,mpiParentComm,MPI_STATUS_IGNORE);
    if(solver.addULs(level,subproblemCurrent)){
	printf("[COMPUTER %d][THREAD %d] UNSAT BY ADD ULs (level : %d)\n"
               ,mpiRank,solver.threadNumber(),level);
        return true;
    }
  }
  return false;
}

/*
 * Return True si we have a subproblem, else False
 */ 
bool AmpharosManager::transmissionFromRoot(ParallelSolver &solver){
  comMPI->startCommunications();
  //printf("[COMPUTER %d][THREAD %d][PHARUS] Start Transmission\n",mpiRank,solver.threadNumber());
  MPI_Send_Manager(PHARUS_TRANSMISSION_FROM_ROOT,TAG_MANAGER);
  MPI_Send_Manager(solver.threadNumber(),TAG_PHARUS);
  currentVariable = MPI_Recv_Manager(TAG_PHARUS);
  nbSubproblems = MPI_Recv_Manager(TAG_PHARUS);
  lenghtSubproblemCurrent=0;
  solver.restart();//To ensure that the solver is clean
  if(receiveULs(lenghtSubproblemCurrent,solver)){
    MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
    return false;
  }else
    MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
  
  if(currentVariable == STOP_AND_KILL){
    solvers->setAllEndSearch(SOLUTION_FOUND_BY_ANOTHER_STATE);
    printf("[COMPUTER %d][THREAD %d][PHARUS] Transmission finished STOP_AND_KILL\n",mpiRank,solver.threadNumber());
    comMPI->endCommunications();
    return false;
  }
  bool hasSubproblem = transmissionGoDown(solver);
  solver.restart();//To ensure that the solver is clean
  if(hasSubproblem)
      solver.loadSubproblem(subproblemCurrent,lenghtSubproblemCurrent);

  //printf("[COMPUTER %d][THREAD %d][PHARUS] Transmission finished (%d) (size %d)\n",mpiRank,solver.threadNumber(),hasSubproblem,lenghtSubproblemCurrent);
  comMPI->endCommunications();
  return hasSubproblem;
}

/*
 * Return True si we have a subproblem, else False
 */ 
bool AmpharosManager::transmissionFromSubproblem(ParallelSolver &solver){
  transmissionGoDown(solver);
  return false;
}


/*
 * Return True si we have a subproblem, else False
 */
bool AmpharosManager::transmissionGoDown(ParallelSolver &solver){
  for(;;){//go down as long as a leaf is reached
    
    if(currentVariable == END_SUBPROBLEM){//When we have our subproblem
      /*printf("[COMPUTER %d][THREAD %d][PHARUS] Subproblem to solve (%d) :\n",
             mpiRank,solver.threadNumber(),lenghtSubproblemCurrent);
      for(int i = 0;i<lenghtSubproblemCurrent;i++){printf("%d ",subproblemCurrent[i]);}
      printf("\n");*/
      return true;
    }
    transmissionRecuperateChildren();
    
    assignmentVariable = solver.isAssigned(currentVariable);//VAR_NO_ASSIGN || VAR_POSITIVE_ASSIGN || VAR_NEGATIVE_ASSIGN
    //break if the requested assumption is UNSAT
    if(assignmentVariable == VAR_NO_ASSIGN){if(!transmissionVarNoAssign(solver))return false;}
    else{if(!transmissionVarAssign(solver))return false;}
  }
}

void AmpharosManager::transmissionRecuperateChildren(){
  
  MPI_Send_Manager(PHARUS_TRANSMISSION_CHILDREN,TAG_PHARUS);
  leftChildVariable = MPI_Recv_Manager(TAG_PHARUS);
  if(leftChildVariable != UNSAT_SUBPROBLEM){
    leftChildNbSubproblems = MPI_Recv_Manager(TAG_PHARUS);
    leftChildNbWorkers = MPI_Recv_Manager(TAG_PHARUS);
  }
  rightChildVariable = MPI_Recv_Manager(TAG_PHARUS);
  if(rightChildVariable != UNSAT_SUBPROBLEM){
    rightChildNbSubproblems = MPI_Recv_Manager(TAG_PHARUS);
    rightChildNbWorkers = MPI_Recv_Manager(TAG_PHARUS);
  }
  if(leftChildVariable == STOP_AND_KILL && rightChildVariable == STOP_AND_KILL){
    printf("TO DO TRANSMISSION STOP_AND_KILL\n");
    exit(0);
  }
}

/*
 * Return True if the subproblem is ok, else False
 */
bool AmpharosManager::transmissionVarNoAssign(ParallelSolver &solver){
    
  //Four cases
  if(leftChildVariable == UNSAT_SUBPROBLEM && rightChildVariable == UNSAT_SUBPROBLEM){
    printf("[WORKER %d]Tree Left And Right both UNSAT Impossible \n",mpiRank);
    solver.restart();   
    return false;
  }else if(leftChildVariable == UNSAT_SUBPROBLEM){
    resultDecisionAndPropagates=solver.decisionAndPropagates(currentVariable,VAR_NEGATIVE_ASSIGN); 
    if (resultDecisionAndPropagates==DECISION_PROPAGATES_SAT){
      transmissionSubproblemSAT();
    }else if(resultDecisionAndPropagates == DECISION_PROPAGATES_CONFLICT){
      //left child is UNSAT for the tree and right child is UNSAT for the solver, 
      MPI_Send_Manager(PHARUS_TRANSMISSION_DELETE_FATHER,TAG_PHARUS);
      solver.restart();
      return false;
    }else{
      transmissionGoToTheRight();
      if(receiveULs(lenghtSubproblemCurrent,solver)){
        MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
        return false;
      }else
        MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
    }
  }else if(rightChildVariable == UNSAT_SUBPROBLEM){
    resultDecisionAndPropagates=solver.decisionAndPropagates(currentVariable,VAR_POSITIVE_ASSIGN); 
    if (resultDecisionAndPropagates==DECISION_PROPAGATES_SAT){
      transmissionSubproblemSAT();
    }else if(resultDecisionAndPropagates == DECISION_PROPAGATES_CONFLICT){
      //right child is UNSAT for the tree and left child is UNSAT for the solver, 
      MPI_Send_Manager(PHARUS_TRANSMISSION_DELETE_FATHER,TAG_PHARUS);
      solver.restart();
      return false;
    }else{
      transmissionGoToTheLeft();      
      if(receiveULs(lenghtSubproblemCurrent,solver)){
        MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
        return false;
      }else
        MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
    }
  }else{//No child is UNSAT for the tree
    polarityVariable=transmissionHeuristicOfPolarity(solver.getPolarity(currentVariable));
    resultDecisionAndPropagates=solver.decisionAndPropagates(currentVariable,polarityVariable); 
    if (resultDecisionAndPropagates==DECISION_PROPAGATES_SAT){
      transmissionSubproblemSAT();
    }else if(resultDecisionAndPropagates == DECISION_PROPAGATES_CONFLICT){
      solver.backtrack();
      polarityVariable=polarityVariable?0:1;
      resultDecisionAndPropagates=solver.decisionAndPropagates(currentVariable,polarityVariable); 
      if (resultDecisionAndPropagates==DECISION_PROPAGATES_SAT){
        transmissionSubproblemSAT();
      }else if(resultDecisionAndPropagates == DECISION_PROPAGATES_CONFLICT){
        // The both polarity are conflicts, we have to delete the father node
        MPI_Send_Manager(PHARUS_TRANSMISSION_DELETE_FATHER,TAG_PHARUS);
        solver.restart();
        return false;
      }else{// One is conflict but not the other, we can directly prevent the manager that this variable is already assigned
        MPI_Send_Manager(PHARUS_TRANSMISSION_ALREADY_ASSIGNED,TAG_PHARUS);
        MPI_Send_Manager(polarityVariable,TAG_PHARUS);
        if(polarityVariable){
          transmissionGoToTheRight();
          if(receiveULs(lenghtSubproblemCurrent,solver)){
            MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
            return false;
          }else
            MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
        }else{
          transmissionGoToTheLeft();
          if(receiveULs(lenghtSubproblemCurrent,solver)){
            MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
            return false;
          }else
            MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
      	}
      }
    }else{//No conflict for this polarity 
      if(polarityVariable){
        transmissionGoToTheRight();
        if(receiveULs(lenghtSubproblemCurrent,solver)){
          MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
          return false;
        }else
          MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS);      
      }else{
        transmissionGoToTheLeft();
        if(receiveULs(lenghtSubproblemCurrent,solver)){
          MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
          return false;
        }else
          MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
      }
    }
  } 
  return true;
}


/*
 * Return True if the subproblem is ok, else False
 */
bool AmpharosManager::transmissionVarAssign(ParallelSolver &solver){
  //Four cases
  if(leftChildVariable == UNSAT_SUBPROBLEM && rightChildVariable == UNSAT_SUBPROBLEM){
    printf("[WORKER %d]Tree Left And Right both UNSAT Impossible \n",mpiRank);
    solver.restart();   
    return false;
  }else if(leftChildVariable == UNSAT_SUBPROBLEM){
    if(!assignmentVariable){ //assignmentVariable == VAR_POSITIVE_ASSIGN
      //We have to propagate a litteral to satisfy a clause (currentVariable) 
      //But the subproblem represented at this point is UNSAT
      //So we can delete the father
      MPI_Send_Manager(PHARUS_TRANSMISSION_DELETE_FATHER,TAG_PHARUS);
      solver.restart();
      return false;
    }else{ //assignmentVariable == VAR_NEGATIVE_ASSIGN 
      transmissionGoToTheRight();
      if(receiveULs(lenghtSubproblemCurrent,solver)){
        MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
        return false;
      }else
        MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
    }
  }else if(rightChildVariable == UNSAT_SUBPROBLEM){
    if(assignmentVariable){ //assignmentVariable == VAR_NEGATIVE_ASSIGN
      //We have to propagate a litteral to satisfy a clause (currentVariable)
      //But the subproblem represented at this point is UNSAT
      //So we can delete the father
      MPI_Send_Manager(PHARUS_TRANSMISSION_DELETE_FATHER,TAG_PHARUS);
      solver.restart();
      return false;
    }else{  //assignmentVariable == VAR_POSITIVE_ASSIGN
      transmissionGoToTheLeft();
      if(receiveULs(lenghtSubproblemCurrent,solver)){
        MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
        return false;
      }else
        MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
    }
  }else{//No children is UNSAT for the tree
    MPI_Send_Manager(PHARUS_TRANSMISSION_ALREADY_ASSIGNED,TAG_PHARUS);
    MPI_Send_Manager(assignmentVariable,TAG_PHARUS);
    if(assignmentVariable){
      transmissionGoToTheRight();
      if(receiveULs(lenghtSubproblemCurrent,solver)){
        MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
        return false;
      }else
        MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
    }else{
      transmissionGoToTheLeft();
      if(receiveULs(lenghtSubproblemCurrent,solver)){
        MPI_Send_Manager(PHARUS_ULS_UNSAT,TAG_PHARUS);
        return false;
      }else
        MPI_Send_Manager(PHARUS_NO_ULS_UNSAT,TAG_PHARUS); 
    }
  } 
  return true;
}

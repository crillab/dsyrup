#include "mpi.h"
#include <iostream>
#include <cstring>

#include <signal.h>

#include "AmpharosManager.h"

/**
   Create a concurrent manager by calling the Manager constructor.   
   @param[in] _instance, the benchmark path
   @param[in] _solvers, the set of solvers that will be ran
 */
AmpharosManager::AmpharosManager(string _instance, vector<Solver> &_solvers, int _size_assignment) : 
  Manager(_instance,_solvers),
  tree(NULL),
  sizeAssignment(_size_assignment),
  nbVariables(0),
  speedExtend(1),
  nbLiterals(0),
  ULs(NULL),
  nbULs(0),
  ULsLimit(NULL),
  nbULsLimit(0),
  ULsToSend(NULL),
  nodesAccordingToWorkers(NULL),
  nbWorkersPerComputers(NULL),
  nbTotalWorkers(0),
  nbTotalPromotedClauses(0),
  nbTotalGetPromotedClauses(0),
  savePromotedClauses(0)
{
}

AmpharosManager::~AmpharosManager(){}

void AmpharosManager::listener()
{
  int messageReceived = 0;
  int flag = 0;
  MPI_Status status;
  MPI_Request request;
  for(;;){
    flag = 0;
    MPI_Irecv(&messageReceived, 1, MPI_INT, MPI_ANY_SOURCE, TAG_MANAGER, intercomm, &request);
    while(!flag){
      MPI_Test(&request, &flag, &status);
      if(flag){
        switch (messageReceived) 
          {
          case SOLUTION_FOUND:solutionFound(status.MPI_SOURCE);break;
          case STOP_COM_ULS:messageStopComUls(status.MPI_SOURCE);break;
          case STOP_COM_CLAUSES:
            messageStopComClauses(status.MPI_SOURCE);
            if(nbWorkerClauses == sizeComm && rankOfTheFirstSolution == -1)return;
            break;
          case PHARUS_INITIALIZATION:pharusInitialization(status.MPI_SOURCE);break;
          case PHARUS_TRANSMISSION_FROM_ROOT:pharusTransmissionFromRoot(status.MPI_SOURCE);break;
          case PHARUS_SUBPROBLEM_UNSAT:pharusSubproblemUNSAT(status.MPI_SOURCE);break;
          case PHARUS_SUBPROBLEM_INDETERMINATE:pharusSubproblemINDETERMINATE(status.MPI_SOURCE);break;
          case JOB_FINISHED:return;
          default:
            printf("Listener manager received a message %d from [Worker %d] problem\n"
                   ,messageReceived,status.MPI_SOURCE);
            exit(0);
            break;
          }
        break;
      }else
        std::this_thread::sleep_for(std::chrono::milliseconds(50));//It's not a busy sleep
    } 
  }
}
/*
void AmpharosManager::listener()
{
  int messageReceived = 0;
  MPI_Status status;
  for(;;){
    MPI_Recv(&messageReceived, 1, MPI_INT, MPI_ANY_SOURCE, TAG_MANAGER, intercomm, &status);
    switch (messageReceived) 
      {
      case SOLUTION_FOUND:solutionFound(status.MPI_SOURCE);break;
      case STOP_COM_ULS:messageStopComUls(status.MPI_SOURCE);break;
      case STOP_COM_CLAUSES:messageStopComClauses(status.MPI_SOURCE);break;
      case PHARUS_INITIALIZATION:pharusInitialization(status.MPI_SOURCE);break;
      case PHARUS_TRANSMISSION_FROM_ROOT:pharusTransmissionFromRoot(status.MPI_SOURCE);break;
      case PHARUS_SUBPROBLEM_UNSAT:pharusSubproblemUNSAT(status.MPI_SOURCE);break;
      case JOB_FINISHED:{return;}
      default:
        printf("Listener manager received a message %d from [COMPUTER %d] problem\n"
               ,messageReceived,status.MPI_SOURCE);
        exit(0);
        break;
      }
  } 
  }*/

/**
   Run the solver.
 */
void AmpharosManager::run()
{ 
  int msg=AMPHAROS_MODE, i=0;// We work in the Ampharos mode
  launch(); //Manager.cc class 
  nbWorkersPerComputers=(int*)malloc(sizeof(int)*sizeComm);
  nodesAccordingToWorkers=(Node***)malloc(sizeof(Node**)*sizeComm);
  for (; i < sizeComm ;++i) MPI_Send(&msg, 1, MPI_INT, i, TAG_MSG, intercomm);//Launch worker
  for (i = 0; i < sizeComm ;++i){
    MPI_Recv(&msg, 1, MPI_INT, i, TAG_MSG, intercomm, MPI_STATUS_IGNORE);
    MPI_Recv(nbWorkersPerComputers+i, 1, MPI_INT, i, TAG_MSG, intercomm, MPI_STATUS_IGNORE);
    nbTotalWorkers+=nbWorkersPerComputers[i];
    nodesAccordingToWorkers[i]=(Node**)malloc(sizeof(Node*)*nbWorkersPerComputers[i]);
    for(int j = 0;j < nbWorkersPerComputers[i];j++)nodesAccordingToWorkers[i][j] = NULL;
  }
  MPI_Recv(&nbVariables, 1, MPI_INT, 0, TAG_MSG, intercomm, MPI_STATUS_IGNORE);
  MPI_Recv(&speedExtend, 1, MPI_INT, 0, TAG_MSG, intercomm, MPI_STATUS_IGNORE);
  
  nbLiterals=nbVariables*2; 
  ULs = (int*)malloc(sizeof(int)*(nbVariables+1));
  nbULs = 0;
  ULsLimit = (int*)malloc(sizeof(int)*(nbVariables+1));
  nbULsLimit = 0;
  ULsToSend = (int*)malloc(sizeof(int)*((nbVariables*2)+1));
  tree = new Tree(nbTotalWorkers,nbLiterals);
  printf("[MANAGER][PHARUS] Ok (Number of variables : %d)\n",nbVariables);
  listener();
}// run

void AmpharosManager::treeUNSAT(){
  if(!solutionOk){
    printf("[MANAGER] Solution : %f (%s)\n",get_wall_time(),"UNSAT (All subproblems solved UNSAT)");
    solutionOk = true;
    rankOfTheFirstSolution = -1; //To don't display the solution by a solver
    const int msg = STOP_AND_KILL;
    for(int j = 0 ; j<sizeComm ; j++){// To stop all workers 
      MPI_Send(&msg, 1, MPI_INT, j, TAG_MANAGER, intercomm);
    }
    printf("real time : %g s\n", get_wall_time());
    printf("s UNSATISFIABLE\n");
  }     
}

void AmpharosManager::pharusInitialization(const int rankOfSender){
  static bool firstTimes = true;
  static int variable = 0;
  variable = MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
  MPI_Send_Worker(rankOfSender,variable,TAG_PHARUS);
  if(firstTimes){
    printf("[MANAGER][PHARUS] Initialization (root of the tree: %d)\n",variable);
    tree->createRoot(variable);
    firstTimes=false;
  }
}

void AmpharosManager::pharusTransmissionFromRoot(const int rankOfSender){
  int thread = MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
  /*if(tree->getRoot() == NULL)
    printf("racine NULL\n");
  else
  printf("racine non null : variable : %d\n",tree->getRoot()->variable);*/
  if(nodesAccordingToWorkers[rankOfSender][thread] != NULL)
    nodesAccordingToWorkers[rankOfSender][thread]->delWorker(rankOfSender,thread);
  if(solutionOk || tree->isUnsat()){//If I have SAT or UNSAT
    MPI_Send_Worker(rankOfSender,STOP_AND_KILL,TAG_PHARUS);
    MPI_Send_Worker(rankOfSender,tree->getNbSubproblems(),TAG_PHARUS);
    pharusSendULs(rankOfSender,thread,tree->getRoot());
    MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
    if(tree->isUnsat())treeUNSAT();
  }else{//Send the root
    MPI_Send_Worker(rankOfSender,tree->getRoot()->variable,TAG_PHARUS);
    MPI_Send_Worker(rankOfSender,tree->getNbSubproblems(),TAG_PHARUS);
    pharusSendULs(rankOfSender,thread,tree->getRoot());
    if(MPI_Recv_Worker(rankOfSender,TAG_PHARUS) == PHARUS_ULS_UNSAT){
      printf("[MANAGER][COMPUTER %d][THREAD %d] SUBPROBLEM UNSAT BY ULs ROOT (NbSubproblems:%d)\n"
             ,rankOfSender,thread,tree->getNbSubproblems());
      pharusRemoveAllWorkersOfNode(tree->getRoot());
      tree->getRoot()->deleteNode();
      if(tree->isUnsat())treeUNSAT();
      return;
    }
    Node* subproblem = pharusTransmission(rankOfSender,thread,tree->getRoot());
    if(subproblem){
      subproblem->addWorker(rankOfSender,thread);
      
      nodesAccordingToWorkers[rankOfSender][thread]=subproblem;
      printf("[MANAGER][COMPUTER %d][THREAD %d] Take a new subproblem (NbSubproblems:%d(size:%d-%s) - NbULs:%d(%d))\n"
             ,rankOfSender,thread
             ,tree->getNbSubproblems()
             ,subproblem->getDepth()
             ,(subproblem->father->leftChild == subproblem)?"Left":"Right"
             ,tree->getNbULs()
             ,tree->getNbULsRoot());
      
      
      printf("[MANAGER][COMPUTER %d][THREAD %d] PromotedClauses:%f (save: %f)\n"
             ,rankOfSender,thread
             ,(double)nbTotalPromotedClauses/(double)nbTotalGetPromotedClauses
             ,savePromotedClauses);
      
      
      //printf("[MANAGER][COMPUTER %d][THREAD %d] WORK on (size : %d) :"
//	     ,rankOfSender,thread,subproblem->getDepth());
    //subproblem->displayVariables();
    }
  }
}

Node* AmpharosManager::pharusTransmission(const int rankOfSender,const int thread,Node* currentNode){
  int msgOfWorker = MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
  switch(msgOfWorker)
    {
    case PHARUS_TRANSMISSION_DELETE_FATHER: 
      {//erase the father and you restart the cummunication from the root  
        Node* nodeToCut=pharusPruning(currentNode);
        pharusRemoveAllWorkersOfNode(nodeToCut);
        nodeToCut->deleteNode();
        printf("[MANAGER][COMPUTER %d][THREAD %d] SUBPROBLEM UNSAT DELETE FATHER (NbSubproblems:%d)\n"
               ,rankOfSender,thread,tree->getNbSubproblems());
        
        return NULL;
      }
    case PHARUS_TRANSMISSION_CHILDREN: 
    {//Send the children at the worker
	MPI_Send_Worker_Node(rankOfSender,currentNode->leftChild,TAG_PHARUS);
        MPI_Send_Worker_Node(rankOfSender,currentNode->rightChild,TAG_PHARUS);
        break;
      }         
    case PHARUS_TRANSMISSION_ALREADY_ASSIGNED: 
      {//The litteral is already assigned (0 <=> positif, 1 <=> negatif)
        if(MPI_Recv_Worker(rankOfSender,TAG_PHARUS)){
          Node* nodeToCut=pharusPruning(currentNode->leftChild);
          pharusRemoveAllWorkersOfNode(nodeToCut);
          nodeToCut->deleteNode();
        }else{
          Node* nodeToCut=pharusPruning(currentNode->rightChild);
          pharusRemoveAllWorkersOfNode(nodeToCut);
          nodeToCut->deleteNode();
        }
        printf("[MANAGER][COMPUTER %d][THREAD %d] SUBPROBLEM UNSAT ALREADY ASSIGNED (NbSubproblems:%d)\n"
               ,rankOfSender,thread,tree->getNbSubproblems());
        break;
      }       
    case PHARUS_TRANSMISSION_GO_LEFT: 
      {//Replace the current_node by the left node
        currentNode = currentNode->leftChild;
        pharusSendULs(rankOfSender,thread,currentNode);
        if(MPI_Recv_Worker(rankOfSender,TAG_PHARUS) == PHARUS_ULS_UNSAT){
          Node* nodeToCut=pharusPruning(currentNode);
          pharusRemoveAllWorkersOfNode(nodeToCut);
          nodeToCut->deleteNode();
          printf("[MANAGER][COMPUTER %d][THREAD %d] SUBPROBLEM UNSAT BY ULs (NbSubproblems:%d)\n"
               ,rankOfSender,thread,tree->getNbSubproblems());
          return NULL;
        }
	if(currentNode->variable == END_SUBPROBLEM)return currentNode;//Transmission of subproblem finished
        break;
      }
    case PHARUS_TRANSMISSION_GO_RIGHT: 
      {//Replace the current_node by the right node
        currentNode = currentNode->rightChild;    
        pharusSendULs(rankOfSender,thread,currentNode);
	
        if(MPI_Recv_Worker(rankOfSender,TAG_PHARUS) == PHARUS_ULS_UNSAT){
          Node* nodeToCut=pharusPruning(currentNode);
          pharusRemoveAllWorkersOfNode(nodeToCut);
          nodeToCut->deleteNode();
          printf("[MANAGER][COMPUTER %d][THREAD %d] SUBPROBLEM UNSAT BY ULs (NbSubproblems:%d)\n"
               ,rankOfSender,thread,tree->getNbSubproblems());
          return NULL;
        }
	if(currentNode->variable == END_SUBPROBLEM)return currentNode;//Transmission of subproblem finished
        break;
      }        
    default: 
      {// For debug
        printf("[MANAGER][COMPUTER %d][THREAD %d] BUG During the Transmission : %d (Unknown Message)\n"
	       ,rankOfSender,thread,msgOfWorker);
        exit(0);
        break;
      }
    }
  return pharusTransmission(rankOfSender,thread,currentNode);
}
void AmpharosManager::pharusSubproblemINDETERMINATE(const int rankOfSender){
  const int thread = MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
  const int promotedClauses = MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
  nbTotalPromotedClauses+=promotedClauses;
  nbTotalGetPromotedClauses++;
  //pharusReceiveULs(rankOfSender,thread);
  Node* nodeSubproblem = nodesAccordingToWorkers[rankOfSender][thread];
  if(nodeSubproblem){//If my subproblem steel exists
    if(nbULs != 0)addULs(nodeSubproblem);
    if(nodeSubproblem->variable != END_SUBPROBLEM)
      MPI_Send_Worker(rankOfSender,PHARUS_SUBPROBLEM_EXIST,TAG_PHARUS);
    else{
      MPI_Send_Worker(rankOfSender,PHARUS_SUBPROBLEM_EXIST_LEAF,TAG_PHARUS);
      if(nodeSubproblem->extend(tree->getNbSubproblems()*speedExtend)){	  
        //double tmp = savePromotedClauses;
        savePromotedClauses=(double)nbTotalPromotedClauses/(double)nbTotalGetPromotedClauses;
        
        
        MPI_Send_Worker(rankOfSender,PHARUS_EXTEND,TAG_PHARUS); 
        /*  if(tmp != 0 && tmp-savePromotedClauses > 50){
          printf("[MANAGER][COMPUTER %d][THREAD %d] RESTART\n",rankOfSender,thread);
          }*/
        nodeSubproblem->extendNode(MPI_Recv_Worker(rankOfSender,TAG_PHARUS));
        
        printf("[MANAGER][COMPUTER %d][THREAD %d] EXTEND (NbSubproblems:%d)\n",rankOfSender,thread
               ,tree->getNbSubproblems());
      }else{
        MPI_Send_Worker(rankOfSender,PHARUS_NO_EXTEND,TAG_PHARUS);
      } 
    }
  }else
    MPI_Send_Worker(rankOfSender,PHARUS_SUBPROBLEM_NOT_EXIST,TAG_PHARUS);
  
}

void AmpharosManager::pharusReceiveULs(const int rankOfSender,const int thread){
  nbULs = MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
  if(nbULs){
    MPI_Recv(ULs, nbULs, MPI_INT, rankOfSender, TAG_PHARUS, intercomm, MPI_STATUS_IGNORE);
    nbULsLimit = MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
    MPI_Recv(ULsLimit, nbULsLimit, MPI_INT, rankOfSender, TAG_PHARUS, intercomm, MPI_STATUS_IGNORE);
  }
}
/*
 * I have to improve this methode => remove malloc
 */
void AmpharosManager::pharusSendULs(const int rankOfSender,const int thread, Node* node){
  /*const int position=node->alreadySentULs[rankOfSender][thread];
  const int nbULs = node->ULs.size();
  const int nbULsToSend = nbULs-position;//To take only ULs that don't send yet 
  MPI_Send_Worker(rankOfSender,nbULsToSend,TAG_PHARUS);
  if(nbULsToSend)//If I have to send ULs
    {
      for(int i = position,j = 0; i < nbULs;i++,j++)ULsToSend[j]=node->ULs[i];//Copy
      MPI_Send(ULsToSend,nbULsToSend,MPI_INT,rankOfSender,TAG_PHARUS,intercomm);
      //Update ULs already sent
      node->alreadySentULs[rankOfSender][thread]=node->ULs.size(); 
      }*/
}

void AmpharosManager::pharusSubproblemUNSAT(const int rankOfSender){
  const int thread = MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
  const int assertifLiteral = MPI_Recv_Worker(rankOfSender,TAG_PHARUS);
  if(nodesAccordingToWorkers[rankOfSender][thread]){//If my subproblem steel exists
    //We find the good node according to the assertif literal
    Node* nodeToCut=pharusGetNodeToCut(rankOfSender,thread,assertifLiteral);
    //We remove all workers from this node to its leaves, this workers is inside  
    if(nodeToCut == NULL)printf("problème\n");
    pharusRemoveAllWorkersOfNode(nodeToCut);
    //Finaly, we delete the node 
    Node* father = nodeToCut->father;
    nodeToCut->deleteNode();
    pharusPushUpULsUNSAT(father);//If others node is UNSAT when ULs are push up ! 
    printf("[MANAGER][COMPUTER %d][THREAD %d] SUBPROBLEM UNSAT (NbSubproblems:%d) (time :%f)\n"
           ,rankOfSender,thread,tree->getNbSubproblems(),get_wall_time());
  }//else{
    //printf("[MANAGER][COMPUTER %d][THREAD %d] SUBPROBLEM ALREADY UNSAT\n",rankOfSender,thread); 
  //}
}

Node* AmpharosManager::pharusGetNodeToCut(const int& rank,const int& thread,const int& assertifLiteral){
  //Find the node to cut : at level of assertif literal
  Node* nodeToCut=nodesAccordingToWorkers[rank][thread];
  const int assertifVariable = assertifLiteral>>1;//Division by 2 => to get the variable of a literal
  const bool assertifSign = assertifLiteral&1;//to get the sign of a literal
  //Always start this while with the node of the subproblem (with its variable to END_SUBPROBLEM);
  while(nodeToCut->variable != assertifVariable && nodeToCut->father)nodeToCut=nodeToCut->father;
  nodeToCut=assertifSign?nodeToCut->leftChild:nodeToCut->rightChild;
  //Now we have to do the pruning step : when two children are UNSAT, we can cut the father.
  nodeToCut=pharusPruning(nodeToCut);
  return nodeToCut;
}

Node* AmpharosManager::pharusPruning(Node* currentNode){
  while(currentNode->father && (!currentNode->father->leftChild || !currentNode->father->rightChild))
    currentNode=currentNode->father;
  return currentNode;
}

void AmpharosManager::pharusRemoveAllWorkersOfNode(Node* node){
  std::vector<int> computers;
  std::vector<int> threads;
  node->getWorkersGoDown(computers,threads);
  
  if(!computers.empty()){
    std::vector<int>::iterator itComputers = computers.begin();
    std::vector<int>::iterator itThreads = threads.begin();
    for(; itComputers != computers.end(); itComputers++,itThreads++){
      if(nodesAccordingToWorkers[*itComputers][*itThreads] == NULL){
        printf("bug\n");
        exit(0);
      }
      nodesAccordingToWorkers[*itComputers][*itThreads]->delWorker(*itComputers,*itThreads);
      nodesAccordingToWorkers[*itComputers][*itThreads]=NULL;
    }
  }
  
}

void AmpharosManager::pharusPushUpULsUNSAT(Node* fatherOfNodeToDelete){
  if(fatherOfNodeToDelete && fatherOfNodeToDelete->puchUpULs(fatherOfNodeToDelete)){
    printf("[MANAGER] SUBPROBLEM UNSAT push up ULs\n");
    //Now, get the good UNSAT node
    Node* tmpFather = fatherOfNodeToDelete;
    Node* lastUNSAT = NULL;
    do{
      if(tmpFather->nodeUNSATbyUL())
        lastUNSAT = tmpFather;
      tmpFather=tmpFather->father;
    }while(tmpFather);
    //Now, delete this node
    lastUNSAT=pharusPruning(lastUNSAT);
    pharusRemoveAllWorkersOfNode(lastUNSAT);
    Node* father = lastUNSAT->father;
    lastUNSAT->deleteNode();
    pharusPushUpULsUNSAT(father);
  }
}
  

void AmpharosManager::messageStopComUls(const int rankOfSender){
  unsigned long nbSendULs = 0;
  MPI_Recv(&nbSendULs, 1, MPI_UNSIGNED_LONG, rankOfSender, TAG_MANAGER, intercomm, MPI_STATUS_IGNORE);
  for(int j = 0 ; j<sizeComm ; j++)
    if(j != rankOfSender)nbULsToReceive[j]+=nbSendULs;
  nbWorkerULs++;
  printf("[MANAGER] [COMPUTER %d] Barrier ULs : %d/%d\n",rankOfSender,nbWorkerULs,sizeComm);        
  if(nbWorkerULs == sizeComm){// Ok, everybody are contact the manager for ULs : Good !!
    //Now, send the number of ULs that each worker has receive
    for(int j = 0 ; j<sizeComm ; j++)
      MPI_Send(nbULsToReceive+j, 1, MPI_UNSIGNED_LONG, j, TAG_MANAGER, intercomm);
  }
}

void AmpharosManager::messageStopComClauses(const int rankOfSender){
  unsigned long nbSendClauses = 0;
  MPI_Recv(&nbSendClauses, 1, MPI_UNSIGNED_LONG, rankOfSender
           ,TAG_MANAGER, intercomm, MPI_STATUS_IGNORE);
  for(int j = 0 ; j<sizeComm ; j++)
    if(j != rankOfSender)nbClausesToReceive[j]+=nbSendClauses;
  nbWorkerClauses++;
  printf("[MANAGER] [COMPUTER %d] Barrier Clauses : %d/%d\n",rankOfSender,nbWorkerClauses,sizeComm);  
  if(nbWorkerClauses == sizeComm){// Ok, everybody are contact the manager for ULs : Good !!
    //Now, send the number of Clauses that each worker has receive
    for(int j = 0 ; j<sizeComm ; j++)
      MPI_Send(nbClausesToReceive+j, 1, MPI_UNSIGNED_LONG, j, TAG_MANAGER, intercomm);
  }
}

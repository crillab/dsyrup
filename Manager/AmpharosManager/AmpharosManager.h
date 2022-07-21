
#ifndef AmpharosManage_h
#define AmpharosManage_h

#include "../Main/Manager.h"
#include "../../MessageCoding.h"
#include "../Main/Parser.h"
#include "Tree.h"
using namespace std;

#define SIZE_BUFFER 1024

/**
   Can run parallel solver in concurrence.
 */
class AmpharosManager  : public Manager {
 private:
  
 public:
  ~AmpharosManager();
  AmpharosManager(string instance, vector<Solver> &solvers,int sizeAssignment);
  
  //Principal methods
  void run();  
  void listener();
  void messageStopComUls(const int rankOfSender);
  void messageStopComClauses(const int rankOfSender);
  void treeUNSAT();
  void pharusInitialization(const int rankOfSender);
  void pharusTransmissionFromRoot(const int rankOfSender);
  Node* pharusTransmission(const int rankOfSender,const int thread,Node* currentNode);
  void pharusSubproblemUNSAT(const int rankOfSender);
  void pharusSubproblemINDETERMINATE(const int rankOfSender);
  
  void pharusReceiveULs(const int rankOfSender,const int thread);
  void pharusSendULs(const int rankOfSender,const int thread, Node* node);
  void pharusPushUpULsUNSAT(Node* fatherOfNodeToDelete);
  /*
   * Add all literals of the table ULs between start and end in the node myNode.
   * If this node is UNSAT, return true, else false 
   */
  inline bool addULs(Node* myNode,const int& start, const int& end){
    for(int i = start; i < end;i++)
      if(myNode->addUL(ULs[i]))return true;
    return false;
  }

   /*
   * Add all literals in the table ULs in the tree (from myNode to the root).
   * If a node is UNSAT, return true, else false 
   */
  
  inline bool addULs(Node* myNode){
    if(nbULsLimit != myNode->getDepth()){
      
      printf("Problème: addULs : the node is not the correct node ! %d %d \n",
             nbULsLimit, myNode->getDepth());
       printf("[MANAGER] PROBLEM (NbSubproblems:%d)\n",tree->getNbSubproblems());
      exit(0);
    }
    int pos = nbULsLimit-1;
    int start = ULsLimit[pos];
    int end = nbULs;
    Node* currentNode = myNode;
    for(;;)
    {
      while(currentNode->adjacentIsUNSAT()){//My neighbour is NULL (UNSAT), I take the father
        currentNode=currentNode->father;
        pos--;
        start = (pos==-1)?0:ULsLimit[pos];
      }
      
      if(addULs(currentNode,start,end))
        {//this node is UNSAT
          printf("[MANAGER] SUBPROBLEM UNSAT during add ULs\n");
          Node* tmpFather = currentNode;
          Node* lastUNSAT = NULL;
          do{
            if(tmpFather->nodeUNSATbyUL())
              lastUNSAT = tmpFather;
            
            tmpFather=tmpFather->father;
          }while(tmpFather);
          lastUNSAT=pharusPruning(lastUNSAT);
          pharusRemoveAllWorkersOfNode(lastUNSAT);
          Node* father = lastUNSAT->father;
          lastUNSAT->deleteNode();
          pharusPushUpULsUNSAT(father);
          return true;
        }
      if(pos==-1)return false;
      pos--;
      start = (pos==-1)?0:ULsLimit[pos];
      end = ULsLimit[pos+1];
      currentNode=currentNode->father;
    }   
    return false;
  } 

  Node* pharusGetNodeToCut(const int& rank,const int& thread,const int& assertifLiteral);
  Node* pharusPruning(Node* currentNode);
  void pharusRemoveAllWorkersOfNode(Node* node);
  
  //To help communications 
  inline void MPI_Send_Worker(const int& rank,const int& msg, const int& tag){
    MPI_Send(&msg, 1, MPI_INT, rank, tag, intercomm); 
  }
  
  inline int MPI_Recv_Worker(const int& rank, const int& tag){
    int msg = 0;
    MPI_Recv(&msg, 1, MPI_INT, rank, tag, intercomm, MPI_STATUS_IGNORE);
    return msg;
  }

  inline void MPI_Send_Worker_Node(const int& rank,const Node* node, const int& tag){
    if(node){
      MPI_Send_Worker(rank,node->variable,tag);
      MPI_Send_Worker(rank,node->nbSubproblems,tag);
      MPI_Send_Worker(rank,node->nbWorkers,tag);
    }else{
      MPI_Send_Worker(rank,UNSAT_SUBPROBLEM,tag);
    }
  }  
  

  //Ampharos's variables 
  Tree* tree;
  int sizeAssignment;
  int nbVariables;
  int speedExtend;
  int nbLiterals;
  int* ULs; //The unit literals under assumption that I have to send or receive
  int nbULs;
  int* ULsLimit; 
  int nbULsLimit;
  int* ULsToSend;
  Node*** nodesAccordingToWorkers; // A worker is represented by its computer and its thread
  
  //Data of threads 
  int* nbWorkersPerComputers;//the number of threads (CDCL solvers <=> workers) for each computer
  int nbTotalWorkers;//The total number of threads (CDCL solvers <=> workers) for all computers
  int nbTotalPromotedClauses;
  int nbTotalGetPromotedClauses;
  double savePromotedClauses;
};

#endif


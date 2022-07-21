#ifndef Tree_h
#define Tree_h

#include "mpi.h"
#include <iostream>
#include <vector>
#include <cassert>
#include "../../MessageCoding.h"
#include "../Main/Assignment.h"

class Node
{
 public:

  Node(int nvariable, 
       Node* nleftChild, 
       Node* nrightChild, 
       Node* nfather,
       int nnbSubproblems);
  
  
  int id; //Unique reference number
  int variable; //Variable of this node

  Node* leftChild; //Left child of this node (binary tree) 
  Node* rightChild; //Right child of this node (binary tree)
  Node* father; //Father of this node
  
  int nbSubproblems; //Numbers of subproblems passing by this node 
  int nbWorkers; //Numbers of workers working on the subproblem associated at this node 
  int nbExtends; //Numbers of times that someone wanted extend this node

  std::vector<int> computers; //A worker is associated to a computer 
  std::vector<int> threads; //A worker is associated to a thread 
  
  //For ULs (unit literals under assumption)
  std::vector<int> ULs; //Assumptive unit literals of this node
  std::vector<bool> dataULs; //To verify faster if a unit literals is present in ULs or not 
  std::vector<std::vector<int>> alreadySentULs; //Positions (of ULs) of units literals already sent for each worker 
  
  //Static varaibles 
  static int nbWorkersMax;  
  static int nbULsMax;
  static int idMax;


  inline ~Node(){  
    if(variable == END_SUBPROBLEM)
      decrementNbSubproblems();
    if(rightChild) delete(rightChild);
    if(leftChild) delete(leftChild);
  }
  
  inline void deleteNode(){
    if(father){
      if(father->leftChild == this)
        father->leftChild = NULL;
      else
        father->rightChild = NULL;
      delete(this);
    }else{
      leftChild = NULL;
      rightChild = NULL;
      nbSubproblems=0;
    }
  }
  
  inline void extendNode(const int& nVariable){
    variable=nVariable;
    leftChild = new Node(END_SUBPROBLEM, NULL, NULL, this,1);
    rightChild = new Node(END_SUBPROBLEM, NULL, NULL, this,1);
    incrementNbSubproblems();
  }

  inline bool extend(const int& maxToExtend){
    nbExtends++;
    if(nbExtends > maxToExtend)return true;
    return false;
  }

  inline void incrementNbSubproblems(){
    nbSubproblems++;
    if(father)father->incrementNbSubproblems();
  }

  inline void decrementNbSubproblems(){
    nbSubproblems--;
    if(father)father->decrementNbSubproblems();
  }

  inline void incrementNbWorkers(){
    nbWorkers++;
    if(father)father->incrementNbWorkers();
  }

  inline void decrementNbWorkers(){
    nbWorkers--;
    if(father)father->decrementNbWorkers();
  }

  
  inline void getWorkers(std::vector<int>& recupComputers,std::vector<int>& recupThreads)
  {
    if(!computers.empty()){
      std::vector<int>::iterator itComputers = computers.begin();
      std::vector<int>::iterator itThreads = threads.begin();
      for(; itComputers != computers.end(); itComputers++,itThreads++){
        //printf("get %d %d (%d)\n",*itComputers,*itThreads,variable);
        recupComputers.push_back(*itComputers);
        recupThreads.push_back(*itThreads);
      }
    }  
  }

  inline void getWorkersGoDown(std::vector<int>& recupComputers,std::vector<int>& recupThreads)
  {
    getWorkers(recupComputers,recupThreads);
    if(rightChild)rightChild->getWorkersGoDown(recupComputers,recupThreads);
    if(leftChild)leftChild->getWorkersGoDown(recupComputers,recupThreads);
  }

  inline void addWorker(const int& computer,const int& thread){
    computers.push_back(computer);
    threads.push_back(thread);
    incrementNbWorkers();
  }

  inline void delWorker(const int& computer,const int& thread){
    std::vector<int>::iterator itComputers = computers.begin();
    std::vector<int>::iterator itThreads = threads.begin();
    
    for(; itComputers != computers.end(); itComputers++,itThreads++)
      {
        if (*itComputers == computer && *itThreads == thread)
          {
            computers.erase(itComputers);
            threads.erase(itThreads);
            decrementNbWorkers();
            break;  //itComputers and itThreads are now invalid !
          }
      }
  }

  /*
   * Verify if a literal and its opposed is in this node 
   * If yes, return true, this node is UNSAT
   * Else return false
   */
  inline bool nodeUNSATbyUL(const int& lit){
    if(dataULs[lit] && dataULs[lit%2?lit-1:lit+1])return true;
    return false;
  }

/*
   * Verify if a literal and its opposed is in this node 
   * If yes, return true, this node is UNSAT
   * Else return false
   */
  inline bool nodeUNSATbyUL(){
    const int size = (int)dataULs.size();
    for(int i = 0; i < size;i+=2){
      if(dataULs[i] && dataULs[i+1])
        return true; 
    } 
    return false;
  }
  
  /*
   * Return True if a node is UNSAT, else false
   */
  inline bool addUL(const int& lit){
    if(dataULs[lit] == false){//We can add this literal      
      ULs.push_back(lit);
      dataULs[lit]=true;
      if(father != NULL 
         && (
           (father->leftChild == this && father->rightChild 
            && father->rightChild->dataULs[lit] == true)
           || (father->rightChild == this && father->leftChild 
               && father->leftChild->dataULs[lit] == true)
           )
        )
      { // I can push up my literal
        const bool nodeUNSAT = nodeUNSATbyUL(lit);//This node is UNSAT or not
        const bool fatherUNSAT = father->addUL(lit);
        return nodeUNSAT || fatherUNSAT;
      }
      return nodeUNSATbyUL(lit);
    }
    return false;
  }

  
  inline void clearULs()
  {
    ULs.clear();
    for (std::vector<bool>::iterator i = dataULs.begin(); i != dataULs.end(); ++i) *i=false;
    for (int i = 0; i < Node::nbWorkersMax; ++i){
      for (int j = 0; j < Node::nbWorkersMax; ++j){
        alreadySentULs[i][j]=0;
      }
    } 
  }

  /*
   * Go down while one of both (left or right) is NULL (UNSAT) to put ULs in the node in parameter
   * Return True if a node is UNSAT, else false
   */
  inline bool puchUpULs(Node* node){
    if(this != node)//If it'is this node, don't touch, else push up ULs
    {
      if(!ULs.empty())
      {
        for (std::vector<int>::const_iterator i = ULs.begin(); i != ULs.end(); ++i)
	    if(node->addUL(*i)){
		return true;
	    }
        clearULs();//Reinit all data structures of ULs of this node 
      }
    }
    
    //When one of both is NULL (UNSAT)
    if((!rightChild && leftChild) || (!leftChild && rightChild)){
      if(rightChild) //The left node is NULL 
        return rightChild->puchUpULs(node);
      else //The right node is NULL
        return leftChild->puchUpULs(node);
    }
    return false;
  }

  
  inline int getNbULsTree(){
      int nbLeft = 0;
      int nbRight = 0;
      if(rightChild)nbRight = rightChild->getNbULs();
      if(leftChild)nbLeft = leftChild->getNbULs();
      return nbLeft+nbRight+ULs.size();
  }

  inline int getNbULs(){return ULs.size();}
  
  inline bool adjacentIsUNSAT(){
    if(father &&
       ((father->leftChild == this && father->rightChild == NULL)
        ||
        (father->rightChild == this && father->leftChild == NULL)
         )
      )
    {
      return true;
    }
    return false;
  }

  
  inline int getDepth()
  {
    if(father)
      return father->getDepth() + 1;
    return 0;
  }
  
  inline void displayWorkers(){
    printf("Display workers of a node (variable : %d) :\n",variable);
    std::vector<int>::iterator itComputers = computers.begin();
    std::vector<int>::iterator itThreads = threads.begin();
    
    for(; itComputers != computers.end(); itComputers++,itThreads++)
      printf("(%d-%d) ", *itComputers,*itThreads);
    printf("\n");
  }
  
  inline void displayVariables(){
    if(father)
      father->displayVariables();
    printf("%d ",variable);
    if(variable == END_SUBPROBLEM)printf("\n");
  }

};




class Tree
{
 public:
  Tree(int nbWorkersMax,int nbULsMax); 

  Node* root; // The racine of this tree
  inline Node* getRoot() {return root;}
  inline void setRoot(Node* nRoot) {root=nRoot;}
  inline void createRoot(const int& nVariable){
    root=new Node(END_SUBPROBLEM,NULL,NULL,NULL,1);
    root->extendNode(nVariable);
  }
  inline int getNbSubproblems(){return root->nbSubproblems;}
  inline int getNbULs(){return root->getNbULsTree();}
  inline int getNbULsRoot(){return root->getNbULs();}
  
  inline bool isUnsat(){return !root->nbSubproblems;}
};

#endif

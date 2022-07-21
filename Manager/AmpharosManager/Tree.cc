using namespace std;

#include "Tree.h"

int Node::nbWorkersMax=0;
int Node::nbULsMax=0;
int Node::idMax=0;

Node::Node(int nvariable, 
           Node* nleftChild, 
           Node* nrightChild, 
           Node* nfather,
           int nnbSubproblems):
  id(++Node::idMax),
  variable(nvariable),
  leftChild(nleftChild),
  rightChild(nrightChild),
  father(nfather),
  nbSubproblems(nnbSubproblems),
  nbWorkers(0),
  nbExtends(0),
  dataULs(nbULsMax,false),
  alreadySentULs(nbWorkersMax, vector<int>(nbWorkersMax,0))
{}

Tree::Tree(int nbWorkersMax,int nbULsMax):
  root(NULL)  
{
  Node::nbWorkersMax=nbWorkersMax;
  Node::nbULsMax=nbULsMax;
}





#include "com/ComSubsum.h"

using namespace Glucose;

ComSubsum::ComSubsum(int PnbLiterals, bool type):
  nbLiterals(PnbLiterals)
  ,deleted(0)
  ,check(0)
  ,localCheck(0)
  ,localDeleted(0)
  ,limClauses(0)
  ,nbClauses(0)
  ,hash(0)
  ,goodClause(NULL)
{  
  if(!type){
    clauses = (int*)malloc(1000*1000*2*sizeof(int));
    currentClause = (bool*)malloc(sizeof(bool)*nbLiterals);
    memset(currentClause,false,nbLiterals);
    watch = (std::vector<hashClause>**)malloc(nbLiterals*sizeof(std::vector<hashClause>*));
    for(int i = 0; i < nbLiterals; i++)watch[i] = new std::vector<hashClause>();
  }else{
    currentClause = (bool*)malloc(sizeof(bool)*nbLiterals);
    memset(currentClause,false,nbLiterals);
    watchGlu = (std::vector<hashClauseGlu>**)malloc(nbLiterals*sizeof(std::vector<hashClauseGlu>*));
    for(int i = 0; i < nbLiterals; i++)watchGlu[i] = new std::vector<hashClauseGlu>();
  }
}


bool ComSubsum::subsum(int* clause, int size){
  bool isSubsum = false;
  int other_size = 0;
  int* other_clause = NULL;
  std::vector<hashClause>* myWatch;
  for(int i = 0; i < size; i++){
    myWatch = (watch[clause[i]]); 
    if(myWatch->size()){
      for (std::vector<hashClause>::iterator it = myWatch->begin();it != myWatch->end();++it){
        if(((hash & (it->hash)) == (it->hash)) && size >= *(it->addr)){
          isSubsum = true;
          other_size = *(it->addr);
          other_clause = (it->addr)+1;
          for(int j = 0;j < other_size && isSubsum;j++)
            isSubsum=currentClause[other_clause[j]];
          if(isSubsum)return true;
        }
      }
    } 
  }
  return false;
}






bool ComSubsum::add(int* clause, int size){
  bool ret = false;
  for(int i = 0; i < size; i++)currentClause[clause[i]]=true;
  hash = getHashClause(clause,size);
  if(subsum(clause,size)){
    localDeleted++;
    deleted++;
    ret = true;
  }else{
    (watch[*clause])->push_back(hashClause());
    (watch[*clause])->back().addr = clauses+limClauses; 
    (watch[*clause])->back().hash = hash; 
    clauses[limClauses++]=size; 
    for(int j = 0; j < size;j++)clauses[limClauses++]=clause[j];       
    nbClauses++;
    ret = false;
  }
  for(int i = 0; i < size; i++)currentClause[clause[i]]=false;
  check++;
  localCheck++;
  return ret;
}

bool ComSubsum::add(Clause& c){
 int size = c.size();
 int clause[size];
 for(int i = 0; i < size; i++)clause[i]=toInt(c[i]);
 return add(clause,size);
}


uint64_t ComSubsum::getHashClause(const int* clause, const int size){
  uint64_t hash = 0;
  for(int i=0;i < size;i++)hash |= 1<<(clause[i]%64);
  return hash;
} 
 


void ComSubsum::clear(bool type){
  if(!type){
    localCheck = 0;
    localDeleted = 0;
    limClauses = 0;
    for(int i = 0; i < nbLiterals; i++)watch[i]->clear();
  }
}


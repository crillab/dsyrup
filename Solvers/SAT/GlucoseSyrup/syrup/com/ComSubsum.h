

#ifndef SUB
#define	SUB
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "core/SolverTypes.h"
#include <mutex>
#include <vector>




namespace Glucose {
 
  struct hashClause{
    int* addr;
    uint64_t hash;
  };
  struct hashClauseGlu{
    Clause* addr;
    uint64_t hash;
  };
  class Clause;  
 
  class ComSubsum {
   public:
    ComSubsum(int PnbLiterals,bool type);
    
    inline bool subsum(int* clause, int size);
    bool add(int* clause, int size);
    bool add(Clause& c);
    inline uint64_t getHashClause(const int* clause, const int size);
    inline Clause* getLastGoodClause(){return goodClause;}

   
    void clear(bool type);

    int nbLiterals;
    int deleted;
    int check;
    int localCheck;
    int localDeleted;
    int limClauses;
    int nbClauses;
    uint64_t hash; 
    int* clauses;
    bool* currentClause;
    
    Clause* goodClause;

    std::vector<hashClause>** watch;
    std::vector<hashClauseGlu>** watchGlu;
  };
}

#endif

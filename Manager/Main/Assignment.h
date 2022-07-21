#ifndef Assignment_h
#define Assignment_h

#include <string>
using namespace std;


/**
   Allow to represent an assignment as a tuple.

   The tuple of size 0 is an unsatisfiable assigment. 
   The tuple assigned (with a non zero size) to NULL is interpreted as
   an UNDEF value.
 */
class Assignment
{
 protected:
  int sizeTuple;
  int *tuple;
  
 public:
  ~Assignment(){if(tuple) delete(tuple);}

  Assignment(); // an empty tuple means that we have an unsatisfiable assignment
  Assignment(int l);
  Assignment(int var, int val);
  Assignment(int sz, int *tab);
  
  inline bool isNotCoherent(){return sizeTuple == 0;}
  inline void makeUnsatisfiable(){if(tuple) delete(tuple); sizeTuple = 0; tuple = NULL;}

  inline void assign(int sz, const int *tab)
  {
    if(sz == sizeTuple)
      {
        if(tuple) tuple = new int[sz];
        for(int i = 0 ; i<sz ; i++) tuple[i] = tab[i];
      }else
      {
        if(tuple) delete []tuple;

        if(sz)
          {
            tuple = new int[sz];
            for(int i = 0 ; i<sz ; i++) tuple[i] = tab[i];
          }else tuple = NULL;
      }
    sizeTuple = sz;
  }// assign
  
  
  /**
     Here we change the meaning of != (is different from ! ==)
     We want to encode the complementary.
  */
  bool operator^(Assignment &d)
  {
    if(!tuple || !d.tuple) return false;    
    if(sizeTuple != d.sizeTuple) return false;
    if(tuple[0] == -d.tuple[0]) return (sizeTuple == 1) ? true : tuple[1] == d.tuple[1];
  }
  
  bool operator==(Assignment &d)
  {
    if(tuple != d.tuple) return false;
    if(!tuple && !d.tuple) return true;
    if(sizeTuple != d.sizeTuple) return false;
    for(int i = 0 ; i<sizeTuple ; i++) if(tuple[i] != d.tuple[i]) return false;
    return true;
  }
  
  friend ostream &operator<<(ostream &os, Assignment &m)
  {
    if(!m.tuple) return os << "@";
    if(m.sizeTuple == 0) return os << "\u2620";    
    if(m.sizeTuple == 1) return os << *m.tuple;    
    if(*m.tuple < 0) return os << -m.tuple[0] << "\u2260" << m.tuple[1];
    return os << m.tuple[0] << "=" << m.tuple[1];
  }  
};


#endif

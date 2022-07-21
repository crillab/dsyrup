#include <iostream>
#include <cassert>


#include "Assignment.h"

/**
   Construct an empty assignment.
 */
Assignment::Assignment()
{
  sizeTuple = 0;
  tuple = NULL;
}


/**
   Construct a boolean assignment.
   
   @param[in] l, the literal
 */
Assignment::Assignment(int l)
{
  sizeTuple = 1;
  tuple = new int[1];
  *tuple = l;
}


/**
   Construct an assignment such as used in CSP.
   
   @param[in] var, the variable (if var < 0, then var != val, else var = val
   @param[in] val, the value
 */
Assignment::Assignment(int var, int val)
{
  sizeTuple = 2;
  tuple = new int[2];
  tuple[0] = var;
  tuple[1] = val;
}


/**
   Construct an assigment which is a tuple.

   @param[in] sz, the number of element in tab
   @param[in] tab, the tuple
 */
Assignment::Assignment(int sz, int *tab)
{
  sizeTuple = sz;
  tuple = tab;
}

#include "InfoSystem.h"

/**
 * 
 */
int InfoSystem::memReadStat(int field)
{
  char name[256];
  pid_t pid = getpid();
  int     value;
  sprintf(name, "/proc/%d/statm", pid);
  FILE*   in = fopen(name, "rb");
  if (in == NULL) return 0;
  for (; field >= 0; field--)
    assert(fscanf(in, "%d", &value));
  fclose(in);
  return value;
}// memReadStat

/**
 * Donne la quantié de mémoire utilisé par le processus
 * @return la quantité de mémoire en mega
 */
uint64_t InfoSystem::memUsed() 
{
  return (uint64_t)memReadStat(0) * (uint64_t) getpagesize();
}// memUsed

/**
 * Elapsed seconds 
 * @return le temps passé depuis la dernière fois qu'on l'a demandé
 */
double InfoSystem::elapsed_seconds() 
{ 
  double answer;   
  answer = (double) clock() / CLOCKS_PER_SEC - totalTime;
  totalTime = (double) clock() / CLOCKS_PER_SEC;
  return answer; 
}/* elapsed_seconds */

/**
 * Fonction permettant de savoir le temps passé depuis le début du
 * programme
 * @return le temps passé depuis le début du programme
 */
double InfoSystem::tpsElapsed()
{
  return (double) clock() / CLOCKS_PER_SEC;
}//tps_ecouler


/**
 * Constructeur
 */
InfoSystem::InfoSystem()
{
  totalTime = 0;
}// InfoSystem

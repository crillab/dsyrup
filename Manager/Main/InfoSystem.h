#ifndef __infoSystem__
#define __infoSystem__

#include <stdio.h>
#include <stdlib.h> 
#include <ctime>

#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/types.h>


/**
 * Classe permettant de fournir des information sur le system tel
 * que :
 *    - le temps CPU depuis le lancement du processus
 *    - le temps CPU depuis le dernière fois que cela a été demandé
 *    - la quantité de mémoire untilisée par le processus
 */
class InfoSystem 
{
private:
  double totalTime;
  int memReadStat(int field);

public:
  InfoSystem();
  uint64_t memUsed();
  double elapsed_seconds();
  double tpsElapsed();
};
#endif


#ifndef OPTION
#define OPTION

//For the option called how_clause_sharing
#define OptComMPIFully 1 // Fully Hybrid Model 
#define OptComMPIPartially 2 // Partial Hybrid Model

#define MAX_SIZE_CLAUSE 400 // Maximal lenght of clauses (400 literals) 

struct OptConcurrent
{
  bool optDisplay;
  bool optClauseSharing;
  bool optUlSharing;
  unsigned int optHowClauseSharing;
  int optAlea;
  unsigned int optConflicts;
  bool optComNoCom;
  bool optComNoCom1;
  bool optComNoClausesFromNetwork;
  int optSleepingTimeBase;
  int optSleepingTimeListener;
  int optSleepingTimeSender;
  int optSleepingTimeCollectif;
  int optRingLimit;
  int optGroup;
  bool optEncoding;
  bool optAnalysePromoted;
};

#define OPT_VSIDS 1
#define OPT_INIT_VSIDS            1
#define OPT_INIT_DECISIONS_VSIDS  2

struct OptAmpharos
{
  unsigned int optInitializationDuration;
  unsigned int optSubproblemWorkDuration;
  unsigned int optTreeHeuristic;
  unsigned int optSpeedExtend;
};

#endif

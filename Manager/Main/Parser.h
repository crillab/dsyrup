#ifndef Parser_h
#define Parser_h
#include <unistd.h>
#include <iostream>
#include <fstream>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <vector>

#include "InfoSystem.h"
#include <regex>
using namespace std;


/**
   Machine: hostname and number of jobs run on this proc.
 */
struct Machine
{
  string hostname;
  int npernode;
};


/**
   Structure to store a solver.
 */
struct Solver
{
  string executablePath;
  vector<string> options;
  Machine machine;
};


/**
   Cube generator information
 */
struct CubeGenerator
{
  string executablePath;
  vector<string> options;
  vector<Machine> setOfMachines;
};


/**
   Class to manage with the config file.
 */
class Parser
{
 public:
  string method;
  CubeGenerator cg;
  
  string instance;
  string problemType;
  InfoSystem *infoSys;
  vector<Solver> solvers;
  int sizeAssignment;
    
  string currentDir;
  
 private:
  xmlDoc *doc;

  void collectInfoBench(xmlNode *instanceNode);
  void collectInformation(xmlNode * root);
  void collectSolvers(xmlNode * root);
  void collectSolver(xmlNode * root);
  void collectInfoMethod(xmlNode *methodNode);
  void collectMachine(xmlNode *machineNode, Machine &machine);
  void collectEps(xmlNode *epsNode);
  void collectOptions(xmlNode *optionsNode, vector<string> &listOfOptions);

  /**
     Print the information relative to the solvers collected in the
     configuration file.
  */
  inline void printSolverInformation()
  {
    cout << "c Solvers' information:" << endl;
    cout << "c ----------------------" << endl;
    for(unsigned int i = 0 ; i<solvers.size() ; i++)
      {
        cout << "c \033[1;31mSolver number " << i + 1 << " \033[0m"<< endl;
        cout << "c Executable Path: " << solvers[i].executablePath << endl;        
        cout << "c Options: " << (solvers[i].options).size() << endl;
        for(unsigned j = 0 ; j<(solvers[i].options).size() ; j++)
          cout << "c      options[" << j << "]: " << (solvers[i].options)[j] << endl;
        
        Machine m = solvers[i].machine;
        cout << "c Npernode: " << m.npernode << endl;
        cout << "c Host: " << m.hostname << endl;
        cout << "c" << endl;
      }
  }// printSolverInformation


  /**
     Print the information about the cube generator.
   */
  inline void printCubeGenerator(CubeGenerator &cg)
  {
    cout << "c Cube generator information: " << endl;
    cout << "c      - executable: " << cg.executablePath << endl;
    
    cout << "c      - options: " << endl;
    for(unsigned int i = 0 ; i<(cg.options).size() ; i++)
      cout << "c                - option[" << i << "]: " << (cg.options)[i] << endl;

    cout << "c      - machines: " << endl;
    for(unsigned int i = 0 ; i<(cg.setOfMachines).size() ; i++)
      cout << "c                - machine[" << i << "]: host=" <<
        (cg.setOfMachines)[i].hostname << " \t np=" << (cg.setOfMachines)[i].npernode << endl;
    
  }// printCubeGenerator
  
  inline void createGoodString(xmlChar* xs, string* p_string){
    string my_string = *p_string;
    *p_string = string((char *) xs);
    *p_string = regex_replace(*p_string, regex("^ +| *$"), "");
    size_t found = p_string->find("$PWD");
    if(found != std::string::npos)p_string->replace(found,  4, currentDir);
  }

 public:
  // constructor/destructor 
  ~Parser();  
  Parser(const char *nameBench, const char *nameFile);
  Parser(const char *nameFile){
    doc = xmlReadFile(nameFile, NULL, 0);
    sizeAssignment = 0;
    infoSys = new InfoSystem();
    Parser("/deb/stdin", nameFile);
  }

  inline void printInformation()
  {
    /*printSolverInformation();

    cout << "c Method runs: " << method << endl;
    if(method == "eps") printCubeGenerator(cg);
    
    cout << "c Instance name: " << instance << endl;
    cout << "c Kind of problem: " << problemType << endl;
    if(sizeAssignment) cout << "c We consider assigments of size: " << sizeAssignment << endl;*/
  }// printInformation
  
};

#endif

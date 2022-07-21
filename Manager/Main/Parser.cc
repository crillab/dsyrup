#include "mpi.h"
#include <vector>
#include <string.h>
#include <cassert>
#include <algorithm>
#include <cctype>
#include <regex>

#include "InfoSystem.h"
#include "Parser.h"


/**
   Close the doc and clean up the memory.
 */
Parser::~Parser()
{
  delete(infoSys);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  xmlMemoryDump();
}// destructor


/**
   Take an input a name file and parse it to extract the necessary
   information to run the solver.

   @param[in] nameFile, the name of the file
 */
Parser::Parser(const char *nameBench, const char *nameFile)
{
  instance = nameBench;
  problemType = "unknown";
  method= "concurrent";
  infoSys = new InfoSystem();
  sizeAssignment = 0;
  
  //Get the current directory
  char cwd[1024];
  int size_cwd = 0;
  if(getcwd(cwd, sizeof(cwd)) == NULL){
    printf("Erreur getcwd\n");
    exit(-1);
  }
  
  for(;cwd[size_cwd] != '\0';size_cwd++);
  currentDir.assign(cwd,size_cwd);
  
  
  xmlNode *root_element = NULL;
  
  /*parse the file and get the DOM */
  doc = xmlReadFile(nameFile, NULL, 0);

  if(!doc)
    {
      cerr << "error: could not parse file " << nameFile << endl;
      exit(2);
    }
  
  // Get the root element node and collect the information
  root_element = xmlDocGetRootElement(doc);
  collectInformation(root_element);
  //cerr << "error: could not parse file " << nameFile << endl; 
}// Parser


/**
   Given a node, collect the information relative to the instance.

   @param[in] instanceNode, an instance node
 */
void Parser::collectInfoBench(xmlNode *root)
{
  assert(root->type == XML_ELEMENT_NODE && !xmlStrcmp(root->name, (const xmlChar *)"problem"));
  xmlNode *c = root->children;
  
  while(c)
    {
      if(c->type == XML_ELEMENT_NODE)
        {
          xmlChar *xs = xmlNodeListGetString(root->doc, c->children, 1);
          
          if(!xmlStrcmp(c->name, (const xmlChar *)"sizeAssignment")) sizeAssignment = stoi(string((char *) xs));
          else if(!xmlStrcmp(c->name, (const xmlChar *)"name"))
            {
              createGoodString(xs,&instance);
            }
          else if(!xmlStrcmp(c->name, (const xmlChar *)"type"))
            {
              createGoodString(xs,&problemType);
            }
          else
            {
              cerr << "c [collectInfoBench] Error, unknown element was reached: " << c->name << endl;
              exit(6);
            }
          
          xmlFree(xs);
        }
      
      c = c->next;
    }  
}// collectNameBench


/**
   Collect the options from a given option node.

   @param[in] optionsNode, an options node
   @param[out] opt, the place where are saved options
 */
void Parser::collectOptions(xmlNode *optionsNode, vector<string> &opt)
{
  for(xmlNode *optNode = optionsNode->children ; optNode ; optNode = optNode->next)
    {
      if(optNode->type == XML_ELEMENT_NODE)
        {
          if(!xmlStrcmp(optNode->name, (const xmlChar *)"option"))
            {
              xmlChar *xr = xmlNodeListGetString(optionsNode->doc, optNode->children, 1);
              string tmp; 
              createGoodString(xr,&tmp);
              opt.push_back(tmp);
              //opt.push_back(string((char *) xr));
              //opt.back() = regex_replace(opt.back(), regex("^ +| *$"), "");
              if(opt.back() == "BENCHNAME") opt.back() = instance;
              xmlFree(xr);
            }
          else
            {
              cerr << "c [collectOptions] An option element was expected: " << optNode->name << endl;
              exit(8);
            }
        }
    }
}// collectOptions


/**
   Given a solver node, collect the information about a solver.

   @param[in] root, a solver node
 */
void Parser::collectSolver(xmlNode * root)
{
  assert(root->type == XML_ELEMENT_NODE && !xmlStrcmp(root->name, (const xmlChar *)"solver"));
  xmlNode *c = root->children;

  bool execPath = false;
  
  Solver s;
  Machine &m = s.machine;
  m = {"localhost", 1};
  
  while(c)
    {
      if(c->type == XML_ELEMENT_NODE)
        {
          xmlChar *xs = xmlNodeListGetString(root->doc, c->children, 1);
          
          if(!xmlStrcmp(c->name, (const xmlChar *)"machine")) collectMachine(c, m);
          else if(!xmlStrcmp(c->name, (const xmlChar *)"executable"))
            {
              createGoodString(xs,&(s.executablePath));
              execPath = true;
            }
          else if(!xmlStrcmp(c->name, (const xmlChar *)"options"))
            {
              vector<string> &opt = s.options;
              collectOptions(c, opt);
            }
          else
            {
              cerr << "c [collectSolver] Error, unknown element was reached: " << c->name << endl;
              exit(6);
            }
          
          xmlFree(xs);
        }
      
      c = c->next;
    }

  if(!execPath)
    {
      cerr << "c You have to define a valuable executable path" << endl;
      exit(7);
    }
  
  solvers.push_back(s);
}// collectSolver

/**
   Given a solvers node, collect all the solvers.

   @param[in] root, a solvers node
 */
void Parser::collectSolvers(xmlNode * root)
{
  assert(root->type == XML_ELEMENT_NODE && !xmlStrcmp(root->name, (const xmlChar *)"solvers"));
  xmlNode *c = root->children;

  while(c)
    {
      if(c->type == XML_ELEMENT_NODE)
        {
          if(!xmlStrcmp(c->name, (const xmlChar *)"solver")) collectSolver(c);
          else
            {
              cerr << "c Error, element solver waited: " << c->name << endl;
              exit(6);
            }
        }
      
      c = c->next;
    }
}// collectSolvers


/**
   Collect information about a node machine.

   @param[in] mn, a machine node
 */
void Parser::collectMachine(xmlNode *machineNode, Machine &m)
{
  m = {"localhost", 1};
  xmlNode *c = machineNode->children;
  
  while(c)
    {
      if(c->type == XML_ELEMENT_NODE)
        {
          xmlChar *xs = xmlNodeListGetString(machineNode->doc, c->children, 1);

          if(!xmlStrcmp(c->name, (const xmlChar *)"npernode")) m.npernode = stoi(string((char *) xs));
          else if(!xmlStrcmp(c->name, (const xmlChar *)"host"))
            {
              createGoodString(xs,&(m.hostname));
            } else
            {
              cerr << "c collectMachine => error unknown element was reached: " << c->name << endl;
              exit(8);
            }

          xmlFree(xs);
        }
      
      c = c->next;
    }
}// collectMachine



/**
   Collect a Eps node.

   @param[in] eps, a method node with the attribute eps.
 */
void Parser::collectEps(xmlNode *epsNode)
 {
   xmlNode *c = epsNode->children;
      
   while(c)
     {
       if(c->type == XML_ELEMENT_NODE)
         {
           xmlChar *xs = xmlNodeListGetString(epsNode->doc, c->children, 1);
                        
           if(!xmlStrcmp(c->name, (const xmlChar *)"generator"))
             {
               createGoodString(xs,&(cg.executablePath));
             }
           else if(!xmlStrcmp(c->name, (const xmlChar *)"options")) collectOptions(c, cg.options);
           else if(!xmlStrcmp(c->name, (const xmlChar *)"machines"))
             {
               xmlNode *mn = c->children;
               while(mn)
                 {
                   if(mn->type == XML_ELEMENT_NODE)
                     {
                       if(!xmlStrcmp(mn->name, (const xmlChar *)"machine"))
                         {
                           Machine machine;
                           collectMachine(mn, machine);
                           (cg.setOfMachines).push_back(machine);
                         }
                       else
                         {
                           cerr << "c A machine node is waited: " << c->name << endl;
                           exit(6);
                         }
                     }
                   mn = mn->next;
                 }
             }
           else 
             {
               cerr << "c Error when collecting the information to built the cubes: " << c->name << endl;
               exit(6);
             }
              
           xmlFree(xs);
         }
          
       c = c->next;
     }
 }// collectEps

/**
   Given a solvers node, collect the information about the parallel
   method used (type and additional material).

   @param[in] root, a solvers node
 */
void Parser::collectInfoMethod(xmlNode *methodNode)
{ 
  xmlAttr* matAttr = methodNode->properties;
  while (matAttr)
    {
      if (string((char *)matAttr->name) == "type")
        {
          xmlChar *s = xmlNodeListGetString(methodNode->doc, matAttr->children, 1);
          method = string((char *) s);
          xmlFree(s);
        }
      else cerr << "Not useful attribute " << string((char *)matAttr->children->content) << endl;
      matAttr = matAttr->next;
    }

  
  if(method == "eps")
    {
      collectEps(methodNode);
      if(!(cg.setOfMachines).size())
        {
          cerr << "c The method to generate the cube is not given" << endl;
          exit(7);
        }      
    }
  else assert(method == "concurrent" || method == "ampharos");

}// collectInfoMethod


/**
   Read the XML tree from the root and prepare the solver.
   
   @param[in] root, the root of the XML file
 */
void Parser::collectInformation(xmlNode * root)
{
  if(root->type != XML_ELEMENT_NODE || xmlStrcmp(root->name, (const xmlChar *)"config"))
    {
      cerr << "c Config file not correct (must start by config)" << endl;
      exit(3);
    }

  assert(!root->next);
  
  xmlNode *configElement = root->children;
  
  for ( ; configElement ; configElement = configElement->next)
    {
      if(configElement->type == XML_ELEMENT_NODE)
        {
          if(!xmlStrcmp(configElement->name, (const xmlChar *)"problem")) collectInfoBench(configElement);
          else if(!xmlStrcmp(configElement->name, (const xmlChar *)"solvers")) collectSolvers(configElement);
          else if(!xmlStrcmp(configElement->name, (const xmlChar *)"method")) collectInfoMethod(configElement);
          else
            {
              cerr << "c Element unknown: " << configElement->name << endl;
              exit(4);
            }
        }
    }
}// collectInformation
 

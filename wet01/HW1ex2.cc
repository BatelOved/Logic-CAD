#include <errno.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include <list>
#include <algorithm>
#include "hcm.h"
#include "flat.h"

using namespace std;

bool verbose = false;

// ./gl_rank TopLevel1355 stdcell.v c1355high.v
// ./gl_rank TopLevel2670 stdcell.v c2670high.v

list<int> getRankInputListInitPropVal(hcmInstance* cell) {
  return list<int>();
}

int getRankInitPropVal(hcmInstance* cell) {
  return 0;
}

template <typename T>
void setPropTree(hcmCell* cell, string PropName, T (*getPropVal)(hcmInstance*)) {

  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    assert(iI->second->setProp<T>(PropName, getPropVal(iI->second)) == OK);
    hcmCell* currCell = iI->second->masterCell();
    setPropTree<T>(currCell, PropName, getPropVal);
  }
}

template <typename T>
void delPropTree(hcmCell* cell, string PropName) {

  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    assert(iI->second->delProp<T>(PropName) == OK);
    hcmCell* currCell = iI->second->masterCell();
    delPropTree<T>(currCell, PropName);
  }
}

list<pair<int, hcmInstPort*>> flatTraverseNode(vector<pair<int, string>>& maxRankVector, pair<int, hcmInstPort*> instPortPair, set<string>* globalNodes, string rankPropName, string rankInputListPropName) {
  list<pair<int, hcmInstPort*>> outputInstPortList;

  // if (instPortPair.first == 0) { // debug
  //   cout << "InstPort Name: " << instPortPair.second->getName() << " Rank: " << instPortPair.first << endl;
  // }

  hcmInstance* currInst = instPortPair.second->getInst();
  hcmCell* currCell = currInst->masterCell();

  list<int> rankInputList;
  
  assert(currInst->getProp<list<int>>(rankInputListPropName, rankInputList) == OK);
  rankInputList.push_back(instPortPair.first);

  assert(currInst->setProp<list<int>>(rankInputListPropName, rankInputList) == OK);
  
  size_t inputSize = 0;

  for (auto port : currCell->getPorts()) {
    // if (globalNodes->find(port->owner()->getName()) != globalNodes->end()) {
    //   continue;
    // }
    inputSize += port->getDirection() == IN; // IN_OUT?
  }

  if (rankInputList.size() == inputSize) {
    int maxRank = 0, currRank = 0;
    for (auto it : rankInputList) {
      currRank = it;
      maxRank = max(maxRank, currRank);
    }
    assert(currInst->setProp<int>(rankPropName, maxRank) == OK);

    maxRankVector.push_back(pair<int, string>(maxRank, currInst->getName()));

    std::map<std::string, hcmInstPort*>::const_iterator ipI1, ipI2;

    for (ipI1 = currInst->getInstPorts().begin(); ipI1 != currInst->getInstPorts().end(); ipI1++) {
      if (ipI1->second->getPort()->getDirection() == OUT) {
        for (ipI2 = ipI1->second->getNode()->getInstPorts().begin(); ipI2 != ipI1->second->getNode()->getInstPorts().end(); ipI2++) {
          if (ipI2->second->getPort()->getDirection() == IN) {
            outputInstPortList.push_back(pair<int, hcmInstPort*>(maxRank+1, ipI2->second));
          }
        }
      }
    }
  }

  return outputInstPortList;
}

vector<pair<int, string>> getMaxRankVectorByProp(hcmCell* cell, list<pair<int, hcmInstPort*>>* instPortList, set<string>* globalNodes, string rankPropName, string rankInputListPropName) {
  vector<pair<int, string>> maxRankVector;
  list<pair<int, hcmInstPort*>> addedinstPortList;

  while(!(instPortList->empty())) {
    pair<int, hcmInstPort*> currNodePair = instPortList->front();
    instPortList->pop_front();

    addedinstPortList = flatTraverseNode(maxRankVector, currNodePair, globalNodes, rankPropName, rankInputListPropName);

    instPortList->splice(instPortList->end(), addedinstPortList);
  }

  sort(maxRankVector.begin(), maxRankVector.end());

  return maxRankVector;
}

vector<pair<int, string>> getMaxRankVector(hcmCell* cell, set<string>* globalNodes) {
  string rankPropName = "rank";
  string rankInputListPropName = "rankInputList";
  list<pair<int, hcmInstPort*>> instPortList;
  setPropTree<int>(cell, rankPropName, getRankInitPropVal);
  setPropTree<list<int>>(cell, rankInputListPropName, getRankInputListInitPropVal);

  std::map<std::string, hcmInstPort*>::const_iterator ipI;
  std::map<std::string, hcmNode*>::const_iterator nI;

  for (auto pI : cell->getPorts()) {
    if (pI->getDirection() == IN) {
      for (ipI = pI->owner()->getInstPorts().begin(); ipI != pI->owner()->getInstPorts().end(); ipI++) {
        if (globalNodes->find(pI->owner()->getName()) != globalNodes->end()) {
          continue;
        }
        instPortList.push_back(pair<int, hcmInstPort*>(0, ipI->second));
      }
    }
  }

  // Global nodes handler
  for (nI = cell->getNodes().begin(); nI != cell->getNodes().end(); nI++) {
    if (globalNodes->find(nI->second->getName()) != globalNodes->end()) {
      for (ipI = nI->second->getInstPorts().begin(); ipI != nI->second->getInstPorts().end(); ipI++) {
        instPortList.push_back(pair<int, hcmInstPort*>(0, ipI->second));
      }
      
    }
  }

  vector<pair<int, string>> maxRankVector = getMaxRankVectorByProp(cell, &instPortList, globalNodes, rankPropName, rankInputListPropName);

  delPropTree<int>(cell, rankPropName);
  delPropTree<list<int>>(cell, rankInputListPropName);

  return maxRankVector;
}

int main(int argc, char **argv) {
  int argIdx = 1;
  int anyErr = 0;
  unsigned int i;
  vector<string> vlgFiles;
  
  if (argc < 3) {
    anyErr++;
  } else {
    if (!strcmp(argv[argIdx], "-v")) {
      argIdx++;
      verbose = true;
    }
    for (;argIdx < argc; argIdx++) {
      vlgFiles.push_back(argv[argIdx]);
    }
    
    if (vlgFiles.size() < 2) {
      cerr << "-E- At least top-level and single verilog file required for spec model" << endl;
      anyErr++;
    }
  }

  if (anyErr) {
    cerr << "Usage: " << argv[0] << "  [-v] top-cell file1.v [file2.v] ... \n";
    exit(1);
  }
 
  set< string> globalNodes;
  globalNodes.insert("VDD");
  globalNodes.insert("VSS");
  
  hcmDesign* design = new hcmDesign("design");
  string cellName = vlgFiles[0];
  for (i = 1; i < vlgFiles.size(); i++) {
    printf("-I- Parsing verilog %s ...\n", vlgFiles[i].c_str());
    if (!design->parseStructuralVerilog(vlgFiles[i].c_str())) {
      cerr << "-E- Could not parse: " << vlgFiles[i] << " aborting." << endl;
      exit(1);
    }
  }
  

  /*direct output to file*/
  string fileName = cellName + string(".rank");
  ofstream fv(fileName.c_str());
  if (!fv.good()) {
    cerr << "-E- Could not open file:" << fileName << endl;
    exit(1);
  }

  hcmCell *topCell = design->getCell(cellName);
  if (!topCell) {
    printf("-E- could not find cell %s\n", cellName.c_str());
    exit(1);
  }
  
  fv << "file name: " << fileName << endl;
  
  hcmCell *flatCell = hcmFlatten(cellName + string("_flat"), topCell, globalNodes);

  /* assign your answer for HW1ex2 to maxRankVector 
   * maxRankVector is a vector of pairs of type <int, string>,
   * int is the rank of the instance,
   * string is the name of the instance
   * Note - maxRankVector should be sorted.
  */
  vector<pair<int, string>> maxRankVector;
  //---------------------------------------------------------------------------------//
  maxRankVector = getMaxRankVector(flatCell, &globalNodes);
  //---------------------------------------------------------------------------------//
  for(auto itr = maxRankVector.begin(); itr != maxRankVector.end(); itr++){
		fv << itr->first << " " << itr->second << endl;
	}

  return(0);
}

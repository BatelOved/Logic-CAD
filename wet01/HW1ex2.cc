#include <errno.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include "hcm.h"
#include "flat.h"
#include <list>

using namespace std;

bool verbose = false;

// ./gl_rank TopLevel1355 stdcell.v c1355high.v
// ./gl_rank TopLevel2670 stdcell.v c2670high.v

string getHierNamePropVal(hcmInstance* cell) {
  // TODO
  return "";
}

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

void traverseNode(list<pair<int, hcmNode*>>* nodesList, pair<int, hcmNode*> nodePair, string rankPropName, string rankInputListPropName, string hierPropName) {
  list<pair<int, hcmNode*>> outputNodePairs;
  std::map<std::string, hcmInstPort*>::const_iterator ipI;

  list<pair<int, hcmNode*>>& nodesListtmp = *nodesList; // tmp

  for (ipI = nodePair.second->getInstPorts().begin(); ipI != nodePair.second->getInstPorts().end(); ipI++) {
    hcmInstance* currInst = ipI->second->getInst();
    hcmCell* currCell = currInst->masterCell();
    hcmNode* currNode = currCell->getNode(ipI->second->getPort()->getName());

    // if (globalNodes->find(currNode->getName()) != globalNodes->end()) {
    //   continue;
    // }

    if (currCell->getInstances().size() == 0) {
      list<int> rankInputList;
      
      assert(currInst->getProp<list<int>>(rankInputListPropName, rankInputList) == OK);
      rankInputList.push_back(nodePair.first); // To Check

      assert(currInst->setProp<list<int>>(rankInputListPropName, rankInputList) == OK);
      

      vector<hcmPort*> cellPorts = currCell->getPorts();

      size_t inputSize = 0;

      for (auto port : cellPorts) { // Change to prop
        inputSize += port->getDirection() == IN; // IN_OUT?
      }

      if (rankInputList.size() == inputSize) {
        int maxRank = 0, currRank = 0;
        for (auto it : rankInputList) {
          currRank = it;
          maxRank = max(maxRank, currRank);
        }
        assert(currInst->setProp<int>(rankPropName, maxRank) == OK);
        cout << "Cell Name: " << currInst->getName() << " Rank: " << maxRank << endl;

        for (auto port : cellPorts) { // Change to prop
          if (port->getDirection() == OUT) {
            outputNodePairs.push_front(pair<int, hcmNode*>(maxRank+1, port->owner()));
          }
        }

        for (auto it : outputNodePairs) {
          nodesList->push_back(it);
        }
      }

      continue;
    }

    traverseNode(nodesList, pair<int, hcmNode*>(nodePair.first, currNode), rankPropName, rankInputListPropName, hierPropName);
  }
}

vector<pair<int, string>> getMaxRankVectorByProp(hcmCell* cell, list<pair<int, hcmNode*>>* nodesList, string rankPropName, string rankInputListPropName, string hierPropName) {
  vector<pair<int, string>> maxRankVector;

  list<pair<int, hcmNode*>>& nodesListtmp = *nodesList; // tmp
  
  while(!(nodesList->empty())) {
    pair<int, hcmNode*> currNodePair = nodesList->front();
    nodesList->pop_front();

    traverseNode(nodesList, currNodePair, rankPropName, rankInputListPropName, hierPropName);
  }

  return maxRankVector; // TODO
}

vector<pair<int, string>> getMaxRankVector(hcmCell* cell) {
  string rankPropName = "rank";
  string rankInputListPropName = "rankInputList";
  string hierPropName = "hierName";
  list<pair<int, hcmNode*>> nodesList;
  setPropTree<int>(cell, rankPropName, getRankInitPropVal);
  setPropTree<list<int>>(cell, rankInputListPropName, getRankInputListInitPropVal);
  setPropTree<string>(cell, hierPropName, getHierNamePropVal);

  for (auto pI : cell->getPorts()) {
    if (pI->getDirection() == IN) {
      nodesList.push_back(pair<int, hcmNode*>(0, pI->owner()));
    }
  }

  vector<pair<int, string>> maxRankVector = getMaxRankVectorByProp(cell, &nodesList, rankPropName, rankInputListPropName, hierPropName);

  delPropTree<int>(cell, rankPropName);
  delPropTree<list<int>>(cell, rankInputListPropName);
  delPropTree<string>(cell, hierPropName);

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
  

  /* assign your answer for HW1ex2 to maxRankVector 
   * maxRankVector is a vector of pairs of type <int, string>,
   * int is the rank of the instance,
   * string is the name of the instance
   * Note - maxRankVector should be sorted.
  */
  vector<pair<int, string>> maxRankVector;
  //---------------------------------------------------------------------------------//
  maxRankVector = getMaxRankVector(topCell);
  //---------------------------------------------------------------------------------//
  for(auto itr = maxRankVector.begin(); itr != maxRankVector.end(); itr++){
		fv << itr->first << " " << itr->second << endl;
	}

  return(0);
}

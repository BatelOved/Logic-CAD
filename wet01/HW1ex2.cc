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

size_t _getRankInputSizePropVal(hcmInstance* inst) {
  size_t inputSize = 0;

  for (auto port : inst->masterCell()->getPorts()) {
    inputSize += port->getDirection() == IN;
  }

  return inputSize;
}

list<int> _getRankInputListInitPropVal(hcmInstance* inst) {
  return list<int>();
}

/**
 * Create and add hcmTypedProperty represented by PropName to all the instances in the hierarchy tree.
 * @param getPropVal Pointer to a function that operates on an instance, and determines its property value.
**/
template <typename T>
void _setInstPropTree(hcmCell* cell, string PropName, T (*getPropVal)(hcmInstance*)) {

  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    assert(iI->second->setProp<T>(PropName, getPropVal(iI->second)) == OK);
    hcmCell* currCell = iI->second->masterCell();
    _setInstPropTree<T>(currCell, PropName, getPropVal);
  }
}

/**
 * Remove the hcmTypedProperty represented by PropName for all the instances in the hierarchy tree.
**/
template <typename T>
void _delInstPropTree(hcmCell* cell, string PropName) {

  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    assert(iI->second->delProp<T>(PropName) == OK);
    hcmCell* currCell = iI->second->masterCell();
    _delInstPropTree<T>(currCell, PropName);
  }
}

/**
 * Decrease by 1 instance property value - inputSize, for the connected instance to the provided instPort (which the global node were connected to it).
 * Instances which are only connected to global nodes will hop recursively to next instance ports, otherwise return.
**/
void _globalNodesHandlerByProp(hcmInstPort* instPort, string inputSizePropName) {
  size_t inputSize = 0;
  hcmInstance* currInst = instPort->getInst();
  std::map<std::string, hcmInstPort*>::const_iterator ipI1, ipI2;

  assert(currInst->getProp<size_t>(inputSizePropName, inputSize) == OK);
  assert(currInst->setProp<size_t>(inputSizePropName, inputSize-1) == OK);
  if (inputSize != 1) return;

  // Instances which are only connected to global nodes - hop recursively to next instance ports
  for (ipI1 = currInst->getInstPorts().begin(); ipI1 != currInst->getInstPorts().end(); ipI1++) {
    if (ipI1->second->getPort()->getDirection() == OUT) {
      for (ipI2 = ipI1->second->getNode()->getInstPorts().begin(); ipI2 != ipI1->second->getNode()->getInstPorts().end(); ipI2++) {
        if (ipI2->second->getPort()->getDirection() == IN) {
          _globalNodesHandlerByProp(ipI2->second, inputSizePropName);
        }
      }
    }
  }
}

/**
 * List all the global nodes in the top hierarchy and activate a handler for each global node.
**/
void _globalNodesHandler(hcmCell* cell, set<string>* globalNodes, string inputSizePropName) {
  list<hcmInstPort*> globalInstPort;
  std::map<std::string, hcmInstPort*>::const_iterator ipI;
  std::map<std::string, hcmNode*>::const_iterator nI;

  // Set a list with all the nodes in the provided cell, which are global nodes
  for (nI = cell->getNodes().begin(); nI != cell->getNodes().end(); nI++) {
    if (globalNodes->find(nI->second->getName()) != globalNodes->end()) {
      for (ipI = nI->second->getInstPorts().begin(); ipI != nI->second->getInstPorts().end(); ipI++) {
        globalInstPort.push_back(ipI->second);
      }
    }
  }

  for (auto ip : globalInstPort) {
    _globalNodesHandlerByProp(ip, inputSizePropName);
  }
}

/**
 * Update instance property - rankInputList, of the input instPort's instance.
 * For resolved instances (which all their inputs has ranks) rank the instance and hop the instance to the next instance ports with direction IN.
 * Return a list of type pair<int, hcmInstPort*> of all the instance ports added.
 * In case of not all input has ranks, return empty list.
**/
list<pair<int, hcmInstPort*>> _rankAndGetHopInstPorts(vector<pair<int, string>>& maxRankVector, pair<int, hcmInstPort*> instPortPair, string inputSizePropName, string rankInputListPropName) {
  list<pair<int, hcmInstPort*>> outputInstPortList;

  hcmInstance* currInst = instPortPair.second->getInst();

  list<int> rankInputList;
  
  assert(currInst->getProp<list<int>>(rankInputListPropName, rankInputList) == OK);
  rankInputList.push_back(instPortPair.first);
  assert(currInst->setProp<list<int>>(rankInputListPropName, rankInputList) == OK);
  
  size_t inputSize = 0;

  assert(currInst->getProp<size_t>(inputSizePropName, inputSize) == OK);

  if (rankInputList.size() == inputSize) { // Resolved instance
    int maxRank = 0, currRank = 0;
    for (auto inputRank : rankInputList) {
      currRank = inputRank;
      maxRank = max(maxRank, currRank);
    }

    maxRankVector.push_back(pair<int, string>(maxRank, currInst->getName())); // Rank the instance

    std::map<std::string, hcmInstPort*>::const_iterator ipI1, ipI2;

    // Set hop instance ports list
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

/**
 * Performs a maximal ranking algorithm to provide each occurrence instance its maximal distance from any of the input ports of the blocks.
 * Returns a vector of pairs of type <int, string>, int is the rank of the instance, string is the name of the instance.
**/
vector<pair<int, string>> _getMaxRankVectorByProp(hcmCell* cell, list<pair<int, hcmInstPort*>>* instPortList, set<string>* globalNodes, string inputSizePropName, string rankInputListPropName) {
  vector<pair<int, string>> maxRankVector;
  list<pair<int, hcmInstPort*>> addedinstPortList;

  while(!(instPortList->empty())) {
    pair<int, hcmInstPort*> currinstPortPair = instPortList->front();
    instPortList->pop_front();

    addedinstPortList = _rankAndGetHopInstPorts(maxRankVector, currinstPortPair, inputSizePropName, rankInputListPropName);

    instPortList->splice(instPortList->end(), addedinstPortList);
  }

  sort(maxRankVector.begin(), maxRankVector.end());

  return maxRankVector;
}

/**
 * Wrapper function to _getMaxRankVectorByProp, sets properties to instances, handles global nodes and inits instPortList to iterate on it.
 * Returns a vector of pairs of type <int, string>, int is the rank of the instance, string is the name of the instance.
**/
vector<pair<int, string>> getMaxRankVector(hcmCell* cell, set<string>* globalNodes) {
  string rankInputListPropName = "rankInputList";
  string inputSizePropName = "inputSize";
                                                                                           // For all the instances in the cell hierarchy:
  _setInstPropTree<list<int>>(cell, rankInputListPropName, _getRankInputListInitPropVal);  // <-> Create a property of type list<int> for the inputs ranks of the instance.
  _setInstPropTree<size_t>(cell, inputSizePropName, _getRankInputSizePropVal);             // <-> Create a property of type size_t for the input size of the instance.

  list<pair<int, hcmInstPort*>> instPortList;
  std::map<std::string, hcmInstPort*>::const_iterator ipI;

  for (auto pI : cell->getPorts()) {                                                                     // <-> For each port,
    if (pI->getDirection() == IN) {                                                                      // <-> Which is an input,
      for (ipI = pI->owner()->getInstPorts().begin(); ipI != pI->owner()->getInstPorts().end(); ipI++) { // <-> For each instance port (connected to the owner node of that port),
        instPortList.push_back(pair<int, hcmInstPort*>(0, ipI->second));                                 // <-> Insert to the instPortList a new pair of that instance port with rank 0.
      }
    }
  }

  _globalNodesHandler(cell, globalNodes, inputSizePropName);

  vector<pair<int, string>> maxRankVector = _getMaxRankVectorByProp(cell, &instPortList, globalNodes, inputSizePropName, rankInputListPropName);

  _delInstPropTree<list<int>>(cell, rankInputListPropName);
  _delInstPropTree<size_t>(cell, inputSizePropName);

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

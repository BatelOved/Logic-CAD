#include <errno.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include "hcm.h"
#include "flat.h"
#include <list>
using namespace std;

bool verbose = false;

int getDeepestHierPropVal(hcmCell* cell) {
  if (cell->getInstances().size() == 0) {
    return 1;
  }

  int value = 0, tmpValue = 0;
  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    hcmCell* currCell = iI->second->masterCell();
    tmpValue = getDeepestHierPropVal(currCell);
    value = max(value, tmpValue);
  }

  return value+1;
}

int deepestReach(hcmCell* cell, hcmNode* node, set<string>* globalNodes) {
  int value = 0, tmpValue = 0;
  std::map<std::string, hcmInstPort*>::const_iterator ipI;

  for (ipI = node->getInstPorts().begin(); ipI != node->getInstPorts().end(); ipI++) {
    hcmCell* currCell = ipI->second->getInst()->masterCell();
    hcmNode* currNode = currCell->getNode(ipI->second->getPort()->getName());

    if (globalNodes->find(currNode->getName()) != globalNodes->end()) {
      continue;
    }

    tmpValue = deepestReach(currCell, currNode, globalNodes);
    value = max(value, tmpValue);
  }

  return value+1;
}

bool getVisitInitPropVal(hcmCell* cell) {
  return false;
}

template <typename T>
void setPropTree(hcmCell* cell, string PropName, T (*getPropVal)(hcmCell*)) {

  assert(cell->setProp<T>(PropName, getPropVal(cell)) == OK);

  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    hcmCell* currCell = iI->second->masterCell();
    setPropTree<T>(currCell, PropName, getPropVal);
  }
}

template <typename T>
void delPropTree(hcmCell* cell, string PropName) {

  assert(cell->delProp<T>(PropName) == OK);

  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    hcmCell* currCell = iI->second->masterCell();
    delPropTree<T>(currCell, PropName);
  }
}

int cntInstByProp(hcmCell* cell, string cellName, string propName) {

  bool value;
  assert(cell->getProp<bool>(propName, value) == OK);

  if (value == true) {
    return 0;
  }

  if (cell->getInstances().size() == 0) {
    return cellName.compare(cell->getName()) == 0 ? 1 : 0;
  }
  
  int cnt = 0;
  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    hcmCell* currCell = iI->second->masterCell();
    cnt += cntInstByProp(currCell, cellName, propName);
    cell->setProp<bool>(propName, true);
  }

  return cnt;
}

int getCntInst(hcmCell* cell, string cellName) {
  string propName = "visit";

  setPropTree<bool>(cell, propName, getVisitInitPropVal);
  int cnt = cntInstByProp(cell, cellName, propName);
  delPropTree<bool>(cell, propName);

  return cnt;
}

int getDeepestReach(hcmCell* cell, set<string>* globalNodes) {
  string propName = "deepestReach";
  std::map<std::string, hcmNode*>::const_iterator nI;
  int deepest = 0;

  for (nI = cell->getNodes().begin(); nI != cell->getNodes().end(); nI++) {
    int deepestTmp = deepestReach(cell, nI->second, globalNodes);
    deepest = max(deepest, deepestTmp);
  }

  return deepest;
}

void deepestReachNodesByProp(hcmCell* cell, string propName, list<string>* nodesList, string deepestNode, set<string>* globalNodes) {
  std::map<std::string, hcmInstance*>::const_iterator iI;
  std::map<std::string, hcmNode*>::const_iterator nI;

  if (cell->getInstances().size() == 0) {
    for (nI = cell->getNodes().begin(); nI != cell->getNodes().end(); nI++) {
      if (globalNodes->find(nI->second->getName()) == globalNodes->end()) {
        nodesList->push_back(deepestNode + "/" + nI->second->getName());
      }
    }
    return;
  }

  int deepestVal = 0, currDeepestVal = 0;
  assert(cell->getProp<int>(propName, deepestVal) == OK);

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    hcmCell* currCell = iI->second->masterCell();

    assert(currCell->getProp<int>(propName, currDeepestVal) == OK);

    if (deepestVal != currDeepestVal+1) {
      continue;
    }

    deepestReachNodesByProp(currCell, propName, nodesList, deepestNode + "/" + iI->second->getName(), globalNodes);
  }
}

list<string> getDeepestHierNodesList(hcmCell* cell, set<string>* globalNodes) {
  string propName = "deepestHier";
  list<string> nodesList;

  setPropTree<int>(cell, propName, getDeepestHierPropVal);
  deepestReachNodesByProp(cell, propName, &nodesList, "", globalNodes);
  delPropTree<int>(cell, propName);

  nodesList.sort();
  return nodesList;
}

int main(int argc, char **argv) {
  int argIdx = 1;
  int anyErr = 0;
  unsigned int i;
  vector<string> vlgFiles;
  
  if (argc < 3) {
    anyErr++;
  } 

  else {
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
  
  /*direct to file*/
  string fileName = cellName + string(".stat");
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
  
  hcmCell *flatCell = hcmFlatten(cellName + string("_flat"), topCell, globalNodes);
  cout << "-I- Top cell flattened" << endl;
  
  fv << "file name: " << fileName << endl;
  
	// section a: find all nodes in top module. excluding global nodes (VSS and VDD).
  /* assign your answer for section a to topLevelNodeCounter */
  int topLevelNodeCounter = 0;
  //---------------------------------------------------------------------------------//
  topLevelNodeCounter = topCell->getNodes().size() - globalNodes.size();
  //---------------------------------------------------------------------------------//
	fv << "a: " << topLevelNodeCounter << endl;

	// section b: find number of instances in top level cell.
  /* assign your answer for section b to topLevelInstanceCounter */
  int topLevelInstanceCounter = 0;
  //---------------------------------------------------------------------------------//
  topLevelInstanceCounter = topCell->getInstances().size();
  //---------------------------------------------------------------------------------//
	fv << "b: " << topLevelInstanceCounter << endl;

	//section c: find cellName instances in folded model.
  /* assign your answer for section c to cellNameFoldedCounter */
  int cellNameFoldedCounter = 0;
  //---------------------------------------------------------------------------------//
  cellNameFoldedCounter = getCntInst(topCell, "and4");
  //---------------------------------------------------------------------------------//
	fv << "c: " << cellNameFoldedCounter << endl;

	//section d: find cellName instances in entire hierarchy, using flat cell.
  /* assign your answer for section d to cellNameFlatendCounter */
  int cellNameFlatendCounter = 0;
  //---------------------------------------------------------------------------------//
  cellNameFlatendCounter = getCntInst(flatCell, "and4");
  //---------------------------------------------------------------------------------//
	fv << "d: " << cellNameFlatendCounter << endl;

	//section e: find the deepest reach of a top level node.
  /* assign your answer for section e to deepestReach */
  int deepestReach = 1;
  //---------------------------------------------------------------------------------//
  deepestReach = getDeepestReach(topCell, &globalNodes);
  //---------------------------------------------------------------------------------//
	fv << "e: " << deepestReach << endl;

	//section f: find hierarchical names of deepest reaching nodes.
  /* assign your answer for section f to listOfHierarchicalNameOfDeepestReachingNodes */
  list<string> listOfHierarchicalNameOfDeepestReachingNodes;
  //---------------------------------------------------------------------------------//
  listOfHierarchicalNameOfDeepestReachingNodes = getDeepestHierNodesList(topCell, &globalNodes);
  //---------------------------------------------------------------------------------//
  for (auto it : listOfHierarchicalNameOfDeepestReachingNodes) {
    // erase the '/' in the beginning of the hierarchical name. 
    // i.e. the name in listOfHierarchicalNameOfDeepestReachingNodes should be "/i1/i2/i3/i5/N1", 
    // and the printed name should be "i1/i2/i3/i5/N1".
    it.erase(0,1); 
    fv << it << endl;
  }

  return(0);
}

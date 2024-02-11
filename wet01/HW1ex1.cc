#include <errno.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include "hcm.h"
#include "flat.h"
#include <list>
using namespace std;

bool verbose = false;

/**
 * Create and add hcmTypedProperty represented by PropName to all the cells in the hierarchy tree.
**/
template <typename T>
void _setCellPropTree(hcmCell* cell, string PropName, T initPropVal) {

  assert(cell->setProp<T>(PropName, initPropVal) == OK);

  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    hcmCell* innerCell = iI->second->masterCell();
    _setCellPropTree<T>(innerCell, PropName, initPropVal);
  }
}

/**
 * Remove the hcmTypedProperty represented by PropName for all the cells in the hierarchy tree.
**/
template <typename T>
void _delCellPropTree(hcmCell* cell, string PropName) {

  assert(cell->delProp<T>(PropName) == OK);

  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    hcmCell* innerCell = iI->second->masterCell();
    _delCellPropTree<T>(innerCell, PropName);
  }
}

/**
 * Returns cell instance count by cell name using the visit property, to avoid traversing visited cells.
**/
int _getInstCntByProp(hcmCell* cell, string cellName, string visitPropName) {

  bool visited;
  assert(cell->getProp<bool>(visitPropName, visited) == OK);

  if (visited == true) {
    return 0;
  }

  if (cell->getInstances().size() == 0) {
    return cellName.compare(cell->getName()) == 0 ? 1 : 0;
  }
  
  int instCnt = 0;
  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    hcmCell* innerCell = iI->second->masterCell();
    instCnt += _getInstCntByProp(innerCell, cellName, visitPropName);
    assert(cell->setProp<bool>(visitPropName, true) == OK);
  }

  return instCnt;
}

/**
 * Returns cell instance count by cell name.
**/
int getInstCnt(hcmCell* cell, string cellName) {
  string propName = "visit";

  _setCellPropTree<bool>(cell, propName, false);
  int cnt = _getInstCntByProp(cell, cellName, propName);
  _delCellPropTree<bool>(cell, propName);

  return cnt;
}

/**
 * Returns the deepest reach value of the provided node.
**/
int _getNodeDeepestReach(hcmNode* node, set<string>* globalNodes) {
  int deepestReach = 0, currDeepestReach = 0;
  std::map<std::string, hcmInstPort*>::const_iterator ipI;

  for (ipI = node->getInstPorts().begin(); ipI != node->getInstPorts().end(); ipI++) {
    hcmCell* innerCell = ipI->second->getInst()->masterCell();
    hcmNode* currNode = innerCell->getNode(ipI->second->getPort()->getName());

    if (globalNodes->find(currNode->getName()) != globalNodes->end()) {
      continue;
    }

    currDeepestReach = _getNodeDeepestReach(currNode, globalNodes);
    deepestReach = max(deepestReach, currDeepestReach);
  }

  return deepestReach+1;
}

/**
 * Returns the deepest reach value among the nodes in the top cell hierarchy.
**/
int getDeepestReach(hcmCell* cell, set<string>* globalNodes) {
  std::map<std::string, hcmNode*>::const_iterator nI;
  int deepestReach = 0, nodeDeepestReach = 0;

  for (nI = cell->getNodes().begin(); nI != cell->getNodes().end(); nI++) {
    nodeDeepestReach = _getNodeDeepestReach(nI->second, globalNodes);
    deepestReach = max(deepestReach, nodeDeepestReach);
  }

  return deepestReach;
}

/**
 * Sets the deepest hierarchy (instance depth) property value for the root cell and its inner cells recursively.
**/
void _setDeepestHierPropVal(hcmCell* cell, string PropName) {
  if (cell->getInstances().size() == 0) {
    assert(cell->setProp<int>(PropName, 1) == OK);
    return;
  }

  int deepestHier = 0, currDeepestHier = 0;
  std::map<std::string, hcmInstance*>::const_iterator iI;

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {
    hcmCell* innerCell = iI->second->masterCell();
    _setDeepestHierPropVal(innerCell, PropName);

    assert(innerCell->getProp<int>(PropName, currDeepestHier) == OK);
    deepestHier = max(deepestHier, currDeepestHier);
  }

  assert(cell->setProp<int>(PropName, deepestHier+1) == OK);
}

/**
 * Sets a list of hierarchical names of deepest reaching nodes, by using deepest reach property value added to the cells to iterate the deepest paths.
**/
void _getDeepestHierNodesListByProp(hcmCell* cell, string propName, list<string>* nodesList, string deepestNode, set<string>* globalNodes) {
  std::map<std::string, hcmInstance*>::const_iterator iI;
  std::map<std::string, hcmNode*>::const_iterator nI;

  if (cell->getInstances().size() == 0) {                                       // <-> If there aren't inner instances (we reached a perimitive cell),
    for (nI = cell->getNodes().begin(); nI != cell->getNodes().end(); nI++) {   // <-> For each of the perimitive cell nodes,
      if (globalNodes->find(nI->second->getName()) == globalNodes->end()) {     // <-> If it's not a global node,
        nodesList->push_back(deepestNode + "/" + nI->second->getName());        // <-> Add the node's hierarchical name to the nodes list output.
      }
    }
    return;
  }

  int deepestVal = 0, innerDeepestVal = 0;
  assert(cell->getProp<int>(propName, deepestVal) == OK);                             // <-> Get the deepest reach value of the current cell,

  for (iI = cell->getInstances().begin(); iI != cell->getInstances().end(); iI++) {   // <-> For each inner instance,
    hcmCell* innerCell = iI->second->masterCell();

    assert(innerCell->getProp<int>(propName, innerDeepestVal) == OK);                 // <-> Get the deepest reach value of that inner cell,

    if (deepestVal != innerDeepestVal+1) {                                            // <-> If the inner cell deepest reach value is not smaller by 1 than the top cell's value,
      continue;                                                                       //     then it's not a deepest reach path and we can skip to other paths search.
    }
                                                                                      // <-> Otherwise, it is a deepest reach path, and we recursively iterate on it.
    _getDeepestHierNodesListByProp(innerCell, propName, nodesList, deepestNode + "/" + iI->second->getName(), globalNodes);
  }
}

/**
 * Returns list of hierarchical names of deepest reaching nodes.
**/
list<string> getDeepestHierNodesList(hcmCell* cell, set<string>* globalNodes) {
  string propName = "deepestHier";
  list<string> nodesList;

  _setCellPropTree<int>(cell, propName, 0);
  _setDeepestHierPropVal(cell, propName);
  _getDeepestHierNodesListByProp(cell, propName, &nodesList, "", globalNodes);
  _delCellPropTree<int>(cell, propName);

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
  cellNameFoldedCounter = getInstCnt(topCell, "and4");
  //---------------------------------------------------------------------------------//
	fv << "c: " << cellNameFoldedCounter << endl;

	//section d: find cellName instances in entire hierarchy, using flat cell.
  /* assign your answer for section d to cellNameFlatendCounter */
  int cellNameFlatendCounter = 0;
  //---------------------------------------------------------------------------------//
  cellNameFlatendCounter = getInstCnt(flatCell, "and4");
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

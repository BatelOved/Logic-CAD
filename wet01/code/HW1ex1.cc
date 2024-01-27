#include <errno.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include "hcm.h"
#include "flat.h"
#include <list>
using namespace std;

bool verbose = false;


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
  //enter your code here 
  //---------------------------------------------------------------------------------//
	fv << "a: " << topLevelNodeCounter << endl;

	// section b: find number of instances in top level cell.
  /* assign your answer for section b to topLevelInstanceCounter */
  int topLevelInstanceCounter = 0;
  //---------------------------------------------------------------------------------//
  //enter your code here 
  //---------------------------------------------------------------------------------//
	fv << "b: " << topLevelInstanceCounter << endl;

	//section c: find cellName instances in folded model.
  /* assign your answer for section c to cellNameFoldedCounter */
  int cellNameFoldedCounter = 0;
  //---------------------------------------------------------------------------------//
  //enter your code here 
  //---------------------------------------------------------------------------------//
	fv << "c: " << cellNameFoldedCounter << endl;

	//section d: find cellName instances in entire hierarchy, using flat cell.
  /* assign your answer for section d to cellNameFlatendCounter */
  int cellNameFlatendCounter = 0;
  //---------------------------------------------------------------------------------//
  //enter your code here 
  //---------------------------------------------------------------------------------//
	fv << "d: " << cellNameFlatendCounter << endl;

	//section e: find the deepest reach of a top level node.
  /* assign your answer for section e to deepestReach */
  int deepestReach = 1;
  //---------------------------------------------------------------------------------//
  //enter your code here 
  //---------------------------------------------------------------------------------//
	fv << "e: " << deepestReach << endl;

	//section f: find hierarchical names of deepest reaching nodes.
  /* assign your answer for section f to listOfHierarchicalNameOfDeepestReachingNodes */
  list<string> listOfHierarchicalNameOfDeepestReachingNodes;
  //---------------------------------------------------------------------------------//
  //enter your code here 
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

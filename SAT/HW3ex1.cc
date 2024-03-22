#include <errno.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include "hcm.h"
#include "flat.h"
#include "utils/System.h"
#include "utils/ParseUtils.h"
#include "utils/Options.h"
#include "core/Dimacs.h"
#include "core/Solver.h"

using namespace std;
using namespace Minisat;

typedef map<string, hcmNode*> PIO; // Primary Inputs\Outputs
typedef map<string, pair<hcmNode*,hcmNode*>> mapPairPIO;

void setPIO(hcmCell* cell, set<string>& globalNodes, PIO& pi, PIO& po) {
    for (auto it : cell->getNodes()) {
        hcmNode* node = it.second;
        hcmPort* port = node->getPort();
        hcmPortDir port_dir = (port != nullptr) ? port->getDirection() : NOT_PORT;

        if (globalNodes.find(node->getName()) != globalNodes.end()) continue;

        if(port_dir == IN) {
            pi.insert(make_pair(node->getName(), node));
        }
        else if (port_dir == OUT) {
            po.insert(make_pair(node->getName(), node));
        }
    }
}

void setPairPIO(PIO& PIO1, PIO& PIO2, mapPairPIO& map_pair_pio) {
    assert(PIO1.size() == PIO2.size());

    PIO::const_iterator it1, it2;

    for(it1 = PIO1.begin(), it2 = PIO2.begin(); (it1 != PIO1.end()) && (it2 != PIO2.end()); it1++, it2++) {
        map_pair_pio.insert(make_pair(it1->first, make_pair(it1->second, it2->second)));
    }
}

//globals:
bool verbose = false;

///////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
    int argIdx = 1;
    int anyErr = 0;
    unsigned int i;
    vector<string> specVlgFiles;
    vector<string> implementationVlgFiles;
    string specCellName;
    string implementationCellName;
    Solver solver;

    if (argc < 8) {
        anyErr++;
    }
    else {
        if (!strcmp(argv[argIdx], "-v")) {
            argIdx++;
            verbose = true;
        }
        if (!strcmp(argv[argIdx], "-s")) {
            argIdx++;
            specCellName = argv[argIdx++];
            while (strcmp(argv[argIdx], "-i")) {
                specVlgFiles.push_back(argv[argIdx++]);
            }
        }
        argIdx++;
        implementationCellName = argv[argIdx++];
        for (;argIdx < argc; argIdx++) {
            implementationVlgFiles.push_back(argv[argIdx]);
        }

        if (implementationVlgFiles.size() < 2 || specVlgFiles.size() < 2) {
            cerr << "-E- At least top-level and single verilog file required for spec model" << endl;
            anyErr++;
        }
    }

    if (anyErr) {
        cerr << "Usage: " << argv[0] << "  [-v] -s top-cell spec_file1.v spec_file2.v -i top-cell impl_file1.v impl_file2.v ... \n";
        exit(1);
    }

    string fileName = specCellName + ".cnf";
    set< string> globalNodes;
    globalNodes.insert("VDD");
    globalNodes.insert("VSS");

    // spec hcm
    hcmDesign* specDesign = new hcmDesign("specDesign");
    for (i = 0; i < specVlgFiles.size(); i++) {
        printf("-I- Parsing verilog %s ...\n", specVlgFiles[i].c_str());
        if (!specDesign->parseStructuralVerilog(specVlgFiles[i].c_str())) {
            cerr << "-E- Could not parse: " << specVlgFiles[i] << " aborting." << endl;
            exit(1);
        }
    }

    hcmCell* topSpecCell = specDesign->getCell(specCellName);
    if (!topSpecCell) {
        printf("-E- could not find cell %s\n", specCellName.c_str());
        exit(1);
    }

    hcmCell* flatSpecCell = hcmFlatten(specCellName + string("_flat"), topSpecCell, globalNodes);

    // implementation hcm
    hcmDesign* impDesign = new hcmDesign("impDesign");
    for (i = 0; i < implementationVlgFiles.size(); i++) {
        printf("-I- Parsing verilog %s ...\n", implementationVlgFiles[i].c_str());
        if (!impDesign->parseStructuralVerilog(implementationVlgFiles[i].c_str())) {
            cerr << "-E- Could not parse: " << implementationVlgFiles[i] << " aborting." << endl;
            exit(1);
        }
    }

    hcmCell* topImpCell = impDesign->getCell(implementationCellName);
    if (!topImpCell) {
        printf("-E- could not find cell %s\n", implementationCellName.c_str());
        exit(1);
    }

    hcmCell* flatImpCell = hcmFlatten(implementationCellName + string("_flat"), topImpCell, globalNodes);

    //---------------------------------------------------------------------------------//
    //enter your code below
    solver.verbosity = false;

    // Declare 6 variables
    for (unsigned int i = 0; i < 6; i++) {
        solver.newVar();
    }

    // each clause needs its own literals vector
    vec<Lit> lits;

    // -1 2 -3 0 
    lits.clear();
    lits.push(~mkLit(0));
    lits.push(mkLit(1));
    lits.push(~mkLit(2));
    solver.addClause(lits);


    hcmDesign* topDesign = new hcmDesign("topDesign");
    hcmCell*   topCell   = topDesign->createCell("topCell");

    hcmInstance* spec_inst = topCell->createInst("inst_SpecCell", flatSpecCell);
    hcmInstance* imp_inst  = topCell->createInst("inst_ImpCell", flatImpCell);

    PIO PI_spec, PO_spec;
    PIO PI_imp, PO_imp;

    setPIO(flatSpecCell, globalNodes, PI_spec, PO_spec);
    setPIO(flatImpCell, globalNodes, PI_imp, PO_imp);

    assert(PI_spec.size() == PI_imp.size());
    assert(PO_spec.size() == PO_imp.size());

    
    mapPairPIO map_pair_pi, map_pair_po;
    setPairPIO(PI_spec, PI_imp, map_pair_pi);
    setPairPIO(PO_spec, PO_imp, map_pair_po);

    for (auto it : map_pair_pi) {
        string nodeName = it.first;
        pair<hcmNode*, hcmNode*> pair_nodes = it.second;
        hcmNode* node = topCell->createNode(string("PI_") + nodeName);
        topCell->connect(spec_inst, node, pair_nodes.first->getPort());
        topCell->connect(imp_inst, node, pair_nodes.second->getPort());
    }
    
    hcmCell* fevCell = topDesign->createCell("fev");
    hcmInstance* fev_inst = topCell->createInst("inst_fev", fevCell);

    hcmCell* xorCell = topDesign->createCell("xor2");

    hcmNode* xor_nodeA = xorCell->createNode("A");
    hcmNode* xor_nodeB = xorCell->createNode("B");
    hcmNode* xor_nodeY = xorCell->createNode("Y");

    hcmPort* portA = xor_nodeA->createPort(IN);
    hcmPort* portB = xor_nodeB->createPort(IN);
    hcmPort* portY = xor_nodeY->createPort(OUT);

    for (auto it : map_pair_po) {
        string nodeName = it.first;
        pair<hcmNode*, hcmNode*> pair_nodes = it.second;

        // ===============================================================================
        hcmNode* spec_to_fev_node = topCell->createNode(string("OUT_spec_") + nodeName);
        hcmNode* imp_to_fev_node  = topCell->createNode(string("OUT_imp_") + nodeName);

        topCell->connect(spec_inst, spec_to_fev_node, pair_nodes.first->getPort());
        topCell->connect(imp_inst, imp_to_fev_node, pair_nodes.second->getPort());

        hcmPort* fev_spec_port = spec_to_fev_node->createPort(IN);
        hcmPort* fev_imp_port  = imp_to_fev_node->createPort(IN);

        topCell->connect(fev_inst, spec_to_fev_node, fev_spec_port);
        topCell->connect(fev_inst, imp_to_fev_node, fev_imp_port);
        // ===============================================================================
        hcmNode* fev_spec_xor_node = fevCell->createNode(string("IN_spec_") + nodeName);
        hcmNode* fev_imp_xor_node  = fevCell->createNode(string("IN_imp_") + nodeName);
        
        //fevCell->connect(fev_inst, fev_spec_xor_node, fev_spec_port);
        //fevCell->connect(fev_inst, fev_spec_xor_node, fev_imp_port);
        // ===============================================================================
        hcmInstance* xor_inst = fevCell->createInst(string("xor2_") + it.first, xorCell);

        fevCell->connect(xor_inst, fev_spec_xor_node, portA);
        fevCell->connect(xor_inst, fev_imp_xor_node, portB);
    }





    FILE* res = fopen("spec-cell.cnf", "wb");

    solver.toDimacs(res, lits);

    fclose(res);

    // if (globalNodes.find(node->getName()) != globalNodes.end()) {
    // }

    //---------------------------------------------------------------------------------//

    bool sat = solver.solve();
    if (sat) {
        cout << "SATISFIABLE!" << endl;
    }
    else {
        cout << "NOT SATISFIABLE!" << endl;
    }
    return 0;

}
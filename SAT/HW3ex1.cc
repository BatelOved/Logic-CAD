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

typedef enum {PI, PO} PIO_TYPE;
typedef map<string, hcmNode*> PIO; // Primary Inputs\Outputs
typedef map<string, pair<hcmNode*,hcmNode*>> mapPairPIO;


class FECCircuit {
    hcmCell*     topCell;
    hcmCell*     cell;
    hcmInstance* inst;
    PIO          pio[2];
    set<string>  globalNodes;

public:
    FECCircuit() = delete;

    FECCircuit(hcmCell* topCell, hcmCell* cell, set<string>& globalNodes);

    ~FECCircuit() {}

    void setPIO();

    void setPIO_FF();

    hcmCell* getTopCell() { return topCell; }

    hcmCell* getCell() { return cell; }

    hcmInstance* getInst() { return inst; }

    PIO* getPIO() { return pio; }
};

FECCircuit::FECCircuit(hcmCell* topCell, hcmCell* cell, set<string>& globalNodes) : topCell(topCell), cell(cell), globalNodes(globalNodes) {
    inst = topCell->createInst(string("inst_") + cell->getName(), cell);

    setPIO();
}

void FECCircuit::setPIO() {
    for (auto it : cell->getNodes()) {
        hcmNode* node = it.second;
        hcmPort* port = node->getPort();
        hcmPortDir port_dir = (port != nullptr) ? port->getDirection() : NOT_PORT;

        if (port_dir == IN) {
            pio[PI].insert(make_pair(node->getName(), node));
        }
        else if (port_dir == OUT) {
            pio[PO].insert(make_pair(node->getName(), node));
        }
    }

    setPIO_FF();
}

void FECCircuit::setPIO_FF() {
    for (auto inst_it : cell->getInstances()) {
        hcmCell* currCell = inst_it.second->masterCell();
        if (currCell->getName() == "dff") {
            for (auto node_it : currCell->getNodes()) {
                hcmNode* ff_node = node_it.second;
                hcmPort* ff_port = ff_node->getPort();

                if(ff_node->getName() == "CLK") continue; // TODO Batel - check if it's ok

                if (globalNodes.find(ff_node->getName()) != globalNodes.end()) continue;

                if (ff_port->getDirection() == IN) {
                    pio[PO].insert(make_pair(inst_it.first + string("_") + ff_node->getName() + string("_"), ff_node));
                }
                else if (ff_port->getDirection() == OUT) {
                    pio[PI].insert(make_pair(inst_it.first + string("_") + ff_node->getName() + string("_"), ff_node));
                }
            }
        }
    }
}


class FEV {
    hcmCell*     topCell;
    hcmCell*     cell;
    hcmInstance* inst;
    hcmPort*     topCell_out_port;

public:
    FEV() = delete;

    FEV(hcmDesign* topDesign, hcmCell* topCell, mapPairPIO map_pair_pi);

    ~FEV() {}

    hcmCell* getCell() { return cell; }

    hcmInstance* getInst() { return inst; }

    hcmPort* getTopCellOutPort() { return topCell_out_port; }
};

FEV::FEV(hcmDesign* topDesign, hcmCell* topCell, mapPairPIO map_pair_pi) : topCell(topCell) {
    cell = topDesign->createCell("fevCell");
    inst = topCell->createInst(string("inst_") + cell->getName(), cell);

    hcmCell* xorCell    = topDesign->createCell("xor2");

    hcmNode* xor_nodeA  = xorCell->createNode("A");
    hcmNode* xor_nodeB  = xorCell->createNode("B");
    hcmNode* xor_nodeY  = xorCell->createNode("Y");

    hcmPort* xor_portA  = xor_nodeA->createPort(IN);
    hcmPort* xor_portB  = xor_nodeB->createPort(IN);
    hcmPort* xor_portY  = xor_nodeY->createPort(OUT);

    hcmCell* orCell     = topDesign->createCell("or");

    hcmNode* or_nodeA   = orCell->createNode("A");
    hcmNode* or_nodeB   = orCell->createNode("B");
    hcmNode* or_nodeY   = orCell->createNode("Y");

    hcmPort* or_portA   = or_nodeA->createPort(IN);
    hcmPort* or_portB   = or_nodeB->createPort(IN);
    hcmPort* or_portY   = or_nodeY->createPort(OUT);

    vector<hcmNode*> xor_out_nodes;

    for(auto it : map_pair_pi) {
        string nodeName = it.first;
        pair<hcmNode*, hcmNode*> pair_nodes = it.second;
        hcmInstance* xorInst = cell->createInst(string("FEV_XOR_") + nodeName, xorCell);

        hcmNode* fev_spec_node = cell->createNode(string("FEV_SPEC_PI_") + nodeName);
        hcmNode* fev_imp_node  = cell->createNode(string("FEV_IMP_PI_") + nodeName);
        hcmNode* fev_xor_node  = cell->createNode(string("FEV_XOR_PO_") + nodeName);

        cell->connect(xorInst, fev_spec_node, xor_portA);
        cell->connect(xorInst, fev_imp_node, xor_portB);
        cell->connect(xorInst, fev_xor_node, xor_portY);

        hcmPort* fev_spec_port = fev_spec_node->createPort(IN);
        hcmPort* fev_imp_port  = fev_imp_node->createPort(IN);

        topCell->connect(inst, pair_nodes.first, fev_spec_port);
        topCell->connect(inst, pair_nodes.second, fev_imp_port);

        xor_out_nodes.push_back(fev_xor_node);
    }

    while(xor_out_nodes.size() > 1) {
        size_t out_size = xor_out_nodes.size();

        hcmNode* out_nodeA = xor_out_nodes[out_size-1];
        hcmNode* out_nodeB = xor_out_nodes[out_size-2];

        xor_out_nodes.pop_back();
        xor_out_nodes.pop_back();

        hcmInstance* orInst = cell->createInst(string("FEV_OR_") + out_nodeA->getName() + out_nodeB->getName(), orCell);

        cell->connect(orInst, out_nodeA, or_portA);
        cell->connect(orInst, out_nodeB, or_portB);

        hcmNode* or_out_node = cell->createNode(string("FEV_OR_OUT_") + out_nodeA->getName() + out_nodeB->getName());

        cell->connect(orInst, or_out_node, or_portY);

        xor_out_nodes.insert(xor_out_nodes.begin(), or_out_node);
    }

    hcmNode* fev_out_node = xor_out_nodes[0];
    xor_out_nodes.pop_back();
    hcmPort* fev_out_port = fev_out_node->createPort(OUT);
    hcmNode* topCell_out_node = topCell->createNode("FEV_PO_");
    topCell->connect(inst, topCell_out_node, fev_out_port);
    topCell_out_port = topCell_out_node->createPort(OUT);
}


/**
 * @brief SAT-based Gate Level Formal Equivalence Checker.
 */
class FEC {
    Solver*      solver;
    string       fileName;

    hcmDesign*   topDesign;
    hcmCell*     wrapperCell;
    hcmCell*     topCell;
    hcmInstance* topInst;

    FECCircuit*  specCircuit;
    FECCircuit*  impCircuit;
    FEV*         fev;

    set<string>  globalNodes;
    mapPairPIO   map_pair_pio[2];

    hcmCell*     flatWrapperCell;

public:
    typedef enum {
        BUFFER, NOT, DFF,
        OR, NOR, AND, NAND, XOR
    } GATES;

    map<string, GATES> gates_map =
    {
        {"buffer",BUFFER}, {"inv",NOT}, {"not",NOT}, {"dff",DFF},
        {"or",OR},  {"nor",NOR},  {"and",AND},  {"nand",NAND},
        {"or2",OR}, {"nor2",NOR}, {"and2",AND}, {"nand2",NAND}, {"xor2",XOR},
        {"or3",OR}, {"nor3",NOR}, {"and3",AND}, {"nand3",NAND},
        {"or4",OR}, {"nor4",NOR}, {"and4",AND}, {"nand4",NAND},
        {"or5",OR}, {"nor5",NOR}, {"and5",AND}, {"nand5",NAND},
        {"or6",OR}, {"nor6",NOR}, {"and6",AND}, {"nand6",NAND},
        {"or7",OR}, {"nor7",NOR}, {"and7",AND}, {"nand7",NAND},
        {"or8",OR}, {"nor8",NOR}, {"and8",AND}, {"nand8",NAND},
        {"or9",OR}, {"nor9",NOR}, {"and9",AND}, {"nand9",NAND}
    };

    FEC() = delete;

    FEC(Solver* solver, string fileName, hcmCell* specCell, hcmCell* impCell, set<string>& globalNodes);

    ~FEC();

    void setPairPIO(PIO& PIO1, PIO& PIO2, mapPairPIO& map_pair_pio, PIO_TYPE type);

    void connectPI();

    void connectPO(mapPairPIO& map_pair_fev_pi);

    void calcFormalEquivalence();

    void calcTseitin(hcmInstance* inst);

    void writeToCNF();

    GATES gateToEnum(string gateName) { return gates_map.find(gateName)->second; }

    void setNodesPropTree();

    void delNodesPropTree();

    void error(string msg);

    void validCheckPIOs(PIO& PIO1, PIO& PIO2, mapPairPIO& map_pair_pio, PIO_TYPE type);
};

FEC::FEC(Solver* solver, string fileName, hcmCell* specCell, hcmCell* impCell, set<string>& globalNodes) : 
    solver(solver), fileName(fileName), globalNodes(globalNodes) {
    
    topDesign   = new hcmDesign("topDesign");
    wrapperCell = topDesign->createCell("wrapperCell");
    topCell     = topDesign->createCell("topCell");
    topInst     = wrapperCell->createInst("topInst", topCell->getName());

    specCircuit = new FECCircuit(topCell, specCell, globalNodes);
    impCircuit  = new FECCircuit(topCell, impCell, globalNodes);

    PIO* specPIO = specCircuit->getPIO();
    PIO* impPIO  = impCircuit->getPIO();

    setPairPIO(specPIO[PI], impPIO[PI], map_pair_pio[PI], PI);
    setPairPIO(specPIO[PO], impPIO[PO], map_pair_pio[PO], PO);

    mapPairPIO map_pair_fev_pi;

    connectPI();
    connectPO(map_pair_fev_pi);

    fev = new FEV(topDesign, topCell, map_pair_fev_pi);

    hcmPort* fec_out_port = fev->getTopCellOutPort();
    hcmNode* wrapper_out_node = wrapperCell->createNode("FEC_PO");
    wrapperCell->connect(topInst, wrapper_out_node, fec_out_port);
    wrapper_out_node->createPort(OUT);

    flatWrapperCell = hcmFlatten("FEC_flat", wrapperCell, globalNodes);

    setNodesPropTree();

    calcFormalEquivalence();

    delNodesPropTree();

    writeToCNF();
}

FEC::~FEC() {
    delete fev;
    delete impCircuit;
    delete specCircuit;
    delete topDesign;
}

void FEC::setPairPIO(PIO& PIO1, PIO& PIO2, mapPairPIO& map_pair_pio, PIO_TYPE type) {
    PIO::const_iterator it1, it2;

    for(it1 = PIO1.begin(); it1 != PIO1.end(); it1++) {
        it2 = PIO2.find(it1->first);
        if(it2 != PIO2.end()) {
            map_pair_pio.insert(make_pair(it1->first, make_pair(it1->second, it2->second)));
        }
    }

    for(it2 = PIO2.begin(); it2 != PIO2.end(); it2++) {
        if(map_pair_pio.find(it2->first) != map_pair_pio.end()) {
            continue;
        }
        it1 = PIO1.find(it2->first);
        if(it1 != PIO1.end()) {
            map_pair_pio.insert(make_pair(it2->first, make_pair(it1->second, it2->second)));
        }
    }

    validCheckPIOs(PIO1, PIO2, map_pair_pio, type);
}

void FEC::error(string msg) {
    cerr << "Error: " << msg << endl;
    exit(1);
}

void FEC::validCheckPIOs(PIO& PIO1, PIO& PIO2, mapPairPIO& map_pair_pio, PIO_TYPE type) {
    // TODO Batel - add that they are not SAT and example for a mismatch
    if(map_pair_pio.empty()) {
        error("The PIs in the spec and imp circuits are completely different!");
    }
    if(type == PO) {
        if(PIO1.size() != PIO2.size()) {
            error("The number of POs in the spec and imp circuits is different!");
        }
        if(PIO1.size() != map_pair_pio.size()) {
            error("Some of the POs in the spec and imp circuits are different!");
        }
    }
}

void FEC::connectPI() {
    for (auto it : map_pair_pio[PI]) {
        string nodeName = it.first;
        pair<hcmNode*, hcmNode*> pair_nodes = it.second;
        hcmNode* node = topCell->createNode(string("PI_") + nodeName);
        topCell->connect(specCircuit->getInst(), node, pair_nodes.first->getPort());
        topCell->connect(impCircuit->getInst(), node, pair_nodes.second->getPort());
        hcmPort* fec_port = node->createPort(IN);
        hcmNode* fec_node = wrapperCell->createNode(string("FEC_PI_") + nodeName);
        wrapperCell->connect(topInst, fec_node, fec_port);
        fec_node->createPort(IN);
    }
}

void FEC::connectPO(mapPairPIO& map_pair_fev_pi) {
    for (auto it : map_pair_pio[PO]) {
        string nodeName = it.first;
        pair<hcmNode*, hcmNode*> pair_nodes = it.second;

        hcmNode* spec_to_fev_node = topCell->createNode(string("PO_spec_") + nodeName);
        hcmNode* imp_to_fev_node  = topCell->createNode(string("PO_imp_") + nodeName);

        topCell->connect(specCircuit->getInst(), spec_to_fev_node, pair_nodes.first->getPort());
        topCell->connect(impCircuit->getInst(), imp_to_fev_node, pair_nodes.second->getPort());

        map_pair_fev_pi.insert(make_pair(nodeName, make_pair(spec_to_fev_node, imp_to_fev_node)));
    }
}

void FEC::setNodesPropTree() {
    int i = 0;

    for (auto it : flatWrapperCell->getNodes()) {
        assert(it.second->setProp<int>("SolverVar", i++) == OK);
        solver->newVar();
    }
}

void FEC::delNodesPropTree() {
    for (auto it : flatWrapperCell->getNodes()) {
        assert(it.second->delProp<int>("SolverVar") == OK);
    }
}

void FEC::calcTseitin(hcmInstance* inst) {
    vec<Lit> lits;
    map<std::string, hcmInstPort*>& instPorts = inst->getInstPorts();
    vector<int> solverVar_inputs, solverVar_outputs;
    int nVar = solver->nVars();

    for (auto it : instPorts) {
        hcmInstPort* instPort = it.second;
        hcmNode* node = instPort->getNode();
        int solverVar;
        assert(node->getProp<int>("SolverVar", solverVar) == OK);
        hcmPort* port = instPort->getPort();
        if (port != nullptr) {
            if (port->getDirection() == IN) {
                solverVar_inputs.push_back(solverVar);
            }
            else if (port->getDirection() == OUT) {
                solverVar_outputs.push_back(solverVar);
            }
        }
    }

    switch (gateToEnum(inst->masterCell()->getName())) {
        case BUFFER:
            lits.clear();
            lits.push(~mkLit(solverVar_inputs[0]));
            lits.push(mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));

            lits.clear();
            lits.push(mkLit(solverVar_inputs[0]));
            lits.push(~mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));
            break;

        case NOT:
            lits.clear();
            lits.push(~mkLit(solverVar_inputs[0]));
            lits.push(~mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));

            lits.clear();
            lits.push(mkLit(solverVar_inputs[0]));
            lits.push(mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));
            break;

        case DFF:
            break;

        case XOR:
            lits.clear();
            lits.push(~mkLit(solverVar_inputs[0]));
            lits.push(~mkLit(solverVar_inputs[1]));
            lits.push(~mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));

            lits.clear();
            lits.push(mkLit(solverVar_inputs[0]));
            lits.push(mkLit(solverVar_inputs[1]));
            lits.push(~mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));

            lits.clear();
            lits.push(mkLit(solverVar_inputs[0]));
            lits.push(~mkLit(solverVar_inputs[1]));
            lits.push(mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));

            lits.clear();
            lits.push(~mkLit(solverVar_inputs[0]));
            lits.push(mkLit(solverVar_inputs[1]));
            lits.push(mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));
            break;

        case OR:
            lits.clear();
            for (int i = 0; i < solverVar_inputs.size(); i++) {
                lits.push(mkLit(solverVar_inputs[i]));
            }
            lits.push(~mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));

            for (int i = 0; i < solverVar_inputs.size(); i++) {
                lits.clear();
                lits.push(~mkLit(solverVar_inputs[i]));
                lits.push(mkLit(solverVar_outputs[0]));
                assert(solver->addClause(lits));
            }
            break;

        case NOR:
            lits.clear();
            for (int i = 0; i < solverVar_inputs.size(); i++) {
                lits.push(mkLit(solverVar_inputs[i]));
            }
            lits.push(mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));

            for (int i = 0; i < solverVar_inputs.size(); i++) {
                lits.clear();
                lits.push(~mkLit(solverVar_inputs[i]));
                lits.push(~mkLit(solverVar_outputs[0]));
                assert(solver->addClause(lits));
            }
            break;

        case AND:
            lits.clear();
            for (int i = 0; i < solverVar_inputs.size(); i++) {
                lits.push(~mkLit(solverVar_inputs[i]));
            }
            lits.push(mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));

            for (int i = 0; i < solverVar_inputs.size(); i++) {
                lits.clear();
                lits.push(mkLit(solverVar_inputs[i]));
                lits.push(~mkLit(solverVar_outputs[0]));
                assert(solver->addClause(lits));
            }
            break;

        case NAND:
            lits.clear();
            for (int i = 0; i < solverVar_inputs.size(); i++) {
                lits.push(~mkLit(solverVar_inputs[i]));
            }
            lits.push(~mkLit(solverVar_outputs[0]));
            assert(solver->addClause(lits));

            for (int i = 0; i < solverVar_inputs.size(); i++) {
                lits.clear();
                lits.push(mkLit(solverVar_inputs[i]));
                lits.push(mkLit(solverVar_outputs[0]));
                assert(solver->addClause(lits));
            }
            break;

        default:
            assert(false);
            break;
    }
}

void FEC::calcFormalEquivalence() {
    for (auto inst_it : flatWrapperCell->getInstances()) {
        calcTseitin(inst_it.second);
    }

    vec<Lit> lits;

    for (auto it : flatWrapperCell->getNodes()) {
        hcmNode* node = it.second;
        hcmPort* port = node->getPort();
        if (port != nullptr) {
            if (port->getDirection() == OUT) {
                lits.clear();
                int solverVar;
                assert(node->getProp<int>("SolverVar", solverVar) == OK);
                lits.push(mkLit(solverVar));
                assert(solver->addClause(lits));
            }
            else if (port->getDirection() == IN) {
                string nodeName = node->getName();
                if (globalNodes.find(nodeName) != globalNodes.end()) {
                    lits.clear();
                    int solverVar;
                    assert(node->getProp<int>("SolverVar", solverVar) == OK);
                    if (nodeName == "VDD") {
                        lits.push(mkLit(solverVar));
                    }
                    else if (nodeName == "VSS") {
                        lits.push(~mkLit(solverVar));
                    }
                    assert(solver->addClause(lits));
                }
            }
        }
    }
}

void FEC::writeToCNF() {
    vec<Lit> tmp;
    solver->toDimacs(fileName.c_str(), tmp);
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

    FEC fec(&solver, fileName, flatSpecCell, flatImpCell, globalNodes);

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
#include <errno.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <queue>
#include "hcm.h"
#include "flat.h"
#include "hcmvcd.h"
#include "hcmsigvec.h"

using namespace std;

const bool INIT_VAL = false;
typedef list<hcmInstance*> FanOutGates;


class Event {
	typedef enum {
		UPDATE_EVENT,
		EVALUATION_EVENT
	} EVENT_TYPE;
	
	hcmNode* 	node;
	EVENT_TYPE  type;
	bool 		value;

public:
	Event(hcmNode* node, EVENT_TYPE type, bool value) : node(node), type(type), value(value) {}

	~Event() {}

	friend class EventQueue;
	friend class EventDrivenSim;
};

class EventQueue {
	list<Event> events;

public:
	EventQueue() {}

	~EventQueue() {}

	bool empty() {
		return events.empty();
	}

	list<Event>::iterator find(Event event) {
		std::list<Event>::iterator event_it;

		for(event_it = events.begin(); event_it != events.end(); event_it++) {
			if(event_it->node == event.node) {
				return event_it;
			}
		}

		return events.end();
	}

	void insert(Event event) {
		std::list<Event>::iterator event_it = find(event);

		if(event_it != events.end()) {
			event_it->value = event.value;
		}
		else {
			events.push_back(event);
		}
	}

	void remove(Event event) {
		list<Event>::iterator event_it = find(event);
		events.erase(event_it);
	}

	friend class EventDrivenSim;
};

class GateQueue {
	list<hcmInstance*> gates;

public:
	typedef enum {
		BUFFER, NOT, DFF,
		OR2, NOR2, AND2, NAND2, XOR2,
		OR3, NOR3, AND3, NAND3,
		OR4, NOR4, AND4, NAND4,
		OR5, NOR5, AND5, NAND5,
		OR6, NOR6, AND6, NAND6,
		OR7, NOR7, AND7, NAND7,
		OR8, NOR8, AND8, NAND8,
		OR9, NOR9, AND9, NAND9
	} GATES;

	map<string, GATES> gates_map = 
	{
		{"buffer",BUFFER}, {"inv",NOT}, {"not",NOT}, {"dff",DFF},
		{"or",OR2},  {"nor",NOR2},  {"and",AND2},  {"nand",NAND2},
		{"or2",OR2}, {"nor2",NOR2}, {"and2",AND2}, {"nand2",NAND2}, {"xor2",XOR2},
		{"or3",OR3}, {"nor3",NOR3}, {"and3",AND3}, {"nand3",NAND3},
		{"or4",OR4}, {"nor4",NOR4}, {"and4",AND4}, {"nand4",NAND4},
		{"or5",OR5}, {"nor5",NOR5}, {"and5",AND5}, {"nand5",NAND5},
		{"or6",OR6}, {"nor6",NOR6}, {"and6",AND6}, {"nand6",NAND6},
		{"or7",OR7}, {"nor7",NOR7}, {"and7",AND7}, {"nand7",NAND7},
		{"or8",OR8}, {"nor8",NOR8}, {"and8",AND8}, {"nand8",NAND8},
		{"or9",OR9}, {"nor9",NOR9}, {"and9",AND9}, {"nand9",NAND9}
	};

	GateQueue() {}

	~GateQueue() {}

	bool empty() {
		return gates.empty();
	}

	void remove(hcmInstance* gate) {
		auto gate_it = std::find(gates.begin(), gates.end(), gate);
		gates.erase(gate_it);
	}

	bool simulate(hcmInstance* gate, vector<bool> oldGateInputs, vector<bool> gateInputs, bool gate_output) {
		switch(gateToEnum(gate->masterCell()->getName())) {
			case BUFFER:
				return gateInputs[0];
				break;
			case NOT:
				return !gateInputs[0];
				break;
			case DFF:
				return gateInputs[0] ? oldGateInputs[1] : gate_output;
				break;
			case OR2:
				return gateInputs[0] || gateInputs[1];
				break;
			case NOR2:
				return !(gateInputs[0] || gateInputs[1]);
				break;
			case AND2:
				return gateInputs[0] && gateInputs[1];
				break;
			case NAND2:
				return !(gateInputs[0] && gateInputs[1]);
				break;
			case XOR2:
				return gateInputs[0] ^ gateInputs[1];
				break;
			case OR3:
				return gateInputs[0] || gateInputs[1] || gateInputs[2];
				break;
			case NOR3:
				return !(gateInputs[0] || gateInputs[1] || gateInputs[2]);
				break;
			case AND3:
				return gateInputs[0] && gateInputs[1] && gateInputs[2];
				break;
			case NAND3:
				return !(gateInputs[0] && gateInputs[1] && gateInputs[2]);
				break;
			case OR4:
				return gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3];
				break;
			case NOR4:
				return !(gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3]);
				break;
			case AND4:
				return gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3];
				break;
			case NAND4:
				return !(gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3]);
				break;
			case OR5:
				return gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4];
				break;
			case NOR5:
				return !(gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4]);
				break;
			case AND5:
				return gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4];
				break;
			case NAND5:
				return !(gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4]);
				break;
			case OR6:
				return gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4] || gateInputs[5];
				break;
			case NOR6:
				return !(gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4] || gateInputs[5]);
				break;
			case AND6:
				return gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4] && gateInputs[5];
				break;
			case NAND6:
				return !(gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4] && gateInputs[5]);
				break;
			case OR7:
				return gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4] || gateInputs[5] || gateInputs[6];
				break;
			case NOR7:
				return !(gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4] || gateInputs[5] || gateInputs[6]);
				break;
			case AND7:
				return gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4] && gateInputs[5] && gateInputs[6];
				break;
			case NAND7:
				return !(gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4] && gateInputs[5] && gateInputs[6]);
				break;
			case OR8:
				return gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4] || gateInputs[5] || gateInputs[6] || gateInputs[7];
				break;
			case NOR8:
				return !(gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4] || gateInputs[5] || gateInputs[6] || gateInputs[7]);
				break;
			case AND8:
				return gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4] && gateInputs[5] && gateInputs[6] && gateInputs[7];
				break;
			case NAND8:
				return !(gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4] && gateInputs[5] && gateInputs[6] && gateInputs[7]);
				break;
			case OR9:
				return gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4] || gateInputs[5] || gateInputs[6] || gateInputs[7] || gateInputs[8];
				break;
			case NOR9:
				return !(gateInputs[0] || gateInputs[1] || gateInputs[2] || gateInputs[3] || gateInputs[4] || gateInputs[5] || gateInputs[6] || gateInputs[7] || gateInputs[8]);
				break;
			case AND9:
				return gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4] && gateInputs[5] && gateInputs[6] && gateInputs[7] && gateInputs[8];
				break;
			case NAND9:
				return !(gateInputs[0] && gateInputs[1] && gateInputs[2] && gateInputs[3] && gateInputs[4] && gateInputs[5] && gateInputs[6] && gateInputs[7] && gateInputs[8]);
				break;
			default:
				assert(false);
				break;
		}
		return true;
	}

	GATES gateToEnum(string gateName) {
		return gates_map.find(gateName)->second;
	}

	friend class EventDrivenSim;
};

class NetTableEntry {
	bool val;
	bool newVal;
	hcmNode* node;
	FanOutGates fanOut_gates;

public:
	NetTableEntry(bool val, bool newVal, hcmNode* node, FanOutGates fanOut_gates) : 
		val(val), newVal(newVal), node(node), fanOut_gates(fanOut_gates) {};

	~NetTableEntry() {};

	void update(bool newVal) {
		this->newVal = newVal;
	}

	void advanceTime() {
		val = newVal;
	}

	FanOutGates& getFanOutGates() {
		return fanOut_gates;
	}

	pair<bool, bool> getValues() {
		return pair<bool, bool>(val, newVal);
	}

	hcmNode* getNode() {
		return node;
	}

	friend class NetTable;
};

class NetTable {
	map<string, NetTableEntry> nets;

public:
	NetTable() {};

	~NetTable() {}

	NetTableEntry* find(hcmNode* node) {
		std::map<string, NetTableEntry>::iterator nte;
		nte = nets.find(node->getName());

		if(nte != nets.end()) {
			return &(nte->second);
		}
		return nullptr;
	}

	FanOutGates getFanoutGates(hcmNode* node) {
		NetTableEntry* nte;
		nte = find(node);

		if(nte != nullptr) {
			return nte->getFanOutGates();
		}

		FanOutGates fanout_gates;
		std::map<std::string, hcmInstPort*>::const_iterator ipI;

		for(ipI = node->getInstPorts().begin(); ipI != node->getInstPorts().end(); ipI++) {
			if(ipI->second->getPort()->getDirection() == IN) {
				fanout_gates.push_back(ipI->second->getInst());
			}
		}
		return fanout_gates;
	}

	void update(hcmNode* node, bool newVal) {
		NetTableEntry* nte;
		nte = find(node);

		if(nte != nullptr) {
			nte->update(newVal);
		}
		else {
			nets.insert({node->getName(), NetTableEntry(INIT_VAL, newVal, node, getFanoutGates(node))});
		}
	}

	void advanceTime() {
		std::map<string, NetTableEntry>::iterator nte;

		for(nte = nets.begin(); nte != nets.end(); nte++) {
			nte->second.advanceTime();
		}
	}

	friend class EventDrivenSim;
};

class EventDrivenSim {
	EventQueue    Event_Queue;
	GateQueue     Gate_Queue;
	NetTable      Net_Table;
	
	vcdFormatter& vcd;
	hcmSigVec&    parser;
	set<string>   signals;
	hcmCell 	  *flatCell;
	set<string>&  globalNodes;
	int  		  time;

public:
	EventDrivenSim(vcdFormatter& vcd, hcmSigVec& parser, hcmCell *flatCell, set<string>& globalNodes, int time):
		vcd(vcd), parser(parser), flatCell(flatCell), globalNodes(globalNodes), time(time) {
		parser.getSignals(signals);
	}

	~EventDrivenSim() {}

	void CircuitInput(vector<pair<string, bool>> input_vector) {
		for(auto input : input_vector) {
			hcmNode* node = flatCell->getNode(input.first);
			Event_Queue.insert(Event(node, Event::UPDATE_EVENT, input.second));
		}
	}

	bool updateEvent(Event event) {
		NetTableEntry* nte;
		nte = Net_Table.find(event.node);

		if(nte == nullptr) {
			Net_Table.update(event.node, event.value);
			return true;
		}
		else {
			pair<bool, bool> values = nte->getValues();
			nte->update(event.value);

			if(event.value != values.second) {
				return true;
			}
		}
		return false;
	}

	void evaluateEvent(Event event) {
		FanOutGates fanout_gates = Net_Table.getFanoutGates(event.node);

		for(auto gate : fanout_gates) {
			auto gate_it = std::find(Gate_Queue.gates.begin(), Gate_Queue.gates.end(), gate);
			if(gate_it == Gate_Queue.gates.end()) {
				Gate_Queue.gates.push_back(gate);
			}
		}
	}

	/**
	 *  For each Event E in Event_Queue do
	 *		N:=Net Identifier of E
	 *		Copy New value from E to the Net Table entry for N.
	 *		For each gate G in the fanout of N do
	 *			If G is not already in the Gate_Queue Then
	 *				Add G to Gate_Queue
	 *			End If
	 *		End For
	 *		Remove E from Event_Queue
	 *	EndFor;
	 **/
	void Event_Processor() {
		list<Event> events = Event_Queue.events;
		for(auto event : events) {
			Event_Queue.remove(event);
			if(event.type == Event::UPDATE_EVENT) {
				if(updateEvent(event)) {
					Event_Queue.insert(Event(event.node, Event::EVALUATION_EVENT, event.value));
				}
			}
			else if(event.type == Event::EVALUATION_EVENT) {
				evaluateEvent(event);
			}
		}
	}

	/**
	 *  For Each Gate G in Gate_Queue do
	 *		N:= The Output of G
	 *		Simulate G, put the result in New_N
	 *		If New_N is different from the current value of N then
	 *			Create a new event E
	 *			Net Identifier of E := N
	 *			New value of E := New_N
	 *			Add E to Event_Queue
	 *		End If
	 *		Remove G from Gate_Queue
	 *	EndFor
	 **/
	void Gate_Processor() {
		FanOutGates gates = Gate_Queue.gates;
		for(auto gate : gates) {
			pair<vector<bool>, vector<bool>> gate_inputs = GetGateInputs(gate);
			bool gate_output = GetGateOutput(gate);
			bool newVal = Gate_Queue.simulate(gate, gate_inputs.first, gate_inputs.second, gate_output);

			std::map<std::string, hcmInstPort*>::const_iterator ipI;

			for(ipI = gate->getInstPorts().begin(); ipI != gate->getInstPorts().end(); ipI++) {
				if(ipI->second->getPort()->getDirection() == OUT) {
					hcmNode* node = ipI->second->getNode();

					if(updateEvent(Event(node, Event::UPDATE_EVENT, newVal))) {
						Event_Queue.insert(Event(node, Event::EVALUATION_EVENT, newVal));
					}
				}
			}
			Gate_Queue.remove(gate);
		}
	}

	pair<vector<bool>, vector<bool>> GetGateInputs(hcmInstance *gate) {
		pair<vector<bool>, vector<bool>> gate_inputs;
		std::map<std::string, hcmInstPort*>::const_iterator ipI;

		for(ipI = gate->getInstPorts().begin(); ipI != gate->getInstPorts().end(); ipI++) {
			if(ipI->second->getPort()->getDirection() == IN) {
				hcmNode* node = ipI->second->getNode();
				NetTableEntry* nte = Net_Table.find(node);

				if(nte) {
					pair<bool, bool> values = nte->getValues();

					if(ipI->second->getPort()->getName().compare("CLK") == 0) {
						gate_inputs.first.insert(gate_inputs.first.begin(), values.first);
						gate_inputs.second.insert(gate_inputs.second.begin(), values.second);
					}
					else {
						gate_inputs.first.push_back(values.first);
						gate_inputs.second.push_back(values.second);
					}
				}
				else {
					gate_inputs.first.push_back(INIT_VAL);
					gate_inputs.second.push_back(INIT_VAL);
				}
			}
		}
		return gate_inputs;
	}

	bool GetGateOutput(hcmInstance *gate) {
		bool gate_output;
		std::map<std::string, hcmInstPort*>::const_iterator ipI;

		for(ipI = gate->getInstPorts().begin(); ipI != gate->getInstPorts().end(); ipI++) {
			if(ipI->second->getPort()->getDirection() == OUT) {
				hcmNode* node = ipI->second->getNode();
				NetTableEntry* nte = Net_Table.find(node);

				if(nte) {
					pair<bool, bool> values = nte->getValues();
					gate_output = values.second;
				}
				else {
					gate_output = INIT_VAL;
				}
			}
		}
		return gate_output;
	}

	void PrintFinalOutput() {
		const hcmNode *node;
		list<const hcmInstance*> parents;
		std::map<string, NetTableEntry>::iterator nte;

		for(nte = Net_Table.nets.begin(); nte != Net_Table.nets.end(); nte++) {
			node = nte->second.getNode();
			hcmNodeCtx *ctx = new hcmNodeCtx(parents, node);
			vcd.changeValue(ctx, nte->second.getValues().second);
		}

		// TODO Batel
		if(time != 1) return;
		
		cout << "Time: " << time-1 << endl;

		for(nte = Net_Table.nets.begin(); nte != Net_Table.nets.end(); nte++) {
			cout << "Net " << nte->first << ": " << "old value: " << nte->second.getValues().first << " new value: " << nte->second.getValues().second << endl;
		}

		cout << endl;
	}

	void SimulateVector(vector<pair<string, bool>> input_vector) {
		CircuitInput(input_vector);
		while(!Event_Queue.empty()) {
			Event_Processor();
			if(!Gate_Queue.empty()) {
				Gate_Processor();
			}
		}
		PrintFinalOutput();
		Net_Table.advanceTime();
	}

	void Simulate() {
		// reading each vector
		while (parser.readVector() == 0) {
			// set the inputs to the values from the input vector.
			vector<pair<string, bool>> in_vector;

			for (set<string>::iterator sig = signals.begin(); sig != signals.end(); sig++) {
				string name = (*sig);
				bool val;
				parser.getSigValue(name, val);
				in_vector.push_back(pair<string, bool>(name, val));
			}

			// simulate the vector
			SimulateVector(in_vector);
			
			vcd.changeTime(time++);
		}
	}
};


//globals:
bool verbose = false;

int main(int argc, char **argv) {
	int argIdx = 1;
	int anyErr = 0;
	unsigned int i;
	vector<string> vlgFiles;
	
	if (argc < 5) {
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
		cerr << "Usage: " << argv[0] << "  [-v] top-cell signal_file.sig.txt vector_file.vec.txt file1.v [file2.v] ... \n";
		exit(1);
	}
 
	set< string> globalNodes;
	globalNodes.insert("VDD");
	globalNodes.insert("VSS");
	
	hcmDesign* design = new hcmDesign("design");
	string cellName = vlgFiles[0];
	for (i = 3; i < vlgFiles.size(); i++) {
		printf("-I- Parsing verilog %s ...\n", vlgFiles[i].c_str());
		if (!design->parseStructuralVerilog(vlgFiles[i].c_str())) {
			cerr << "-E- Could not parse: " << vlgFiles[i] << " aborting." << endl;
			exit(1);
		}
	}

	hcmCell *topCell = design->getCell(cellName);
	if (!topCell) {
		printf("-E- could not find cell %s\n", cellName.c_str());
		exit(1);
	}
	
	hcmCell *flatCell = hcmFlatten(cellName + string("_flat"), topCell, globalNodes);
	cout << "-I- Top cell flattened" << endl;

	string signalTextFile = vlgFiles[1];
	string vectorTextFile = vlgFiles[2];

	
	// Genarate the vcd file
	//-----------------------------------------------------------------------------------------//
	// If you need to debug internal nodes, set debug_mode to true in order to see in 
	// the vcd file and waves the internal nodes.
	// NOTICE !!!
	// you need to submit your work with debug_mode = false
	// vcdFormatter vcd(cellName + ".vcd", flatCell, globalNodes, true);  <--- for debug only!
	//-----------------------------------------------------------------------------------------//
	vcdFormatter vcd(cellName + ".vcd", flatCell, globalNodes); // TODO Batel
	if(!vcd.good()) {
		printf("-E- vcd initialization error.\n");
		exit(1);
	}

	// initiate the time variable "time" to 1 
	int time = 1;
	hcmSigVec parser(signalTextFile, vectorTextFile, verbose);
	set<string> signals;
	parser.getSignals(signals);

	//-----------------------------------------------------------------------------------------//

	EventDrivenSim sim(vcd, parser, flatCell, globalNodes, time);

	sim.Simulate();

	return(0);

	//-----------------------------------------------------------------------------------------//
}	

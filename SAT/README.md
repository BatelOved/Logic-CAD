# SAT-based Gate Level Formal Equivalence Checker 

## Overview
Formal Equivalence Verification (FEV) provides a key technology that enables comparing the RTL to the synthesized gate-level or multiple different gate-level implementations to each other. In recent years, BDDs which were the cornerstone of formal models and FEV was replaced by SAT solvers to resolve the capacity limitations of BDDs.

## How to Run

### Build:
From Logic-CAD directory, run `make`.\
From SAT directory, run `make`.\
From minisat directory, follow the README instructions.

### Formal Equivalence Checker:
* `./gl_verilog_fev -s TopLevel2000 stdcell.v tests/toplevels.v -i TopLevel2001 stdcell.v tests/toplevels.v`\
Expected output: NOT SATISFIABLE! (equivalent circuits)

* `./gl_verilog_fev -s TopLevel2000 stdcell.v tests/toplevels.v -i TopLevel2002 stdcell.v tests/toplevels.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel3000 stdcell.v tests/toplevels.v -i TopLevel3001 stdcell.v tests/toplevels.v`\
Expected output: NOT SATISFIABLE! (equivalent circuits)

* `./gl_verilog_fev -s TopLevel3000 stdcell.v tests/toplevels.v -i TopLevel3002 stdcell.v tests/toplevels.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel3000 stdcell.v tests/toplevels.v -i TopLevel3003 stdcell.v tests/toplevels.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel3000 stdcell.v tests/toplevels.v -i TopLevel3004 stdcell.v tests/toplevels.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel4000 stdcell.v tests/toplevels.v -i TopLevel4001 stdcell.v tests/toplevels.v`\
Expected output: NOT SATISFIABLE! (equivalent circuits)

* `./gl_verilog_fev -s TopLevel4000 stdcell.v tests/toplevels.v -i TopLevel4002 stdcell.v tests/toplevels.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel5000 stdcell.v tests/toplevels.v -i TopLevel5001 stdcell.v tests/toplevels.v`\
Expected output: NOT SATISFIABLE! (equivalent circuits)

* `./gl_verilog_fev -s TopLevel5000 stdcell.v tests/toplevels.v -i TopLevel5002 stdcell.v tests/toplevels.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel1355 stdcell.v tests/c1355.v -i TopLevel1356 stdcell.v tests/c1356.v`\
Expected output: NOT SATISFIABLE! (equivalent circuits)

* `./gl_verilog_fev -s TopLevel0409 stdcell.v tests/c0409.v -i TopLevel0410 stdcell.v tests/c0410.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

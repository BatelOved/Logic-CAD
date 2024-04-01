# SAT-based Gate Level Formal Equivalence Checker 

## Overview
`TODO`

## How to Run

### Build:
From Logic-CAD directory, run `make`.\
From SAT directory, run `make`.\
From minisat directory, follow the README instructions.

### Formal Equivalence Checker:
* `./gl_verilog_fev -s TopLevel2000 stdcell.v tests/c2000.v -i TopLevel2001 stdcell.v tests/c2001.v`\
Expected output: NOT SATISFIABLE! (equivalent circuits)

* `./gl_verilog_fev -s TopLevel2000 stdcell.v tests/c2000.v -i TopLevel2002 stdcell.v tests/c2002.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel3000 stdcell.v tests/c3000.v -i TopLevel3001 stdcell.v tests/c3001.v`\
Expected output: NOT SATISFIABLE! (equivalent circuits)

* `./gl_verilog_fev -s TopLevel3000 stdcell.v tests/c3000.v -i TopLevel3002 stdcell.v tests/c3002.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel3000 stdcell.v tests/c3000.v -i TopLevel3003 stdcell.v tests/c3003.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel3000 stdcell.v tests/c3000.v -i TopLevel3004 stdcell.v tests/c3004.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

* `./gl_verilog_fev -s TopLevel4000 stdcell.v tests/c4000.v -i TopLevel4001 stdcell.v tests/c4001.v`\
Expected output: NOT SATISFIABLE! (equivalent circuits)

* `./gl_verilog_fev -s TopLevel1355 stdcell.v tests/c1355.v -i TopLevel1356 stdcell.v tests/c1356.v`\
Expected output: NOT SATISFIABLE! (equivalent circuits)

* `./gl_verilog_fev -s TopLevel0409 stdcell.v tests/c0409.v -i TopLevel0410 stdcell.v tests/c0410.v`\
Expected output: SATISFIABLE! (non-equivalent circuits)

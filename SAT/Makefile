# Update the paths below according to your directory structure
HCMPATH=$(shell pwd)/../
MINISAT=$(HCMPATH)/minisat

# required for adding code of minisat to your program
MINISAT_OBJS=$(MINISAT)/core/Solver.o $(MINISAT)/utils/Options.o $(MINISAT)/utils/System.o

CXXFLAGS=-ggdb -O0 -fPIC -I$(HCMPATH)/include -I$(MINISAT) -I$(HCMPATH)/flattener -fpermissive -Wliteral-suffix
CFLAGS=-ggdb -O0 -fPIC -I$(HCMPATH)/include -I$(MINISAT) -I$(HCMPATH)/flattener -fpermissive -Wliteral-suffix
CC=g++ -g
LDFLAGS=$(MINISAT_OBJS) -L$(HCMPATH)/src -lhcm -Wl,-rpath=$(HCMPATH)/src 

all: gl_verilog_fev

gl_verilog_fev: HW3ex1.o
	$(CC) -o $@ $^ $(LDFLAGS) $(HCMPATH)/flattener/flat.o $(HCMPATH)/hcm_vcd/vcd.o

clean:
	 @ rm *.o gl_verilog_fev

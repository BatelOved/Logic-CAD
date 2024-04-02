/*************************************************************************/
// Basic circuits, some with DFFs.
//
// Expected outputs:
// TopLevel2000 & TopLevel2001 - NOT SATISFIABLE! (equivalent circuits)
// TopLevel3000 & TopLevel3001 - NOT SATISFIABLE! (equivalent circuits)
// TopLevel4000 & TopLevel4001 - NOT SATISFIABLE! (equivalent circuits)
// All other combinations  - SATISFIABLE!     (non-equivalent circuits)
/*************************************************************************/

module  TopLevel2000 (IN, OUT);

  input  [0:1]  IN;
  output [0:0]  OUT;

  nand nand0 (.A(IN[0]), .B(IN[1]), .Y(OUT[0]));

endmodule

module  TopLevel2001 (IN, OUT);

  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:0]   W;

  and and0 (.A(IN[0]), .B(IN[1]), .Y(W[0]));
  not not0 (.A(W[0]), .Y(OUT[0]));

endmodule

module  TopLevel2002 (IN, OUT);

  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:0]   W;

  nand and0 (.A(IN[0]), .B(IN[1]), .Y(W[0]));
  not not0 (.A(W[0]), .Y(OUT[0]));

endmodule

module  TopLevel3000 (CLK, IN, OUT);

  input         CLK;
  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:1]  W;

  dff dff0 (.CLK(CLK), .D(IN[0]), .Q(W[0]));
  dff dff1 (.CLK(CLK), .D(IN[1]), .Q(W[1]));

  nand nand0 (.A(W[0]), .B(W[1]), .Y(OUT[0]));

endmodule

module  TopLevel3001 (CLK, IN, OUT);

  input         CLK;
  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:2]   W;

  dff dff0 (.CLK(CLK), .D(IN[0]), .Q(W[0]));
  dff dff1 (.CLK(CLK), .D(IN[1]), .Q(W[1]));

  and and0 (.A(W[0]), .B(W[1]), .Y(W[2]));
  not not0 (.A(W[2]), .Y(OUT[0]));

endmodule

module  TopLevel3002 (CLK, IN, OUT);

  input         CLK;
  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:2]   W;

  dff dff0 (.CLK(CLK), .D(IN[0]), .Q(W[0]));
  dff dff2 (.CLK(CLK), .D(IN[1]), .Q(W[1]));

  and and0 (.A(W[0]), .B(W[1]), .Y(W[2]));
  not not0 (.A(W[2]), .Y(OUT[0]));

endmodule

module  TopLevel3003 (IN, OUT);

  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:0]   W;

  or  or0 (.A(IN[0]), .B(IN[1]), .Y(W[0]));
  not not0 (.A(W[0]), .Y(OUT[0]));

endmodule

module  TopLevel3004 (CLK, IN, OUT);

  input         CLK;
  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:3]   W;

  dff dff0 (.CLK(CLK), .D(IN[0]), .Q(W[0]));
  dff dff1 (.CLK(CLK), .D(IN[1]), .Q(W[1]));

  and and0 (.A(W[0]), .B(W[1]), .Y(W[2]));
  not not0 (.A(W[2]), .Y(W[3]));

  dff dff2 (.CLK(CLK), .D(W[3]), .Q(OUT[0]));

endmodule

module  TopLevel4000 (CLK, IN, OUT);

  input         CLK;
  input  [0:0]  IN;
  output [0:0]  OUT;


  dff dff0 (.CLK(CLK), .D(IN[0]), .Q(OUT[0]));

endmodule

module  TopLevel4001 (CLK, IN, OUT);

  input         CLK;
  input  [0:0]  IN;
  output [0:0]  OUT;

  wire [0:1]  W;

  not not0 (.A(IN[0]), .Y(W[0]));
  not not1 (.A(W[0]), .Y(W[1]));

  dff dff0 (.CLK(CLK), .D(W[1]), .Q(OUT[0]));

endmodule

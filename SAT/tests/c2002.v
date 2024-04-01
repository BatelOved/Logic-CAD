
module  TopLevel2002 (IN, OUT);

  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:0]   W;

  nand and0 (.A(IN[0]), .B(IN[1]), .Y(W[0]));
  not not0 (.A(W[0]), .Y(OUT[0]));

endmodule /* TopLevel2002 */


/*************************************************************************/

module Circuit2002 (in0, in1,
                    out0);

  input      in0, in1;
  output     out0;


  wire [0:1]  IW;
  wire [0:0]  OW;


  buffer addedBuf0 (.A(in0), .Y(IW[0]));
  buffer addedBuf1 (.A(in1), .Y(IW[1]));

  TopLevel2002 Ckt2002 (IW, OW);

  buffer addedBuf2 (.A(OW[0]), .Y(out0));

endmodule /* Circuit2002 */

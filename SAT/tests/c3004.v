
module  TopLevel3004 (IN, OUT);

  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:3]   W;

  dff dff0 (.D(IN[0]), .Q(W[0]));
  dff dff1 (.D(IN[1]), .Q(W[1]));

  and and0 (.A(W[0]), .B(W[1]), .Y(W[2]));
  not not0 (.A(W[2]), .Y(W[3]));

  dff dff2 (.D(W[3]), .Q(OUT[0]));

endmodule /* TopLevel3004 */


/*************************************************************************/

module Circuit3004 (in0, in1,
                    out0);

  input      in0, in1;
  output     out0;


  wire [0:1]  IW;
  wire [0:0]  OW;


  buffer addedBuf0 (.A(in0), .Y(IW[0]));
  buffer addedBuf1 (.A(in1), .Y(IW[1]));

  TopLevel3004 Ckt3004 (IW, OW);

  buffer addedBuf2 (.A(OW[0]), .Y(out0));

endmodule /* Circuit3004 */

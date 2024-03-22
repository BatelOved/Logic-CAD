
module  TopLevel1997 (IN, OUT);

  input  [0:1]  IN;
  output [0:0]  OUT;

  nand nand0 (.A(IN[0]), .B(IN[1]), .Y(OUT[0]));

endmodule /* TopLevel1997 */


/*************************************************************************/

module Circuit1997 (in0, in1,
                    out0);

  input      in0, in1;
  output     out0;


  wire [0:1]  IW;
  wire [0:0]  OW;


  buffer addedBuf0 (.A(in0), .Y(IW[0]));
  buffer addedBuf1 (.A(in1), .Y(IW[1]));

  TopLevel1997 Ckt1997 (IW, OW);

  buffer addedBuf2 (.A(OW[0]), .Y(out0));

endmodule /* Circuit1997 */


module  TopLevel4001 (IN, OUT);

  input  [0:0]  IN;
  output [0:0]  OUT;

  wire [0:1]  W;

  not not0 (.A(IN[0]), .Y(W[0]));
  not not1 (.A(W[0]), .Y(W[1]));

  dff dff0 (.D(W[1]), .Q(OUT[0]));

endmodule /* TopLevel4001 */


/*************************************************************************/

module Circuit4001 (in0,
                    out0);

  input      in0;
  output     out0;


  wire [0:0]  IW;
  wire [0:0]  OW;


  buffer addedBuf0 (.A(in0), .Y(IW[0]));

  TopLevel4001 Ckt4001 (IW, OW);

  buffer addedBuf1 (.A(OW[0]), .Y(out0));

endmodule /* Circuit4001 */

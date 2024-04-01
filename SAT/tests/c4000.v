/*************************************************************************/
// Basic circuit with 1 inputs and 1 output, with 1 DFF.
//
// Expected outputs:
// with TopLevel4001 - NOT SATISFIABLE! (equivalent circuits)
/*************************************************************************/


module  TopLevel4000 (IN, OUT);

  input  [0:0]  IN;
  output [0:0]  OUT;


  dff dff0 (.D(IN[0]), .Q(OUT[0]));

endmodule /* TopLevel4000 */


/*************************************************************************/

module Circuit4000 (in0,
                    out0);

  input      in0;
  output     out0;


  wire [0:0]  IW;
  wire [0:0]  OW;


  buffer addedBuf0 (.A(in0), .Y(IW[0]));

  TopLevel4000 Ckt4000 (IW, OW);

  buffer addedBuf1 (.A(OW[0]), .Y(out0));

endmodule /* Circuit4000 */

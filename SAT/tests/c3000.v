/*************************************************************************/
// Basic circuit with 2 inputs and 1 output, with DFFs.
//
// Expected outputs:
// with TopLevel3001 - NOT SATISFIABLE! (equivalent circuits)
// with TopLevel3002 - SATISFIABLE!     (non-equivalent circuits)
// with TopLevel3003 - SATISFIABLE!     (non-equivalent circuits)
/*************************************************************************/


module  TopLevel3000 (IN, OUT);

  input  [0:1]  IN;
  output [0:0]  OUT;

  wire [0:1]  W;

  dff dff0 (.D(IN[0]), .Q(W[0]));
  dff dff1 (.D(IN[1]), .Q(W[1]));

  nand nand0 (.A(W[0]), .B(W[1]), .Y(OUT[0]));

endmodule /* TopLevel3000 */


/*************************************************************************/

module Circuit3000 (in0, in1,
                    out0);

  input      in0, in1;
  output     out0;


  wire [0:1]  IW;
  wire [0:0]  OW;


  buffer addedBuf0 (.A(in0), .Y(IW[0]));
  buffer addedBuf1 (.A(in1), .Y(IW[1]));

  TopLevel3000 Ckt3000 (IW, OW);

  buffer addedBuf2 (.A(OW[0]), .Y(out0));

endmodule /* Circuit3000 */

module shiftReg(I, CLK, S);
   input  		I;
   input       C1;
   output  		S;
	
   wire [1:0] N;

   dff ff1( .CLK(C1), .D(I), .Q(N[0]) );

   dff ff2( .CLK(C1), .D(N[0]), .Q(N[1]) );
   
   dff ff3( .CLK(C1), .D(N[1]), .Q(S) );

endmodule
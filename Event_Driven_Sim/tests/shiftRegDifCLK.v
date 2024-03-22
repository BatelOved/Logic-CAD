module shiftRegDifCLK(I, C1 , C2 , C3, S);
   input  		I;
   input       C1; 
   input       C2; 
   input       C3; 
   output  		S;
	
   wire [1:0] N;

   dff ff1( .CLK(C1), .D(I), .Q(N[0]) );

   dff ff2( .CLK(C2), .D(N[0]), .Q(N[1]) );
   
   dff ff3( .CLK(C3), .D(N[1]), .Q(S) );

endmodule
extern module sm_test
(  
 clock clk,
 input  bit rst_n,
 
 input  bit in,
 
 output bit top_sig,
 output bit top_reg,

 output bit sub_sig,
 output bit sub_reg

 )
  
{
  timing to rising clock clk  
    rst_n,
    in;
  timing from rising clock clk 
    top_sig,
    top_reg,
    sub_sig,
    sub_reg;
    timing comb input in;
    timing comb output top_sig, sub_sig;
}

extern module sm_sub
(  
 clock clk,
 input  bit rst_n,
 input  bit sub_in,
 output bit sub_out_sig,
 output bit sub_out_reg 
 )
  
{
    timing comb input sub_in;
    timing comb output sub_out_sig;
  timing to rising clock clk  
    rst_n,
    sub_in;
  timing from rising clock clk 
    sub_out_sig,
    sub_out_reg;
}

/*a Copyright
  
  This file 'md_target_c.cpp' copyright Gavin J Stark 2003, 2004, 2007
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a Documentation

The theory is that we should have:

a) a prepreclock function - INPUTS ARE NOT VALID:
this would clear 'inputs copied', 'clocks at this step'

b) preclock function for each clock about to fire- INPUTS ARE VALID:
if (!inputs copied) {capture inputs to input_state; inputs_copied=1;}
set bit for 'clocks at this step'

c) clock function - INPUTS ARE NOT VALID, NON-COMB OUTPUTS MUST BE VALID ON EXIT
Propagate from inputs
Do all internal combs and set inputs to all instances and flops (current preclock functionality)
  calls some submodule comb functions if they are purely combinatorial
set bits for gated clocks derived from 'clocks at this step'
for all clocks with bits set, call prepreclock submodule functions
for all clocks with bits set, call preclock submodule functions
for all clocks with bits set, call clock submodule functions
copy next state to state
generate outputs from state
clear 'clocks at this step'

e) comb function
only for combinatorial functions; input to outputs only.
  calls some submodule comb functions if they are purely combinatorial

f) propagate function for simulation waveform validity
do all internal combs and set inputs to all instances and flops
call propagate submodule functions
do all internal state to outputs
do all inputs to outputs


The dependency stuff (which is correct in c_model_descriptor.cpp) is not correct here.
We should pull out the output_marker stuff to another file
We should invoke some of that in the md_target_xml so we can see whether/how it is working

New cdl and new code:


real    4m13.925s
user    26m38.042s
sys     1m36.487s
1

>2.5MB

  5664 -rw-r--r--  1 gstark  staff   2899345 Aug  7 12:23 mu_ae_resource_handler.cpp
  6048 -rw-r--r--  1 gstark  staff   3093716 Aug  7 12:26 mu_ctm_dcache.o
  6104 -rw-r--r--  1 gstark  staff   3124944 Aug  7 12:23 mu_dc_mbistin_wrapper_128_to_8.cpp
  6440 -rw-r--r--  1 gstark  staff   3293297 Aug  7 12:23 mu_pe_dma_resource_handler.cpp
  6584 -rw-r--r--  1 gstark  staff   3367344 Aug  7 12:26 mu_external_dcache.o
  8728 -rw-r--r--  1 gstark  staff   4466664 Aug  7 12:26 mu_be_ext_orderer_channel.o
  9656 -rw-r--r--  1 gstark  staff   4940982 Aug  1 15:55 mu_ctm.cpp~
  9776 -rw-r--r--  1 gstark  staff   5005056 Aug  7 12:26 mu_internal_dcache.o
 11704 -rw-r--r--  1 gstark  staff   5989289 Aug  7 12:23 mu_ctm_dcache.cpp
 12864 -rw-r--r--  1 gstark  staff   6584183 Aug  7 12:23 mu_external_dcache.cpp
 19056 -rw-r--r--  1 gstark  staff   9754663 Aug  7 12:23 mu_internal_dcache.cpp
 20376 -rw-r--r--  1 gstark  staff  10430940 Aug  7 12:23 mu_be_ext_orderer_channel.cpp
105144 -rwxr-xr-x  1 gstark  staff  53833288 Aug  7 12:26 sim
105224 -rwxr-xr-x  1 gstark  staff  53873884 Aug  7 12:26 py_engine.so


New CDL
 real    22m25.052s
 user    20m36.721s
 sys     0m51.255s

  4048 -rw-r--r--  1 gstark  staff   2068615 Aug  1 13:43 mu_ctm_no_mbist.cpp
>2.5MB
  5304 -rw-r--r--  1 gstark  staff   2713665 Aug  1 13:43 mu_ctm.cpp
  5840 -rw-r--r--  1 gstark  staff   2987755 Aug  1 13:44 mu_ae_resource_handler.cpp
  6096 -rw-r--r--  1 gstark  staff   3118934 Aug  1 13:44 mu_dc_mbistin_wrapper_128_to_8.cpp
  6384 -rw-r--r--  1 gstark  staff   3266602 Aug  1 13:44 mu_pe_dma_resource_handler.cpp
  7176 -rw-r--r--  1 gstark  staff   3673992 Aug  1 13:48 mu_ctm_dc.o
  7184 -rw-r--r--  1 gstark  staff   3677316 Aug  1 13:48 mu_external_dc.o
  9032 -rw-r--r--  1 gstark  staff   4620872 Aug  1 13:49 mu_be_ext_orderer_channel.o
  9136 -rw-r--r--  1 gstark  staff   4677228 Aug  1 13:49 mu_internal_dc.o
 13376 -rw-r--r--  1 gstark  staff   6844469 Aug  1 13:44 mu_ctm_dc.cpp
 13520 -rw-r--r--  1 gstark  staff   6918750 Aug  1 13:44 mu_external_dc.cpp
 17496 -rw-r--r--  1 gstark  staff   8956023 Aug  1 13:48 mu_internal_dc.cpp
 20568 -rw-r--r--  1 gstark  staff  10529190 Aug  1 13:45 mu_be_ext_orderer_channel.cpp
116560 -rwxr-xr-x  1 gstark  staff  59675448 Aug  1 13:50 sim
116632 -rwxr-xr-x  1 gstark  staff  59711948 Aug  1 13:49 py_engine.so

13:38:09:~/Desktop/Development/Mercurial/me_island.hg:750$ time make
(cd build/cdl; make DEBUG_BUILD="yes" all verilog)
CDL design/mu_dcache.hg/cdl/src/mu_internal_dc.cdl -cpp -mu_internal_dc.cpp
CC mu_internal_dc.cpp -o -mu_internal_dc.o
Building command line simulation osx/sim
Building Python simulation library for GUI sims osx/py_engine.so
CDL mu_internal_dc.v -v -design/mu_dcache.hg/cdl/src/mu_internal_dc.cdl
cp osx/mu_internal_dc.v /Users/gstark/Desktop/Development/Mercurial/me_island.hg/design/mu_dcache.hg/genrtl/mu_internal_dc.v

real    3m55.372s
user    3m43.922s
sys     0m1.826s

CDl 1.4.2

real    32m4.009s
user    30m22.281s
sys     1m0.746s

>5MB
 10536 -rw-r--r--  1 gstark  staff    5392984 Aug  1 12:28 mu_pe_dma_resource_handler.o
 10720 -rw-r--r--  1 gstark  staff    5487467 Aug  1 12:28 mu_pe_dma_resource_handler.cpp
 12120 -rw-r--r--  1 gstark  staff    6203820 Aug  1 12:13 mu_ctm_no_mbist.o
 13176 -rw-r--r--  1 gstark  staff    6744044 Aug  1 12:13 mu_ctm.o
 22856 -rw-r--r--  1 gstark  staff   11700716 Aug  1 12:35 mu_be_ext_orderer_channel.o
 25088 -rw-r--r--  1 gstark  staff   12841795 Aug  1 12:26 mu_ctm_dc.cpp
 25192 -rw-r--r--  1 gstark  staff   12897216 Aug  1 12:24 mu_external_dc.cpp
 27624 -rw-r--r--  1 gstark  staff   14141830 Aug  1 12:34 mu_be_ext_orderer_channel.cpp
 31592 -rw-r--r--  1 gstark  staff   16173360 Aug  1 12:28 mu_ctm_dc.o
 31688 -rw-r--r--  1 gstark  staff   16222524 Aug  1 12:26 mu_external_dc.o
 33288 -rw-r--r--  1 gstark  staff   17039957 Aug  1 12:20 mu_internal_dc.cpp
 43368 -rw-r--r--  1 gstark  staff   22202160 Aug  1 12:24 mu_internal_dc.o
352152 -rwxr-xr-x  1 gstark  staff  180299728 Aug  1 12:35 sim
352224 -rwxr-xr-x  1 gstark  staff  180335124 Aug  1 12:36 py_engine.so

13:29:23:~/Desktop/Development/Mercurial/me_island.hg:746$ rm build/cdl/osx/mu_internal_dc.*
13:30:07:~/Desktop/Development/Mercurial/me_island.hg:747$ time make
(cd build/cdl; make DEBUG_BUILD="yes" all verilog)
CDL design/mu_dcache.hg/cdl/src/mu_internal_dc.cdl -cpp -mu_internal_dc.cpp
CC mu_internal_dc.cpp -o -mu_internal_dc.o
Building command line simulation osx/sim
Building Python simulation library for GUI sims osx/py_engine.so
CDL mu_internal_dc.v -v -design/mu_dcache.hg/cdl/src/mu_internal_dc.cdl
cp osx/mu_internal_dc.v /Users/gstark/Desktop/Development/Mercurial/me_island.hg/design/mu_dcache.hg/genrtl/mu_internal_dc.v

real    7m38.912s
user    7m5.664s
sys     0m3.591s
13:37:49:~/Desktop/Development/Mercurial/me_island.hg:748$ 

 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "be_errors.h"
#include "cdl_version.h"
#include "md_output_markers.h"
#include "c_model_descriptor.h"

/*a Static variables
 */
static const char *edge_name[] = {
     "posedge",
     "negedge",
     "bug in use of edge!!!!!"
};
static const char *level_name[] = {
     "active_low",
     "active_high",
     "bug in use of level!!!!!"
};
static const char *usage_type_comment[] = { // Note this must match md_usage_type
    "/* rtl */",
    "/* assert use only */",
    "/* cover use only */",
};
static const char *bit_mask[] = {
     "0ULL", "1ULL", "3ULL", "7ULL",
     "0xfULL", "0x1fULL", "0x3fULL", "0x7fULL",
     "0xffULL", "0x1ffULL", "0x3ffULL", "0x7ffULL",
     "0xfffULL", "0x1fffULL", "0x3fffULL", "0x7fffULL",
     "((1ULL<<16)-1)", "((1ULL<<17)-1)", "((1ULL<<18)-1)", "((1ULL<<19)-1)", 
     "((1ULL<<20)-1)", "((1ULL<<21)-1)", "((1ULL<<22)-1)", "((1ULL<<23)-1)", 
     "((1ULL<<24)-1)", "((1ULL<<25)-1)", "((1ULL<<26)-1)", "((1ULL<<27)-1)", 
     "((1ULL<<28)-1)", "((1ULL<<29)-1)", "((1ULL<<30)-1)", "((1ULL<<31)-1)", 
     "((1ULL<<32)-1)", "((1ULL<<33)-1)", "((1ULL<<34)-1)", "((1ULL<<35)-1)", 
     "((1ULL<<36)-1)", "((1ULL<<37)-1)", "((1ULL<<38)-1)", "((1ULL<<39)-1)", 
     "((1ULL<<40)-1)", "((1ULL<<41)-1)", "((1ULL<<42)-1)", "((1ULL<<43)-1)", 
     "((1ULL<<44)-1)", "((1ULL<<45)-1)", "((1ULL<<46)-1)", "((1ULL<<47)-1)", 
     "((1ULL<<48)-1)", "((1ULL<<49)-1)", "((1ULL<<50)-1)", "((1ULL<<51)-1)", 
     "((1ULL<<52)-1)", "((1ULL<<53)-1)", "((1ULL<<54)-1)", "((1ULL<<55)-1)", 
     "((1ULL<<56)-1)", "((1ULL<<57)-1)", "((1ULL<<58)-1)", "((1ULL<<59)-1)", 
     "((1ULL<<60)-1)", "((1ULL<<61)-1)", "((1ULL<<62)-1)", "((1ULL<<63)-1)", 
     "(-1LL)"
};
static char type_buffer[64];

/*a Types
 */
enum
{
    om_valid_mask = 1,    // Set if signal is valid, clear if not
    om_invalid = 0,       // Zero value - indicates signal has not been made valid
    om_valid = 1,         // One value - indicates signal has been made valid
    om_must_be_valid = 2, // Set this is the signal must be made valid
    om_make_valid_mask = 3,
};

/*a Forward function declarations
 */
static void output_simulation_methods_statement( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance );
static void output_simulation_methods_expression( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_expression *expr, int main_indent, int sub_indent );

/*a Output functions
 */
/*f string_type
 */
static char *string_type( int width )
{
     if (width<=MD_BITS_PER_UINT64)
     {
          strcpy( type_buffer, "" );
     }
     else
     {
          sprintf( type_buffer, "[%d]", (width+MD_BITS_PER_UINT64-1)/MD_BITS_PER_UINT64 );
     }
     return type_buffer;
}

/*f output_header
 */
static void output_header( c_model_descriptor *model, t_md_output_fn output, void *handle )
{

     output( handle, 0, "/*a Note: created by cyclicity CDL " __CDL__VERSION_STRING " - do not hand edit without adding a comment line here \n");
     output( handle, 0, " */\n");
     output( handle, 0, "\n");
     output( handle, 0, "/*a Includes\n");
     output( handle, 0, " */\n");
     output( handle, 0, "#include <stdio.h>\n");
     output( handle, 0, "#include <stdlib.h>\n");
     output( handle, 0, "#include <string.h>\n");
     output( handle, 0, "#include \"be_model_includes.h\"\n");
     output( handle, 0, "\n");

}

/*f output_defines
 */
static void output_defines( c_model_descriptor *model, t_md_output_fn output, void *handle, int include_assertions, int include_coverage, int include_stmt_coverage )
{

    output( handle, 0, "/*a Defines\n");
    output( handle, 0, " */\n");
    output( handle, 0, "#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)\n");
    output( handle, 0, "#define struct_resolve( t, ptr, a ) ((t)(((char *)(ptr))+(a)))\n");
    output( handle, 0, "#define DEBUG_PROBE_CYCLE_284 {if (engine->cycle()==284) {WHERE_I_AM_VERBOSE_ENGINE;}}\n");
    output( handle, 0, "#define DEBUG_PROBE {}\n");
    output( handle, 0, "#define ASSIGN_TO_BIT(vector,size,bit,value) se_cmodel_assist_assign_to_bit(vector,size,bit,value)\n" ) ;
    output( handle, 0, "#define ASSIGN_TO_BIT_RANGE(vector,size,bit,length,value) se_cmodel_assist_assign_to_bit_range(vector,size,bit,length,value)\n" ) ;
    output( handle, 0, "#define WIRE_OR_TO_BIT(vector,size,bit,value) se_cmodel_assist_or_to_bit(vector,size,bit,value)\n" ) ;
    output( handle, 0, "#define WIRE_OR_TO_BIT_RANGE(vector,size,bit,length,value) se_cmodel_assist_or_to_bit_range(vector,size,bit,length,value)\n" ) ;
    output( handle, 0, "#define DISPLAY_STRING(error_number,string) { \\\n" );
    output( handle, 1, "engine->message->add_error( NULL, error_level_info, error_number, error_id_sl_exec_file_allocate_and_read_exec_file, \\\n");
    output( handle, 2, "error_arg_type_integer, engine->cycle(),\\\n" );
    output( handle, 2, "error_arg_type_const_string, \"%s\",\\\n", model->get_name() );
    output( handle, 2, "error_arg_type_malloc_string, string,\\\n" );
    output( handle, 2, "error_arg_type_none ); }\n" );
    output( handle, 0, "#define PRINT_STRING(string) { DISPLAY_STRING(error_number_se_dated_message,string); }\n" );
    if (include_stmt_coverage)
    {
        output( handle, 0, "#define COVER_STATEMENT(stmt_number) {if(stmt_number>=0)se_cmodel_assist_stmt_coverage_statement_reached(stmt_coverage,stmt_number);}\n" ) ;
    }
    else
    {
        output( handle, 0, "#define COVER_STATEMENT(stmt_number) {};\n" ) ;
    }
    if (include_assertions)
    {
        output( handle, 0, "#define ASSERT_STRING(string) { DISPLAY_STRING(error_number_se_dated_assertion,string); }\n" );
        output( handle, 0, "#define ASSERT_START        { if (!(   \n" );
        output( handle, 0, "#define ASSERT_COND_NEXT           ||  \n" );
        output( handle, 0, "#define ASSERT_COND_END             )) { \n" );
        output( handle, 0, "#define ASSERT_START_UNCOND { if (1) {  \n" );
        output( handle, 0, "#define ASSERT_END                   }}\n" );
    }
    else
    {
        output( handle, 0, "#define ASSERT_STRING(string) {}\n" );
        output( handle, 0, "#define ASSERT_START    { if (0 && !(   \n" );
        output( handle, 0, "#define ASSERT_COND_NEXT       &&  \n" );
        output( handle, 0, "#define ASSERT_COND_END        )) { \n" );
        output( handle, 0, "#define ASSERT_START_UNCOND { if (0) {  \n" );
        output( handle, 0, "#define ASSERT_END      }}\n" );
    }

    if (include_coverage)
    {
        output( handle, 0, "#define COVER_STRING(string) { DISPLAY_STRING(error_number_se_dated_coverage,string); }\n" );
        output( handle, 0, "#define COVER_CASE_START(c,b)   { if ( se_cmodel_assist_code_coverage_case_already_reached(code_coverage,c,b) && (\n" );
        output( handle, 0, "#define COVER_CASE_MESSAGE(c,b)       ) ) { se_cmodel_assist_code_coverage_case_now_reached(code_coverage,c,b); \n" );
        output( handle, 0, "#define COVER_CASE_END                 }} \n" );

        output( handle, 0, "#define COVER_START_UNCOND { if (1) {  \n" );
        output( handle, 0, "#define COVER_END                   }}\n" );
    }
    else
    {
        output( handle, 0, "#define COVER_STRING(string) {}\n" );
        output( handle, 0, "#define COVER_CASE_START(c,b) { if ( 0 && ( \n" );
        output( handle, 0, "#define COVER_CASE_MESSAGE(c,b)            )) { \n" );
        output( handle, 0, "#define COVER_CASE_END               }} \n" );

        output( handle, 0, "#define COVER_START_UNCOND { if (0) {  \n" );
        output( handle, 0, "#define COVER_END                   }}\n" );
    }
    output( handle, 0, "#define WHERE_I_AM_VERBOSE_ENGINE {fprintf(stderr,\"%%s,%%s,%%s,%%p,%%d\\n\",__FILE__,engine->get_instance_name(engine_handle),__func__,this,__LINE__ );}\n");
    output( handle, 0, "#define WHERE_I_AM_VERBOSE {fprintf(stderr,\"%%s:%%s:%%p:%%d\\n\",__FILE__,__func__,this,__LINE__ );}\n");
    output( handle, 0, "#define WHERE_I_AM {}\n");
    output( handle, 0, "#define DEFINE_DUMMY_INPUT static t_sl_uint64 dummy_input[16]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};\n");
    output( handle, 0, "\n");
}

/*f output_global_types_fns
 */
static void output_global_types_fns( c_model_descriptor *model, t_md_output_fn output, void *handle )
{
    /*b Output header for types
     */
    output( handle, 0, "/*a Global types\n");
    output( handle, 0, "*/\n");

    /*b Output descriptor types
     */
    output( handle, 0, "/*t t_clock_desc\n" );
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_clock_desc\n" );
    output( handle, 0, "{\n");
    output( handle, 1, "const char *name;\n");
    output( handle, 1, "int *posedge_inputs;\n");
    output( handle, 1, "int *negedge_inputs;\n");
    output( handle, 1, "int *posedge_outputs;\n");
    output( handle, 1, "int *negedge_outputs;\n");
    output( handle, 0, "} t_clock_desc;\n" );
    output( handle, 0, "\n");

    output( handle, 0, "/*t t_input_desc\n" );
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_input_desc\n" );
    output( handle, 0, "{\n");
    output( handle, 1, "const char *port_name;\n");
    output( handle, 1, "int driver_ofs;\n");
    output( handle, 1, "int input_state_ofs;\n");
    output( handle, 1, "char width;\n");
    output( handle, 1, "char is_comb;\n");
    output( handle, 0, "} t_input_desc;\n" );
    output( handle, 0, "\n");

    output( handle, 0, "/*t t_output_desc\n" );
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_output_desc\n" );
    output( handle, 0, "{\n");
    output( handle, 1, "const char *port_name;\n");
    output( handle, 1, "int value_ofs;\n");
    output( handle, 1, "char width;\n");
    output( handle, 1, "char is_comb;\n");
    output( handle, 0, "} t_output_desc;\n" );
    output( handle, 0, "\n");

    output( handle, 0, "/*t t_net_desc\n" );
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_net_desc\n" );
    output( handle, 0, "{\n");
    output( handle, 1, "const char *port_name;\n");
    output( handle, 1, "int net_driver_offset;\n");
    output( handle, 1, "char width;\n");
    output( handle, 1, "char vector_driven_in_parts;\n");
    output( handle, 0, "} t_net_desc;\n" );
    output( handle, 0, "\n");

    output( handle, 0, "/*t t_instance_port\n" );
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_instance_port\n" );
    output( handle, 0, "{\n");
    output( handle, 1, "const char *port_name;\n");
    output( handle, 1, "const char *output_port_name;\n");
    output( handle, 1, "int instance_port_offset;\n");
    output( handle, 1, "int net_port_offset;\n"); // Only set for some nets - those not vector_driven_in_parts
    output( handle, 1, "char width;\n");
    output( handle, 1, "char is_input;\n");
    output( handle, 1, "char comb;\n");
    output( handle, 0, "} t_instance_port;\n" );
    output( handle, 0, "\n");

    output( handle, 0, "/*t t_module_desc\n" );
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_module_desc\n" );
    output( handle, 0, "{\n");
    output( handle, 1, "t_input_desc  *input_descs;\n");
    output( handle, 1, "t_output_desc *output_descs;\n");
    output( handle, 1, "t_clock_desc  **clock_descs_list;\n");
    output( handle, 0, "} t_module_desc;\n" );
    output( handle, 0, "\n");

    /*b Output header for functions
     */
    output( handle, 0, "/*a Global functions\n");
    output( handle, 0, "*/\n");


    /*b Output module declaration function - declares inputs, outputs, and clocks they depend on
     */
    output( handle, 0, "/*f module_declaration\n");
    output( handle, 0, "*/\n");
    output( handle, 0, "static void module_declaration( c_engine *engine, void*engine_handle, void *base, t_module_desc *module_desc )\n" );
    output( handle, 0, "{\n");
    output( handle, 1, "for (int i=0; module_desc->input_descs && module_desc->input_descs[i].port_name; i++)\n" );
    output( handle, 1, "{\n");
    output( handle, 2, "engine->register_input_signal( engine_handle, module_desc->input_descs[i].port_name, module_desc->input_descs[i].width, struct_resolve( t_sl_uint64 **, base, module_desc->input_descs[i].driver_ofs )  );\n" );
    output( handle, 2, "if (module_desc->input_descs[i].is_comb) engine->register_comb_input( engine_handle, module_desc->input_descs[i].port_name );\n" );
    output( handle, 1, "}\n");
    output( handle, 1, "for (int i=0; module_desc->output_descs && module_desc->output_descs[i].port_name; i++)\n" );
    output( handle, 1, "{\n");
    output( handle, 2, "engine->register_output_signal( engine_handle, module_desc->output_descs[i].port_name, module_desc->output_descs[i].width, struct_resolve( t_sl_uint64 *, base, module_desc->output_descs[i].value_ofs )  );\n" );    
    output( handle, 2, "if (module_desc->output_descs[i].is_comb) engine->register_comb_output( engine_handle, module_desc->output_descs[i].port_name );\n" );
    output( handle, 1, "}\n");
    output( handle, 1, "for (int i=0; module_desc->clock_descs_list[i]; i++)\n" );
    output( handle, 1, "{\n");
    output( handle, 2, "t_clock_desc *clock_desc = module_desc->clock_descs_list[i];\n");
    for (int edge=0; edge<2; edge++)
    {
        output( handle, 2, "for (int j=0; clock_desc->%s_inputs && clock_desc->%s_inputs[j]>=0; j++)\n", edge_name[edge], edge_name[edge] );
        output( handle, 2, "{\n");
        output( handle, 3, "int input_number=clock_desc->%s_inputs[j];\n", edge_name[edge] );
        output( handle, 3, "engine->register_input_used_on_clock( engine_handle, module_desc->input_descs[input_number].port_name, clock_desc->name, %d );\n", edge==md_edge_pos );
        output( handle, 2, "}\n");
        output( handle, 2, "for (int j=0; clock_desc->%s_outputs && clock_desc->%s_outputs[j]>=0; j++)\n", edge_name[edge], edge_name[edge] );
        output( handle, 2, "{\n");
        output( handle, 3, "int output_number=clock_desc->%s_outputs[j];\n", edge_name[edge] );
        output( handle, 3, "engine->register_output_generated_on_clock( engine_handle, module_desc->output_descs[output_number].port_name, clock_desc->name, %d );\n", edge==md_edge_pos );
        output( handle, 2, "}\n");
    }
    output( handle, 1, "}\n");
    output( handle, 0, "}\n\n");

    /*b Output clock desc declaration function
     */
    output( handle, 0, "/*f clock_desc_declaration\n");
    output( handle, 0, "*/\n");
    output( handle, 0, "static void clock_desc_declaration( c_engine *engine, void*engine_handle, void *base, t_clock_desc *clock_desc, t_module_desc *module_desc )\n" );
    output( handle, 0, "{\n");
    for (int edge=0; edge<2; edge++)
    {
        output( handle, 1, "for (int i=0; clock_desc->%s_inputs && clock_desc->%s_inputs[i]>=0; i++)\n", edge_name[edge], edge_name[edge] );
        output( handle, 1, "{\n");
        output( handle, 2, "int input_number=clock_desc->%s_inputs[i];\n", edge_name[edge] );
        output( handle, 2, "engine->register_input_used_on_clock( engine_handle, module_desc->input_descs[input_number].port_name, clock_desc->name, %d );\n", edge==md_edge_pos );
        output( handle, 1, "}\n");
        output( handle, 1, "for (int i=0; clock_desc->%s_outputs && clock_desc->%s_outputs[i]>=0; i++)\n", edge_name[edge], edge_name[edge] );
        output( handle, 1, "{\n");
        output( handle, 2, "int output_number=clock_desc->%s_outputs[i];\n", edge_name[edge] );
        output( handle, 2, "engine->register_output_generated_on_clock( engine_handle, module_desc->output_descs[output_number].port_name, clock_desc->name, %d );\n", edge==md_edge_pos );
        output( handle, 1, "}\n");
    }
    output( handle, 0, "}\n\n");

    /*b Output instantiation wiring function
     */
    output( handle, 0, "/*f instantiation_wire_ports\n");
    output( handle, 0, "*/\n");
    output( handle, 0, "static void instantiation_wire_ports( c_engine *engine, void*engine_handle, void *base, const char *module_name, const char *module_instance_name, void *instance_handle, t_instance_port *port_list )\n" );
    output( handle, 0, "{\n");
    output( handle, 1, "for (int i=0; port_list[i].port_name; i++)\n" );
    output( handle, 1, "{\n");
    output( handle, 2, "t_instance_port *port = &(port_list[i]);\n" );
    output( handle, 2, "int comb, size;\n");
    output( handle, 2, "if (port->is_input)\n");
    output( handle, 2, "{\n");
    output( handle, 3, "engine->submodule_input_type( instance_handle, port->port_name, &comb, &size );\n" );
    output( handle, 3, "if (!comb && port->comb) {fprintf(stderr,\"Warning, incorrect timing file (no impact to simulation): submodule input %%s.%%s.%%s is used for flops (no combinatorial input-to-output path) in that submodule, but timing file used by the calling module indicated that the input was used combinatorially (with 'timing comb input').\\n\",module_name,module_instance_name,port->port_name); }\n" );
    output( handle, 3, "if (comb && !port->comb) {fprintf(stderr,\"Serious error, potential missimulation: submodule input %%s.%%s.%%s is used combinatorially in that submodule to produce an output, but timing file used by the calling module indicated that it was not. If the input is part of a structure and ANY element is combinatorial, then all elements of the structure are deemed combinatorial.\\n\",module_name,module_instance_name,port->port_name); }\n" );
    output( handle, 3, "engine->submodule_drive_input( instance_handle, port->port_name, struct_resolve( t_sl_uint64 *, base, port->instance_port_offset), port->width );\n" );
    output( handle, 2, "}\n");
    output( handle, 2, "else\n");
    output( handle, 2, "{\n");
    output( handle, 3, "engine->submodule_output_type( instance_handle, port->port_name, &comb, &size );\n" );
    output( handle, 3, "if (!comb && port->comb) {fprintf(stderr,\"Warning, incorrect timing file (no impact to simulation): submodule output %%s.%%s.%%s is generated only off a clock (no combinatorial input-to-output path) in that submodule, but timing file used by the calling module indicated that the output was generated combinatorially from an input (with 'timing comb output').\\n\",module_name, module_instance_name,port->port_name); }\n" );
    output( handle, 3, "if (comb && !port->comb) {fprintf(stderr,\"Serious error, potential missimulation: submodule output %%s.%%s.%%s is generated combinatorially in that submodule from an input, but timing file used by the calling module indicated that it was not. If the output is part of a structure and ANY element is combinatorial, then all elements of the structure are deemed combinatorial.\\n\",module_name,module_instance_name,port->port_name); }\n" );
    output( handle, 3, "engine->submodule_output_add_receiver( instance_handle, port->port_name, struct_resolve( t_sl_uint64 **, base, port->instance_port_offset), port->width );\n" );
    output( handle, 3, "if (port->net_port_offset>=0) {engine->submodule_output_add_receiver( instance_handle, port->port_name, struct_resolve( t_sl_uint64 **, base, port->net_port_offset), port->width );}\n");
    output( handle, 3, "if (port->output_port_name) {engine->submodule_output_drive_module_output( instance_handle, port->port_name, engine_handle, port->output_port_name );}\n");
    output( handle, 2, "}\n");
    output( handle, 1, "}\n");
    output( handle, 0, "}\n");

    /*b All done
     */
}

/*f output_type
 */
static void output_type( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_type_instance *instance, int indent, int indirect )
{
    switch (instance->type)
    {
    case md_type_instance_type_bit_vector:
        if (instance->type_def.data.width>64)
        {
          model->error->add_error( NULL, error_level_fatal, error_number_be_signal_width, error_id_be_backend_message,
                                   error_arg_type_malloc_string, instance->output_name,
                                   error_arg_type_none );
        }
        output( handle, indent, "t_sl_uint64 %s%s%s;\n", indirect?"*":"", instance->output_name, string_type(instance->type_def.data.width) );
        break;
    case md_type_instance_type_array_of_bit_vectors:
        if (instance->type_def.data.width>64)
        {
          model->error->add_error( NULL, error_level_fatal, error_number_be_signal_width, error_id_be_backend_message,
                                   error_arg_type_malloc_string, instance->output_name,
                                   error_arg_type_none );
        }
        output( handle, indent, "t_sl_uint64 %s%s[%d]%s;\n", indirect?"*":"", instance->output_name, instance->size, string_type(instance->type_def.data.width) );
        break;
    default:
        output( handle, indent, "<NO TYPE FOR STRUCTURES>\n");
        break;
    }
}

/*f output_types
 */
static void output_types( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle, int include_coverage, int include_stmt_coverage )
{
    int edge, level;
    t_md_signal *clk;
    t_md_signal *signal;
    t_md_state *reg;
    int i;
    t_md_module_instance *module_instance;

    /*b Output types header
     */
    output( handle, 0, "/*a Types for %s\n", module->output_name);
    output( handle, 0, "*/\n");

    /*b Output clocked storage types
     */ 
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        for (edge=0; edge<2; edge++)
        {
            if (clk->data.clock.edges_used[edge])
            {
                output( handle, 0, "/*t t_%s_%s_%s_state\n", module->output_name, edge_name[edge], clk->name );
                output( handle, 0, "*/\n");
                output( handle, 0, "typedef struct t_%s_%s_%s_state\n", module->output_name, edge_name[edge], clk->name );
                output( handle, 0, "{\n");
                for (reg=module->registers; reg; reg=reg->next_in_list)
                {
                    if (reg->usage_type!=md_usage_type_rtl) output( handle, -1, usage_type_comment[reg->usage_type] );
                    if ( (reg->clock_ref==clk) && (reg->edge==edge) )
                    {
                        for (i=0; i<reg->instance_iter->number_children; i++)
                        {
                            output_type( model, output, handle, reg->instance_iter->children[i], 1, 0 );
                        }
                    }
                }
                output( handle, 0, "} t_%s_%s_%s_state;\n", module->output_name, edge_name[edge], clk->name );
                output( handle, 0, "\n");
            }
        }
    }

    /*b Output input pointer type
     */ 
    output( handle, 0, "/*t t_%s_inputs\n", module->output_name);
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_%s_inputs\n", module->output_name);
    output( handle, 0, "{\n");
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output( handle, 1, "t_sl_uint64 *%s;\n", signal->instance_iter->children[i]->output_name );
        }
    }
    output( handle, 0, "} t_%s_inputs;\n", module->output_name );
    output( handle, 0, "\n");

    /*b Output input storage type
     */
    output( handle, 0, "/*t t_%s_input_state\n", module->output_name);
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_%s_input_state\n", module->output_name);
    output( handle, 0, "{\n");
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output_type( model, output, handle, signal->instance_iter->children[i], 1, 0 );
        }
    }
    output( handle, 0, "} t_%s_input_state;\n", module->output_name );
    output( handle, 0, "\n");

    /*b Output combinatorials storage type
     */ 
    output( handle, 0, "/*t t_%s_combinatorials\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_%s_combinatorials\n", module->output_name );
    output( handle, 0, "{\n");
    for (signal=module->combinatorials; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output_type( model, output, handle, signal->instance_iter->children[i], 1, 0 );
        }
        if (signal->usage_type!=md_usage_type_rtl) output( handle, -1, usage_type_comment[signal->usage_type] );
    }
    output( handle, 0, "} t_%s_combinatorials;\n", module->output_name );
    output( handle, 0, "\n");

    /*b Output nets storage type
     */ 
    output( handle, 0, "/*t t_%s_nets\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "typedef struct t_%s_nets\n", module->output_name );
    output( handle, 0, "{\n");
    for (signal=module->nets; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            if (signal->instance_iter->children[i]->vector_driven_in_parts) // The latter here appears to be synonymous with 'is an array'
            {
                output_type( model, output, handle, signal->instance_iter->children[i], 1, 0 );
            }
            else // If an array driven in parts or a vector not driven in parts, we indirect
            {
                output_type( model, output, handle, signal->instance_iter->children[i], 1, 1 );
            }
        }
    }
    output( handle, 0, "} t_%s_nets;\n", module->output_name );
    output( handle, 0, "\n");

    /*b Output instantiations storage types
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        t_md_module_instance_input_port *input_port;
        t_md_module_instance_output_port *output_port;

        output( handle, 0, "/*t t_%s_instance__%s %s\n", module->output_name, module_instance->name, module_instance->type );
        output( handle, 0, "*/\n");
        output( handle, 0, "typedef struct t_%s_instance__%s\n", module->output_name, module_instance->name );
        output( handle, 0, "{\n");
        output( handle, 1, "void *handle;\n" );
        if (module_instance->module_definition)
        {
            for (clk=module_instance->module_definition->clocks; clk; clk=clk->next_in_list)
            {
                output( handle, 1, "void *%s__clock_handle;\n", clk->name );
            }
            output( handle, 1, "struct\n" );
            output( handle, 1, "{\n" );
            for (input_port=module_instance->inputs; input_port; input_port=input_port->next_in_list)
            {
                output_type( model, output, handle, input_port->module_port_instance, 2, 0 );
            }        
            output( handle, 1, "} inputs;\n" );
            output( handle, 1, "struct\n" );
            output( handle, 1, "{\n" );
            for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
            {
                if (output_port->module_port_instance->output_name )
                {
                    output( handle, 2, "t_sl_uint64 *%s;\n", output_port->module_port_instance->output_name );
                }
                else
                {
                    output( handle, 2, "t_sl_uint64 *<<UNKNOWN OUTPUT - PORT ON INSTANCE NOT ON ACTUAL MODULE?>>;\n" );
                }
            }
            output( handle, 1, "} outputs;\n" );
        }
        output( handle, 0, "} t_%s_instance__%s;\n", module->output_name, module_instance->name );
        output( handle, 0, "\n");
    }

    /*b Output coverage storage types - if we include coverage
     */ 
    if (include_coverage)
    {
        output( handle, 0, "/*t t_%s_code_coverage\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "typedef struct t_%s_code_coverage\n", module->output_name );
        output( handle, 0, "{\n");
        output( handle, 1, "t_sl_uint64 bitmap[%d];\n", module->next_cover_case_entry );
        output( handle, 0, "} t_%s_code_coverage;\n", module->output_name );
        output( handle, 0, "\n");
    }
    if (include_stmt_coverage)
    {
        output( handle, 0, "/*t t_%s_stmt_coverage\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "typedef struct t_%s_stmt_coverage\n", module->output_name );
        output( handle, 0, "{\n");
        output( handle, 1, "unsigned char map[%d];\n", module->last_statement_enumeration?module->last_statement_enumeration:1 );
        output( handle, 0, "} t_%s_stmt_coverage;\n", module->output_name );
        output( handle, 0, "\n");
    }

    /*b Output C++ class for the module
     */ 
    output( handle, 0, "/*t c_%s\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "class c_%s;\n", module->output_name );
    output( handle, 0, "typedef t_sl_error_level t_%s_clock_callback_fn( void );\n", module->output_name );
    output( handle, 0, "typedef t_%s_clock_callback_fn (c_%s::*t_c_%s_clock_callback_fn);\n", module->output_name, module->output_name, module->output_name );
    output( handle, 0, "typedef struct\n");
    output( handle, 0, "{\n");
    output( handle, 1, "t_c_%s_clock_callback_fn preclock;\n", module->output_name );
    output( handle, 1, "t_c_%s_clock_callback_fn clock;\n", module->output_name );
    output( handle, 0, "} t_c_%s_clock_callback_fns;\n", module->output_name );
    output( handle, 0, "class c_%s\n", module->output_name );
    output( handle, 0, "{\n");
    output( handle, 0, "public:\n");
    output( handle, 1, "c_%s( class c_engine *eng, void *eng_handle );\n", module->output_name);
    output( handle, 1, "~c_%s();\n", module->output_name);
    output( handle, 1, "t_c_%s_clock_callback_fns clocks_fired[1000];\n", module->output_name);
    output( handle, 1, "t_sl_error_level delete_instance( void );\n" );
    output( handle, 1, "t_sl_error_level reset( int pass );\n" );
    output( handle, 1, "t_sl_error_level capture_inputs( void );\n" );
    output( handle, 1, "t_sl_error_level comb( void );\n" );
    output( handle, 1, "t_sl_error_level propagate_inputs( void );\n" );
    output( handle, 1, "t_sl_error_level propagate_inputs_to_combs_and_submodule_inputs( void );\n" );
    output( handle, 1, "t_sl_error_level propagate_state_to_outputs( void );\n" );
    output( handle, 1, "t_sl_error_level propagate_state( void );\n" );
    output( handle, 1, "t_sl_error_level propagate_all( void );\n" );
    output( handle, 1, "t_sl_error_level prepreclock( void );\n" );
    output( handle, 1, "t_sl_error_level preclock( t_c_%s_clock_callback_fn preclock, t_c_%s_clock_callback_fn clock );\n", module->output_name, module->output_name, module->output_name );
    output( handle, 1, "t_sl_error_level clock( void );\n" );
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        for (edge=0; edge<2; edge++)
        {
            if (clk->data.clock.edges_used[edge])
            {
                output( handle, 1, "t_sl_error_level preclock_%s_%s( void );\n", edge_name[edge], clk->name );
                output( handle, 1, "t_sl_error_level clock_%s_%s( void );\n", edge_name[edge], clk->name );
            }
        }
    }
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (level=0; level<2; level++)
        {
            if (signal->data.input.levels_used_for_reset[level])
            {
                output( handle, 1, "t_sl_error_level reset_%s_%s( void );\n", level_name[level], signal->name );
            }
        }
    }
    output( handle, 1, "c_engine *engine;\n");
    output( handle, 1, "int clocks_to_call;\n" );
    output( handle, 1, "void *engine_handle;\n");
    output( handle, 1, "int inputs_captured;\n" );
    if (module->number_submodule_clock_calls>0)
    {
        output( handle, 1, "int submodule_clock_guard[%d];\n", (module->number_submodule_clock_calls+31)/32 );
    }

    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        for (edge=0; edge<2; edge++)
        {
            if (clk->data.clock.edges_used[edge])
            {
                t_md_signal *clk2;
                for (clk2=module->clocks; clk2; clk2=clk2->next_in_list)
                {
                    if (clk2->data.clock.clock_ref == clk)
                    {
                        if (clk2->data.clock.edges_used[edge])
                        {
                            output( handle, 1, "int clock_enable__%s_%s;\n", edge_name[edge], clk2->name );
                        }
                    }
                }
            }
        }
    }

    output( handle, 1, "t_%s_inputs inputs;\n", module->output_name );
    output( handle, 1, "t_%s_input_state input_state;\n", module->output_name );
    output( handle, 1, "t_%s_combinatorials combinatorials;\n", module->output_name);
    output( handle, 1, "t_%s_nets nets;\n", module->output_name);
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        output( handle, 1, "t_%s_instance__%s instance_%s;\n", module->output_name, module_instance->name, module_instance->name );
    }
    if (include_coverage)
    {
        output( handle, 1, "t_%s_code_coverage code_coverage;\n", module->output_name);
    }
    if (include_stmt_coverage)
    {
        output( handle, 1, "t_%s_stmt_coverage stmt_coverage;\n", module->output_name);
    }
    if (1)// include_log
    {
        output( handle, 1, "struct t_engine_log_event_array *log_event_array;\n");
        output( handle, 1, "t_se_signal_value *log_signal_data;\n");
    }
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        for (edge=0; edge<2; edge++)
        {
            if (clk->data.clock.edges_used[edge])
            {
                output( handle, 1, "t_%s_%s_%s_state next_%s_%s_state;\n", module->output_name, edge_name[edge], clk->name, edge_name[edge], clk->name );
                output( handle, 1, "t_%s_%s_%s_state %s_%s_state;\n", module->output_name, edge_name[edge], clk->name, edge_name[edge], clk->name );
            }
        }
    }
    output( handle, 0, "};\n");
    output( handle, 0, "\n");
}

/*f output_static_variables
 */
static void output_static_variables( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle )
{
    int i;
    int edge;
    t_md_signal *clk, *signal;
    t_md_state *reg;
    t_md_type_instance *instance;
    t_md_reference_iter iter;
    t_md_reference *reference;
    t_md_module_instance *module_instance;
    int signal_number;

    /*b Output header with #define
     */
    output( handle, 0, "/*a Static variables for %s\n", module->output_name);
    output( handle, 0, "*/\n");

    output( handle, 0, "/*v struct_offset required variable\n");
    output( handle, 0, "*/\n");
    output( handle, 0, "static c_%s *___%s__ptr;\n", module->output_name, module->output_name );

    /*b Output input descriptor
     */
    if (module->inputs)
    {
        output( handle, 0, "/*v input_desc_%s\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "static t_input_desc input_desc_%s[] = \n", module->output_name );
        output( handle, 0, "{\n");
        signal_number = 0;
        for (signal=module->inputs; signal; signal=signal->next_in_list)
        {
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                instance = signal->instance_iter->children[i];
                instance->output_args[0] = signal_number;
                signal_number++;
                if (instance->type != md_type_instance_type_bit_vector) { fprintf(stderr,"BUG - port is an array, and cannot output that\n"); }
                output( handle, 1, "{\"%s\",struct_offset(___%s__ptr, inputs.%s),struct_offset(___%s__ptr, input_state.%s),%d,%d},\n",
                        instance->output_name,
                        module->output_name, instance->output_name,
                        module->output_name, instance->output_name,
                        instance->type_def.data.width,
                        signal->data.input.used_combinatorially );
            }
        }
        output( handle, 1, "{NULL,0,0,0,0},\n" );
        output( handle, 0, "};\n\n", module->output_name );
    }

    /*b Output net descriptor
     */
    if (module->nets)
    {
        output( handle, 0, "/*v net_desc_%s\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "static t_%s_nets *___%s_nets__ptr;\n", module->output_name, module->output_name );
        output( handle, 0, "static t_net_desc net_desc_%s[] = \n", module->output_name );
        output( handle, 0, "{\n");
        signal_number = 0;
        for (signal=module->nets; signal; signal=signal->next_in_list)
        {
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                instance = signal->instance_iter->children[i];
                instance->output_args[0] = signal_number;
                signal_number++;
                if (instance->vector_driven_in_parts)
                {
                    output( handle, 1, "{\"%s\",-1,%d,0},/*vector driven in parts - should set driver*/\n", instance->output_name, instance->type_def.data.width );
                }
                else
                {
                    output( handle, 1, "{\"%s\",struct_offset(___%s_nets__ptr, %s),%d,0},\n",
                            instance->output_name,
                            module->output_name, instance->output_name,
                            instance->type_def.data.width );
                }
            }
        }
        output( handle, 1, "{NULL,-1,0,0},\n" );
        output( handle, 0, "};\n\n" );
    }

    /*b Output instantiations descriptors
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        if (module_instance->module_definition)
        {
            output( handle, 0, "/*v instantiation_desc_%s_%s\n", module->output_name, module_instance->name );
            output( handle, 0, "*/\n");
            output( handle, 0, "static t_instance_port instantiation_desc_%s_%s[] = \n", module->output_name, module_instance->name );
            output( handle, 0, "{\n");
            t_md_module_instance_input_port *input_port;
            t_md_module_instance_output_port *output_port;
            for (input_port=module_instance->inputs; input_port; input_port=input_port->next_in_list)
            {
                output( handle, 1, "{ \"%s\", NULL, struct_offset( ___%s__ptr, instance_%s.inputs.%s ), -1, %d, 1, %d },\n",
                        input_port->module_port_instance->output_name,
                        module->output_name,
                        module_instance->name, input_port->module_port_instance->output_name,
                        input_port->module_port_instance->type_def.data.width,
                        input_port->module_port_instance->reference.data.signal->data.input.used_combinatorially );
            }
            for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
            {
                char driven_net[1024];
                char output_port_name[1024];
                if (output_port->lvar->instance->reference.data.signal->data.net.output_ref) // If the submodule directly drives an output wholly
                {
                    snprintf( output_port_name, sizeof(output_port_name), "\"%s\"", output_port->lvar->instance->output_name );
                }
                else
                {
                    sprintf( output_port_name, "NULL" );
                }
                if (output_port->lvar->instance->vector_driven_in_parts)
                {
                    sprintf(driven_net,"-1");
                }
                else if (output_port->lvar->instance->array_driven_in_parts)
                {
                    snprintf( driven_net, sizeof(driven_net), "struct_offset( ___%s__ptr, nets.%s[%lld] )",
                              module->output_name, 
                              output_port->lvar->instance->output_name,
                              output_port->lvar->index.data.integer );
                }
                else
                {
                    snprintf( driven_net, sizeof(driven_net), "struct_offset( ___%s__ptr, nets.%s )",
                              module->output_name, 
                              output_port->lvar->instance->output_name );
                }
                output( handle, 1, "{ \"%s\", %s, struct_offset( ___%s__ptr, instance_%s.outputs.%s ), %s, %d, 0, %d },\n",
                        output_port->module_port_instance->output_name,
                        output_port_name,
                        module->output_name, module_instance->name, output_port->module_port_instance->output_name,
                        driven_net,
                        output_port->module_port_instance->type_def.data.width,
                        output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially );

            }
            output( handle, 1, "{ NULL, NULL, -1, -1, 0, 0, 0 }\n");
            output( handle, 0, "};\n\n");
        }
    }

    /*b Output output descriptor
     */
    if (module->outputs)
    {
        output( handle, 0, "/*v output_desc_%s\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "static t_output_desc output_desc_%s[] = \n", module->output_name );
        output( handle, 0, "{\n");
        signal_number = 0;
        for (signal=module->outputs; signal; signal=signal->next_in_list)
        {
            if (signal->data.output.register_ref)
            {
                reg = signal->data.output.register_ref;
                for (i=0; i<reg->instance_iter->number_children; i++)
                {
                    instance = reg->instance_iter->children[i];
                    instance->output_args[0] = signal_number;
                    signal_number++;
                    output( handle, 1, "{\"%s\", struct_offset(___%s__ptr, %s_%s_state.%s), %d, %d },\n", instance->output_name, module->output_name, edge_name[reg->edge], reg->clock_ref->name, instance->output_name, instance->type_def.data.width, 0);
                }
            }
            if (signal->data.output.combinatorial_ref)
            {
                for (i=0; i<signal->data.output.combinatorial_ref->instance_iter->number_children; i++)
                {
                    instance = signal->data.output.combinatorial_ref->instance_iter->children[i];
                    instance->output_args[0] = signal_number;
                    signal_number++;
                    output( handle, 1, "{\"%s\", struct_offset(___%s__ptr, combinatorials.%s), %d, %d },\n", instance->output_name, module->output_name, instance->output_name, instance->type_def.data.width, signal->data.output.derived_combinatorially);
                }
            }
            if (signal->data.output.net_ref)
            {
                for (i=0; i<signal->data.output.net_ref->instance_iter->number_children; i++)
                {
                    instance = signal->data.output.net_ref->instance_iter->children[i];
                    instance->output_args[0] = signal_number;
                    signal_number++;
                    output( handle, 1, "{\"%s\", -1, %d, %d },\n", instance->output_name, instance->type_def.data.width, signal->data.output.derived_combinatorially); // -1 as it will be tied to a submodule output
                }
            }
        }
        output( handle, 1, "{NULL,-1,0,0},\n" );
        output( handle, 0, "};\n\n" );
    }

    /*b Output individual clock descriptors
     */
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        if (!clk->data.clock.clock_ref) // Not a gated clock, i.e. a root clock
        {
            if (clk->data.clock.edges_used[1] || clk->data.clock.edges_used[0]) // 0 is posedge
            {
                for (edge=0; edge<2; edge++)
                {
                    int used_on_clock_edge = 0;

                    output( handle, 0, "/*v clock_desc_%s_%s_%s_inputs\n", module->output_name, edge_name[edge], clk->name );
                    output( handle, 0, "*/\n");
                    output( handle, 0, "static int clock_desc_%s_%s_%s_inputs[] = \n", module->output_name, edge_name[edge], clk->name );
                    output( handle, 0, "{\n");

                    signal_number=0;
                    for (signal=module->inputs; signal; signal=signal->next_in_list)
                    {
                        for (i=0; i<signal->instance_iter->number_children; i++)
                        {
                            t_md_signal *clk2;
                            instance = signal->instance_iter->children[i];
                            for (clk2=module->clocks; clk2; clk2=clk2?clk2->next_in_list:NULL)
                            {
                                if ((clk2==clk) || (clk2->data.clock.clock_ref==clk))
                                {
                                    if (model->reference_set_includes( &instance->dependents, clk2, edge ))
                                    {
                                        used_on_clock_edge = 1;
                                        clk2=NULL;
                                    }
                                }
                            }
                            if (used_on_clock_edge) break;
                        }
                        if (used_on_clock_edge)
                        {
                            output( handle, 1, "%d,\n", signal_number);
                        }
                        signal_number=signal_number+1;
                    }
                    output( handle, 1, "-1\n");
                    output( handle, 0, "};\n");
                    output( handle, 0, "\n");

                    output( handle, 0, "/*v clock_desc_%s_%s_%s_outputs\n", module->output_name, edge_name[edge], clk->name );
                    output( handle, 0, "*/\n");
                    output( handle, 0, "static int clock_desc_%s_%s_%s_outputs[] = \n", module->output_name, edge_name[edge], clk->name );
                    output( handle, 0, "{\n");
                    signal_number = 0;
                    for (signal=module->outputs; signal; signal=signal->next_in_list)
                    {
                        if (signal->data.output.register_ref)
                        {
                            reg = signal->data.output.register_ref;
                            if ((reg->clock_ref->data.clock.root_clock_ref == clk) && (reg->edge==edge))
                            {
                                for (i=0; i<reg->instance_iter->number_children; i++)
                                {
                                    instance = reg->instance_iter->children[i];
                                    output( handle, 1, "%d,//r\n", instance->output_args[0]);
                                }
                            }
                        }
                        if (signal->data.output.combinatorial_ref)
                        {
                            for (i=0; i<signal->data.output.combinatorial_ref->instance_iter->number_children; i++)
                            {
                                int derived_from_clock_edge;
                                instance = signal->data.output.combinatorial_ref->instance_iter->children[i];
                                model->reference_set_iterate_start( &signal->data.output.clocks_derived_from, &iter ); // For every clock that the prototype says the output is derived from, map back to clock name, go to top of clock gate tree, and say that generates it
                                derived_from_clock_edge=0;
                                while ((reference = model->reference_set_iterate(&iter))!=NULL)
                                {
                                    t_md_signal *clk2;
                                    clk2 = reference->data.signal;
                                    if ((clk2->data.clock.root_clock_ref==clk) && (edge==reference->edge))
                                    {
                                        derived_from_clock_edge=1;
                                    }
                                }
                                if (derived_from_clock_edge)
                                {
                                    output( handle, 1, "%d,\n", instance->output_args[0]);
                                }
                            }
                        }
                        if (signal->data.output.net_ref)
                        {
                            for (i=0; i<signal->data.output.net_ref->instance_iter->number_children; i++)
                            {
                                int derived_from_clock_edge;
                                instance = signal->data.output.net_ref->instance_iter->children[i];
                                model->reference_set_iterate_start( &signal->data.output.clocks_derived_from, &iter ); // For every clock that the prototype says the output is derived from, map back to clock name, go to top of clock gate tree, and say that generates it
                                derived_from_clock_edge=0;
                                while ((reference = model->reference_set_iterate(&iter))!=NULL)
                                {
                                    t_md_signal *clk2;
                                    clk2 = reference->data.signal;
                                    if ((clk2->data.clock.root_clock_ref==clk) && (edge==reference->edge))
                                    {
                                        derived_from_clock_edge = 1;
                                    }
                                }
                                if (derived_from_clock_edge)
                                {
                                    output( handle, 1, "%d,\n", instance->output_args[0]);
                                }
                            }
                        }
                    }
                    output( handle, 1, "-1\n");
                    output( handle, 0, "};\n");
                    output( handle, 0, "\n");
                }
                output( handle, 0, "/*v clock_desc_%s_%s\n", module->output_name, clk->name );
                output( handle, 0, "*/\n");
                output( handle, 0, "static t_clock_desc clock_desc_%s_%s = \n", module->output_name, clk->name );
                output( handle, 0, "{\n");
                output( handle, 1, "\"%s\",\n",clk->name);
                output( handle, 1, "clock_desc_%s_posedge_%s_inputs,\n", module->output_name, clk->name);
                output( handle, 1, "clock_desc_%s_negedge_%s_inputs,\n", module->output_name, clk->name);
                output( handle, 1, "clock_desc_%s_posedge_%s_outputs,\n", module->output_name, clk->name);
                output( handle, 1, "clock_desc_%s_negedge_%s_outputs,\n", module->output_name, clk->name);
                output( handle, 0, "};\n");
            }
        }
    }

    /*b Output combined clock descriptor list
     */
    output( handle, 0, "/*v clock_desc_%s\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "static t_clock_desc *clock_desc_%s[] = \n", module->output_name );
    output( handle, 0, "{\n");
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        if (!clk->data.clock.clock_ref) // Not a gated clock, i.e. a root clock
        {
            if (clk->data.clock.edges_used[1] || clk->data.clock.edges_used[0]) // 0 is posedge
            {
                output( handle, 1, "&clock_desc_%s_%s,\n", module->output_name, clk->name );
            }
        }
    }
    output( handle, 1, "NULL\n");
    output( handle, 0, "};\n\n");

    /*b Output module descriptor
     */
    output( handle, 0, "/*v module_desc_%s\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "static t_module_desc module_desc_%s = \n", module->output_name );
    output( handle, 0, "{\n");
    if (module->inputs)
    {
        output( handle, 1, "input_desc_%s,\n" , module->output_name );
    }
    else
    {
        output( handle, 1, "NULL,\n" );
    }
    if (module->outputs)
    {
        output( handle, 1, "output_desc_%s,\n", module->output_name );
    }
    else
    {
        output( handle, 1, "NULL,\n" );
    }
    output( handle, 1, "clock_desc_%s,\n" , module->output_name );
    output( handle, 0, "};\n");

    /*b Output logging
     */
    module->output.total_log_args = 0;
    if (1) // include_log)
    {
        t_md_statement *stmt;
        int num_args;
        int i, j;
        i=0;
        num_args=0;
        for (stmt=module->statements; stmt; stmt=stmt->next_in_module)
        {
            if (stmt->type==md_statement_type_log)
            {
                t_md_labelled_expression *arg;
                stmt->data.log.id_within_module = i;
                stmt->data.log.arg_id_within_module = num_args;
                for (j=0, arg=stmt->data.log.arguments; arg; arg=arg->next_in_chain, j++);
                if (i==0)
                {
                    output( handle, 0, "\n/*v %s_log_event_descriptor\n", module->output_name );
                    output( handle, 0, "*/\n");
                    output( handle, 0, "static t_engine_text_value_pair %s_log_event_descriptor[] = \n", module->output_name );
                    output( handle, 0, "{\n");
                }
                output( handle, 0, "{\"%s\", %d},\n", stmt->data.log.message, j );
                for (j=0, arg=stmt->data.log.arguments; arg; arg=arg->next_in_chain, j++)
                {
                    output( handle, 0, "{\"%s\", %d},\n", arg->text, j+num_args ); // Consumes log_signal_data[num_args -> num_args+j-1]
                }
                num_args+=j;
                i++;
            }
        }
        if (i!=0)
        {
            output( handle, 0, "{NULL, 0},\n" );
            output( handle, 0, "};\n\n" );
        }
        module->output.total_log_args = num_args;
    }

    /*b Output state desc nets structures if required
     */
    if (module->nets)
    {
        int has_indirect;
        int has_direct;
        has_indirect = has_direct = 0;
        for (signal=module->nets; signal; signal=signal->next_in_list)
        {
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                if (signal->instance_iter->children[i]->vector_driven_in_parts)
                {
                    has_direct=1;
                }
                else
                {
                    has_indirect=1;
                }
            }
        }

        if (has_indirect)
        {
            output( handle, 0, "/*v state_desc_%s_nets\n", module->output_name );
            output( handle, 0, "*/\n");
            output( handle, 0, "static t_%s_nets *___%s_indirect_net__ptr;\n", module->output_name, module->output_name );
            output( handle, 0, "static t_engine_state_desc state_desc_%s_indirect_nets[] =\n", module->output_name );
            output( handle, 0, "{\n");
            for (signal=module->nets; signal; signal=signal->next_in_list)
            {
                for (i=0; i<signal->instance_iter->number_children; i++)
                {
                    if (!signal->instance_iter->children[i]->vector_driven_in_parts)
                    {
                        instance = signal->instance_iter->children[i];
                        switch (instance->type)
                        {
                        case md_type_instance_type_bit_vector:
                            output( handle, 1, "{\"%s\",engine_state_desc_type_bits, NULL, struct_offset(___%s_indirect_net__ptr, %s), {%d,0,0,0}, {NULL,NULL,NULL,NULL} },\n", instance->output_name, module->output_name, instance->output_name, instance->type_def.data.width );
                            break;
                        case md_type_instance_type_array_of_bit_vectors:
                            output( handle, 1, "{\"%s\",engine_state_desc_type_memory, NULL, struct_offset(___%s_indirect_net__ptr, %s), {%d,%d,0,0}, {NULL,NULL,NULL,NULL} },\n", instance->output_name, module->output_name, instance->output_name, instance->type_def.data.width, instance->size );
                            break;
                        default:
                            output( handle, 1, "<NO TYPE FOR STRUCTURES>\n");
                            break;
                        }
                    }
                }
            }
            output( handle, 1, "{\"\", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }\n");
            output( handle, 0, "};\n\n");
        }
        if (has_direct)
        {
            output( handle, 0, "/*v state_desc_%s_nets\n", module->output_name );
            output( handle, 0, "*/\n");
            output( handle, 0, "static t_%s_nets *___%s_direct_net__ptr;\n", module->output_name, module->output_name );
            output( handle, 0, "static t_engine_state_desc state_desc_%s_direct_nets[] =\n", module->output_name );
            output( handle, 0, "{\n");
            for (signal=module->nets; signal; signal=signal->next_in_list)
            {
                for (i=0; i<signal->instance_iter->number_children; i++)
                {
                    if (signal->instance_iter->children[i]->vector_driven_in_parts)
                    {
                        instance = signal->instance_iter->children[i];
                        switch (instance->type)
                        {
                        case md_type_instance_type_bit_vector:
                            output( handle, 1, "{\"%s\",engine_state_desc_type_bits, NULL, struct_offset(___%s_direct_net__ptr, %s), {%d,0,0,0}, {NULL,NULL,NULL,NULL} },\n", instance->output_name, module->output_name, instance->output_name, instance->type_def.data.width );
                            break;
                        case md_type_instance_type_array_of_bit_vectors:
                            output( handle, 1, "{\"%s\",engine_state_desc_type_memory, NULL, struct_offset(___%s_direct_net__ptr, %s), {%d,%d,0,0}, {NULL,NULL,NULL,NULL} },\n", instance->output_name, module->output_name, instance->output_name, instance->type_def.data.width, instance->size );
                            break;
                        default:
                            output( handle, 1, "<NO TYPE FOR STRUCTURES>\n");
                            break;
                        }
                    }
                }
            }
            output( handle, 1, "{\"\", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }\n");
            output( handle, 0, "};\n\n");
        }
    }

    /*b Output state desc combinatorials structure if required
     */
    if (module->combinatorials)
    {
        output( handle, 0, "/*v state_desc_%s_combs\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "static t_%s_combinatorials *___%s_comb__ptr;\n", module->output_name, module->output_name );
        output( handle, 0, "static t_engine_state_desc state_desc_%s_combs[] =\n", module->output_name );
        output( handle, 0, "{\n");
        for (signal=module->combinatorials; signal; signal=signal->next_in_list)
        {
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                instance = signal->instance_iter->children[i];
                switch (instance->type)
                {
                case md_type_instance_type_bit_vector:
                    output( handle, 1, "{\"%s\",engine_state_desc_type_bits, NULL, struct_offset(___%s_comb__ptr, %s), {%d,0,0,0}, {NULL,NULL,NULL,NULL} },\n", instance->output_name, module->output_name, instance->output_name, instance->type_def.data.width );
                    break;
                case md_type_instance_type_array_of_bit_vectors:
                    output( handle, 1, "{\"%s\",engine_state_desc_type_memory, NULL, struct_offset(___%s_comb__ptr, %s), {%d,%d,0,0}, {NULL,NULL,NULL,NULL} },\n", instance->output_name, module->output_name, instance->output_name, instance->type_def.data.width, instance->size );
                    break;
                default:
                    output( handle, 1, "<NO TYPE FOR STRUCTURES>\n");
                    break;
                }
            }
        }
        output( handle, 1, "{\"\", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }\n");
        output( handle, 0, "};\n\n");
    }

    /*b Output state desc structure for each clock, if required
     */
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        int header_done;
        for (edge=0; edge<2; edge++)
        {
            if (clk->data.clock.edges_used[edge])
            {
                header_done = 0;
                for (reg=module->registers; reg; reg=reg->next_in_list)
                {
                    if ( (reg->clock_ref==clk) && (reg->edge==edge) )
                    {
                        if (!header_done)
                        {
                            output( handle, 0, "/*v state_desc_%s_%s_%s\n", module->output_name, edge_name[edge], clk->name );
                            output( handle, 0, "*/\n");
                            output( handle, 0, "static t_%s_%s_%s_state *___%s_%s_%s__ptr;\n", module->output_name, edge_name[edge], clk->name, module->output_name, edge_name[edge], clk->name );
                            output( handle, 0, "static t_engine_state_desc state_desc_%s_%s_%s[] =\n", module->output_name, edge_name[edge], clk->name );
                            output( handle, 0, "{\n");
                            header_done = 1;
                        }
                        for (i=0; i<reg->instance_iter->number_children; i++)
                        {
                            instance = reg->instance_iter->children[i];
                            switch (instance->type)
                            {
                            case md_type_instance_type_bit_vector:
                                output( handle, 1, "{\"%s\",engine_state_desc_type_bits, NULL, struct_offset(___%s_%s_%s__ptr, %s), {%d,0,0,0}, {NULL,NULL,NULL,NULL} },\n", instance->output_name, module->output_name, edge_name[edge], clk->name, instance->output_name, instance->type_def.data.width );
                                break;
                            case md_type_instance_type_array_of_bit_vectors:
                                output( handle, 1, "{\"%s\",engine_state_desc_type_memory, NULL, struct_offset(___%s_%s_%s__ptr, %s), {%d,%d,0,0}, {NULL,NULL,NULL,NULL} },\n", instance->output_name, module->output_name, edge_name[edge], clk->name, instance->output_name, instance->type_def.data.width, instance->size );
                                break;
                            default:
                                output( handle, 1, "<NO TYPE FOR STRUCTURES>\n");
                                break;
                            }
                        }
                    }
                }
                if (header_done)
                {
                    output( handle, 1, "{\"\", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }\n");
                    output( handle, 0, "};\n");
                    output( handle, 0, "\n");
                }
            }
        }
    }

    /*b All done
     */
}

/*f output_wrapper_functions
 */
static void output_wrapper_functions( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle )
{
    int edge;
    t_md_signal *clk;

    output( handle, 0, "/*a Static wrapper functions for %s\n", module->output_name);
    output( handle, 0, "*/\n");
    output( handle, 0, "/*f %s_instance_fn\n", module->output_name);
    output( handle, 0, "*/\n");
    output( handle, 0, "static t_sl_error_level %s_instance_fn( c_engine *engine, void *engine_handle )\n", module->output_name);
    output( handle, 0, "{\n");
    output( handle, 1, "c_%s *mod;\n", module->output_name );
    output( handle, 1, "mod = new c_%s( engine, engine_handle );\n", module->output_name);
    output( handle, 1, "if (!mod)\n");
    output( handle, 2, "return error_level_fatal;\n");
    output( handle, 1, "return error_level_okay;\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");
    output( handle, 0, "/*f %s_delete_fn - simple callback wrapper for the main method\n", module->output_name);
    output( handle, 0, "*/\n");
    output( handle, 0, "static t_sl_error_level %s_delete_fn( void *handle )\n", module->output_name);
    output( handle, 0, "{\n");
    output( handle, 1, "c_%s *mod;\n", module->output_name);
    output( handle, 1, "t_sl_error_level result;\n");
    output( handle, 1, "mod = (c_%s *)handle;\n", module->output_name);
    output( handle, 1, "result = mod->delete_instance();\n");
    output( handle, 1, "delete( mod );\n");
    output( handle, 1, "return result;\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");
    output( handle, 0, "/*f %s_reset_fn\n", module->output_name);
    output( handle, 0, "*/\n");
    output( handle, 0, "static t_sl_error_level %s_reset_fn( void *handle, int pass )\n", module->output_name);
    output( handle, 0, "{\n");
    output( handle, 1, "c_%s *mod;\n", module->output_name);
    output( handle, 1, "mod = (c_%s *)handle;\n", module->output_name);
    output( handle, 1, "return mod->reset( pass );\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");

    if ( module->combinatorial_component )
    {
        output( handle, 0, "/*f %s_combinatorial_fn\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "static t_sl_error_level %s_combinatorial_fn( void *handle )\n", module->output_name );
        output( handle, 0, "{\n");
        output( handle, 1, "c_%s *mod;\n", module->output_name);
        output( handle, 1, "mod = (c_%s *)handle;\n", module->output_name);
        output( handle, 1, "mod->capture_inputs();\n" );
        output( handle, 1, "return mod->comb();\n" );
        output( handle, 0, "}\n");
        output( handle, 0, "\n");
    }

    output( handle, 0, "/*f %s_propagate_fn\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "static t_sl_error_level %s_propagate_fn( void *handle )\n", module->output_name );
    output( handle, 0, "{\n");
    output( handle, 1, "c_%s *mod;\n", module->output_name);
    output( handle, 1, "mod = (c_%s *)handle;\n", module->output_name);
    output( handle, 1, "mod->capture_inputs();\n" );
    output( handle, 1, "return mod->propagate_all();\n" );
    output( handle, 0, "}\n");
    output( handle, 0, "\n");

    if (module->clocks)
    {
        output( handle, 0, "/*f %s_prepreclock_fn\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "static t_sl_error_level %s_prepreclock_fn( void *handle )\n", module->output_name );
        output( handle, 0, "{\n");
        output( handle, 1, "c_%s *mod;\n", module->output_name);
        output( handle, 1, "mod = (c_%s *)handle;\n", module->output_name);
        output( handle, 1, "return mod->prepreclock();\n" );
        output( handle, 0, "}\n");
        output( handle, 0, "\n");

        output( handle, 0, "/*f %s_clock_fn\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "static t_sl_error_level %s_clock_fn( void *handle )\n", module->output_name );
        output( handle, 0, "{\n");
        output( handle, 1, "c_%s *mod;\n", module->output_name);
        output( handle, 1, "mod = (c_%s *)handle;\n", module->output_name);
        output( handle, 1, "return mod->clock();\n" );
        output( handle, 0, "}\n");
        output( handle, 0, "\n");
    }

    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        if (!clk->data.clock.clock_ref) // Not a gated clock - they do not need static functions
        {
            for (edge=0; edge<2; edge++)
            {
                if (clk->data.clock.edges_used[edge])
                {
                    output( handle, 0, "/*f %s_preclock_%s_%s_fn\n", module->output_name, edge_name[edge], clk->name );
                    output( handle, 0, "*/\n");
                    output( handle, 0, "static t_sl_error_level %s_preclock_%s_%s_fn( void *handle )\n", module->output_name, edge_name[edge], clk->name );
                    output( handle, 0, "{\n");
                    output( handle, 1, "c_%s *mod;\n", module->output_name);
                    output( handle, 1, "mod = (c_%s *)handle;\n", module->output_name);
                    output( handle, 1, "mod->preclock( &c_%s::preclock_%s_%s, &c_%s::clock_%s_%s );\n",
                            module->output_name, edge_name[edge], clk->name,
                            module->output_name, edge_name[edge], clk->name );
                    output( handle, 1, "return error_level_okay;\n" );
                    output( handle, 0, "}\n");
                    output( handle, 0, "\n");
                }
            }
        }
    }
}

/*f output_constructors_destructors
 */
static void output_constructors_destructors( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle, int include_coverage, int include_stmt_coverage )
{
    int edge;
    t_md_signal *clk;
    t_md_signal *signal;
    t_md_state *reg;
    t_md_type_instance *instance;
    t_md_reference_iter iter;
    t_md_reference *reference;
    t_md_module_instance *module_instance;
    int i;

    /*b Header
     */
    output( handle, 0, "/*a Constructors and destructors for %s\n", module->output_name);
    output( handle, 0, "*/\n");

    /*b Constructor header
     */
    output( handle, 0, "/*f c_%s::c_%s\n", module->output_name, module->output_name);
    output( handle, 0, "*/\n");
    output( handle, 0, "c_%s::c_%s( class c_engine *eng, void *eng_handle )\n", module->output_name, module->output_name);
    output( handle, 0, "{\n");
    output( handle, 1, "engine = eng;\n");
    output( handle, 1, "engine_handle = eng_handle;\n");
    output( handle, 0, "\n");
    output( handle, 1, "engine->register_delete_function( engine_handle, (void *)this, %s_delete_fn );\n", module->output_name);
    output( handle, 1, "engine->register_reset_function( engine_handle, (void *)this, %s_reset_fn );\n", module->output_name);
    output( handle, 0, "\n");

    /*b Clear all state and combinatorial data in case of use of unused data elsewhere
     */
    if (1)
    {
        output( handle, 1, "memset(&inputs, 0, sizeof(inputs));\n" ); 
        output( handle, 1, "memset(&input_state, 0, sizeof(input_state));\n" ); 
        output( handle, 1, "memset(&combinatorials, 0, sizeof(combinatorials));\n" ); 
        output( handle, 1, "memset(&nets, 0, sizeof(nets));\n" ); 
        for (clk=module->clocks; clk; clk=clk->next_in_list)
        {
            for (edge=0; edge<2; edge++)
            {
                if (clk->data.clock.edges_used[edge])
                {
                    output( handle, 1, "memset(&next_%s_%s_state, 0, sizeof(next_%s_%s_state));\n", edge_name[edge], clk->name, edge_name[edge], clk->name );
                    output( handle, 1, "memset(&%s_%s_state, 0, sizeof(%s_%s_state));\n", edge_name[edge], clk->name, edge_name[edge], clk->name );
                }
            }
        }
        output( handle, 0, "\n");
    }

    /*b Register combinatorial, input propagation and clock/preclock functions
     */
    if (module->combinatorial_component) // If there is a combinatorial component to this module, then propagate_inputs will work wonders
    {
        output( handle, 1, "engine->register_comb_fn( engine_handle, (void *)this, %s_combinatorial_fn );\n", module->output_name );
    }
    output( handle, 1, "engine->register_propagate_fn( engine_handle, (void *)this, %s_propagate_fn );\n", module->output_name );
    if (module->clocks)
    {
        output( handle, 1, "engine->register_prepreclock_fn( engine_handle, (void *)this, %s_prepreclock_fn );\n", module->output_name );
    }
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        if (!clk->data.clock.clock_ref) // Not a gated clock
        {
            if (clk->data.clock.edges_used[1])
            {
                if (clk->data.clock.edges_used[0])
                {
                    output( handle, 1, "engine->register_preclock_fns( engine_handle, (void *)this, \"%s\", %s_preclock_%s_%s_fn, %s_preclock_%s_%s_fn );\n", clk->name,
                            module->output_name, edge_name[0], clk->name, 
                            module->output_name, edge_name[1], clk->name );
                    output( handle, 1, "engine->register_clock_fn( engine_handle, (void *)this, \"%s\", engine_sim_function_type_posedge_clock, %s_clock_fn );\n", clk->name, module->output_name );
                    output( handle, 1, "engine->register_clock_fn( engine_handle, (void *)this, \"%s\", engine_sim_function_type_negedge_clock, %s_clock_fn );\n", clk->name, module->output_name );
                }
                else
                {
                    output( handle, 1, "engine->register_preclock_fns( engine_handle, (void *)this, \"%s\", (t_engine_callback_fn) NULL, %s_preclock_%s_%s_fn );\n", clk->name,
                            module->output_name, edge_name[1], clk->name );
                    output( handle, 1, "engine->register_clock_fn( engine_handle, (void *)this, \"%s\", engine_sim_function_type_negedge_clock, %s_clock_fn );\n", clk->name, module->output_name );
                }
            }
            else
            {
                if (clk->data.clock.edges_used[0])
                {
                    output( handle, 1, "engine->register_preclock_fns( engine_handle, (void *)this, \"%s\", %s_preclock_%s_%s_fn, (t_engine_callback_fn) NULL );\n", clk->name,
                            module->output_name, edge_name[0], clk->name );
                    output( handle, 1, "engine->register_clock_fn( engine_handle, (void *)this, \"%s\", engine_sim_function_type_posedge_clock, %s_clock_fn );\n", clk->name, module->output_name );
                }
            }
        }
    }
    output( handle, 0, "\n");

    /*b Register inputs and outputs and clocks they depend on
     */
    output( handle, 1, "module_declaration( engine, engine_handle, (void *)this, &module_desc_%s );\n", module->output_name);

    /*b Instantiate submodules and get their handles
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        if (module_instance->module_definition)
        {
            output( handle, 1, "engine->instantiate( engine_handle, \"%s\", \"%s\", NULL );\n", module_instance->output_type, module_instance->name );
            output( handle, 1, "instance_%s.handle = engine->submodule_get_handle( engine_handle, \"%s\" );\n", module_instance->name, module_instance->name);
            for (clk=module_instance->module_definition->clocks; clk; clk=clk->next_in_list)
            {
                output( handle, 1, "instance_%s.%s__clock_handle = engine->submodule_get_clock_handle( instance_%s.handle, \"%s\" );\n", module_instance->name, clk->name, module_instance->name, clk->name );
            }
        }
    }
    output( handle, 0, "\n");

    /*b Submodule clock worklist
     */
    if (module->number_submodule_clock_calls>0)
    {
        t_md_module_instance_clock_port *clock_port;
        output( handle, 1, "engine->submodule_init_clock_worklist( engine_handle, %d );\n", module->number_submodule_clock_calls );

        for (clk=module->clocks; clk; clk=clk->next_in_list)
        {
            for (edge=0; edge<2; edge++)
            {
                if (clk->data.clock.edges_used[edge])
                {
                    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
                    {
                        if (model->reference_set_includes( &module_instance->dependencies, clk, edge ))
                        {
                            for (clock_port=module_instance->clocks; clock_port; clock_port=clock_port->next_in_list)
                            {
                                if (clock_port->local_clock_signal==clk)
                                {
                                    int worklist_item;
                                    worklist_item = output_markers_value( clock_port, -1 );
                                    if (output_markers_value(module_instance,om_valid_mask)!=om_valid)
                                    {
                                        output( handle, 1, "engine->submodule_set_clock_worklist_prepreclock( engine_handle, %d, instance_%s.handle );\n", worklist_item, module_instance->name );
                                        output_markers_mask(module_instance,om_valid,0);
                                    }
                                    output( handle, 1, "engine->submodule_set_clock_worklist_clock( engine_handle, instance_%s.handle, %d, instance_%s.%s__clock_handle, %d );\n", // Do not use a gate for the enable, as the parent will enable it
                                            module_instance->name,
                                            worklist_item,
                                            module_instance->name, clock_port->port_name, edge==md_edge_pos );
                                }
                            }
                        }
                    }
                }
            }
        }
        output( handle, 0, "\n" );
    }

    /*b Tie instance inputs and outputs
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        if (module_instance->module_definition)
        {
            output( handle, 1, "instantiation_wire_ports( engine, engine_handle, (void *)this, \"%s\", \"%s\", instance_%s.handle, instantiation_desc_%s_%s );\n" , module->output_name, module_instance->name, module_instance->name, module->output_name, module_instance->name );
        }
    }
    output( handle, 0, "\n");

    /*b Register state descriptors
     */
    if (module->nets)
    {
        int has_indirect;
        int has_direct;
        has_indirect = has_direct = 0;
        for (signal=module->nets; signal; signal=signal->next_in_list)
        {
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                if (signal->instance_iter->children[i]->vector_driven_in_parts)
                {
                    has_direct=1;
                }
                else
                {
                    has_indirect=1;
                }
            }
        }
        if (has_direct)
        {
            output( handle, 1, "engine->register_state_desc( engine_handle, 1, state_desc_%s_direct_nets, &nets, NULL );\n", module->output_name );
        }
        if (has_indirect)
        {
            output( handle, 1, "engine->register_state_desc( engine_handle, 3, state_desc_%s_indirect_nets, &nets, NULL );\n", module->output_name );
        }
    }
    if (module->combinatorials)
    {
        output( handle, 1, "engine->register_state_desc( engine_handle, 1, state_desc_%s_combs, &combinatorials, NULL );\n", module->output_name );
    }
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        for (edge=0; edge<2; edge++)
        {
            if (clk->data.clock.edges_used[edge])
            {
                for (reg=module->registers; reg; reg=reg->next_in_list)
                {
                    if ( (reg->clock_ref==clk) && (reg->edge==edge) )
                    {
                        output( handle, 1, "engine->register_state_desc( engine_handle, 1, state_desc_%s_%s_%s, &%s_%s_state, NULL );\n", module->output_name, edge_name[edge], clk->name, edge_name[edge], clk->name );
                        break;
                    }
                }
            }
        }
    }

    /*b Register for coverage
     */
    if (include_coverage)
    {
        output( handle, 1, "se_cmodel_assist_code_coverage_register( engine, engine_handle, code_coverage );\n" );
        output( handle, 0, "\n");
    }
    if (include_stmt_coverage)
    {
        output( handle, 1, "se_cmodel_assist_stmt_coverage_register( engine, engine_handle, stmt_coverage );\n" );
        output( handle, 0, "\n");
    }

    /*b Logging declarations
     */
    if (module->output.total_log_args>0)
    {
        output( handle, 1, "log_signal_data = (t_se_signal_value *)malloc(sizeof(t_se_signal_value)*%d);\n", module->output.total_log_args );
        output( handle, 1, "log_event_array = engine->log_event_register_array( engine_handle, %s_log_event_descriptor, log_signal_data );\n", module->output_name );
    }
    output( handle, 0, "}\n");
    output( handle, 0, "\n");

    /*b Output destructor
     */
    output( handle, 0, "/*f c_%s::~c_%s\n", module->output_name, module->output_name);
    output( handle, 0, "*/\n");
    output( handle, 0, "c_%s::~c_%s()\n", module->output_name, module->output_name);
    output( handle, 0, "{\n");
    output( handle, 1, "delete_instance();\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");
    output( handle, 0, "/*f c_%s::delete_instance\n", module->output_name);
    output( handle, 0, "*/\n");
    output( handle, 0, "t_sl_error_level c_%s::delete_instance( void )\n", module->output_name);
    output( handle, 0, "{\n");
    output( handle, 1, "return error_level_okay;\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");
}

/*a Output simulation methods functions (to convert CDL model to C)
 */
/*f output_simulation_methods_lvar
  If in_expression is 0, then we are assigning so require 'next_state', and we don't insert the bit subscripting
  If in_expression is 1, then we can also add the bit subscripting
 */
static void output_simulation_methods_lvar( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_lvar *lvar, int main_indent, int sub_indent, int in_expression )
{
    int indirect=0;

    if (in_expression && (lvar->subscript_start.type != md_lvar_data_type_none))
    {
        output( handle, -1, "((" );
    }
    if (lvar->instance_type == md_lvar_instance_type_state)
    {
        t_md_state *state;
        state = lvar->instance_data.state;
        if (in_expression)
        {
            output( handle, -1, "%s_%s_state.%s", edge_name[state->edge], state->clock_ref->name, lvar->instance->output_name );
        }
        else
        {
            output( handle, -1, "next_%s_%s_state.%s", edge_name[state->edge], state->clock_ref->name, lvar->instance->output_name );
        }
    }
    else
    {
        t_md_signal *signal;
        signal = lvar->instance_data.signal;
        switch (signal->type)
        {
        case md_signal_type_input:
            output( handle, -1, "input_state.%s", lvar->instance->output_name );
            break;
        case md_signal_type_output:
            if (signal->data.output.combinatorial_ref)
            {
                output( handle, -1, "combinatorials.%s", lvar->instance->output_name );
            }
            else
            {
                output( handle, -1, "<unresolved output %s>", signal->name );
            }
            break;
        case md_signal_type_combinatorial:
            output( handle, -1, "combinatorials.%s", lvar->instance->output_name );
            break;
        case md_signal_type_net:
            output( handle, -1, "nets.%s", lvar->instance->output_name );
            if (!lvar->instance->vector_driven_in_parts)
            {
                indirect = 1;
            }
            break;
        default:
            output( handle, -1, "<clock signal in expression %s>", signal->name );
            break;
        }
    }
    if (lvar->index.type != md_lvar_data_type_none)
    {
        output( handle, -1, "[" );
        switch (lvar->index.type)
        {
        case md_lvar_data_type_integer:
            output( handle, -1, "%d", lvar->index.data.integer );
            break;
        case md_lvar_data_type_expression:
            output_simulation_methods_expression( model, output, handle, code_block, lvar->index.data.expression, main_indent, sub_indent+1 );
            break;
        default:
            break;
        }
        output( handle, -1, "]" );
    }
    if (indirect)
    {
        output( handle, -1, "[0]" );
    }
    if ((in_expression) && (lvar->subscript_start.type != md_lvar_data_type_none))
    {
        if (lvar->subscript_start.type == md_lvar_data_type_integer)
        {
            output( handle, -1, ">>%d)", lvar->subscript_start.data.integer );
        }
        else if (lvar->subscript_start.type == md_lvar_data_type_expression)
        {
            output( handle, -1, ">>(" );
            output_simulation_methods_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, main_indent, sub_indent+1 );
            output( handle, -1, "))" );
        }
        if (lvar->subscript_length.type != md_lvar_data_type_none)
        {
            output( handle, -1, "&%s/*%d*/)", bit_mask[lvar->subscript_length.data.integer],lvar->subscript_length.data.integer );
        }
        else
        {
            output( handle, -1, "&1)" );
        }
    }
}

/*f output_simulation_methods_expression
 */
static void output_simulation_methods_expression( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_expression *expr, int main_indent, int sub_indent )
{
    /*b If the expression is NULL then something went wrong...
     */
    if (!expr)
    {
        model->error->add_error( NULL, error_level_fatal, error_number_be_expression_width, error_id_be_backend_message,
                                 error_arg_type_malloc_string, "NULL expression encountered - bug in CDL",
                                 error_arg_type_none );
        output( handle, -1, "0 /*<NULL expression>*/" );
        return;
    }

    /*b Output code for the expression
     */
    if (expr->width>64)
    {
        model->error->add_error( NULL, error_level_fatal, error_number_be_expression_width, error_id_be_backend_message,
                                 error_arg_type_malloc_string, code_block ? (code_block->name) : ("<outside a code block, possibly in a reset>"),
                                 error_arg_type_none );
        output( handle, -1, "0 /* EXPRESSION GREATER THAN 64 BITS WIDE */" );
    }
    switch (expr->type)
    {
    case md_expr_type_value:
        output( handle, -1, "0x%llxULL", expr->data.value.value.value[0] );
        break;
    case md_expr_type_lvar:
        output_simulation_methods_lvar( model, output, handle, code_block, expr->data.lvar, main_indent, sub_indent, 1 );
        break;
    case md_expr_type_bundle:
        output( handle, -1, "(((" );
        output_simulation_methods_expression( model, output, handle, code_block, expr->data.bundle.a, main_indent, sub_indent+1 );
        output( handle, -1, ")<<%d) | (", expr->data.bundle.b->width );
        output_simulation_methods_expression( model, output, handle, code_block, expr->data.bundle.b, main_indent, sub_indent+1 );
        output( handle, -1, "))" );
        break;
    case md_expr_type_cast:
        if ( (expr->data.cast.expression) &&
             (expr->data.cast.expression->width < expr->width) &&
             (expr->data.cast.signed_cast) )
        {
            output( handle, -1, "(SIGNED LENGTHENING CASTS NOT IMPLEMENTED YET: %d to %d)", expr->data.cast.expression->width, expr->width );
        }
        else
        {
            output( handle, -1, "(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.cast.expression, main_indent, sub_indent+1 );
            output( handle, -1, ")&%s/*%d*/", (expr->width<0)?"!!!<0!!!":((expr->width>64)?"!!!!>64":bit_mask[expr->width]),expr->width );
        }
        break;
    case md_expr_type_fn:
        switch (expr->data.fn.fn)
        {
        case md_expr_fn_logical_not:
        case md_expr_fn_not:
        case md_expr_fn_neg:
            switch (expr->data.fn.fn)
            {
            case md_expr_fn_neg:
                output( handle, -1, "(-(" );
                break;
            case md_expr_fn_not:
                output( handle, -1, "(~(" );
                break;
            default:
                output( handle, -1, "(!(" );
            }
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1 );
            output( handle, -1, "))&%s", bit_mask[expr->width] );
            break;
        case md_expr_fn_add:
        case md_expr_fn_sub:
        case md_expr_fn_mult:
        case md_expr_fn_lsl:
        case md_expr_fn_lsr:
            output( handle, -1, "((" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1 );
            if (expr->data.fn.fn==md_expr_fn_sub)
                output( handle, -1, ")-(" );
            else if (expr->data.fn.fn==md_expr_fn_mult)
                output( handle, -1, ")*(" );
            else if (expr->data.fn.fn==md_expr_fn_lsl)
                output( handle, -1, ")<<(" );
            else if (expr->data.fn.fn==md_expr_fn_lsr)
                output( handle, -1, ")>>(" );
            else
                output( handle, -1, ")+(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent+1 );
            output( handle, -1, "))&%s", bit_mask[expr->width] );
            break;
        case md_expr_fn_and:
        case md_expr_fn_or:
        case md_expr_fn_xor:
        case md_expr_fn_bic:
        case md_expr_fn_logical_and:
        case md_expr_fn_logical_or:
        case md_expr_fn_eq:
        case md_expr_fn_neq:
        case md_expr_fn_ge:
        case md_expr_fn_gt:
        case md_expr_fn_le:
        case md_expr_fn_lt:
            output( handle, -1, "(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1 );
            if (expr->data.fn.fn==md_expr_fn_and)
                output( handle, -1, ")&(" );
            else if (expr->data.fn.fn==md_expr_fn_or)
                output( handle, -1, ")|(" );
            else if (expr->data.fn.fn==md_expr_fn_xor)
                output( handle, -1, ")^(" );
            else if (expr->data.fn.fn==md_expr_fn_bic)
                output( handle, -1, ")&~(" );
            else if (expr->data.fn.fn==md_expr_fn_logical_and)
                output( handle, -1, ")&&(" );
            else if (expr->data.fn.fn==md_expr_fn_logical_or)
                output( handle, -1, ")||(" );
            else if (expr->data.fn.fn==md_expr_fn_eq)
                output( handle, -1, ")==(" );
            else if (expr->data.fn.fn==md_expr_fn_ge)
                output( handle, -1, ")>=(" );
            else if (expr->data.fn.fn==md_expr_fn_le)
                output( handle, -1, ")<=(" );
            else if (expr->data.fn.fn==md_expr_fn_gt)
                output( handle, -1, ")>(" );
            else if (expr->data.fn.fn==md_expr_fn_lt)
                output( handle, -1, ")<(" );
            else
                output( handle, -1, ")!=(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent+1 );
            output( handle, -1, ")" );
            break;
        case md_expr_fn_query:
            output( handle, -1, "(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1 );
            output( handle, -1, ")?(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent+1 );
            output( handle, -1, "):(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[2], main_indent, sub_indent+1 );
            output( handle, -1, ")" );
            break;
        default:
            output( handle, -1, "<unknown expression function type %d>", expr->data.fn.fn );
            break;
        }
        break;
    default:
        output( handle, -1, "<unknown expression type %d>", expr->type );
        break;
    }
}

/*f output_simulation_methods_statement_if_else
 */
static void output_simulation_methods_statement_if_else( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance )
{
     if (statement->data.if_else.if_true)
     {
          output( handle, indent+1, "if (");
          output_simulation_methods_expression( model, output, handle, code_block, statement->data.if_else.expr, indent+1, 4 );
          output( handle, -1, ")\n");
          output( handle, indent+1, "{\n");
          output_simulation_methods_statement( model, output, handle, code_block, statement->data.if_else.if_true, indent+1, clock, edge, instance );
          output( handle, indent+1, "}\n");
          if  (statement->data.if_else.if_false)
          {
               output( handle, indent+1, "else\n");
               output( handle, indent+1, "{\n");
               output_simulation_methods_statement( model, output, handle, code_block, statement->data.if_else.if_false, indent+1, clock, edge, instance );
               output( handle, indent+1, "}\n");
          }
     }
     else if (statement->data.if_else.if_false)
     {
          output( handle, indent+1, "if (!(");
          output_simulation_methods_expression( model, output, handle, code_block, statement->data.if_else.expr, indent+1, 4 );
          output( handle, -1, "))\n");
          output( handle, indent+1, "{\n");
          output_simulation_methods_statement( model, output, handle, code_block, statement->data.if_else.if_false, indent+1, clock, edge, instance );
          output( handle, indent+1, "}\n");
     }
}

/*f output_simulation_methods_statement_parallel_switch
 */
static void output_simulation_methods_statement_parallel_switch( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance )
{
    if (statement->data.switch_stmt.all_static && statement->data.switch_stmt.all_unmasked)
    {
        t_md_switch_item *switem;
        int defaulted = 0;

        output( handle, indent+1, "switch (");
        output_simulation_methods_expression( model, output, handle, code_block, statement->data.switch_stmt.expr, indent+1, 4 );
        output( handle, -1, ")\n");
        output( handle, indent+1, "{\n");
        for (switem = statement->data.switch_stmt.items; switem; switem=switem->next_in_list)
        {
            int stmts_reqd;
            stmts_reqd = 1;
            if (switem->statement)
            {
                stmts_reqd = 0;
                t_md_statement *substmt;
                for (substmt=switem->statement; substmt && !stmts_reqd; substmt=substmt->next_in_code)
                {
                    if (clock && model->reference_set_includes( &substmt->effects, clock, edge ))
                        stmts_reqd = 1;
                    if (instance && model->reference_set_includes( &substmt->effects, instance ))
                        stmts_reqd = 1;
                }
            }

            if (switem->type == md_switch_item_type_static)
            {
                if (stmts_reqd || statement->data.switch_stmt.full || 1)
                {
                    output( handle, indent+1, "case 0x%x: // req %d\n", switem->data.value.value.value[0], stmts_reqd );
                    if (switem->statement)
                        output_simulation_methods_statement( model, output, handle, code_block, switem->statement, indent+1, clock, edge, instance );
                    output( handle, indent+2, "break;\n");
                }
            }
            else if (switem->type == md_switch_item_type_default)
            {
                if (stmts_reqd || statement->data.switch_stmt.full)
                {
                    output( handle, indent+1, "default: // req %d\n", stmts_reqd);
                    if (switem->statement)
                        output_simulation_methods_statement( model, output, handle, code_block, switem->statement, indent+1, clock, edge, instance );
                    output( handle, indent+2, "break;\n");
                    defaulted = 1;
                }
            }
            else
            {
                fprintf( stderr, "BUG - non static unmasked case item in parallel static unmasked switch C output\n" );
            }
        }
        if (!defaulted && statement->data.switch_stmt.full)
        {
            char buffer[512];
            model->client_string_from_reference_fn( model->client_handle,
                                                    statement->client_ref.base_handle,
                                                    statement->client_ref.item_handle,
                                                    statement->client_ref.item_reference,
                                                    buffer,
                                                    sizeof(buffer),
                                                    md_client_string_type_human_readable );
            output( handle, indent+1, "default:\n");
            output( handle, indent+2, "ASSERT_STRING(\"%s:Full switch statement did not cover all values\");\n", buffer );
            output( handle, indent+2, "break;\n");
        }
        output( handle, indent+1, "}\n");
    }
    else
    {
        output( handle, indent, "!!!PARALLEL SWITCH ONLY WORKS NOW FOR STATIC UNMASKED EXPRESSIONS\n");
    }
}

/*f output_simulation_methods_port_net_assignment
 */
static void output_simulation_methods_port_net_assignment( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, int indent, t_md_module_instance *module_instance, t_md_lvar *lvar, t_md_type_instance *port_instance )
{
    switch (lvar->instance->type)
    {
    case md_type_instance_type_bit_vector:
    case md_type_instance_type_array_of_bit_vectors:
        if (1) // Was lvar->instance->size<=MD_BITS_PER_UINT64)
        {
            //printf("output_simulation_methods_port_net_assignment: %s.%s: type %d lvar %p\n", module_instance->name, port_instance->output_name, lvar->subscript_start.type, lvar );
            switch (lvar->subscript_start.type)
            {
            case md_lvar_data_type_none:
                output( handle, indent+1, "" );
                output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                output( handle, -1, " = instance_%s.outputs.%s[0];\n", module_instance->name, port_instance->output_name );
                break;
            case md_lvar_data_type_integer:
                if (lvar->subscript_length.type == md_lvar_data_type_none)
                {
                    output( handle, indent+1, "ASSIGN_TO_BIT( &( " );
                    output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                    output( handle, -1, "), %d, %d, ", lvar->instance->type_def.data.width, lvar->subscript_start.data.integer );
                }
                else
                {
                    output( handle, indent+1, "ASSIGN_TO_BIT_RANGE( &(" );
                    output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                    output( handle, -1, "), %d, %d, %d, ", lvar->instance->type_def.data.width, lvar->subscript_start.data.integer, lvar->subscript_length.data.integer );
                }
                output( handle, -1, "instance_%s.outputs.%s[0]", module_instance->name, port_instance->output_name );
                output( handle, -1, ");\n" );
                break;
            case md_lvar_data_type_expression:
                if (lvar->subscript_length.type == md_lvar_data_type_none)
                {
                    output( handle, indent+1, "ASSIGN_TO_BIT( &(");
                    output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                    output( handle, -1, "), %d, ", lvar->instance->type_def.data.width );
                    output_simulation_methods_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, indent+1, 0 );
                    output( handle, -1, ", " );
                }
                else
                {
                    output( handle, indent+1, "ASSIGN_TO_BIT_RANGE( &(" );
                    output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                    output( handle, -1, "), %d, ", lvar->instance->type_def.data.width );
                    output_simulation_methods_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, indent+1, 0 );
                    output( handle, -1, ", %d,", lvar->subscript_length.data.integer );
                }
                output( handle, -1, "instance_%s.outputs.%s[0]", module_instance->name, port_instance->output_name );
                output( handle, -1, ");\n" );
                break;
            }
        }
        else
        {
            output( handle, indent, "TYPES AND VALUES WIDER THAN 64 NOT IMPLEMENTED YET\n" );
        }
        break;
    case md_type_instance_type_structure:
        if (lvar->subscript_start.type != md_lvar_data_type_none)
        {
            output( handle, indent, "SUBSCRIPT INTO ASSIGNMENT DOES NOT WORK\n" );
        }
        output( handle, indent, "STRUCTURE ASSIGNMENT %p NOT IMPLEMENTED YET\n", lvar );
        break;
    }
}

/*f output_simulation_methods_assignment
 */
static void output_simulation_methods_assignment( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, int indent, t_md_lvar *lvar, int clocked, int wired_or, t_md_expression *expr )
{
    switch (lvar->instance->type)
    {
    case md_type_instance_type_bit_vector:
    case md_type_instance_type_array_of_bit_vectors:
        if (1) // Was lvar->instance->size<=MD_BITS_PER_UINT64)
        {
            switch (lvar->subscript_start.type)
            {
            case md_lvar_data_type_none:
                output( handle, indent+1, "" );
                output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                if (wired_or) // Only legal way back up for combinatorials
                    output( handle, -1, " |= " );
                else
                    output( handle, -1, " = " );
                output_simulation_methods_expression( model, output, handle, code_block, expr, indent+1, 0 );
                output( handle, -1, ";\n" );
                break;
            case md_lvar_data_type_integer:
                if (lvar->subscript_length.type == md_lvar_data_type_none)
                {
                    if (wired_or)
                        output( handle, indent+1, "WIRE_OR_TO_BIT( &( " );
                    else
                        output( handle, indent+1, "ASSIGN_TO_BIT( &( " );
                    output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                    output( handle, -1, "), %d, %d, ", lvar->instance->type_def.data.width, lvar->subscript_start.data.integer );
                }
                else
                {
                    if (wired_or)
                        output( handle, indent+1, "WIRE_OR_TO_BIT_RANGE( &( " );
                    else
                        output( handle, indent+1, "ASSIGN_TO_BIT_RANGE( &(" );
                    output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                    output( handle, -1, "), %d, %d, %d, ", lvar->instance->type_def.data.width, lvar->subscript_start.data.integer, lvar->subscript_length.data.integer );
                }
                output_simulation_methods_expression( model, output, handle, code_block, expr, indent+1, 0 );
                output( handle, -1, ");\n" );
                break;
            case md_lvar_data_type_expression:
                if (lvar->subscript_length.type == md_lvar_data_type_none)
                {
                    if (wired_or)
                        output( handle, indent+1, "WIRE_OR_TO_BIT( &( " );
                    else
                        output( handle, indent+1, "ASSIGN_TO_BIT( &(");
                    output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                    output( handle, -1, "), %d, ", lvar->instance->type_def.data.width );
                    output_simulation_methods_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, indent+1, 0 );
                    output( handle, -1, ", " );
                }
                else
                {
                    if (wired_or)
                        output( handle, indent+1, "WIRE_OR_TO_BIT_RANGE( &( " );
                    else
                        output( handle, indent+1, "ASSIGN_TO_BIT_RANGE( &(" );
                    output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent+1, 0 );
                    output( handle, -1, "), %d, ", lvar->instance->type_def.data.width );
                    output_simulation_methods_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, indent+1, 0 );
                    output( handle, -1, ", %d,", lvar->subscript_length.data.integer );
                }
                output_simulation_methods_expression( model, output, handle, code_block, expr, indent+1, 0 );
                output( handle, -1, ");\n" );
                break;
            }
        }
        else
        {
            output( handle, indent, "TYPES AND VALUES WIDER THAN 64 NOT IMPLEMENTED YET\n" );
        }
        break;
    case md_type_instance_type_structure:
        if (lvar->subscript_start.type != md_lvar_data_type_none)
        {
            output( handle, indent, "SUBSCRIPT INTO ASSIGNMENT DOES NOT WORK\n" );
        }
        output( handle, indent, "STRUCTURE ASSIGNMENT %p NOT IMPLEMENTED YET\n", lvar );
        break;
    }
}

/*f output_simulation_methods_display_message_to_buffer
 */
static void output_simulation_methods_display_message_to_buffer( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_message *message, const char *buffer_name )
{
    if (message)
    {
        int i;
        t_md_expression *expr;
        char buffer[512];
        model->client_string_from_reference_fn( model->client_handle,
                                                 statement->client_ref.base_handle,
                                                 statement->client_ref.item_handle,
                                                 statement->client_ref.item_reference,
                                                 buffer,
                                                 sizeof(buffer),
                                                 md_client_string_type_human_readable );
        for (i=0, expr=message->arguments; expr; i++, expr=expr->next_in_chain);
        output( handle, indent+1, "se_cmodel_assist_vsnprintf( %s, sizeof(%s), \"%s:%s\", %d \n", buffer_name, buffer_name, buffer, message->text, i );
        for (expr=message->arguments; expr; expr=expr->next_in_chain)
        {
            output( handle, indent+2, "," );
            output_simulation_methods_expression( model, output, handle, code_block, expr, indent+2, 0 );
            output( handle, -1, "\n" );
        }
        output( handle, indent+2, " );\n");
    }
}

/*f output_simulation_methods_statement_print_assert_cover
 */
static void output_simulation_methods_statement_print_assert_cover( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance )
{
    const char *string_type = (statement->type==md_statement_type_cover)?"COVER":"ASSERT";
    if ((!clock) && (!statement->data.print_assert_cover.statement))
    {
        return; // If there is no code to run, just a string to display, then only insert code for a clocked process, as the display is on the clock edge
    }
    /*b Handle cover expressions - Note that if cover has an expression, it does NOT have a statement, but it might have a message (though they are optional)
     */
    if ((statement->type==md_statement_type_cover) && statement->data.print_assert_cover.expression && !statement->data.print_assert_cover.statement)
    {
        t_md_expression *expr;
        int i;
        expr = statement->data.print_assert_cover.value_list;
        i = 0;
        do
        {
            output( handle, indent+1, "COVER_CASE_START(%d,%d) ", statement->data.print_assert_cover.cover_case_entry, i );
            output_simulation_methods_expression( model, output, handle, code_block, statement->data.print_assert_cover.expression, indent+2, 0 );
            if (expr)
            {
                output( handle, -1, "==" );
                output_simulation_methods_expression( model, output, handle, code_block, expr, indent+2, 0 );
                output( handle, -1, "\n" );
                expr = expr->next_in_chain;
            }
            output( handle, indent+2, "COVER_CASE_MESSAGE(%d,%d)\n", statement->data.print_assert_cover.cover_case_entry, i );
            output( handle, indent+2, "char buffer[512], buffer2[512];\n");
            output_simulation_methods_display_message_to_buffer( model, output, handle, code_block, statement, indent+2, statement->data.print_assert_cover.message, "buffer" );
            output( handle, indent+2, "snprintf( buffer2, sizeof(buffer2), \"Cover case entry %d subentry %d hit: %%s\", buffer);\n", statement->data.print_assert_cover.cover_case_entry, i );
            output( handle, indent+2, "COVER_STRING(buffer2)\n");
            output( handle, indent+2, "COVER_CASE_END\n" );
            i++;
        } while (expr);
        return;
    }

    /*b Handle assert expressions - either a message or a statement will follow
    */
    if (statement->data.print_assert_cover.expression)
    {
        output( handle, indent+1, "ASSERT_START " );
        if (statement->data.print_assert_cover.value_list)
        {
            t_md_expression *expr;
            expr = statement->data.print_assert_cover.value_list;
            while (expr)
            {
                output( handle, -1, "(" );
                output_simulation_methods_expression( model, output, handle, code_block, statement->data.print_assert_cover.expression, indent+2, 0 );
                output( handle, -1, ")==(" );
                output_simulation_methods_expression( model, output, handle, code_block, expr, indent+2, 0 );
                output( handle, -1, ")\n" );
                expr = expr->next_in_chain;
                if (expr)
                {
                    output( handle, indent+2, "ASSERT_COND_NEXT " );
                }
            }
        }
        else
        {
            output_simulation_methods_expression( model, output, handle, code_block, statement->data.print_assert_cover.expression, indent+2, 0 );
        }
        output( handle, indent+1, "ASSERT_COND_END\n" );
    }
    else
    {
        output( handle, indent+1, "%s_START_UNCOND ", string_type );
    }
    if (clock && statement->data.print_assert_cover.message)
    {
        output( handle, indent+2, "char buffer[512];\n");
        output_simulation_methods_display_message_to_buffer( model, output, handle, code_block, statement, indent+2, statement->data.print_assert_cover.message, "buffer" );
        if (statement->data.print_assert_cover.expression) // Assertions have an expression, prints do not
        {
            output( handle, indent+2, "ASSERT_STRING(buffer)\n");
        }
        else
        {
            output( handle, indent+2, "PRINT_STRING(buffer)\n");
        }
    }
    if (statement->data.print_assert_cover.statement)
    {
        output_simulation_methods_statement( model, output, handle, code_block, statement->data.print_assert_cover.statement, indent+1, clock, edge, instance );
    }
    output( handle, indent+1, "%s_END\n", string_type );
}

/*f output_simulation_methods_statement_log
 */
static void output_simulation_methods_statement_log( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance )
{
    t_md_labelled_expression *arg;
    int i;

    /*b At present, nothing to do unless clocked
     */
    if (!clock)
    {
        return;
    }
    /*b Set data for the occurrence from expressions, then hit the event
    */
    for (arg=statement->data.log.arguments, i=0; arg; arg=arg->next_in_chain, i++)
    {
        if (arg->expression)
        {
            output( handle, indent+1, "log_signal_data[%d] = ", statement->data.log.arg_id_within_module+i );
            output_simulation_methods_expression( model, output, handle, code_block, arg->expression, indent+2, 0 );
            output( handle, -1, ";\n" );
        }
    }
    /*b Invoke event occurrence
    */
    output( handle, indent+1, "engine->log_event_occurred( engine_handle, log_event_array, %d );\n", statement->data.log.id_within_module ); 
}

/*f output_simulation_methods_statement_clocked
 */
static void output_simulation_methods_statement_clocked( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge )
{
    if (!statement)
        return;

    /*b If the statement does not effect this clock/edge, then return with outputting nothing
     */
    if (!model->reference_set_includes( &statement->effects, clock, edge ))
    {
        output_simulation_methods_statement_clocked( model, output, handle, code_block, statement->next_in_code, indent, clock, edge );
        return;
    }

    /*b Increment coverage
     */
    output( handle, indent+1, "COVER_STATEMENT(%d);\n", statement->enumeration );

    /*b Display the statement
     */
    switch (statement->type)
    {
    case md_statement_type_state_assign:
    {
        output_simulation_methods_assignment( model, output, handle, code_block, indent, statement->data.state_assign.lvar, 1, 0, statement->data.state_assign.expr );
        break;
    }
    case md_statement_type_comb_assign:
        break;
    case md_statement_type_if_else:
        output_simulation_methods_statement_if_else( model, output, handle, code_block, statement, indent, clock, edge, NULL );
        break;
    case md_statement_type_parallel_switch:
        output_simulation_methods_statement_parallel_switch( model, output, handle, code_block, statement, indent, clock, edge, NULL );
        break;
    case md_statement_type_print_assert:
    case md_statement_type_cover:
        output_simulation_methods_statement_print_assert_cover( model, output, handle, code_block, statement, indent, clock, edge, NULL );
        break;
    case md_statement_type_log:
        output_simulation_methods_statement_log( model, output, handle, code_block, statement, indent, clock, edge, NULL );
        break;
    default:
        output( handle, 1, "Unknown statement type %d\n", statement->type );
        break;
    }
    output_simulation_methods_statement_clocked( model, output, handle, code_block, statement->next_in_code, indent, clock, edge );
}

/*f output_simulation_methods_statement_combinatorial
 */
static void output_simulation_methods_statement_combinatorial( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_type_instance *instance )
{
     if (!statement)
          return;

     /*b If the statement does not effect this signal instance, then return with outputting nothing
      */
     if (!model->reference_set_includes( &statement->effects, instance ))
     {
          output_simulation_methods_statement_combinatorial( model, output, handle, code_block, statement->next_in_code, indent, instance );
          return;
     }

     /*b Increment coverage
      */
     output( handle, indent+1, "COVER_STATEMENT(%d);\n", statement->enumeration );

     /*b Display the statement
      */
     switch (statement->type)
     {
     case md_statement_type_state_assign:
          break;
     case md_statement_type_comb_assign:
         output_simulation_methods_assignment( model, output, handle, code_block, indent, statement->data.comb_assign.lvar, 0, statement->data.comb_assign.wired_or, statement->data.comb_assign.expr );
        break;
     case md_statement_type_if_else:
          output_simulation_methods_statement_if_else( model, output, handle, code_block, statement, indent, NULL, -1, instance );
          break;
     case md_statement_type_parallel_switch:
          output_simulation_methods_statement_parallel_switch( model, output, handle, code_block, statement, indent, NULL, -1, instance );
          break;
    case md_statement_type_print_assert:
    case md_statement_type_cover:
        output_simulation_methods_statement_print_assert_cover( model, output, handle, code_block, statement, indent, NULL, -1, instance );
        break;
     case md_statement_type_log:
        output_simulation_methods_statement_log( model, output, handle, code_block, statement, indent, NULL, -1, instance );
        break;
     default:
          output( handle, 1, "Unknown statement type %d\n", statement->type );
          break;
     }
     output_simulation_methods_statement_combinatorial( model, output, handle, code_block, statement->next_in_code, indent, instance );
}

/*f output_simulation_methods_statement
 */
static void output_simulation_methods_statement( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance )
{
    if (clock)
    {
        output_simulation_methods_statement_clocked( model, output, handle, code_block, statement, indent, clock, edge );
    }
            else
    {
        output_simulation_methods_statement_combinatorial( model, output, handle, code_block, statement, indent, instance );
    }
}

/*f output_simulation_methods_code_block
 */
static void output_simulation_methods_code_block( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_signal *clock, int edge, t_md_type_instance *instance )
{
     /*b If the code block does not effect this signal/clock/edge, then return with outputting nothing
      */
     if (clock && !model->reference_set_includes( &code_block->effects, clock, edge ))
          return;
     if (instance && !model->reference_set_includes( &code_block->effects, instance ))
          return;

     /*b Insert comment for the block
      */
     if (instance)
         output( handle, 1, "/*b %s (%d): %s\n", instance->name, instance->output_args[0], code_block->name );
     else
          output( handle, 1, "/*b %s\n", code_block->name );
     output( handle, 1, "*/\n" );

     /*b Display each statement that effects state on the specified clock
      */
     output_simulation_methods_statement( model, output, handle, code_block, code_block->first_statement, 0, clock, edge, instance );

     /*b Close out with a blank line
      */
     output( handle, 0, "\n" );
}

/*f output_simulation_code_to_make_combinatorial_signals_valid
  Output combinatorial code and combinatorial submodule calls to make all combinatorials valid

  If combinatorials depend on nets from submodules that are clocked (and those nets are just clocked) then we have a problem
 */
static void output_simulation_code_to_make_combinatorial_signals_valid( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle )
{
    int more_to_output, did_output;
    t_md_signal *signal;
    t_md_type_instance *instance;
    int i;
    t_md_reference_iter iter;
    t_md_module_instance *module_instance;
    t_md_module_instance_input_port *input_port;
    t_md_module_instance_output_port *output_port;
    int first_output;

    /*b Mark all modules as to be output ONLY if at least one of their outputs are marked as invalid and in need of output
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        int need_to_output=0;
        for (output_port=module_instance->outputs; output_port && (!need_to_output); output_port=output_port->next_in_list)
        {
            //output( handle, 1, "//   Module output %s derived combinatorially %d\n", output_port->lvar->instance->name, output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially );
            if (output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially)
            {
                //output( handle, 1, "//   Module output %s has valid mask %d\n", output_port->lvar->instance->name,output_markers_value( output_port->lvar->instance, om_make_valid_mask ) );
                if (output_markers_value( output_port->lvar->instance, om_make_valid_mask )==(om_invalid | om_must_be_valid))
                {
                    output( handle, 1, "//   Module output %s needs to be made valid\n", output_port->lvar->instance->name );
                    need_to_output=1;
                }
            }
        }
        if (need_to_output)
        {
            output( handle, 1, "//   Module %s being marked as required combinatorially for some signals that depend on it\n", module_instance->name );
            output_markers_mask( module_instance, om_must_be_valid, -1 );
        }
        else
        {
            output( handle, 1, "//   Module %s not being marked as required combinatorially as no signals depend on it\n", module_instance->name );
            output_markers_mask( module_instance, 0, -1 );
        }
    }

    /*b Check all clocked module instances whose outputs are required to be valid and that and are not combinatorial
     */
    first_output = 1;
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        int need_to_output;

        /*b If module_instance is not clocked AND is combinatorial then skip to next
         */
        need_to_output=1;
        if (!module_instance->module_definition)
        {
            need_to_output=0;
        }
        if ( need_to_output &&
             (!module_instance->module_definition->clocks) &&
             (module_instance->module_definition->combinatorial_component) )
        {
            need_to_output=0;
        }
        if (!need_to_output) continue;

        /*b Find all purely clocked outputs and mark them valid
         */
        for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
        {
            int used_comb = output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially;
            if (!used_comb) // If it is completely clocked
            {
                if (output_port->lvar->instance->vector_driven_in_parts)
                {
                    if (first_output)
                    {
                        output( handle, 1, "/*b Assignments to nets driven in more than one part\n" );
                        output( handle, 1, "*/\n" );
                        first_output = 0;
                    }
                    output_simulation_methods_port_net_assignment( model, output, handle, NULL, 0, module_instance, output_port->lvar, output_port->module_port_instance );
                }
                //output( handle, 1, "//   Marking output %s . %s as valid as it is purely clocked %d\n", module_instance->name, output_port->lvar->instance->name, output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially );
                output_markers_mask( output_port->lvar->instance, om_must_be_valid|om_valid, 0 );
            }
        }

        /*b Next module instance
         */
    }
    if (!first_output)
        output( handle, 0, "\n" );

    /*b   Run through combinatorials and nets to see if we can output the code or module instances for one (are all its dependencies done?) - and repeat until we have nothing left
     */
    more_to_output = 1;
    did_output = 1;
    while (more_to_output && did_output)
    {
        int check_nets_driven_in_parts=0;

        /*b Mark us as done nothing yet
         */
        more_to_output = 0;
        did_output = 0;

        /*b Check for combinatorials that are required to be valid and that are ready
         */
        for (signal=module->combinatorials; signal; signal=signal->next_in_list)
        {
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                /*b If instance has been done, skip to next
                 */
                instance = signal->instance_iter->children[i];
                if (output_markers_value(instance,om_make_valid_mask) != (om_invalid | om_must_be_valid)) // Skip if the instance does not need to be made valid
                    continue;
                more_to_output=1;

                /*b Check dependencies (net and combinatorial) are all done
                 */
                //fprintf(stderr, "check dependencies of not-yet-done signal %s (%p)\n", instance->name, instance);
                model->reference_set_iterate_start( &instance->dependencies, &iter );
                if (output_markers_check_iter_any_match( model, &iter, (void *)instance, om_valid_mask, om_invalid))
                {
                    continue;
                }

                /*b All dependencies are valid; mark as valid too, and output the code
                 */
                did_output=1;
                output_markers_mask( instance, om_valid, 0 );

                if (!instance->code_block)
                {
                    output( handle, 1, "//No definition of signal %s (%p)\n", instance->name, instance);
                    //break;
                }
                else
                {
                    output_simulation_methods_code_block( model, output, handle, instance->code_block, NULL, -1, instance );
                }

                /*b Next instance
                 */
            }
        }

        /*b Check all combinatorial module instances whose outputs are required to be valid and that and are ready
         */
        for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
        {
            int need_to_output;

            /*b If module_instance is not combinatorial then skip to next
             */
            need_to_output=1;
            if ( (!module_instance->module_definition) ||
                 (!module_instance->module_definition->combinatorial_component) )
            {
                need_to_output=0;
            }
            if (!need_to_output)
            {
                output( handle, 1, "//   Not outputting module %s as it is external or not combinatorial\n", module_instance->name );
                continue;
            }

            /*b Check if the module is marked as needed to be output as it is marked as invalid
             */
            need_to_output=0;
            if (output_markers_value(module_instance, om_make_valid_mask)==(om_invalid|om_must_be_valid))
                need_to_output = 1;
            if (!need_to_output)
            {
                output( handle, 1, "//   Not outputting module %s as not marked as required\n", module_instance->name );
                continue;
            }
            more_to_output=1;

            /*b Check dependencies (net and combinatorial) for combinatorial function are all done
             */
            model->reference_set_iterate_start( &module_instance->combinatorial_dependencies, &iter );
            if (output_markers_check_iter_any_match( model, &iter, (void *)NULL, om_valid_mask, om_invalid))
            {
                output( handle, 1, "//   Not outputting module %s as dependencies are not all valid\n", module_instance->name );
                continue;
            }
        
            /*b Dependencies are all done; output the code, and mark output nets as done if not driven in parts
             */
            did_output=1;
            output_markers_mask( module_instance, om_valid, 0 );
            for (input_port=module_instance->inputs; input_port; input_port=input_port->next_in_list)
            {
                // if (input_port->module_port_instance->reference.data.signal->data.input.used_combinatorially)
                output( handle, 1, "instance_%s.inputs.%s = ", module_instance->name, input_port->module_port_instance->output_name );
                output_simulation_methods_expression( model, output, handle, NULL, input_port->expression, 2, 0 );
                output( handle, -1, ";\n" );
            }
            output( handle, 1, "engine->submodule_call_comb( instance_%s.handle );\n", module_instance->name );
            for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
            {
                if (output_port->lvar->instance->vector_driven_in_parts)
                {
                    // Should we only do this assignment if the output is derived combinatorially?
                    output_simulation_methods_port_net_assignment( model, output, handle, NULL, 1, module_instance, output_port->lvar, output_port->module_port_instance );
                    check_nets_driven_in_parts = 1;
                }
                else
                {
                    if (output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially)
                    {
                        output_markers_mask( output_port->lvar->instance, om_valid, 0 );
                    }
                }
            }

            /*b Next module instance
             */
        }

        /*b If nets driven in parts were driven valid (but not marked as such), check all such nets to see if they should be marked
         */
        if (check_nets_driven_in_parts)
        {
            for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
            {
                if (output_markers_value(module_instance, om_make_valid_mask)==(om_valid|om_must_be_valid))
                {
                    for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
                    {
                        if ( output_port->lvar->instance->vector_driven_in_parts &&
                             output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially &&
                             (output_markers_value(output_port->lvar->instance, om_make_valid_mask)==(om_invalid | om_must_be_valid)) )
                        {
                            if (output_markers_check_net_driven_in_parts_modules_all_match( module, output_port->lvar->instance, om_valid_mask, om_valid ))
                            {
                                output_markers_mask( output_port->lvar->instance, om_valid, 0 );
                            }
                        }
                    }
                }
            }
        }

        /*b Done outputing the code for that loop
         */
    }

    /*b Error if there is more to output but no output went out - this indicates circular dependencies
     */
    if (more_to_output)
    {
        char buffer[1024];

        /*b Error about combinatorials that have not been done and are ready
         */
        for (signal=module->combinatorials; signal; signal=signal->next_in_list)
        {
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                /*b If instance has been done or need not be done, skip
                 */
                instance = signal->instance_iter->children[i];
                if (output_markers_value(instance,om_make_valid_mask) != (om_invalid | om_must_be_valid))
                    continue;

                /*b Error
                 */
                if (1)
                {
                    t_md_type_instance *dependency;
                    output( handle, 1, "//Combinatorial %s has code whose dependents seem cyclic\n", instance->name );
                    snprintf(buffer,sizeof(buffer),"DESIGN ERROR: circular dependency found for %s (depends on...):", instance->name );
                    model->reference_set_iterate_start( &instance->dependencies, &iter );
                    while ((dependency=output_markers_find_iter_match( model, &iter, (void *)signal, om_valid_mask, om_invalid))!=NULL)
                    {
                        output( handle, 1, "//    dependent '%s' not ready (%d)\n", dependency->reference.data.signal->name, output_markers_value(dependency,-1) );
                        snprintf(buffer+strlen(buffer),sizeof(buffer)-strlen(buffer),"%s ", dependency->reference.data.signal->name );
                    }
                    buffer[sizeof(buffer)-1]=0;
                    model->error->add_error( module, error_level_fatal, error_number_sl_message, error_id_be_backend_message,
                                             error_arg_type_malloc_string, buffer,
                                             error_arg_type_malloc_filename, module->output_name,
                                             error_arg_type_none );
                }
            }
        }

        /*b Error about all module instances which have not been done
         */
        for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
        {
            /*b If module_instance is not combinatorial or has been done, skip to next
             */
//            output( handle, 1, "//Module instance %s has defn %p\n", module_instance->name, module_instance->module_definition );
//            output( handle, 1, "//Module instance %s is comb %d\n", module_instance->name, module_instance->module_definition->combinatorial_component );
//            output( handle, 1, "//Module instance %s has been output %d\n", module_instance->name, module_instance->output_args[0] );
            if ( (!module_instance->module_definition) ||
                 (!module_instance->module_definition->combinatorial_component) || 
                 (output_markers_value(module_instance, om_valid_mask)==om_valid) ||
                 (output_markers_value(module_instance, om_make_valid_mask)!=om_must_be_valid) )
            {
                continue;
            }

            /*b Warn
             */
            {
                t_md_type_instance *dependency;
                output( handle, 1, "//Module instance %s has dependents which seem cyclic\n", module_instance->name );
                model->reference_set_iterate_start( &module_instance->combinatorial_dependencies, &iter );
                while ((dependency=output_markers_find_iter_match( model, &iter, NULL, 0, 0))!=NULL)
                {
                    output( handle, 1, "//   Dependency %s has output state %d\n", dependency->name, output_markers_value( dependency, -1 ) );
                }

                snprintf(buffer,sizeof(buffer),"DESIGN ERROR: circular dependency found for module %s:", module_instance->name );
                model->error->add_error( module, error_level_info, error_number_sl_message, error_id_be_backend_message,
                                         error_arg_type_malloc_string, buffer,
                                         error_arg_type_malloc_filename, module->output_name,
                                         error_arg_type_none );
            }
        }

        /*b Errors all done
         */
    }
}

/*a Output C model methods
 */
/*f output_simulation_methods
 */
static void output_simulation_methods( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle )
{
    int edge, level;
    t_md_signal *clk;
    t_md_signal *signal;
    t_md_state *reg;
    t_md_code_block *code_block;
    t_md_type_instance *instance;
    t_md_module_instance *module_instance;
    t_md_module_instance_clock_port *clock_port;
    t_md_module_instance_input_port *input_port;
    int i;

    /*b Output the main reset function
     */
    output( handle, 0, "/*a Class reset/preclock/clock methods for %s\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "/*f c_%s::reset\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "t_sl_error_level c_%s::reset( int pass )\n", module->output_name );
    output( handle, 0, "{\n");

    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (level=0; level<2; level++)
        {
            if (signal->data.input.levels_used_for_reset[level])
            {
                output( handle, 1, "reset_%s_%s( );\n", level_name[level], signal->name );
            }
        }
    }
    output( handle, 1, "if (pass==0)\n" );
    output( handle, 1, "{\n" );
    if (module->inputs || module->nets)
    {
        output( handle, 2, "DEFINE_DUMMY_INPUT;\n");
        output( handle, 2, "t_sl_uint64 **ptr;\n");
    }
    if (module->inputs)
    {
        output( handle, 2, "int unconnected_inputs=0;\n");
        output( handle, 2, "ptr = (t_sl_uint64 **)(&inputs);\n");
        output( handle, 2, "for (int i=0; input_desc_%s[i].port_name; i++)\n", module->output_name);
        output( handle, 2, "{\n");
        output( handle, 3, "if (ptr[i]==NULL)\n");
        output( handle, 3, "{\n");
        output( handle, 4, "fprintf( stderr,\"POSSIBLY FATAL: Unconnected input port '%%s' on module '%%s' (of type %s) at initial reset\\n\", input_desc_%s[i].port_name, engine->get_instance_name(engine_handle));\n", module->output_name, module->output_name );
        output( handle, 4, "unconnected_inputs++;\n");
        output( handle, 4, "ptr[i]=&dummy_input[0];\n");
        output( handle, 3, "}\n");
        output( handle, 2, "}\n");
    }
    if (module->nets)
    {
        output( handle, 2, "int unconnected_nets=0;\n");
        output( handle, 2, "for (int i=0; net_desc_%s[i].port_name; i++)\n", module->output_name);
        output( handle, 2, "{\n");
        output( handle, 3, "int j=net_desc_%s[i].net_driver_offset;\n", module->output_name);
        output( handle, 3, "t_sl_uint64 **net_receiver_ptr=(t_sl_uint64 **)((((char *)(&nets))+j));\n");
        output( handle, 3, "if ((j>=0) && (net_receiver_ptr[0]==NULL))\n");
        output( handle, 3, "{\n");
        output( handle, 4, "fprintf( stderr,\"POSSIBLY FATAL: Undriven internal net '%%s' in module '%%s' (of type %s) at initial reset (did its driver module instantiate okay?)\\n\", input_desc_%s[i].port_name, engine->get_instance_name(engine_handle));\n", module->output_name, module->output_name );
        output( handle, 4, "unconnected_nets++;\n");
        output( handle, 4, "net_receiver_ptr[0]=&dummy_input[0];\n");
        output( handle, 3, "}\n");
        output( handle, 2, "}\n");
    }
    output( handle, 1, "}\n" );
    output( handle, 1, "if (pass>0) {capture_inputs(); propagate_all();} // Dont call capture_inputs on first pass as they may be invalid; wait for second pass\n");
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        output( handle, 1, "engine->submodule_call_reset( instance_%s.handle, pass );\n", module_instance->name );
        if (module_instance->module_definition)
        {
            t_md_module_instance_output_port *output_port;
            for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
            {
                if (output_port->lvar->instance->vector_driven_in_parts)
                {
                    output_simulation_methods_port_net_assignment( model, output, handle, NULL, 1, module_instance, output_port->lvar, output_port->module_port_instance );
                }
            }
        }
    }

    output( handle, 1, "return error_level_okay;\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");

    /*b Output the individual reset functions
     */
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (level=0; level<2; level++)
        {
            if (signal->data.input.levels_used_for_reset[level])
            {
                output( handle, 0, "/*f c_%s::reset_%s_%s\n", module->output_name, level_name[level], signal->name );
                output( handle, 0, "*/\n");
                output( handle, 0, "t_sl_error_level c_%s::reset_%s_%s( void )\n", module->output_name, level_name[level], signal->name );
                output( handle, 0, "{\n");

                for (clk=module->clocks; clk; clk=clk->next_in_list)
                {
                    for (edge=0; edge<2; edge++)
                    {
                        if (clk->data.clock.edges_used[edge])
                        {
                            for (reg=module->registers; reg; reg=reg->next_in_list)
                            {
                                if ( (reg->clock_ref==clk) && (reg->edge==edge) && (reg->reset_ref==signal) && (reg->reset_level==level))
                                {
                                    for (i=0; i<reg->instance_iter->number_children; i++)
                                    {
                                        int j, array_size;
                                        t_md_type_instance_data *ptr;
                                        instance = reg->instance_iter->children[i];
                                        array_size = instance->size;
                                        for (j=0; j<(array_size>0?array_size:1); j++)
                                        {
                                            for (ptr = &instance->data[j]; ptr; ptr=ptr->reset_value.next_in_list)
                                            {
                                                if (ptr->reset_value.expression)
                                                {
                                                    if (ptr->reset_value.subscript_start>=0)
                                                    {
                                                        output( handle, 1, "ASSIGN_TO_BIT( &( " );
                                                    }
                                                    if (array_size>1)
                                                    {
                                                        output( handle, 1, "%s_%s_state.%s[%d]", edge_name[reg->edge], reg->clock_ref->name, instance->output_name, j );
                                                    }
                                                    else
                                                    {
                                                        output( handle, 1, "%s_%s_state.%s", edge_name[reg->edge], reg->clock_ref->name, instance->output_name );
                                                    }
                                                    if (ptr->reset_value.subscript_start>=0)
                                                    {
                                                        output( handle, -1, "), %d, %d, ", instance->type_def.data.width, ptr->reset_value.subscript_start );
                                                        output_simulation_methods_expression( model, output, handle, NULL, ptr->reset_value.expression, 2, 0 );
                                                        output( handle, -1, ");\n" );
                                                    }
                                                    else
                                                    {
                                                        output( handle, -1, " = " );
                                                        output_simulation_methods_expression( model, output, handle, NULL, ptr->reset_value.expression, 2, 0 );
                                                        output( handle, -1, ";\n" );
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                output( handle, 1, "return error_level_okay;\n");
                output( handle, 0, "}\n");
                output( handle, 0, "\n");
            }
        }
    }

    /*b Output the function to capture inputs
     */
    output( handle, 0, "/*f c_%s::capture_inputs\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "t_sl_error_level c_%s::capture_inputs( void )\n", module->output_name );
    output( handle, 0, "{\n");
    output( handle, 1, "DEBUG_PROBE;\n");

    if (module->inputs)
    {
        output( handle, 1, "for (int i=0; input_desc_%s[i].port_name; i++)\n", module->output_name);
        output( handle, 1, "{\n");
        output( handle, 2, "t_sl_uint64 **input_ptr      = struct_resolve( t_sl_uint64 **, this, input_desc_%s[i].driver_ofs );\n", module->output_name);
        output( handle, 2, "t_sl_uint64 *input_state_ptr = struct_resolve( t_sl_uint64 *, this, input_desc_%s[i].input_state_ofs );\n", module->output_name);
        output( handle, 2, "input_state_ptr[0] = *(input_ptr[0]);\n");
        output( handle, 1, "}\n");
    }
    output( handle, 1, "return error_level_okay;\n");
    output( handle, 0, "}\n");

    /*b Output the function to propagate inputs/state to internal combs, through submodules, and through again all submodules
      This function assumes all inputs and state have changed, and propagates appropriately
      In theory this will only get called after clock and comb have both been called
      We assume here after clock, and we ignore that clock has got its outputs correct (for now)
      Note that if reset is now asserted (and it was not before) then we should do this work anyway for any state that is reset
     */
    output( handle, 0, "/*f c_%s::propagate_all\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "t_sl_error_level c_%s::propagate_all( void )\n", module->output_name );
    output( handle, 0, "{\n");
    output( handle, 1, "DEBUG_PROBE;\n");

    /*b   Handle asynchronous reset
     */
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (level=0; level<2; level++)
        {
            if (signal->data.input.levels_used_for_reset[level])
            {
                output( handle, 1, "if (inputs.%s[0]==%d)\n", signal->name, level );
                output( handle, 1, "{\n");
                output( handle, 2, "reset_%s_%s();\n", level_name[level], signal->name );
                output( handle, 1, "}\n");
            }
        }
    }

    /*b   Output code for this module (makes all its signals valid, including going through comb modules as required)
     */
    output_markers_mask_all( model, module, om_must_be_valid, -1 );
    output_markers_mask_input_dependents( model, module, om_must_be_valid, 0 );
    output_markers_mask_clock_edge_dependents( model, module, NULL, 0, om_must_be_valid, 0 );// All clock edges
    output_simulation_code_to_make_combinatorial_signals_valid( model, module, output, handle );

    /*b   Set inputs to all clocked modules even if the code has output them already
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        /*b If module_instance is not purely clocked then it has already been done
         */
        if ( (!module_instance->module_definition) )
        {
            continue;
        }

        /*b Set inputs
         */
        for (input_port=module_instance->inputs; input_port; input_port=input_port->next_in_list)
        {
            output( handle, 1, "instance_%s.inputs.%s = ", module_instance->name, input_port->module_port_instance->output_name );
            output_simulation_methods_expression( model, output, handle, NULL, input_port->expression, 2, 0 );
            output( handle, -1, ";\n" );
        }

        /*b Next module instance
         */
    }

    /*b   Propagate to all submodules also - could be done in the worklist
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        output( handle, 1, "engine->submodule_call_propagate( instance_%s.handle );\n", module_instance->name );
    }

    /*b   Finish
     */
    output( handle, 1, "return error_level_okay;\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");

    /*b Output the method to propagate inputs/state to internal combs and to submodule inputs
      This function assumes all inputs and state have changed, and propagates appropriately
      This is called on any clock edge for the module to get all submodule inputs valid
      Note that if reset is now asserted (and it was not before) then we should do this work anyway for any state that is reset
      If a submodule has a combinatorial path from an input to an output, then that output IS NOT valid at the start of this call
      Note that this does a small amount of overkill: any work that is only required by an output, and not a next-state function or
      a submodule input used on a clock edge, is work that need not be done. This is deemed irrelevant for now.

      One can try all clock dependents, and all input dependents
      However, this seems to fail to capture the clock enables for gated clocks
     */
    output( handle, 0, "/*f c_%s::propagate_inputs_to_combs_and_submodule_inputs\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "t_sl_error_level c_%s::propagate_inputs_to_combs_and_submodule_inputs( void )\n", module->output_name );
    output( handle, 0, "{\n");
    output( handle, 1, "DEBUG_PROBE;\n");
    output( handle, 0, "// COULD IMPROVE THIS CODE TO REDUCE WHAT IT OUTPUTS\n");

    /*b   Handle asynchronous reset
     */
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (level=0; level<2; level++)
        {
            if (signal->data.input.levels_used_for_reset[level])
            {
                output( handle, 1, "if (inputs.%s[0]==%d)\n", signal->name, level );
                output( handle, 1, "{\n");
                output( handle, 2, "reset_%s_%s();\n", level_name[level], signal->name );
                output( handle, 1, "}\n");
            }
        }
    }

    /*b   Output code for this module (makes all its signals valid, including going through comb modules as required)
     */
    output_markers_mask_all( model, module, om_valid, -1 );
    output_markers_mask_modules( model, module, 0, -1 );
    output_markers_mask_input_dependents( model, module, om_must_be_valid, -1 );
    output_markers_mask_clock_edge_dependents( model, module, NULL, 0, om_must_be_valid, -1 );// All clock edges
    output_simulation_code_to_make_combinatorial_signals_valid( model, module, output, handle );

    /*b   Set inputs to all clocked modules even if the code has output them already
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        /*b If module_instance is not purely clocked then it has already been done
         */
        if ( (!module_instance->module_definition) )
        {
            continue;
        }

        /*b Set inputs
         */
        for (input_port=module_instance->inputs; input_port; input_port=input_port->next_in_list)
        {
            output( handle, 1, "instance_%s.inputs.%s = ", module_instance->name, input_port->module_port_instance->output_name );
            output_simulation_methods_expression( model, output, handle, NULL, input_port->expression, 2, 0 );
            output( handle, -1, ";\n" );
        }

        /*b Next module instance
         */
    }

    /*b   Finish
     */
    output( handle, 1, "return error_level_okay;\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");

    /*b Output the method for combinatorial input-to-output paths
      This function assumes all inputs has changed
     */
    output( handle, 0, "/*f c_%s::comb\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "t_sl_error_level c_%s::comb( void )\n", module->output_name );
    output( handle, 0, "{\n");
    output( handle, 1, "DEBUG_PROBE;\n");

    /*b   Handle asynchronous reset
     */
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (level=0; level<2; level++)
        {
            if (signal->data.input.levels_used_for_reset[level])
            {
                output( handle, 1, "if (inputs.%s[0]==%d)\n", signal->name, level );
                output( handle, 1, "{\n");
                output( handle, 2, "reset_%s_%s();\n", level_name[level], signal->name );
                output( handle, 1, "}\n");
            }
        }
    }

    /*b   Output code for this module (makes all its signals valid, including going through comb modules as required)
     */
    output_markers_mask_all( model, module, 0, -1 );
    output_markers_mask_comb_output_dependencies( model, module, om_must_be_valid, 0 ); // All combs must be made valid
    //output_markers_mask_comb_modules_with_matching_outputs( model, module, om_valid_mask, om_invalid, om_make_valid_mask, 0 ); // Mark all comb modules that drive invalid nets as needing to be output
    output_simulation_code_to_make_combinatorial_signals_valid( model, module, output, handle );

    /*b   Finish
     */
    output( handle, 1, "return error_level_okay;\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");

    /*b Output the method to propagate state from every used clock edge to outputs (submodules must have already done so)
      This function assumes all internal state for the clock edge and submodule state for the clock edge may have changed
     */
    output( handle, 0, "/*f c_%s::propagate_state_to_outputs\n", module->output_name );
    output( handle, 0, "*/\n");
    output( handle, 0, "t_sl_error_level c_%s::propagate_state_to_outputs( void )\n", module->output_name );
    output( handle, 0, "{\n");
    output( handle, 1, "DEBUG_PROBE;\n");

    /*b   Output code for this module (makes all its signals valid, including going through comb modules as required)
      Since inputs are not valid, we want ALL signals that effect outputs, that depend on states, that DO NOT depend directly on inputs
      We mark signals that depend on any form of state as 0x10
      We mark signals that outputs depend on (including outputs :-)) as 0x20
      We mark signals that depend on inputs (i.e. cannot be valid) as 0x40
      Note that static signals (i.e. those that are hard-wired) are marked as 0x20
      Anything marked as 0x20 or 0x30 should be marked as 'must be made valid';
      Outputs that depend purely on static signals end up with 0x20
     */
    output_markers_mask_all( model, module, 0, -1 );
    output_markers_mask_clock_edge_dependents( model, module, NULL, 0, 0x10, 0 );
    output_markers_mask_output_dependencies( model, module, 0x20, 0 );
    output_markers_mask_input_dependents( model, module, 0x40, 0 );
    //output_markers_mask_all_matching( model, module, 0x70, 0x30,   om_must_be_valid, 0,   0, 0 ); // Everything marked as '0x30' must be valid; others 0 - This was CDL1.4.1a6
    output_markers_mask_all_matching( model, module, 0x60, 0x20,   om_must_be_valid, 0,   0, 0 ); // Everything marked as '0x20' or '0x30' must be valid; others 0 - this makes static tie-offs also need to be valid
    output_simulation_code_to_make_combinatorial_signals_valid( model, module, output, handle );

    /*b   Finish
     */
    output( handle, 1, "return error_level_okay;\n");
    output( handle, 0, "}\n");
    output( handle, 0, "\n");

    /*b Output the method to clock a particular clock edge; this just copies next state to state
     */
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        for (edge=0; edge<2; edge++)
        {
            if (clk->data.clock.edges_used[edge])
            {
                /*b   Output method header and copy of next state to state
                 */
                output( handle, 0, "/*f c_%s::clock_%s_%s\n", module->output_name, edge_name[edge], clk->name );
                output( handle, 0, "*/\n");
                output( handle, 0, "t_sl_error_level c_%s::clock_%s_%s( void )\n", module->output_name, edge_name[edge], clk->name );
                output( handle, 0, "{\n");
                output( handle, 1, "DEBUG_PROBE;\n");
                output( handle, 1, "memcpy( &%s_%s_state, &next_%s_%s_state, sizeof(%s_%s_state) );\n", edge_name[edge], clk->name, edge_name[edge], clk->name, edge_name[edge], clk->name);

                /*b   Handle asynchronous reset
                 */
                for (signal=module->inputs; signal; signal=signal->next_in_list)
                {
                    for (level=0; level<2; level++)
                    {
                        if (signal->data.input.levels_used_for_reset[level])
                        {
                            output( handle, 1, "if (inputs.%s[0]==%d)\n", signal->name, level );
                            output( handle, 1, "{\n");
                            output( handle, 2, "reset_%s_%s();\n", level_name[level], signal->name );
                            output( handle, 1, "}\n");
                        }
                    }
                }

                /*b   If worklisting the submodule clocks will already occur; do it here otherwise
                 */
                for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
                {
                    if (model->reference_set_includes( &module_instance->dependencies, clk, edge ))
                    {
                        for (clock_port=module_instance->clocks; clock_port; clock_port=clock_port->next_in_list)
                        {
                            if (clock_port->local_clock_signal==clk)
                            {
                                if (module->number_submodule_clock_calls==0)
                                {
                                    output( handle, 1, "engine->submodule_call_clock( instance_%s.%s__clock_handle, %d );\n", module_instance->name, clock_port->port_name, edge==md_edge_pos );
                                }
                            }
                        }
                    }
                }

                /*b   Call gated clocks 'clock' functions (dependent on run-time enables)
                 */
                {
                    t_md_signal *clk2;
                    for (clk2=module->clocks; clk2; clk2=clk2->next_in_list)
                    {
                        if (clk2->data.clock.clock_ref == clk)
                        {
                            if (clk2->data.clock.edges_used[edge])
                            {
                                output( handle, 1, "if (clock_enable__%s_%s)\n", edge_name[edge], clk2->name );
                                output( handle, 2, "clock_%s_%s();\n", edge_name[edge], clk2->name );
                            }
                        }
                    }
                }

                /*b   Finish
                 */
                output( handle, 1, "return error_level_okay;\n");
                output( handle, 0, "}\n");
                output( handle, 0, "\n");
            }
        }
    }

    /*b Output the method to preclock a particular clock edge
      Prior to this all combinatorials and submodule inputs have been made valid
      Copy state to next state
      Run next state function
      Determine any derived clock gates; call this model's preclock method for those
      Mark submodules based on this clock as to-be-executed by the worklist
     */
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        for (edge=0; edge<2; edge++)
        {
            if (clk->data.clock.edges_used[edge])
            {
                /*b   Output preclock_CLK_EDGE method header
                 */
                output( handle, 0, "/*f c_%s::preclock_%s_%s\n", module->output_name, edge_name[edge], clk->name );
                output( handle, 0, "*/\n");
                output( handle, 0, "t_sl_error_level c_%s::preclock_%s_%s( void )\n", module->output_name, edge_name[edge], clk->name );
                output( handle, 0, "{\n");
                output( handle, 1, "DEBUG_PROBE;\n");
                output( handle, 1, "memcpy( &next_%s_%s_state, &%s_%s_state, sizeof(%s_%s_state) );\n", edge_name[edge], clk->name, edge_name[edge], clk->name, edge_name[edge], clk->name);

                /*b   Output code for this clock for this module - effects next state only
                 */
                for (code_block = module->code_blocks; code_block; code_block=code_block->next_in_list)
                {
                    output_simulation_methods_code_block( model, output, handle, code_block, clk, edge, NULL );
                }

                /*b   Enable all submodules that run off this clock edge if worklisting, or call it otherwise
                 */
                for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
                {
                    if (model->reference_set_includes( &module_instance->dependencies, clk, edge ))
                    {
                        for (clock_port=module_instance->clocks; clock_port; clock_port=clock_port->next_in_list)
                        {
                            if (clock_port->local_clock_signal==clk)
                            {
                                if (module->number_submodule_clock_calls==0)
                                {
                                    output( handle, 1, "engine->submodule_call_preclock( instance_%s.%s__clock_handle, %d );\n", module_instance->name, clock_port->port_name, edge==md_edge_pos );
                                }
                                else
                                {
                                    int worklist_item;
                                    worklist_item = output_markers_value( clock_port, -1 );
                                    output( handle, 1, "submodule_clock_guard[%d] |= 0x%08x;\n", worklist_item/32, 1<<(worklist_item%32) );
                                }
                            }
                        }
                    }
                }

                /*b   Determine clock gate enables for every gated version of this clock
                 */
                {
                    t_md_signal *clk2;
                    for (clk2=module->clocks; clk2; clk2=clk2->next_in_list)
                    {
                        if (clk2->data.clock.clock_ref == clk)
                        {
                            if (clk2->data.clock.edges_used[edge])
                            {
                                output( handle, 1, "clock_enable__%s_%s = ", edge_name[edge], clk2->name );
                                if (clk2->data.clock.gate_state)
                                {
                                    output( handle, -1, "(%s_%s_state.%s==%d);\n",
                                            edge_name[clk2->data.clock.gate_state->edge], clk2->data.clock.gate_state->clock_ref->name, clk2->data.clock.gate_state->name,
                                            clk2->data.clock.gate_level );
                                }
                                else
                                {
                                    output( handle, -1, "(combinatorials.%s==%d);\n", clk2->data.clock.gate_signal->name, clk2->data.clock.gate_level);
                                }
                            }
                        }
                    }
                }

                /*b   Call gated clocks 'preclock' functions (dependent on run-time enables)
                 */
                {
                    t_md_signal *clk2;
                    for (clk2=module->clocks; clk2; clk2=clk2->next_in_list)
                    {
                        if (clk2->data.clock.clock_ref == clk)
                        {
                            if (clk2->data.clock.edges_used[edge])
                            {
                                output( handle, 1, "if (clock_enable__%s_%s)\n", edge_name[edge], clk2->name );
                                output( handle, 2, "preclock_%s_%s();\n", edge_name[edge], clk2->name );
                            }
                        }
                    }
                }

                /*b   Finish
                 */
                output( handle, 1, "return error_level_okay;\n");
                output( handle, 0, "}\n");
                output( handle, 0, "\n");

            }
        }
    }

    /*b Output the toplevel clock function calls - prepreclock, preclock per clock edge, clock per clock edge
     */
    if (module->clocks)
    {
        /*b Model prepreclock method - invoked with no arguments by top level prepreclock function
          The prepreclock marks inputs as uncaptured and clears the clock edges that need to be invoked
         */
        output( handle, 0, "/*f c_%s::prepreclock\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "t_sl_error_level c_%s::prepreclock( void )\n", module->output_name );
        output( handle, 0, "{\n");
        output( handle, 1, "DEBUG_PROBE;\n");
        output( handle, 1, "WHERE_I_AM;\n");
        output( handle, 1, "inputs_captured=0;\n");
        output( handle, 1, "clocks_to_call=0;\n");
        output( handle, 1, "return error_level_okay;\n");
        output( handle, 0, "}\n\n");

        /*b Model preclock method - invoked by toplevel clock preclock functions with model clock preclock/clock function pairs
          The preclock captures inputs (once) and records a clock edge to be invoked
         */
        output( handle, 0, "/*f c_%s::preclock\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "t_sl_error_level c_%s::preclock( t_c_%s_clock_callback_fn preclock, t_c_%s_clock_callback_fn clock )\n", module->output_name, module->output_name, module->output_name );
        output( handle, 0, "{\n");
        output( handle, 1, "DEBUG_PROBE;\n");
        output( handle, 1, "WHERE_I_AM;\n");
        output( handle, 1, "if (!inputs_captured) { capture_inputs(); inputs_captured++; }\n" );
        output( handle, 1, "if (clocks_to_call>%d)\n", 16 );
        output( handle, 1, "{\n" );
        output( handle, 2, "fprintf(stderr,\"BUG - too many preclock calls after prepreclock\\n\");\n");
        output( handle, 1, "} else {\n" );
        output( handle, 2, "clocks_fired[clocks_to_call].preclock=preclock;\n");
        output( handle, 2, "clocks_fired[clocks_to_call].clock=clock;\n");
        output( handle, 2, "clocks_to_call++;\n");
        output( handle, 1, "}\n" );
        output( handle, 1, "return error_level_okay;\n" );
        output( handle, 0, "}\n");

        /*b Model clock method - invoked by toplevel clock clock functions - executes effectively once per prepreclock
          The clock function assumes all input has changed and been captured
          1. Propagate inputs to the inputs of submodules that are clocked, and to all clock gate enable signals - to all combinatorials in fact
          2. prepreclock submodules
          3a. do next-state function for this module for each clock (given combinatorials are all valid) [preclock this module this clock edge]
          3b. preclock submodules (they capture their inputs) for each clock edge - depends on 3a really to define the clock edges to call
          4a. copy next-state to state for each clock for this module [clock this module this clock edge]
          4b. clock submodules (for each clock) (will propagate state to output)
          5. propagate all state (and submodule outputs) to outputs for this module
         */
        output( handle, 0, "/*f c_%s::clock\n", module->output_name );
        output( handle, 0, "*/\n");
        output( handle, 0, "t_sl_error_level c_%s::clock( void )\n", module->output_name );
        output( handle, 0, "{\n");
        output( handle, 1, "DEBUG_PROBE;\n");
        output( handle, 1, "WHERE_I_AM;\n");
        output( handle, 1, "if (clocks_to_call>0)\n" );
        output( handle, 1, "{\n" );
        // Stage 1
        output( handle, 2, "propagate_inputs_to_combs_and_submodule_inputs();\n" );             // At this point all submodules should have valid inputs
        // Stage 2
        if (module->number_submodule_clock_calls>0)
        {
            output( handle, 2, "engine->submodule_call_worklist( engine_handle, se_wl_item_prepreclock, NULL );\n" ); // prepreclock all submodules
            for (i=0; i<(module->number_submodule_clock_calls+31)/32; i++)
                output( handle, 2, "submodule_clock_guard[%d] = 0;\n", i ); // Clear all submodule clock guard entries
        }
        else
        {
            for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
            {
                if ( module_instance->module_definition &&
                     module_instance->module_definition->clocks)
                {
                    output( handle, 1, "engine->submodule_call_prepreclock( instance_%s.handle );\n", module_instance->name );
                }
            }
        }
        output( handle, 2, "int i;\n" );
        // Stage 3a. Call all required local preclock functions - calculates next_state
        output( handle, 2, "for (i=0; i<clocks_to_call; i++)\n" );
        output( handle, 3, "(this->*clocks_fired[i].preclock)();\n" );
        // Stage 3b. Call all required submodule preclock functions
        if (module->number_submodule_clock_calls>0)
        {
            output( handle, 2, "engine->submodule_call_worklist( engine_handle, se_wl_item_preclock, submodule_clock_guard );\n" ); // preclock all submodule's preclock functions required
        }
        // Stage 4a. Call all required local preclock functions - calculates next_state
        output( handle, 2, "for (i=0; i<clocks_to_call; i++)\n" );
        output( handle, 3, "(this->*clocks_fired[i].clock)();\n" );
        // Stage 4b. Call all required submodule clock functions - their outputs will be valid afterwards
        if (module->number_submodule_clock_calls>0)
        {
            output( handle, 2, "engine->submodule_call_worklist( engine_handle, se_wl_item_clock, submodule_clock_guard );\n" ); // preclock all submodule's preclock functions required
        }
        // Stage 5. Propagate from all state (inc submodule outputs dependent on state) to outputs
        output( handle, 2, "propagate_state_to_outputs();\n" );
        output( handle, 2, "clocks_to_call=0;\n");
        output( handle, 1, "}\n" );
        output( handle, 1, "return error_level_okay;\n");
        output( handle, 0, "}\n");

        /*b All model clocked methods done
         */
    }

    /*b All done
     */
}

/*f output_initalization_functions
 */
static void output_initalization_functions( c_model_descriptor *model, t_md_output_fn output, void *handle )
{
    t_md_module *module;
    output( handle, 0, "/*a Initialization functions\n");
    output( handle, 0, "*/\n");
    output( handle, 0, "/*f %s__init\n", model->get_name() );
    output( handle, 0, "*/\n");
    output( handle, 0, "extern void %s__init( void )\n", model->get_name() );
    output( handle, 0, "{\n");
    for (module=model->module_list; module; module=module->next_in_list)
    {
        if (module->external)
            continue;
        output( handle, 1, "se_external_module_register( 1, \"%s\", %s_instance_fn );\n", module->output_name, module->output_name );
    }
    output( handle, 0, "}\n");
    output( handle, 0, "\n");
    output( handle, 0, "/*a Scripting support code\n");
    output( handle, 0, "*/\n");
    output( handle, 0, "/*f init%s\n", model->get_name() );
    output( handle, 0, "*/\n");
    output( handle, 0, "extern \"C\" void init%s( void )\n", model->get_name() );
    output( handle, 0, "{\n");
    output( handle, 1, "%s__init( );\n", model->get_name() );
    output( handle, 1, "scripting_init_module( \"%s\" );\n", model->get_name() );
    output( handle, 0, "}\n");
}

/*a External functions
 */
/*f target_c_output
 */
extern void target_c_output( c_model_descriptor *model, t_md_output_fn output_fn, void *output_handle, int include_assertions, int include_coverage, int include_stmt_coverage, int multithread )
{
    t_md_module *module;

    /*b Output the header, defines, and global types/fns
     */
    output_header( model, output_fn, output_handle );
    output_defines( model, output_fn, output_handle, include_assertions, include_coverage, include_stmt_coverage );
    output_global_types_fns( model, output_fn, output_handle );

    /*b Output the modules
     */
    for (module=model->module_list; module; module=module->next_in_list)
    {
        if (module->external)
            continue;

        module->number_submodule_clock_calls=0;
        if (multithread)
        {
            t_md_module_instance_clock_port *clock_port;
            t_md_module_instance *module_instance;
            int edge;
            t_md_signal *clk;

            for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
            {
                output_markers_mask( module_instance, 0, -1 );
                for (clock_port=module_instance->clocks; clock_port; clock_port=clock_port->next_in_list)
                {
                    output_markers_mask( clock_port, 0, -1 );
                }
            }
            for (clk=module->clocks; clk; clk=clk->next_in_list)
                for (edge=0; edge<2; edge++)
                    if (clk->data.clock.edges_used[edge])
                        for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
                            if (model->reference_set_includes( &module_instance->dependencies, clk, edge ))
                                for (clock_port=module_instance->clocks; clock_port; clock_port=clock_port->next_in_list)
                                    if (clock_port->local_clock_signal==clk)
                                        output_markers_mask( clock_port, module->number_submodule_clock_calls++, -1 );
        }

        output_types( model, module, output_fn, output_handle, include_coverage, include_stmt_coverage );
        output_static_variables( model, module, output_fn, output_handle );
        output_wrapper_functions( model, module, output_fn, output_handle );
        output_constructors_destructors( model, module, output_fn, output_handle, include_coverage, include_stmt_coverage );
        output_simulation_methods( model, module, output_fn, output_handle );
    }

    /*b Output the initialization functions
     */
    output_initalization_functions( model, output_fn, output_handle );
}

/*a To do
  Output constants - whatever they may be!
  Values wider than 64 bits - a lot later
 */

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


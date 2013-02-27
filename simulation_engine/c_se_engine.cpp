/*a Copyright
  
  This file 'c_se_engine.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Outline
  Models register with this base engine when in cycle simulation mode
  They register by first declaring themselves, and getting a handle in return
  They are then instantiated by this base engine, at which point:
    they may attach themselves to a clock, and specify a reset, preclock, clock and combinatorial functions
    they may register input and output signals
    they may specify which signals depend/are generated on what clocks or combinatorially
    they may declare state that may be interrogated

  The base engine can then have global buses/signals instantiated, and this wires things up
  Inputs to modules are implemented by setting an 'int **' in the module to point to an 'int *' which is an output of another module.
  Inputs, outputs and global signals all have a bit length (size) which must be specified.

 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_cons.h"
#include "sl_token.h"
#include "sl_option.h"
#include "se_internal_module.h"
#include "se_errors.h"
#include "se_engine_function.h"
#include "se_external_module.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Statics
 */
/*v engine_list
 */
static c_engine *engine_list=NULL;

/*v default_error_messages
 */
static t_sl_error_text default_error_messages[] = {
     C_SE_ERROR_DEFAULT_MESSAGES
};

/*a Constructors and destructors
 */
/*f c_engine::c_engine
 */
c_engine::c_engine( c_sl_error *error, const char *name )
{
     int i;

     instance_name = NULL;
     next_instance = engine_list;
     engine_list = this;
     this->error = error;
     this->message = new c_sl_error();

     cycle_number = 0;
     module_instance_list = NULL;
     toplevel_module_instance_list = NULL;
     module_forced_options = NULL;
     global_signal_list = NULL;
     global_clock_list = NULL;
     monitors = NULL;
     simulation_callbacks = NULL;
     interrogation_handles = NULL;
     vcd_files = NULL;
     checkpoint_list = NULL;
     checkpoint_tail = NULL;

     thread_pool = NULL;
     thread_pool_mapping = NULL;

     module_data = NULL;

     if (engine_from_name( name ))
     {
          error->add_error( (void *)name, error_level_fatal, error_number_se_duplicate_name, error_id_se_c_engine,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return;
     }
     else
     {
          instance_name = sl_str_alloc_copy( name );
     }
     for (i=0; i<(signed int)((sizeof(dummy_input_value)/sizeof(dummy_input_value[0]))); i++)
     {
          dummy_input_value[i] = 0;
     }

     internal_module_register_all( this );
     //fprintf(stderr,"****************************************\nRegistering default_error_messages\n");
     error->add_text_list( 1, default_error_messages );
     message->add_text_list( 1, default_error_messages );
}

/*f c_engine::~c_engine
 */
c_engine::~c_engine()
{
     c_engine **eptr;


     /*b Delete our engine instance from the global list
      */
     for (eptr=&engine_list; (*eptr) && (*eptr!=this); eptr = &((*eptr)->next_instance));
     if (*eptr)
     {
          *eptr = next_instance;
     }

     /* Free our data
      */
     thread_pool_delete();
     delete_instances_and_signals();
     error->reset();
     message->reset();
     free(instance_name);

}

/*f c_engine::delete_instances_and_signals -  also disposes of forced options
 */
void c_engine::delete_instances_and_signals( void )
{
     t_engine_module_instance *emi, *next_emi;
     t_engine_state_desc_list *sdl, *next_sdl;
     t_engine_clock *clk, *next_clk;
     t_engine_signal *sig, *next_sig;
     t_engine_function *efn;

     /*b Get rid of forced options
      */
     free_forced_options();

     /*b Delete module instances and their data
      */
     for (emi=module_instance_list; emi; emi = next_emi)
     {
          next_emi = emi->next_instance;
          se_engine_function_call_invoke_all( emi->delete_fn_list );
          for (sdl=emi->state_desc_list; sdl; sdl=next_sdl)
          {
               next_sdl = sdl->next_in_list;
               free(sdl->prefix);
               free(sdl);
          }
          se_engine_function_call_free( emi->delete_fn_list );
          se_engine_function_call_free( emi->reset_fn_list );
          se_engine_function_free_functions( emi->clock_fn_list );
          se_engine_function_free_functions( emi->comb_fn_list );
          for (efn=emi->input_list; efn; efn=efn->next_in_list)
          {
               se_engine_function_references_free( efn->data.input.used_by_clocks );
          }
          se_engine_function_free_functions( emi->input_list );
          se_engine_function_free_functions( emi->output_list );
          free(emi->name);
          sl_option_free_list( emi->option_list );
          free(emi);
     }
     module_instance_list = NULL;
     toplevel_module_instance_list = NULL;

     /*b Delete clocks and global signals
      */
     for (clk=global_clock_list; clk; clk=next_clk)
     {
          next_clk = clk->next_in_list;
          free(clk->global_name);
          se_engine_function_call_free( clk->posedge.prepreclock );
          se_engine_function_call_free( clk->posedge.preclock );
          se_engine_function_call_free( clk->posedge.clock );
          se_engine_function_call_free( clk->posedge.comb );
          se_engine_function_call_free( clk->posedge.propagate );
          se_engine_function_call_free( clk->negedge.prepreclock );
          se_engine_function_call_free( clk->negedge.preclock );
          se_engine_function_call_free( clk->negedge.clock );
          se_engine_function_call_free( clk->negedge.propagate );
          se_engine_function_call_free( clk->negedge.comb );
          se_engine_function_references_free( clk->clocks_list );
          free(clk);
     }
     global_clock_list = NULL;
     for (sig=global_signal_list; sig; sig=next_sig)
     {
          next_sig = sig->next_in_list;
          free(sig->global_name);
          se_engine_function_references_free( sig->drives_list );
          se_engine_function_references_free( sig->driven_by );
          free(sig);
     }
     global_signal_list = NULL;
}

/*a Clock/global creation functions
 */
/*f c_engine::create_global
 */
t_engine_signal *c_engine::create_global( const char *name, int size )
{
     t_engine_signal *sig;

     if (find_global(name))
          return NULL;

     sig = (t_engine_signal *)malloc(sizeof(t_engine_signal));
     sig->next_in_list = global_signal_list;
     global_signal_list = sig;
     sig->global_name = sl_str_alloc_copy( name );
     sig->size = size;
     sig->drives_list = NULL;
     sig->driven_by = NULL;

     return sig;
}

/*f c_engine::create_clock
 */
t_engine_clock *c_engine::create_clock( const char *name )
{
     t_engine_clock *clk;

     if (find_clock(name))
          return NULL;

     clk = (t_engine_clock *)malloc(sizeof(t_engine_clock));
     clk->next_in_list = global_clock_list;
     global_clock_list = clk;
     clk->global_name = sl_str_alloc_copy( name );
     clk->clocks_list = NULL;
     clk->posedge.prepreclock = NULL;
     clk->posedge.preclock = NULL;
     clk->posedge.clock = NULL;
     clk->posedge.propagate = NULL;
     clk->posedge.comb = NULL;
     clk->negedge.prepreclock = NULL;
     clk->negedge.preclock = NULL;
     clk->negedge.clock = NULL;
     clk->negedge.propagate = NULL;
     clk->negedge.comb = NULL;
     clk->delay = 0;
     clk->high_cycles = 0;
     clk->low_cycles = 0;
     clk->cycle_length = 0;
     clk->posedge_remainder = 0;
     clk->negedge_remainder = 0;
     clk->next_value = 0;
     clk->value = 0;

     return clk;
}

/*a Option handling methods
 */
/*f c_engine::get_option_object
 */
void *c_engine::get_option_object( void *handle, const char *keyword )
{
     t_engine_module_instance *emi;

     emi = (t_engine_module_instance *)handle;
     return sl_option_get_object( emi->option_list, keyword );
}

/*f c_engine::get_option_int
 */
int c_engine::get_option_int( void *handle, const char *keyword, int default_value )
{
     t_engine_module_instance *emi;

     emi = (t_engine_module_instance *)handle;
     return sl_option_get_int( emi->option_list, keyword, default_value );
}

/*f c_engine::get_option_string
 */
const char *c_engine::get_option_string( void *handle, const char *keyword, const char *default_value )
{
     t_engine_module_instance *emi;

     emi = (t_engine_module_instance *)handle;
     return sl_option_get_string_with_default( emi->option_list, keyword, default_value );
}

/*f c_engine::get_option_list
 */
t_sl_option_list c_engine::get_option_list( void *handle )
{
     t_engine_module_instance *emi;

     emi = (t_engine_module_instance *)handle;
     return emi->option_list;
}

/*f c_engine::set_option_list
 */
t_sl_option_list c_engine::set_option_list( void *handle, t_sl_option_list option_list )
{
     t_engine_module_instance *emi;
     emi = (t_engine_module_instance *)handle;
     emi->option_list = option_list;
     return emi->option_list;
}

/*f c_engine::bfm_add_exec_file_enhancements
 */
void c_engine::bfm_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data, void *engine_handle, const char *clock, int posedge )
{
    simulation_add_exec_file_enhancements( file_data, engine_handle, clock, posedge );
    waveform_add_exec_file_enhancements( file_data );
    coverage_add_exec_file_enhancements( file_data );
    log_add_exec_file_enhancements( file_data, engine_handle );
    register_add_exec_file_enhancements( file_data, engine_handle );
    checkpoint_add_exec_file_enhancements( file_data, engine_handle );
}

/*f c_engine::bfm_add_exec_file_enhancements
 */
void c_engine::bfm_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data, void *engine_handle )
{
    bfm_add_exec_file_enhancements( file_data, engine_handle, NULL, 0 );
}

/*a External functions
 */
/*f engine_from_name
 */
extern c_engine *engine_from_name( const char *name )
{
     c_engine *eng;
     const char *inst_name;

     for (eng=engine_list; eng; eng=eng->get_next_instance())
     {
          inst_name = eng->get_instance_name();
          if ( (inst_name) &&
               (!strcmp(inst_name, name)) )
          {
               return eng;
          }
     }
     return NULL;
}

/*f se_c_engine_init
 */
extern void se_c_engine_init( void )
{
     se_external_module_init();
}

/*f se_c_engine_exit
 */
extern void se_c_engine_exit( void )
{
    se_external_module_deregister_all();
}

/*a To do
  add framework for getting state of global signal values - could tie this in to state, with a module_instance of NULL

  check what we will need for MTI - dummy, and tying stuff in, and allowing state to be seen

  add ability to checkpoint data, and restore checkpoint data
  add callback function for checkpointing and restoring, which may be optionally registered (AHDL won't use this)

  add integer and string capability to indented files in support.cpp

  add memory display in UI
  add memory write state
  add save file ability to MIF file
  add support for memory files in exec_files - memory save, memory write
  add callbacks for memory value setting and reading in c_engine.cpp

  provide for state change by user

  add strings to the exec_file language

  add sparse memories to support.cpp

 */


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


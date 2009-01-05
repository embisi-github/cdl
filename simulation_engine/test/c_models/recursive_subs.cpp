/*a Copyright
  
  This file 'recursive_subs.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/
/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "be_model_includes.h"

/*a Types
 */
/*t t_state
 */
typedef struct t_state
{
    int sum;
} t_state;

/*t t_inputs
 */
typedef struct t_inputs
{
    int *data_in;
} t_inputs;

/*t	c_recursive_subs
*/
class c_recursive_subs
{
public:
     c_recursive_subs::c_recursive_subs( class c_engine *eng, void *engine_handle );
     c_recursive_subs::~c_recursive_subs();
     t_sl_error_level c_recursive_subs::delete_instance( void );
     t_sl_error_level c_recursive_subs::reset( void );
     t_sl_error_level c_recursive_subs::preclock( void );
     t_sl_error_level c_recursive_subs::clock( void );
     c_engine *engine;
     void *engine_handle;
private:
    int depth_remaining; // Depth of recursion remaining
    
    t_inputs inputs;
    int data_in_copy; // Kludge as input to submodule is our input too; need a way to rebind this later (or somesuch)
    int *sub_output; // This will point to the submodule's output
    t_state next_state;
    t_state state;

    void *submodule_handle;
    void *submodule_clock_handle;
};

/*a Static variables
 */
/*f state_desc
 */
static t_state *_ptr;
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_engine_state_desc state_desc[] =
{
     {"sum", engine_state_desc_type_bits, NULL, struct_offset(_ptr, sum), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static callback functions
 */
/*f instance_fn - simple callback wrapper for the main method
 */
static t_sl_error_level instance_fn( c_engine *engine, void *engine_handle )
{
     c_recursive_subs *mod;
     mod = new c_recursive_subs( engine, engine_handle );
     if (!mod)
          return error_level_fatal;
     return error_level_okay;
}

/*f delete_fn - simple callback wrapper for the main method
 */
static t_sl_error_level delete_fn( void *handle )
{
     c_recursive_subs *mod;
     t_sl_error_level result;
     mod = (c_recursive_subs *)handle;
     result = mod->delete_instance();
     delete( mod );
     return result;
}

/*f reset_fn - simple callback wrapper for the main method
 */
static t_sl_error_level reset_fn( void *handle )
{
     c_recursive_subs *mod;
     mod = (c_recursive_subs *)handle;
     return mod->reset();
}

/*f preclock_fn - simple callback wrapper for the main method
 */
static t_sl_error_level preclock_fn( void *handle )
{
     c_recursive_subs *mod;
     mod = (c_recursive_subs *)handle;
     return mod->preclock();
}

/*f clock_fn - simple callback wrapper for the main method
 */
static t_sl_error_level clock_fn( void *handle )
{
     c_recursive_subs *mod;
     mod = (c_recursive_subs *)handle;
     return mod->clock();
}

/*a Constructors/destructors
 */

/*f c_recursive_subs::c_recursive_subs
 */
c_recursive_subs::c_recursive_subs( class c_engine *eng, void *eng_handle )
{
     this->engine = eng;
     this->engine_handle = eng_handle;

     SL_DEBUG( sl_debug_level_info, "c_recursive_subs::c_recursive_subs", "New instance %p", eng );

     memset( &state, 0, sizeof(state) );
     memset( &next_state, 0, sizeof(next_state) );

     engine->register_delete_function( engine_handle, (void *)this, delete_fn );
     engine->register_reset_function( engine_handle, (void *)this, reset_fn );

     engine->register_clock_fns( engine_handle, (void *)this, "main_clock", preclock_fn, clock_fn );
     engine->register_output_signal( engine_handle, "sum", 32, &state.sum );
     engine->register_output_generated_on_clock( engine_handle, "sum", "main_clock", 1 );

     engine->register_input_signal( engine_handle, "data_in", 32, &inputs.data_in );
     engine->register_input_used_on_clock( engine_handle, "data_in", "main_clock", 1 );

     engine->register_state_desc( engine_handle, 1, state_desc, &state, NULL );

     depth_remaining = engine->get_option_int( engine_handle, "depth", 0 );

     if (depth_remaining>0)
     {
         t_sl_option_list option_list;
         option_list = sl_option_list( NULL, "depth", depth_remaining-1 );
         if (engine->instantiate( engine_handle, "recursive_subs", "rs", option_list )==error_level_okay)
         {
             submodule_handle = engine->submodule_get_handle( engine_handle, "rs" );
             submodule_clock_handle = engine->submodule_get_clock_handle( submodule_handle, "main_clock" );
             engine->submodule_drive_input( submodule_handle, "data_in", &data_in_copy, 32 );
             engine->submodule_output_add_receiver( submodule_handle, "sum", &sub_output, 32 );
         }
     }
}

/*f c_recursive_subs::~c_recursive_subs
 */
c_recursive_subs::~c_recursive_subs()
{
     delete_instance();
}

/*f c_recursive_subs::delete_instance
 */
t_sl_error_level c_recursive_subs::delete_instance( void )
{
     return error_level_okay;
}

/*a Engine invoked methods
 */
/*f c_recursive_subs::reset
 */
t_sl_error_level c_recursive_subs::reset( void )
{
     SL_DEBUG( sl_debug_level_info,
              "c_recursive_subs::reset",
              "reset depth %d", depth_remaining );
     state.sum = 0;
     return error_level_okay;
}

/*f c_recursive_subs::preclock
 */
t_sl_error_level c_recursive_subs::preclock( void )
{
     memcpy( &next_state, &state, sizeof(state) );
     if (depth_remaining>0)
     {
         next_state.sum = sub_output[0]+state.sum;
         data_in_copy = inputs.data_in[0];
         engine->submodule_call_preclock( submodule_clock_handle, 1 );
     }
     else
     {
         next_state.sum = inputs.data_in[0]+state.sum;
     }
     return error_level_okay;
}

/*f c_recursive_subs::clock
 */
t_sl_error_level c_recursive_subs::clock( void )
{

     memcpy( &state, &next_state, sizeof(state) );
     if (depth_remaining>0)
     {
         engine->submodule_call_clock( submodule_clock_handle, 1 );
     }
     return error_level_okay;
}

/*a Initialization functions
 */
/*f recursive_subs__init
 */
extern void recursive_subs__init( void )
{
     se_external_module_register( 1, "recursive_subs", instance_fn );
}

/*a Scripting support code
 */
/*f initc_recursive_subs
 */
extern "C" void initc_recursive_subs( void )
{
     recursive_subs__init( );
     scripting_init_module( "recursive_subs" );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


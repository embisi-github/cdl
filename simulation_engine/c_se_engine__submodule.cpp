/*a Copyright
  
  This file 'c_se_engine__submodule.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "sl_exec_file.h"
#include "se_engine_function.h"
#include "se_errors.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Types
 */
/*t t_engine_submodule_clock_set_entry
 */
typedef struct t_engine_submodule_clock_set_entry
{
    void *submodule_clock_handle;
} t_engine_submodule_clock_set_entry;

/*t t_engine_submodule_clock_set
 */
typedef struct t_engine_submodule_clock_set
{
    int num_submodule_clocks;
    t_engine_submodule_clock_set_entry entries[1];
} t_engine_submodule_clock_set;

/*a Handle management methods
 */
/*f c_engine::submodule_get_handle
  Get a handle for submodule calls
 */
void *c_engine::submodule_get_handle( void *engine_handle, const char *submodule_name )
{
    t_engine_module_instance *pemi;
    pemi = (t_engine_module_instance *)engine_handle;
    return find_module_instance( pemi, submodule_name );
}

/*f c_engine::submodule_get_clock_handle
  Get a handle for submodule clock calls
 */
void *c_engine::submodule_get_clock_handle( void *submodule_handle, const char *clock_name )
{
    t_engine_module_instance *emi;
    t_engine_function *clk;

    emi = (t_engine_module_instance *)submodule_handle;
    if (!emi)
        return NULL;

    clk = se_engine_function_find_function( emi->clock_fn_list, clock_name );
    if (!clk)
    {
        error->add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_clock, error_id_se_c_engine_bind_clock,
                          error_arg_type_malloc_string, clock_name,
                          error_arg_type_malloc_filename, emi->full_name,
                          error_arg_type_none );
        return NULL;
    }
    return (void *)clk;
}

/*f c_engine::submodule_clock_has_edge
  Return 1 if the module uses the requested edge clock edge, and therefore should have that edge function invoked
 */
int c_engine::submodule_clock_has_edge( void *submodule_clock_handle, int posedge )
{
    t_engine_function *clk;
    clk = (t_engine_function *)submodule_clock_handle;
    if (!clk)
        return 0;
    if (posedge && clk->data.clock.posedge_clock_fn)
        return 1;
    if (!posedge && clk->data.clock.negedge_clock_fn)
        return 1;
    return 0;
}

/*a Handle calls of submodule functions
 */
/*f c_engine::submodule_call_reset
 */
void c_engine::submodule_call_reset( void *submodule_handle, int pass )
{
    t_engine_module_instance *emi;
    emi = (t_engine_module_instance *)submodule_handle;
    SL_DEBUG( sl_debug_level_info,
              "c_se_engine::submodule_call_reset",
              "calling reset of %s", emi->name); 
    if (emi->reset_fn_list)
    {
        ((t_engine_callback_arg_fn)(emi->reset_fn_list->callback_fn))( emi->reset_fn_list->handle, pass );
    }
}

/*f c_engine::submodule_call_prepreclock
 */
void c_engine::submodule_call_prepreclock( void *submodule_handle )
{
    t_engine_module_instance *emi;
    emi = (t_engine_module_instance *)submodule_handle;
    if (emi->prepreclock_fn_list)
    {
        (emi->prepreclock_fn_list->data.prepreclock.prepreclock_fn)( emi->prepreclock_fn_list->handle );
    }
}

/*f c_engine::submodule_call_preclock
 */
void c_engine::submodule_call_preclock( void *submodule_clock_handle, int posedge )
{
    t_engine_function *clk;
    clk = (t_engine_function *)submodule_clock_handle;
    if (!clk)
        return;
    if (posedge && clk->data.clock.posedge_preclock_fn)
    {
        (clk->data.clock.posedge_preclock_fn)(clk->handle);
    }
    if (!posedge && clk->data.clock.negedge_clock_fn)
    {
        (clk->data.clock.negedge_preclock_fn)(clk->handle);
    }
}

/*f c_engine::submodule_call_clock
 */
void c_engine::submodule_call_clock( void *submodule_clock_handle, int posedge )
{
    t_engine_function *clk;
    clk = (t_engine_function *)submodule_clock_handle;
    if (!clk)
        return;
    if (posedge && clk->data.clock.posedge_preclock_fn)
    {
        (clk->data.clock.posedge_clock_fn)(clk->handle);
    }
    if (!posedge && clk->data.clock.negedge_clock_fn)
    {
        (clk->data.clock.negedge_clock_fn)(clk->handle);
    }
}

/*f c_engine::submodule_call_propagate
  Call the propagate inputs function of a submodule
 */
void c_engine::submodule_call_propagate( void *submodule_handle )
{
    t_engine_module_instance *emi;
    emi = (t_engine_module_instance *)submodule_handle;
    if (emi->propagate_fn_list)
    {
        (emi->propagate_fn_list->data.propagate.propagate_fn)( emi->propagate_fn_list->handle );
    }
}

/*f c_engine::submodule_call_comb
  Call the combinatorial function of a submodule
 */
void c_engine::submodule_call_comb( void *submodule_handle )
{
    t_engine_module_instance *emi;
    emi = (t_engine_module_instance *)submodule_handle;
    if (emi->comb_fn_list)
    {
        (emi->comb_fn_list->data.comb.comb_fn)( emi->comb_fn_list->handle );
    }
}

/*a Handle clock sets
 */
/*f c_engine::submodule_clock_set_declare
 */
t_engine_submodule_clock_set_ptr c_engine::submodule_clock_set_declare( int n )
{
    t_engine_submodule_clock_set *scs;
    int i;

    scs = (t_engine_submodule_clock_set *) malloc(sizeof(t_engine_submodule_clock_set) + n*sizeof(t_engine_submodule_clock_set_entry));
    scs->num_submodule_clocks = 0;
    for (i=0; i<n; i++)
    {
        scs->entries[i].submodule_clock_handle = NULL;
    }
    return scs;
}

/*f c_engine::submodule_clock_set_entry
 */
int c_engine::submodule_clock_set_entry( t_engine_submodule_clock_set *scs, int n, void *submodule_handle )
{
    if (!scs) return 0;
    if ((n<0) || (n>=scs->num_submodule_clocks)) return 0;
    scs->entries[n].submodule_clock_handle = submodule_handle;
    return 1;
}

/*f c_engine::submodule_clock_set_invoke
  This is where we will parallelize
 */
void c_engine::submodule_clock_set_invoke( t_engine_submodule_clock_set_ptr scs, int *bitmask, t_engine_sim_function_type type )
{
    int i;
    for (i=0; i<scs->num_submodule_clocks; i++)
    {
        if ( (!bitmask) ||
             (bitmask[i/32]&(1<<i)) )
        {
            t_engine_function *clk;
            
            clk = (t_engine_function *)scs->entries[i].submodule_clock_handle;
            if (clk)
            {
                switch (type)
                {
                case engine_sim_function_type_posedge_prepreclock: clk->data.clock.posedge_prepreclock_fn(clk->handle); break;
                case engine_sim_function_type_posedge_preclock:    clk->data.clock.posedge_preclock_fn(clk->handle); break;
                case engine_sim_function_type_posedge_clock:       clk->data.clock.posedge_clock_fn(clk->handle); break;
                case engine_sim_function_type_negedge_prepreclock: clk->data.clock.negedge_prepreclock_fn(clk->handle); break;
                case engine_sim_function_type_negedge_preclock:    clk->data.clock.negedge_preclock_fn(clk->handle); break;
                case engine_sim_function_type_negedge_clock:       clk->data.clock.negedge_clock_fn(clk->handle); break;
                }
            }
        }
    }
}

/*a Submodule inputs
 */
/*f c_engine::submodule_input_type
 */
int c_engine::submodule_input_type( void *submodule_handle, const char *name, int *comb, int *size )
{
     t_engine_module_instance *emi;
     t_engine_function *input;

     emi = (t_engine_module_instance *)submodule_handle;
     if (!emi)
         return error_level_serious;

     input = se_engine_function_find_function( emi->input_list, name );
     if (!input)
     {
         return 0;
     }
     if (size) { *size = input->data.input.size; }
     if (comb) { *comb = input->data.input.combinatorial; }
     return 1;
}

/*f c_engine::submodule_drive_input
  Drive the input of a submodule with a signal
 */
t_sl_error_level c_engine::submodule_drive_input( void *submodule_handle, const char *name, t_se_signal_value *signal, int size )
{
     t_engine_module_instance *emi;
     t_engine_function *input;

     emi = (t_engine_module_instance *)submodule_handle;
     if (!emi)
         return error_level_serious;

     input = se_engine_function_find_function( emi->input_list, name );
     if (!input)
     {
          return error->add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_input, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, name,
                                   error_arg_type_malloc_filename, emi->full_name,
                                   error_arg_type_none );
     }
     if (input->data.input.size!=size)
     {
         return error->add_error( (void *)emi->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
                                  error_arg_type_integer, size,
                                  error_arg_type_integer, input->data.input.size,
                                  error_arg_type_malloc_filename, name,
                                  error_arg_type_none );
     }
     module_drive_input( input, signal );
     return error_level_okay;
}

/*f c_engine::submodule_drive_input_with_module_input
  Drive the input of a submodule with a module input
 */
t_sl_error_level c_engine::submodule_drive_input_with_module_input( void *module_handle, const char *module_name, void *submodule_handle, const char *submodule_name )
{
     t_engine_module_instance *emi, *semi;
     t_engine_function *module_input, *submodule_input;

     emi = (t_engine_module_instance *)module_handle;
     semi = (t_engine_module_instance *)submodule_handle;
     if (!emi || !semi)
         return error_level_serious;

     module_input = se_engine_function_find_function( emi->input_list, module_name );
     if (!module_input)
     {
          return error->add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_input, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, module_name,
                                   error_arg_type_malloc_filename, emi->full_name,
                                   error_arg_type_none );
     }
     submodule_input = se_engine_function_find_function( semi->input_list, submodule_name );
     if (!submodule_input)
     {
          return error->add_error( (void *)semi->full_name, error_level_serious, error_number_se_unknown_input, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, submodule_name,
                                   error_arg_type_malloc_filename, semi->full_name,
                                   error_arg_type_none );
     }
     if (module_input->data.input.size!=submodule_input->data.input.size)
     {
         return error->add_error( (void *)semi->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
                                  error_arg_type_integer, module_input->data.input.size,
                                  error_arg_type_integer, submodule_input->data.input.size,
                                  error_arg_type_malloc_filename, submodule_name,
                                  error_arg_type_none );
     }
     se_engine_function_references_add( &module_input->data.input.connected_to, submodule_input );
     return error_level_okay;
}

/*f c_engine::submodule_drive_input_with_submodule_output
  Drive the input of a submodule with a submodule output
 */
t_sl_error_level c_engine::submodule_drive_input_with_submodule_output( void *submodule_output_handle, const char *submodule_output_name, void *submodule_input_handle, const char *submodule_input_name )
{
     t_engine_module_instance *semi_i, *semi_o;
     t_engine_function *submodule_input, *submodule_output;

     semi_i = (t_engine_module_instance *)submodule_input_handle;
     semi_o = (t_engine_module_instance *)submodule_output_handle;
     if (!semi_i || !semi_o)
         return error_level_serious;

     submodule_input = se_engine_function_find_function( semi_i->input_list, submodule_input_name );
     if (!submodule_input)
     {
          return error->add_error( (void *)semi_i->full_name, error_level_serious, error_number_se_unknown_input, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, submodule_input_name,
                                   error_arg_type_malloc_filename, semi_i->full_name,
                                   error_arg_type_none );
     }
     submodule_output = se_engine_function_find_function( semi_o->output_list, submodule_output_name );
     if (!submodule_output)
     {
          return error->add_error( (void *)semi_o->full_name, error_level_serious, error_number_se_unknown_output, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, submodule_output_name,
                                   error_arg_type_malloc_filename, semi_o->full_name,
                                   error_arg_type_none );
     }
     if (submodule_input->data.input.size!=submodule_output->data.output.size)
     {
         return error->add_error( (void *)semi_i->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
                                  error_arg_type_integer, submodule_input->data.input.size,
                                  error_arg_type_integer, submodule_output->data.output.size,
                                  error_arg_type_malloc_filename, submodule_input_name,
                                  error_arg_type_none );
     }
     module_drive_input( submodule_input, submodule_output->data.output.value_ptr );
     return error_level_okay;
}

/*a Submodule outputs
 */
/*f c_engine::submodule_output_type
 */
int c_engine::submodule_output_type( void *submodule_handle, const char *name, int *comb, int *size )
{
     t_engine_module_instance *emi;
     t_engine_function *output;

     emi = (t_engine_module_instance *)submodule_handle;
     if (!emi)
         return error_level_serious;

     output = se_engine_function_find_function( emi->output_list, name );
     if (!output)
     {
         return 0;
     }
     if (size) { *size = output->data.output.size; }
     if (comb) { *comb = output->data.output.combinatorial; }
     return 1;
}

/*f c_engine::submodule_output_add_receiver
  Use an output of a submodule to drive a signal
 */
t_sl_error_level c_engine::submodule_output_add_receiver( void *submodule_handle, const char *name, t_se_signal_value **signal, int size )
{
     t_engine_module_instance *emi;
     t_engine_function *output;

     emi = (t_engine_module_instance *)submodule_handle;
     if (!emi)
         return error_level_serious;

     output = se_engine_function_find_function( emi->output_list, name );
     if (!output)
     {
          return error->add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_output, error_id_se_c_engine_name,
                                   error_arg_type_malloc_string, name,
                                   error_arg_type_malloc_filename, emi->full_name,
                                   error_arg_type_none );
     }

     if (output->data.input.size!=size)
     {
         return error->add_error( (void *)emi->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
                                  error_arg_type_integer, output->data.input.size,
                                  error_arg_type_integer, size,
                                  error_arg_type_malloc_filename, name,
                                  error_arg_type_none );
     }
     *signal = output->data.output.value_ptr;
     return error_level_okay;
}

/*f c_engine::submodule_output_drive_module_output
  Drive the output of a module with the output of a submodule
 */
t_sl_error_level c_engine::submodule_output_drive_module_output( void *submodule_output_handle, const char *submodule_output_name, void *module_output_handle, const char *module_output_name )
{
     t_engine_module_instance *emi, *semi;
     t_engine_function *module_output, *submodule_output;

     emi = (t_engine_module_instance *)module_output_handle;
     semi = (t_engine_module_instance *)submodule_output_handle;
     if (!emi || !semi)
         return error_level_serious;

     module_output = se_engine_function_find_function( emi->output_list, module_output_name );
     if (!module_output)
     {
          return error->add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_output, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, module_output_name,
                                   error_arg_type_malloc_filename, emi->full_name,
                                   error_arg_type_none );
     }
     submodule_output = se_engine_function_find_function( semi->output_list, submodule_output_name );
     if (!submodule_output)
     {
          return error->add_error( (void *)semi->full_name, error_level_serious, error_number_se_unknown_output, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, submodule_output_name,
                                   error_arg_type_malloc_filename, semi->full_name,
                                   error_arg_type_none );
     }
     if (module_output->data.output.size!=submodule_output->data.output.size)
     {
         return error->add_error( (void *)semi->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
                                  error_arg_type_integer, module_output->data.input.size,
                                  error_arg_type_integer, submodule_output->data.input.size,
                                  error_arg_type_malloc_filename, submodule_output->name,
                                  error_arg_type_none );
     }
     module_output->data.output.value_ptr = submodule_output->data.output.value_ptr;
     return error_level_okay;
}


/*f To do
NEED MUCH BETTER ERROR CHECKING
NEED BETTER ERROR ON 'drive a b' if a is not a global
*/

/*a Submodule work list
 */
/*
worklist c_engine::submodule_init_clock_worklist( int number_of_calls )
{
    t_sl_wl_thread_pool_ptr thread_pool; // A copy of the engine if really threading, else local if not
    t_sl_wl_worklist *worklist;
    if (want_to_subthread(module_instance)) // Then the module_instance thread pool should be valid
    {
        module_instance->thread_pool = engine->thread_pool;
    }
    else
    {
        module_instance->thread_pool = sl_wl_create_thread_pool();
    }
    module_instance->worklist = sl_wl_add_worklist( module_instance->thread_pool, module_instance->name, number_of_calls, (int)wl_item_count );
}
*/

/*f submodule_set_clock_worklist_prepreclock
  Set a worklist item to 'prepreclock' a module
 */
 /*
t_sl_wl_callback_fn wl_callback_prepreclock( void *submodule_handle );
t_sl_error_level c_engine::submodule_set_clock_worklist_prepreclock( submodule_clock_worklist, int call_number, void *submodule_handle )
{
    t_engine_module_instance *emi;
    emi = (t_engine_module_instance *)submodule_handle;

    thread_name = thread_name_from_submodule( module, submodule_handle );
    sl_wl_set_work_head( module_instance->worklist, call_number, const char *name, "prepreclock", NULL );
    sl_wl_set_work_item( module_instance->worklist, call_number, wl_item_prepreclock, wl_callback_prepreclock, submodule_handle );
    if (thread_name)
        return sl_wl_assign_work_to_thread( module_instance->worklist, call_number, thread_name );
    return error_level_okay; // No thread to assign it to; use default
}
 */

/*f submodule_set_clock_worklist_clock
  Set a worklist item to 'preclock' and 'clock' a module clock
 */
/*
t_sl_error_level c_engine::submodule_set_clock_worklist_clock( submodule_clock_worklist, int call_number, void *submodule_clock_handle, int posedge, NULL )
{
    t_engine_function *clk;
    clk = (t_engine_function *)submodule_clock_handle;
    if (!clk)
        return error_level_fatal;

    sl_wl_set_work_head( module_instance->worklist, call_number, const char *name, char *clockname, NULL );
    sl_wl_set_work_item( module_instance->worklist, call_number, wl_item_preclock, wl_callback_preclock, submodule_clock_handle );
    sl_wl_set_work_item( module_instance->worklist, call_number, wl_item_clock,    wl_callback_clock,    submodule_clock_handle );

    return error_level_okay;
}
*/

/*f submodule_enable_clock_worklist_item
  Enable a preclock/clock call for a worklist item - actually should be done in md_target_c.cpp
 */
 /*
t_sl_error_level c_engine::submodule_enable_clock_worklist_item( int call_number )
{
    return error_level_okay;
}

typedef enum
{
    wl_item_prepreclock,
    wl_item_preclock,
    wl_item_clock,
    wl_item_count // Must be last - indicates the number of elements
} t_se_worklist_call;

t_sl_error_level c_engine::submodule_call_worklist( submodule_clock_worklist, t_se_worklist_call wl_call, int *guard )
{
    return sl_wl_execute_worklist( submodule_clock_worklist->worklist, guard, wl_call );
}
 */
    
/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


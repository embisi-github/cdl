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
#include "sl_work_list.h"
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
        add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_clock, error_id_se_c_engine_bind_clock,
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
    if (!emi) return;
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
    if (!emi) return;
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
    if (!emi) return;
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
    if (!emi) return;
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
          return add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_input, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, name,
                                   error_arg_type_malloc_filename, emi->full_name,
                                   error_arg_type_none );
     }
     if (input->data.input.size!=size)
     {
         return add_error( (void *)emi->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
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
          return add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_input, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, module_name,
                                   error_arg_type_malloc_filename, emi->full_name,
                                   error_arg_type_none );
     }
     submodule_input = se_engine_function_find_function( semi->input_list, submodule_name );
     if (!submodule_input)
     {
          return add_error( (void *)semi->full_name, error_level_serious, error_number_se_unknown_input, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, submodule_name,
                                   error_arg_type_malloc_filename, semi->full_name,
                                   error_arg_type_none );
     }
     if (module_input->data.input.size!=submodule_input->data.input.size)
     {
         return add_error( (void *)semi->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
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
          return add_error( (void *)semi_i->full_name, error_level_serious, error_number_se_unknown_input, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, submodule_input_name,
                                   error_arg_type_malloc_filename, semi_i->full_name,
                                   error_arg_type_none );
     }
     submodule_output = se_engine_function_find_function( semi_o->output_list, submodule_output_name );
     if (!submodule_output)
     {
          return add_error( (void *)semi_o->full_name, error_level_serious, error_number_se_unknown_output, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, submodule_output_name,
                                   error_arg_type_malloc_filename, semi_o->full_name,
                                   error_arg_type_none );
     }
     if (submodule_input->data.input.size!=submodule_output->data.output.size)
     {
         return add_error( (void *)semi_i->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
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
          return add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_output, error_id_se_c_engine_name,
                                   error_arg_type_malloc_string, name,
                                   error_arg_type_malloc_filename, emi->full_name,
                                   error_arg_type_none );
     }

     if (output->data.input.size!=size)
     {
         return add_error( (void *)emi->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
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
          return add_error( (void *)emi->full_name, error_level_serious, error_number_se_unknown_output, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, module_output_name,
                                   error_arg_type_malloc_filename, emi->full_name,
                                   error_arg_type_none );
     }
     submodule_output = se_engine_function_find_function( semi->output_list, submodule_output_name );
     if (!submodule_output)
     {
          return add_error( (void *)semi->full_name, error_level_serious, error_number_se_unknown_output, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, submodule_output_name,
                                   error_arg_type_malloc_filename, semi->full_name,
                                   error_arg_type_none );
     }
     if (module_output->data.output.size!=submodule_output->data.output.size)
     {
         return add_error( (void *)semi->full_name, error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_drive,
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

/*a Thread pool methods
 */
/*f thread_pool_init
 */
t_sl_error_level c_engine::thread_pool_init( void )
{
    if (thread_pool)
        return error_level_fatal;
    thread_pool = sl_wl_create_thread_pool();
    if (!thread_pool)
        return error_level_fatal;
    return error_level_okay;
}

/*f thread_pool_delete
 */
t_sl_error_level c_engine::thread_pool_delete( void )
{
    if (thread_pool)
        sl_wl_terminate_all_worker_threads((t_sl_wl_thread_pool_ptr)thread_pool);
    thread_pool = NULL;
    return error_level_okay;
}

/*f thread_pool_add_thread
 */
t_sl_error_level c_engine::thread_pool_add_thread( const char *thread_name )
{
    return sl_wl_spawn_worker_thread( (t_sl_wl_thread_pool_ptr) thread_pool, thread_name, NULL ); // No affinity to start with
}

/*f thread_pool_map_thread_to_module
 */
t_sl_error_level c_engine::thread_pool_map_thread_to_module( const char *thread_name, const char *submodule_name )
{
    t_sl_wl_thread_ptr thread_ptr;
    t_thread_pool_mapping *tpm;
    t_thread_pool_submodule_mapping *tpsm;
    int i;

    thread_ptr = sl_wl_find_thread( (t_sl_wl_thread_pool_ptr) thread_pool, thread_name );
    if (!thread_ptr)
        return error_level_fatal;

    for (i=strlen(submodule_name)-1; (i>=0) && (submodule_name[i]!='.'); i--);

    if (i<=0)
        return error_level_fatal;

    tpm = thread_pool_mapping_find( submodule_name, i );
    if (!tpm)
        tpm = thread_pool_mapping_add( submodule_name, i );
    if (!tpm)
        return error_level_serious;
    tpsm = thread_pool_submodule_mapping_find( tpm, submodule_name+i+1 );
    if (!tpsm)
        tpsm = thread_pool_submodule_mapping_add( tpm, submodule_name+i+1, thread_ptr );
    if (!tpsm)
        return error_level_serious;
    return error_level_okay;
}

/*f thread_pool_mapped_submodules
 */
int c_engine::thread_pool_mapped_submodules(t_engine_module_instance *emi )
{
    if (thread_pool_mapping_find( emi->full_name, strlen(emi->full_name) ))
        return 1;
    return 0;
}

/*f thread_pool_mapping_find
 */
t_thread_pool_mapping *c_engine::thread_pool_mapping_find( const char *emi_full_name, int length )
{
    t_thread_pool_mapping *tpm;
    if (!emi_full_name)
        return NULL;
    for (tpm=thread_pool_mapping; tpm; tpm=tpm->next_in_list)
        if (!strncmp(tpm->module_name,emi_full_name,length))
            if (tpm->module_name[length]==0)
                break;
    if (!tpm)
        return NULL;
    return tpm;
}

/*f thread_pool_mapping_add
 */
t_thread_pool_mapping *c_engine::thread_pool_mapping_add( const char *emi_full_name, int length )
{
    t_thread_pool_mapping *tpm;
    if (!emi_full_name)
        return NULL;
    tpm = (t_thread_pool_mapping *)malloc(sizeof(t_thread_pool_mapping)+length+1);
    tpm->next_in_list = thread_pool_mapping;
    thread_pool_mapping = tpm;
    strncpy( tpm->module_name, emi_full_name, length );
    tpm->module_name[length] = 0;
    tpm->mapping_list = NULL;
    return tpm;
}

/*f thread_pool_submodule_mapping_add
 */
t_thread_pool_submodule_mapping *c_engine::thread_pool_submodule_mapping_add( t_thread_pool_mapping *tpm, const char *smi_name, void *thread_ptr )
{
    t_thread_pool_submodule_mapping *tpsm;
    if ((!smi_name) || (!tpm))
        return NULL;

    printf("Add submapping for %s, %s\n", tpm->module_name, smi_name );

    tpsm = (t_thread_pool_submodule_mapping *)malloc(sizeof(t_thread_pool_submodule_mapping)+strlen(smi_name)+4);
    tpsm->next_in_list = tpm->mapping_list;
    strcpy( tpsm->submodule_name, smi_name );
    tpsm->thread_ptr = (t_sl_wl_thread_ptr)thread_ptr;
    tpm->mapping_list = tpsm;
    return tpsm;
}

/*t thread_pool_submodule_mapping_find
 */
t_thread_pool_submodule_mapping *c_engine::thread_pool_submodule_mapping_find( t_thread_pool_mapping *tpm, const char *name )
{
    t_thread_pool_submodule_mapping *tpsm;
    if (!name)
        return NULL;
    for (tpsm=tpm->mapping_list; tpsm; tpsm=tpsm->next_in_list)
        if (!strcmp(tpsm->submodule_name,name))
            return tpsm;
    return NULL;
}

/*t thread_pool_submodule_mapping
 */
const char *c_engine::thread_pool_submodule_mapping( t_engine_module_instance *emi, t_engine_module_instance *smi )
{
    t_thread_pool_mapping *tpm;
    t_thread_pool_submodule_mapping *tpsm;
    tpm = thread_pool_mapping_find( emi->full_name, strlen(emi->full_name) );
    if (!tpm)
        return NULL;

    tpsm = thread_pool_submodule_mapping_find( tpm, smi->name );
    printf("Find submapping for %s, %s got %p\n", emi->full_name, smi->name, tpsm );
    if (!tpsm)
        return NULL;
    return sl_wl_thread_name( tpsm->thread_ptr );
}

/*a Submodule work list
 */
/*f submodule_init_clock_worklist
  Create a worklist of the given size - each call must permit prepreclock, preclock and clock function calls, and will be assigned to a module
 */
void c_engine::submodule_init_clock_worklist( void *engine_handle, int number_of_calls )
{
    t_engine_module_instance *emi;
    emi = (t_engine_module_instance *)engine_handle;

    if (thread_pool_mapped_submodules(emi)) // Then the module_instance thread pool should be valid
    {
        emi->thread_pool = this->thread_pool;
    }
    else
    {
        emi->thread_pool = sl_wl_create_thread_pool();
    }
    emi->worklist = sl_wl_add_worklist( (t_sl_wl_thread_pool_ptr)(emi->thread_pool), emi->full_name, number_of_calls, (int)se_wl_item_count );
}

/*f submodule_set_clock_worklist_prepreclock
  Set a worklist item to 'prepreclock' a module
 */
t_sl_error_level c_engine::submodule_set_clock_worklist_prepreclock( void *engine_handle, int call_number, void *submodule_handle )
{
    t_engine_module_instance *emi, *smi;
    emi = (t_engine_module_instance *)engine_handle;
    smi = (t_engine_module_instance *)submodule_handle;

    if ((!emi) || (!smi))
        return error_level_serious;

    sl_wl_set_work_head( (t_sl_wl_worklist *)(emi->worklist), call_number, smi->name, "prepreclock", NULL );
    if (smi->prepreclock_fn_list)
    {
        sl_wl_set_work_item( (t_sl_wl_worklist *)(emi->worklist), call_number, se_wl_item_prepreclock, (t_sl_wl_callback_fn)(smi->prepreclock_fn_list->data.prepreclock.prepreclock_fn), smi->prepreclock_fn_list->handle );
    }

    const char *thread_name;
    thread_name = thread_pool_submodule_mapping( emi, smi );
    if (thread_name)
        return sl_wl_assign_work_to_thread( (t_sl_wl_worklist *)(emi->worklist), call_number, thread_name );
    return error_level_okay; // No thread to assign it to; use default
}

/*f submodule_set_clock_worklist_clock
  Set a worklist item to 'preclock' and 'clock' a module clock
 */
t_sl_error_level c_engine::submodule_set_clock_worklist_clock( void *engine_handle, void *submodule_handle, int call_number, void *submodule_clock_handle, int posedge )
{
    t_engine_module_instance *emi, *smi;
    t_engine_function *clk;
    emi = (t_engine_module_instance *)engine_handle;
    smi = (t_engine_module_instance *)submodule_handle;
    clk = (t_engine_function *)submodule_clock_handle;

    if ((!emi) || (!smi))
        return error_level_serious;

    if (!clk)
        return error_level_fatal;

    sl_wl_set_work_head( (t_sl_wl_worklist *)(emi->worklist), call_number, smi->name, clk->name, NULL );
    if (posedge)
    {
        sl_wl_set_work_item( (t_sl_wl_worklist *)(emi->worklist), call_number, se_wl_item_preclock, (t_sl_wl_callback_fn)(clk->data.clock.posedge_preclock_fn), clk->handle );
        sl_wl_set_work_item( (t_sl_wl_worklist *)(emi->worklist), call_number, se_wl_item_clock,    (t_sl_wl_callback_fn)(clk->data.clock.posedge_clock_fn), clk->handle );
    }
    else
    {
        sl_wl_set_work_item( (t_sl_wl_worklist *)(emi->worklist), call_number, se_wl_item_preclock, (t_sl_wl_callback_fn)(clk->data.clock.negedge_preclock_fn), clk->handle );
        sl_wl_set_work_item( (t_sl_wl_worklist *)(emi->worklist), call_number, se_wl_item_clock,    (t_sl_wl_callback_fn)(clk->data.clock.negedge_clock_fn), clk->handle );
    }

    const char *thread_name;
    thread_name = thread_pool_submodule_mapping( emi, smi );
    if (thread_name)
        return sl_wl_assign_work_to_thread( (t_sl_wl_worklist *)(emi->worklist), call_number, thread_name );
    return error_level_okay;
}

/*f submodule_call_worklist
 */
t_sl_error_level c_engine::submodule_call_worklist( void *engine_handle, t_se_worklist_call wl_call, int *guard )
{
    t_sl_error_level r;
    t_engine_module_instance *emi;
    emi = (t_engine_module_instance *)engine_handle;

    if (!emi)
        return error_level_serious;

    mutex_claim(engine_mutex_call_worklist);
    r = sl_wl_execute_worklist( (t_sl_wl_worklist *)(emi->worklist), guard, wl_call );
    mutex_release(engine_mutex_call_worklist);
    return r;
}
    
/*a Mutex handling
 */
/*f c_engine::mutex_claim
 */
void c_engine::mutex_claim( t_engine_mutex mutex )
{
    if (!thread_pool) return;
    sl_wl_mutex_claim( (t_sl_wl_thread_pool_ptr) thread_pool, mutex );
}

/*f c_engine::mutex_release
 */
void c_engine::mutex_release( t_engine_mutex mutex )
{
    if (!thread_pool) return;
    sl_wl_mutex_release( (t_sl_wl_thread_pool_ptr) thread_pool, mutex );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


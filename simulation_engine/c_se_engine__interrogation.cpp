/*a Copyright
  
  This file 'c_se_engine__interrogation.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "sl_debug.h"
#include "sl_general.h"
#include "sl_cons.h"
#include "se_external_module.h"
#include "se_engine_function.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Types
 */
/*t t_se_interrogation_data
 */
typedef enum
{
    signal_type_none,
    signal_type_input,
    signal_type_output,
    signal_type_clock
} t_se_signal_type;
typedef struct t_se_interrogation_data
{
    struct t_se_interrogation_data *next_in_list;
    struct t_se_interrogation_data *prev_in_list;
    t_engine_signal *global;
    t_engine_clock *clock;
    t_engine_function *signal;
    t_engine_module_instance *module_instance;
    int state_number;
    t_engine_state_desc_list *sdl;
    t_engine_state_desc *state_desc;
    t_se_signal_type signal_type;
} t_se_interrogation_data;

/*a Instance and state interrogation methods
 */
/*f c_engine::enumerate_instances
 */
void *c_engine::enumerate_instances( void *previous_handle, int skip )
{
     t_engine_module_instance *emi;
     emi = (t_engine_module_instance *)previous_handle;

     if (!emi)
     {
          emi = module_instance_list;
     }
     else
     {
          emi=emi->next_instance;
     }

     for (; emi && (skip>0); emi=emi->next_instance, skip--);

     return (void *)emi;
}

/*f c_engine::enumerate_instances
 */
t_sl_error_level c_engine::enumerate_instances( t_sl_cons_list *cl )
{
     t_engine_module_instance *emi;
     t_sl_cons_list item_list;

     sl_cons_reset_list( cl );
     for (emi = module_instance_list; emi; emi=emi->next_instance)
     {
          sl_cons_reset_list( &item_list );
          sl_cons_append( &item_list, sl_cons_item( emi->full_name, 1 ));
          sl_cons_append( &item_list, sl_cons_item( se_external_module_name(emi->module_handle), 1 ));
          sl_cons_append( cl, sl_cons_item( &item_list ) );
     }
     return error_level_okay;
}

/*f c_engine::set_state_handle
 */
int c_engine::set_state_handle( void *engine_handle, int n )
{
     t_engine_module_instance *emi;
     t_engine_state_desc_list *sdl;

     emi = (t_engine_module_instance *)engine_handle;
     if (!emi)
     {
          return 0;
     }

     emi->sdl_to_view = NULL;
     emi->state_to_view = 0;

     for ( sdl=emi->state_desc_list; sdl; sdl=sdl->next_in_list)
     {
          if (n<sdl->num_state)
               break;
          n-=sdl->num_state;
     }

     if (!sdl)
          return 0;

     emi->sdl_to_view = sdl;
     emi->state_to_view = n;

     return 1;
}

/*f c_engine::get_state_name_and_type
 */
t_engine_state_desc_type c_engine::get_state_name_and_type( void *engine_handle, const char **module_name, const char **state_prefix, const char **state_name )
{
     t_engine_module_instance *emi;

     emi = (t_engine_module_instance *)engine_handle;
     if ( (!emi) || (!emi->sdl_to_view) )
     {
          return engine_state_desc_type_none;
     }
     if (module_name)
          *module_name = emi->full_name;
     if (state_prefix)
          *state_prefix = emi->sdl_to_view->prefix;
     if (state_name)
         *state_name = emi->sdl_to_view->state_desc[ emi->state_to_view ].name;
     return emi->sdl_to_view->state_desc[ emi->state_to_view ].type;
}

/*f c_engine::get_state_value_string
 */
int c_engine::get_state_value_string( void *engine_handle, char *buffer, int buffer_size )
{
     t_engine_module_instance *emi;
     t_engine_state_desc *desc;
     void *ptr;
     int n;
     int state;

     emi = (t_engine_module_instance *)engine_handle;
     if ( (!emi) || (!emi->sdl_to_view) )
     {
          return 0;
     }

     desc = &emi->sdl_to_view->state_desc[ emi->state_to_view ];
     ptr = emi->sdl_to_view->data_base_ptr;
     if (desc->ptr)
     {
          ptr = desc->ptr;
     }
     ptr = (void *) (((char *)ptr)+desc->offset);
     if (emi->sdl_to_view->data_indirected)
     {
         ptr = *((void **)ptr);
     }
     n=0;
     switch (desc->type)
     {
     case engine_state_desc_type_none:
          n = snprintf( buffer, buffer_size, "<none>" );
          break;
     case engine_state_desc_type_bits:
          sl_print_bits_hex( buffer, buffer_size, (int *)ptr, desc->args[0] );
          n = strlen(buffer);
          break;
     case engine_state_desc_type_array:
          n = snprintf( buffer, buffer_size, "%d %d", desc->args[0], desc->args[1] );
          break;
     case engine_state_desc_type_fsm:
          state = ((int *)ptr)[0];
          if ((state<0) || (state>=desc->args[1]))
          {
               n = snprintf( buffer, buffer_size, "out of range: %d", state );
          }
          else if (desc->arg_ptr[0])
          {
               n = snprintf( buffer, buffer_size, "%s", ((char **)(desc->arg_ptr[0]))[state] );
          }
          else
          {
               n = snprintf( buffer, buffer_size, "%d", state );
          }
          break;
     case engine_state_desc_type_exec_file:
          n = snprintf( buffer, buffer_size, "not implemented" );
          break;
     }
     buffer[buffer_size-1] = 0;
     return (n<(buffer_size-1));
}

/*f c_engine::get_state_value_data_and_sizes
 */
int c_engine::get_state_value_data_and_sizes( void *engine_handle, int **data, int *sizes )
{
     t_engine_module_instance *emi;
     t_engine_state_desc *desc;
     void *ptr;

     emi = (t_engine_module_instance *)engine_handle;
     if ( (!emi) || (!emi->sdl_to_view) )
     {
          return 0;
     }

     desc = &emi->sdl_to_view->state_desc[ emi->state_to_view ];
     ptr = emi->sdl_to_view->data_base_ptr;
     if (desc->ptr)
     {
          ptr = desc->ptr;
     }
     ptr = (void *) (((char *)ptr)+desc->offset);
     if (emi->sdl_to_view->data_indirected)
     {
         ptr = *((void **)ptr);
     }
     switch (desc->type)
     {
     case engine_state_desc_type_none:
          data[0] = NULL;
          sizes[0] = 0;
          break;
     case engine_state_desc_type_bits:
          data[0] = (int *)ptr;
          sizes[0] = desc->args[0];
          sizes[1] = 0;
          break;
     case engine_state_desc_type_array:
         //printf("Warning - desc_type_array used to expect int **, now expects int *\n");
          data[0] = (int *)ptr;
          sizes[0] = desc->args[0];
          sizes[1] = desc->args[1];
          sizes[2] = 0;
          break;
     case engine_state_desc_type_fsm:
          data[0] = (int *)ptr;
          sizes[0] = desc->args[0];
          sizes[1] = desc->args[1];
          sizes[2] = 0;
          break;
     case engine_state_desc_type_exec_file:
          data[0] = NULL;
          sizes[0] = 0;
          break;
     }
     return 1;
}

/*f c_engine::get_instance_name
 */
const char *c_engine::get_instance_name( void *engine_handle )
{
     t_engine_module_instance *emi;
     emi = (t_engine_module_instance *)engine_handle;
     if (!emi)
          return "<none>";
     return emi->name;
}

/*f c_engine::get_instance_full_name
 */
const char *c_engine::get_instance_full_name( void *engine_handle )
{
     t_engine_module_instance *emi;
     emi = (t_engine_module_instance *)engine_handle;
     if (!emi)
          return "<none>";
     return emi->full_name;
}

/*f c_engine::get_instance_name
 */
const char *c_engine::get_instance_name( void )
{
     return instance_name;
}

/*f c_engine::get_next_instance
 */
c_engine *c_engine::get_next_instance( void )
{
     return next_instance;
}

/*a Find-from-name methods
 */
/*f c_engine::find_module_instance( parent, name )
 */
void *c_engine::find_module_instance( void *parent, const char *name )
{
     t_engine_module_instance *emi;
     t_engine_module_instance *pemi = (t_engine_module_instance *)parent;
     unsigned int name_hash = sl_str_hash( name, -1 );

     for (emi=pemi?pemi->first_child_instance:toplevel_module_instance_list; emi; emi=emi->next_sibling_instance )
     {
          if ( (parent==emi->parent_instance) &&
               (name_hash == emi->name_hash) && 
               (!strcmp(emi->name,name)) )
          {
               return (void *)emi;
          }
     }
     return NULL;
}

/*f c_engine::find_module_instance( parent, name, length )
 */
void *c_engine::find_module_instance( void *parent, const char *name, int length )
{
     t_engine_module_instance *emi;
     t_engine_module_instance *pemi = (t_engine_module_instance *)parent;
     unsigned int name_hash = sl_str_hash( name, length );

     SL_DEBUG( sl_debug_level_info, "c_engine::find_module_instance", "Find entity with parent %p, name %s (length %d)", parent, name, length );

     for (emi=pemi?pemi->first_child_instance:toplevel_module_instance_list; emi; emi=emi->next_sibling_instance )
     {
         if ( (parent==emi->parent_instance) && (parent))
         {
             SL_DEBUG( sl_debug_level_info, "c_engine::find_module_instance:loop", "Check entity %p parent %p name %s", emi, emi->parent_instance, emi->name );
         }
         if ( (name_hash=emi->name_hash) && 
              (parent==emi->parent_instance) &&
              !strncmp(emi->name, name, length) &&
              (emi->name[length]==0) )
         {
             return (void *)emi;
         }
     }
     return NULL;
}

/*f c_engine::find_module_instance( full_name )
 */
void *c_engine::find_module_instance( const char *full_name )
{
     t_engine_module_instance *emi;
     unsigned int full_name_hash = sl_str_hash( full_name, -1 );
     
     for (emi=module_instance_list; emi; emi=emi->next_instance )
     {
         if ( (full_name_hash==emi->full_name_hash) &&
              (!strcmp(emi->full_name,full_name)) )
          {
               return (void *)emi;
          }
     }
     return NULL;
}

/*f c_engine::find_module_instance( full_name, length )
 */
void *c_engine::find_module_instance( const char *full_name, int length )
{
    t_engine_module_instance *emi;
    unsigned int full_name_hash = sl_str_hash( full_name, length );

    for (emi=module_instance_list; emi; emi=emi->next_instance )
    {
        if ( (full_name_hash==emi->full_name_hash) &&
             !strncmp(emi->full_name, full_name, length) &&
             (emi->full_name[length]==0) )
        {
            return (void *)emi;
        }
    }
    return NULL;
}

/*f c_engine::module_instance_send_message( handle, message )
  Return -1 for bad ptr, -2 for instance has no message function, else 0 for okay and message should have a result
 */
int c_engine::module_instance_send_message( void *module, t_se_message *message )
{
     t_engine_module_instance *emi;
     emi = (t_engine_module_instance *)module;
     if (!emi)
         return -1;
     if (!emi->message_fn_list)
         return 0;
     se_engine_function_call_invoke_all_argp( emi->message_fn_list, (void *) message );
     return 1;
}

/*f c_engine::find_global
 */
void *c_engine::find_global( const char *name )
{
     t_engine_signal *sig;
     for (sig=global_signal_list; sig; sig=sig->next_in_list)
     {
          if (!strcmp(sig->global_name, name))
          {
               return (void *)sig;
          }
     }
     return NULL;
}

/*f c_engine::find_state
    find the state (clock, global, or state of a module instance)
    return 0 for none, 1 for clock, 2 for global, 3 for module instance state
    For module instance state, update the string at name to be the module instance name and make (*id) the number of the state in the instance
 */
int c_engine::find_state( char *name, int *id )
{
     int i, j, n;
     char *state_name;     
     t_engine_module_instance *emi;
     t_engine_state_desc_list *sdl;

     if (find_clock(name))
     {
          return 1;
     }
     if (find_global(name))
     {
          return 2;
     }

     for (i=0; (name[i]!='.') && (name[i]); i++);
     if (!name[i])
          return 0;
     name[i] = 0;
     state_name = name+i+1;

     emi = (t_engine_module_instance *)find_module_instance( name );
     if (!emi)
          return 0;

     for (n=0, sdl=emi->state_desc_list; sdl; n+=sdl->num_state,sdl=sdl->next_in_list )
     {
          if (!sdl->prefix)
          {
               j=-1;
          }
          else
          {
               j = strlen(sdl->prefix);
          }
          if ( (j<0) ||
               ( (!strncmp(sdl->prefix, state_name, j)) &&
                 (state_name[j]=='.') ) )
          {
               for (i=0; i<sdl->num_state; i++)
               {
                    if (!strcmp( sdl->state_desc[i].name, state_name+j+1 ))
                    {
                         *id = i+n;
                         if (j>=0)
                              state_name[j] = 0;
                         return 2;
                    }
               }
          }
     }
     return 0;
}

/*f c_engine::find_clock
 */
t_engine_clock *c_engine::find_clock( const char *clock_name )
{
    t_engine_clock *clk;
    for (clk=global_clock_list; clk; clk=clk->next_in_list)
    {
        if (!strcmp(clk->global_name, clock_name))
        {
            return clk;
        }
    }
    return NULL;
}

/*f c_engine::interrogation_handle_create
 */
t_se_interrogation_handle c_engine::interrogation_handle_create( void )
{
     t_se_interrogation_data *id;

     id = (t_se_interrogation_data *)malloc(sizeof(t_se_interrogation_data));
     if (!id)
          return NULL;

     id->next_in_list = interrogation_handles;
     id->prev_in_list = NULL;
     if (interrogation_handles)
          interrogation_handles->prev_in_list = id;
     interrogation_handles = id;

     id->global = NULL;
     id->clock = NULL;
     id->signal = NULL;
     id->module_instance = NULL;
     id->state_number = -1;
     id->sdl = NULL;
     id->state_desc = NULL;
     id->signal_type = signal_type_none;
     return id;
}

/*f c_engine::interrogation_handle_free
 */
void c_engine::interrogation_handle_free( t_se_interrogation_handle entity )
{
     if (!entity)
          return;

     if (entity->prev_in_list)
     {
          entity->prev_in_list->next_in_list = entity->next_in_list;
     }
     else
     {
          interrogation_handles = entity->next_in_list;
     }

     if (entity->next_in_list)
     {
          entity->next_in_list->prev_in_list = entity->prev_in_list;
     }

     free(entity);
}

/*f c_engine::find_entity - find an entity (clock, signal, module instance, input, output, state) and return an interrogation handle
  Find a named entity.
  It could be a global
  It could be a module instance
  It could be an input, an output, or registered state of the module
  If a global, set the global entry of the structure
  If a clock, set the global entry of the structure
  If a module instance, set the module_instance entry of the structure
  If an input of a module_instance, set the module_instance
 */
t_se_interrogation_handle c_engine::find_entity( const char *string )
{
     int i, j, k, n;
     t_se_interrogation_handle ih;
     t_engine_state_desc_list *sdl;

     SL_DEBUG( sl_debug_level_info, "c_engine::find_entity", "Find entity %s", string );

     if (!string)
          return NULL;
     ih = interrogation_handle_create();
     if (!ih)
          return NULL;

     ih->global = (t_engine_signal *)find_global( string );
     ih->clock  = find_clock( string );
     for (i=0; (string[i]) && (string[i]!='.'); i++);
     ih->module_instance = (t_engine_module_instance *)find_module_instance( string, i );
     if (ih->module_instance && string[i])
     {
         t_engine_module_instance *smi;
         while (string[i])
         {
             for (j=i+1; (string[j]) && (string[j]!='.'); j++);
             SL_DEBUG( sl_debug_level_info, "c_engine::find_entity", "Found entity %p in %s to %d, looking for submodule %s", ih->module_instance, string, i, string+i+1 );
             smi = (t_engine_module_instance *)find_module_instance( ih->module_instance, string+i+1, j-(i+1) );
             if (smi)
             {
                 ih->module_instance = smi;
                 i = j;
                 SL_DEBUG( sl_debug_level_info, "c_engine::find_entity", "Found submodule %p", ih->module_instance );
             }
             else
             {
                 break;
             }
         }
         SL_DEBUG( sl_debug_level_info, "c_engine::find_entity", "Settled on entity %p in %s to %d, looking for submodule", ih->module_instance, string, i );
         if (string[i])
         {
             for ( n=0,sdl=ih->module_instance->state_desc_list; sdl; n+=sdl->num_state,sdl=sdl->next_in_list )
             {
                 if (!sdl->prefix)
                 {
                     j=-1; // -1 let's us look at the correct character in i+j+2 if there is no prefix
                 }
                 else
                 {
                     j = strlen(sdl->prefix);
                 }
                 if ( (j<0) ||
                      ( (!strncmp(sdl->prefix, string+i+1, j)) &&
                        (string[i+j+1]=='.') ) )
                 {
                     for (k=0; k<sdl->num_state; k++)
                     {
                         if (!strcmp( sdl->state_desc[k].name, string+i+j+2 ))
                         {
                             ih->state_number = k+n;
                             ih->sdl = sdl;
                             ih->state_desc = sdl->state_desc+k;
                             break;
                         }
                     }
                 }
             }
             if ((ih->signal = se_engine_function_find_function( ih->module_instance->input_list, string+i+1 ))!=NULL)
             {
                 ih->signal_type = signal_type_input;
             }
             else if ((ih->signal = se_engine_function_find_function( ih->module_instance->output_list, string+i+1 ))!=NULL)
             {
                 ih->signal_type = signal_type_output;
             }
         }
     }
     SL_DEBUG( sl_debug_level_info, "c_engine::find_entity", "Global %p module instance %p", ih->global, ih->module_instance );
     if ((!ih->global) && (!ih->clock) && (!ih->module_instance))
     {
         interrogation_handle_free( ih );
         return NULL;
     }
     return ih;
}

/*f c_engine::interrogate_count_hierarchy - Count the number of items in an entity which are of a particular subset of types; 0 or 1 for signals, clocks etc, count of things in a module
  If the entity is a signal, then the answer is 1.
  If the main entity is a module, then count its state descriptors (and inputs and outputs if required)
 */
int c_engine::interrogate_count_hierarchy( t_se_interrogation_handle entity, t_engine_state_desc_type_mask type_mask, t_engine_interrogate_include_mask include_mask )
{
     t_engine_state_desc_list *sdl;
     t_engine_function *signal;
     int i;
     int num;

     if (!entity)
          return 0;
     if (entity->module_instance && entity->state_desc) // State in a specified module
     {
          return (type_mask>>((int)entity->state_desc->type)&1);
     }
     if (entity->global) // global signal - return 1 if bits were requested, 0 otherwise
     {
          return ((type_mask & engine_state_desc_type_mask_bits)!=0);
     }
     if (entity->clock) // clock - return 1 if bits were requested, 0 otherwise
     {
          return ((type_mask & engine_state_desc_type_mask_bits)!=0);
     }
     if (entity->signal) // input (or output?) of a module - return 1 if bits were requested, 0 otherwise
     {
          return ((type_mask & engine_state_desc_type_mask_bits)!=0);
     }
     if (entity->module_instance) // asked for a module; count all that was asked, but don't duplicate state if its an output
     {
          num=0;
          if (include_mask & engine_interrogate_include_mask_submodules)
          {
              t_engine_module_instance *emi;
              for (emi=module_instance_list; emi; emi=emi->next_instance)
              {
                  if (emi->parent_instance==entity->module_instance)
                  {
                      num++;
                  }
              }
          }
          if (include_mask & engine_interrogate_include_mask_state)
          {
               for (sdl=entity->module_instance->state_desc_list; sdl; sdl=sdl->next_in_list)
               {
                    for (i=0; i<sdl->num_state; i++)
                    {
                        if (type_mask&(1<<sdl->state_desc[i].type))
                        {
                            //signal = NULL;
                            //if (include_mask & engine_interrogate_include_mask_outputs) // Don't include in state if it is an output
                            //{
                            //    for (signal = entity->module_instance->output_list; signal; signal=signal->next_in_list)
                            //    {
                            //        if (!strcmp(signal->name, sdl->state_desc[i].name))
                            //        {
                            //            break;
                            //        }
                            //    }
                            //}
                            //if (!signal)
                            //{
                                num++;
                                //}
                        }
                    }
               }
          }
          if (include_mask & engine_interrogate_include_mask_inputs)
          {
               for (signal = entity->module_instance->input_list; signal; signal=signal->next_in_list)
               {
                    num += ((type_mask & engine_state_desc_type_mask_bits)!=0);
               }
          }
          if (include_mask & engine_interrogate_include_mask_outputs)
          {
               for (signal = entity->module_instance->output_list; signal; signal=signal->next_in_list)
               {
                   if (signal->data.output.has_clocked_state_desc==-1) // Not known if it is shadowed by state yet - find out
                   {
                       check_output_shadows_state( entity->module_instance, signal );
                   }
                   if ( (signal->data.output.has_clocked_state_desc==0) ||
                        !(include_mask & engine_interrogate_include_mask_state) ) // Only consider if it is not shadowed by state or we don't want state
                   {
                       num += ((type_mask & engine_state_desc_type_mask_bits)!=0);
                   }
               }
          }
          if (include_mask & engine_interrogate_include_mask_clocks)
          {
              for (signal = entity->module_instance->clock_fn_list; signal; signal=signal->next_in_list)
              {
                  num += ((type_mask & engine_state_desc_type_mask_bits)!=0);
              }
          }
          return num;
     }

     return 0;
}

/*f c_engine::check_output_shadows_state
 */
void c_engine::check_output_shadows_state( t_engine_module_instance *emi, t_engine_function *signal )
{
    t_engine_state_desc_list *sdl;
    int found=0;
    for (sdl=emi->state_desc_list; sdl && !found; sdl=sdl->next_in_list)
    {
        for (int i=0; i<sdl->num_state; i++)
        {
            if (!strcmp(signal->name, sdl->state_desc[i].name))
            {
                found = 1;
                break;
            }
        }
    }
    signal->data.output.has_clocked_state_desc = found;
}

/*f c_engine::interrogate_enumerate_hierarchy
  Return a new (or reused) sub_ih for a sub_entity of an entity, given the number of that subentity and selecting from a particular subset of types
  Return 0 and free the sub_ih if there is no sub_entity of that number
  Return 1 and create or reuse sub_ih if there is a sub_entity of that number
  If the main entity is a signal, then the sub_entity number 0 is still that signal, and set sub_ih to that
  If the main entity is a module, then scan its state descriptors, inputs and outputs (dependent on mask)
  Do not include state and output if one has both
 */
int c_engine::interrogate_enumerate_hierarchy( t_se_interrogation_handle entity, int sub_number, t_engine_state_desc_type_mask type_mask, t_engine_interrogate_include_mask include_mask, t_se_interrogation_handle *sub_entity )
{
     int i, num;
     int result;
     t_se_interrogation_handle value;
     t_engine_state_desc_list *sdl;
     t_engine_function *signal;

     if (*sub_entity)
     {
          value = *sub_entity;
          value->global = NULL;
          value->signal = NULL;
          value->module_instance = NULL;
          value->state_number = -1;
          value->sdl = NULL;
          value->state_desc = NULL;
          value->signal_type = signal_type_none;
     }
     else
     {
          value = interrogation_handle_create();
     }
     result = 0;
     if (entity)
     {
          if (entity->module_instance && entity->state_desc && (type_mask&(1<<entity->state_desc->type)) && (sub_number==0))
          {
               value->module_instance = entity->module_instance;
               value->state_desc = entity->state_desc;
               value->sdl = entity->sdl;
               value->state_number = entity->state_number;
               result = 1;
          }
          if (entity->global && (type_mask&engine_state_desc_type_mask_bits) && (sub_number==0))
          {
               value->global = entity->global;
               result = 1;
          }
          if (entity->clock && (type_mask&engine_state_desc_type_mask_bits) && (sub_number==0))
          {
              value->clock = entity->clock;
              result = 1;
          }
          if (entity->signal && (type_mask&engine_state_desc_type_mask_bits) && (sub_number==0))
          {
               value->signal = entity->signal;
               value->signal_type = entity->signal_type;
               result = 1;
          }
          if (entity->module_instance && !entity->state_desc && !entity->signal && !entity->global)
          {
               num=0;
               if (include_mask & engine_interrogate_include_mask_submodules)
               {
                   t_engine_module_instance *emi;
                   for (emi=module_instance_list; (num<=sub_number) && emi; emi=emi->next_instance)
                   {
                       if (emi->parent_instance==entity->module_instance)
                       {
                           if (sub_number==num)
                           {
                               value->module_instance = emi;
                               result = 1;
                           }
                           num++;
                       }
                   }
               }
               if (include_mask & engine_interrogate_include_mask_state)
               {
                    for (sdl=entity->module_instance->state_desc_list; (num<=sub_number) && sdl; sdl=sdl->next_in_list)
                    {
                        //fprintf(stderr, "sdl list start %p length %d\n", sdl, sdl->num_state );
                        for (i=0; (num<=sub_number) && (i<sdl->num_state); i++)
                        {
                            //fprintf(stderr, "sdl %p n %d i %d mask %08x\n", sdl, num, i, include_mask );
                            if (type_mask&(1<<sdl->state_desc[i].type))
                            {
                                if (sub_number==num)
                                {
                                    value->module_instance = entity->module_instance;
                                    value->sdl = sdl;
                                    value->state_number = i;
                                    value->state_desc = sdl->state_desc+i;
                                    result = 1;
                                }
                                num++;
                            }
                        }
                    }
               }
               if (include_mask & engine_interrogate_include_mask_inputs)
               {
                    for (signal = entity->module_instance->input_list; (num<=sub_number) && signal; signal=signal->next_in_list)
                    {
                        //fprintf(stderr, "signal %p n %d\n", signal, num );
                         if (type_mask & engine_state_desc_type_mask_bits)
                         {
                              if (sub_number==num)
                              {
                                   value->module_instance = entity->module_instance;
                                   value->signal = signal;
                                   value->signal_type = signal_type_input;
                                   result = 1;
                              }
                              num++;
                         }
                    }
               }
               if (include_mask & engine_interrogate_include_mask_outputs)
               {
                    for (signal = entity->module_instance->output_list; (num<=sub_number) && signal; signal=signal->next_in_list)
                    {
                        if (signal->data.output.has_clocked_state_desc==-1) // Not known if it is shadowed by state yet - find out
                        {
                            check_output_shadows_state( entity->module_instance, signal );
                        }
                        if ( (signal->data.output.has_clocked_state_desc==0) ||
                             !(include_mask & engine_interrogate_include_mask_state) ) // Only consider if it is not shadowed by state or we don't want state
                        {
                            if (type_mask & engine_state_desc_type_mask_bits)
                            {
                                if (sub_number==num)
                                {
                                    value->module_instance = entity->module_instance;
                                    value->signal = signal;
                                    value->signal_type = signal_type_output;
                                    result = 1;
                                }
                                num++;
                            }
                        }
                    }
               }
               if (include_mask & engine_interrogate_include_mask_clocks)
               {
                   for (signal = entity->module_instance->clock_fn_list; (num<=sub_number) && signal; signal=signal->next_in_list)
                   {
                       if (type_mask & engine_state_desc_type_mask_bits)
                       {
                           if (sub_number==num)
                           {
                               value->module_instance = entity->module_instance;
                               value->signal = signal;
                               value->signal_type = signal_type_clock;
                               result = 1;
                           }
                           num++;
                       }
                   }
               }
          }
     }
     if (!result)
     {
          interrogation_handle_free( value );
          *sub_entity = NULL;
          return 0;
     }
     *sub_entity = value;
     return 1;
}

/*f c_engine::interrogate_find_entity
  Return a new (or reused) sub_ih for a sub_entity of an entity, given the entity_name of that subentity and selecting from a particular subset of types
  Return 0 and free the sub_ih if there is no sub_entity of that entity_name
  Return 1 and create or reuse sub_ih if there is a sub_entity of that entity_name
  Only operate if the main entity is a module; scan its state descriptors, inputs and outputs (dependent on mask)
 */
int c_engine::interrogate_find_entity( t_se_interrogation_handle entity, const char *entity_name, t_engine_state_desc_type_mask type_mask, t_engine_interrogate_include_mask include_mask, t_se_interrogation_handle *sub_entity )
{
    int i;
    int result;
    t_se_interrogation_handle value;
    t_engine_state_desc_list *sdl;
    t_engine_function *signal;

    if (*sub_entity)
    {
        value = *sub_entity;
        value->global = NULL;
        value->signal = NULL;
        value->module_instance = NULL;
        value->state_number = -1;
        value->sdl = NULL;
        value->state_desc = NULL;
        value->signal_type = signal_type_none;
    }
    else
    {
        value = interrogation_handle_create();
    }
    result = 0;
    if (entity)
    {
        if (entity->module_instance && !entity->state_desc && !entity->signal && !entity->global)
        {
            if (include_mask & engine_interrogate_include_mask_submodules)
            {
                t_engine_module_instance *emi;
                for (emi=module_instance_list; emi && !result; emi=emi->next_instance)
                {
                    if (emi->parent_instance==entity->module_instance)
                    {
                        if (!strcmp(emi->name, entity_name))
                        {
                            value->module_instance = emi;
                            result = 1;
                        }
                    }
                }
            }
            if (include_mask & engine_interrogate_include_mask_state)
            {
                for (sdl=entity->module_instance->state_desc_list; sdl && !result; sdl=sdl->next_in_list)
                {
                    int j=-1; // so that j+1 is state of tail if there is no prefix
                    if (sdl->prefix)
                    {
                        j = strlen(sdl->prefix);
                    }
                    if ( (j<0) ||
                         ( (!strncmp(sdl->prefix, entity_name, j)) &&
                           (entity_name[j]=='.') ) )
                    {
                        for (i=0; (i<sdl->num_state) && !result; i++)
                        {
                            if (type_mask&(1<<sdl->state_desc[i].type))
                            {
                                if (!strcmp( sdl->state_desc[i].name, entity_name+j+1 )) // compare the tail
                                {
                                    value->module_instance = entity->module_instance;
                                    value->sdl = sdl;
                                    value->state_number = i;
                                    value->state_desc = sdl->state_desc+i;
                                    result = 1;
                                }
                            }
                        }
                    }
                }
            }
            if (include_mask & engine_interrogate_include_mask_inputs)
            {
                for (signal = entity->module_instance->input_list; signal && !result; signal=signal->next_in_list)
                {
                    if (type_mask & engine_state_desc_type_mask_bits)
                    {
                        if (!strcmp(signal->name, entity_name))
                        {
                            value->module_instance = entity->module_instance;
                            value->signal = signal;
                            value->signal_type = signal_type_input;
                            result = 1;
                        }
                    }
                }
            }
            if (include_mask & engine_interrogate_include_mask_outputs)
            {
                for (signal = entity->module_instance->output_list; signal && !result; signal=signal->next_in_list)
                {
                    if (type_mask & engine_state_desc_type_mask_bits)
                    {
                        if (!strcmp(signal->name, entity_name))
                        {
                            value->module_instance = entity->module_instance;
                            value->signal = signal;
                            value->signal_type = signal_type_output;
                            result = 1;
                        }
                    }
                }
            }
            if (include_mask & engine_interrogate_include_mask_clocks)
            {
                for (signal = entity->module_instance->clock_fn_list; signal && !result; signal=signal->next_in_list)
                {
                    if (type_mask & engine_state_desc_type_mask_bits)
                    {
                        if (!strcmp(signal->name, entity_name))
                        {
                            value->module_instance = entity->module_instance;
                            value->signal = signal;
                            value->signal_type = signal_type_clock;
                            result = 1;
                        }
                    }
                }
            }
        }
    }
    if (!result)
    {
        interrogation_handle_free( value );
        *sub_entity = NULL;
        return 0;
    }
    *sub_entity = value;
    return 1;
}

/*f c_engine::interrogate_get_internal_fn
 */
t_engine_state_desc_type c_engine::interrogate_get_internal_fn( t_se_interrogation_handle entity, void **fn )
{
     if (!entity)
          return engine_state_desc_type_none;

     if (entity->signal && (entity->signal_type==signal_type_input))
     {
         *fn = entity->signal;
         return engine_state_desc_type_bits;
     }
     if (entity->signal && (entity->signal_type==signal_type_output))
     {
         *fn = entity->signal;
         return engine_state_desc_type_bits;
     }
     if (entity->signal && (entity->signal_type==signal_type_clock))
     {
         *fn = entity->signal;
         return engine_state_desc_type_bits;
     }
     return engine_state_desc_type_none;
}

/*f c_engine::interrogate_get_data_sizes_and_type - return size, type and where an entity's value is stored
 */
t_engine_state_desc_type c_engine::interrogate_get_data_sizes_and_type( t_se_interrogation_handle entity, t_se_signal_value **data, int *sizes )
{
     t_engine_state_desc *desc;
     void *ptr;

     if (!entity)
          return engine_state_desc_type_none;

     if (entity->module_instance && entity->state_desc)
     {
          desc = entity->state_desc;
          ptr = entity->sdl->data_base_ptr;
          if (desc->ptr)
          {
               ptr = desc->ptr;
          }
          ptr = (void *) (((char *)ptr)+desc->offset);
          if (entity->sdl->data_indirected)
          {
              ptr = *((void **)ptr);
          }
          switch (desc->type)
          {
          case engine_state_desc_type_none:
               *data = NULL;
               sizes[0] = 0;
               break;
          case engine_state_desc_type_bits:
               data[0] = (t_se_signal_value *)ptr;
               sizes[0] = desc->args[0];
               sizes[1] = 0;
               break;
          case engine_state_desc_type_array:
               data[0] = (t_se_signal_value *)ptr;
               sizes[0] = desc->args[0];
               sizes[1] = desc->args[1];
               sizes[2] = 0;
               break;
          case engine_state_desc_type_fsm:
               data[0] = (t_se_signal_value *)ptr;
               sizes[0] = desc->args[0];
               sizes[1] = desc->args[1];
               sizes[2] = 0;
               break;
          case engine_state_desc_type_exec_file:
               data[0] = NULL;
               sizes[0] = 0;
               break;
          }
          return desc->type;
     }
     if (entity->global)
     {
          if (!entity->global->driven_by)
               return engine_state_desc_type_none;
          data[0] = (t_se_signal_value *)entity->global->driven_by->signal->data.output.value_ptr;
          sizes[0] = entity->global->size;
          return engine_state_desc_type_bits;
     }
     if (entity->clock)
     {
         data[0] = &entity->clock->value;
         sizes[0] = 1;
         return engine_state_desc_type_bits;
     }
     if (entity->signal && (entity->signal_type==signal_type_input))
     {
          if (*entity->signal->data.input.value_ptr_ptr)
          {
              data[0] = (t_se_signal_value *)*entity->signal->data.input.value_ptr_ptr;
              sizes[0] = entity->signal->data.input.size;
              return engine_state_desc_type_bits;
          }
          else
          {
              data[0] = NULL;
              sizes[0] = entity->signal->data.input.size;
              return engine_state_desc_type_bits;
          }
     }
     if (entity->signal && (entity->signal_type==signal_type_output))
     {
         data[0] = (t_se_signal_value *)(entity->signal->data.output.value_ptr);
          sizes[0] = entity->signal->data.output.size;
          return engine_state_desc_type_bits;
     }
     if (entity->signal && (entity->signal_type==signal_type_clock))
     {
         int edges=0;
         data[0] = (t_se_signal_value*)sizes;
         edges |= (entity->signal->data.clock.posedge_clock_fn)?1:0;
         edges |= (entity->signal->data.clock.negedge_clock_fn)?1:0;
         sizes[0] = 1;
         sizes[1] = edges;
         return engine_state_desc_type_bits;
     }
     return engine_state_desc_type_none;
}


/*f c_engine::interrogate_get_entity_strings - get pointers to entity with possible prefix and tail
 */
int c_engine::interrogate_get_entity_strings( t_se_interrogation_handle entity, const char **module_or_global, const char **prefix, const char **tail )
{
     *module_or_global = NULL;
     *prefix = NULL;
     *tail = NULL;

     if (!entity)
     {
          return 0;
     }

     if (entity->global)
     {
         *module_or_global = entity->global->global_name;
         *prefix = NULL;
         *tail = NULL;
         return 1;
     }
     if (entity->clock)
     {
         *module_or_global = entity->clock->global_name;
         *prefix = NULL;
         *tail = NULL;
         return 1;
     }
     if (entity->signal)
     {
         if (entity->module_instance)
         {
             *module_or_global = entity->module_instance->full_name;
             *prefix = NULL;
             *tail = entity->signal->name;
         }
         else
         {
             *module_or_global = NULL;
             *prefix = NULL;
             *tail = entity->signal->name;
         }
         return 1;
     }
     if (entity->module_instance && entity->state_desc)
     {
          if (entity->sdl->prefix)
          {
             *module_or_global = entity->module_instance->full_name;
             *prefix = entity->sdl->prefix;
             *tail = entity->state_desc->name;
          }
          else
          {
             *module_or_global = entity->module_instance->full_name;
             *prefix = NULL;
             *tail = entity->state_desc->name;
          }
          return 1;
     }
     if (entity->module_instance)
     {
         *module_or_global = entity->module_instance->full_name;
         *prefix = NULL;
         *tail = NULL;
         return 1;
     }
     return 0;
}

/*f c_engine::interrogate_get_entity_path_string - get full path to an entity
 */
int c_engine::interrogate_get_entity_path_string( t_se_interrogation_handle entity, char *buffer, int buffer_size )
{
     if (!entity)
     {
          return 0;
     }

     buffer[0] = 0;
     if (entity->global)
     {
          sl_str_copy_max( buffer, entity->global->global_name, buffer_size );
          return 1;
     }
     if (entity->clock)
     {
          sl_str_copy_max( buffer, entity->clock->global_name, buffer_size );
          return 1;
     }
     if (entity->signal)
     {
          if (entity->module_instance)
          {
               snprintf( buffer, buffer_size, "%s.%s", entity->module_instance->full_name, entity->signal->name );
          }
          else
          {
               sl_str_copy_max( buffer, entity->signal->name, buffer_size );
          }
          return 1;
     }
     if (entity->module_instance && entity->state_desc)
     {
          if (entity->sdl->prefix)
          {
               snprintf( buffer, buffer_size, "%s.%s.%s", entity->module_instance->full_name, entity->sdl->prefix, entity->state_desc->name );
          }
          else
          {
               snprintf( buffer, buffer_size, "%s.%s", entity->module_instance->full_name, entity->state_desc->name );
          }
          buffer[buffer_size-1] = 0;
          return 1;
     }
     if (entity->module_instance)
     {
          sl_str_copy_max( buffer, entity->module_instance->full_name, buffer_size );
          return 1;
     }
     return 0;
}

/*f c_engine::interrogate_get_entity_clue - return some idea as to a string for the entity
 */
const char *c_engine::interrogate_get_entity_clue( t_se_interrogation_handle entity )
{
     if (!entity)
     {
          return NULL;
     }

     if (entity->global)
     {
          return entity->global->global_name;
     }
     if (entity->clock)
     {
          return entity->clock->global_name;
     }
     if (entity->signal)
     {
         return entity->signal->name;
     }
     if (entity->module_instance && entity->state_desc)
     {
         return entity->state_desc->name;
     }
     if (entity->module_instance)
     {
          return entity->module_instance->full_name;
     }
     return NULL;
}

/*f c_engine::interrogate_get_entity_value_string - get the value of an entity as a string
 */
int c_engine::interrogate_get_entity_value_string( t_se_interrogation_handle entity, char *buffer, int buffer_size )
{
     int n;
     int state;
     t_engine_state_desc_type type;
     t_se_signal_value *datas[4];
     int sizes[4];

     type = interrogate_get_data_sizes_and_type( entity, datas, sizes );
     n=0;
     switch (type)
     {
     case engine_state_desc_type_none:
          n = snprintf( buffer, buffer_size, "<none>" );
          break;
     case engine_state_desc_type_bits:
         if (datas[0])
         {
             sl_print_bits_hex( buffer, buffer_size, ((int **)datas)[0], sizes[0] );
         }
         else
         {
             strcpy( buffer, "<unc>");
         }
          n = strlen(buffer);
          break;
     case engine_state_desc_type_array:
          n = snprintf( buffer, buffer_size, "%d %d", sizes[0], sizes[1] );
          break;
     case engine_state_desc_type_fsm:
          state = datas[0][0];
          if ((state<0) || (state>=sizes[1]))
          {
               n = snprintf( buffer, buffer_size, "out of range: %d", state );
          }
//          else if (desc->arg_ptr[0])
//          {
//               n = snprintf( buffer, buffer_size, "%s", ((char **)(desc->arg_ptr[0]))[state] );
//          }
          else
          {
               n = snprintf( buffer, buffer_size, "%d", state );
          }
          break;
     case engine_state_desc_type_exec_file:
          n = snprintf( buffer, buffer_size, "<exec file>" );
          break;
     }
     buffer[buffer_size-1] = 0;
     return (n<(buffer_size-1));
}




/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


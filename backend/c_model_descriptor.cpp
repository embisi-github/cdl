/*a Copyright
  
  This file 'c_model_descriptor.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a To do
Add index/subscript expressions as dependencies for expressions if they are used in lvars which are part of an expression
 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_general.h"
#include "sl_option.h"
#include "sl_debug.h"
#include "c_sl_error.h"
#include "be_errors.h"
#include "md_target_c.h"
#include "md_target_xml.h"
#include "md_target_vhdl.h"
#include "md_target_verilog.h"
#include "c_model_descriptor.h"

/*a Defines
 */
#define instance_set_null(is) {is=NULL;}
#define t_instance_set t_md_reference_set
#define instance_set_free(is) {reference_set_free(&is);}
#define instance_set_union(is,to_add) {reference_union_sets(to_add,is);}
#define instance_set_intersect(is,to_join) {reference_intersect_sets(to_join,is);}
#define instance_set_add(is,instance) {reference_add(is,instance);}
#define instance_set_display(is) { t_md_reference_iter iter; t_md_reference *ref; reference_set_iterate_start( &is, &iter ); while ((ref=reference_set_iterate(&iter))!=NULL) { fprintf(stderr,"%s ",ref->data.instance->output_name); } }
#if 0
#define WHERE_I_AM {fprintf(stderr,"%s:%d\n",__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#endif

/*a Globals
 */
t_md_signal_value md_signal_value_zero;
t_md_signal_value md_signal_value_one;

/*a Statics
 */
/*v default_error_messages
 */
static t_sl_error_text default_error_messages[] = {
     C_BE_ERROR_DEFAULT_MESSAGES
};

/*v indent_string
 */
static const char *indent_string = "    ";

/*v output_separator
 */
static const char *output_separator="__";

/*a Constructors and destructors
 */
/*f c_model_descriptor::c_model_descriptor
 */
c_model_descriptor::c_model_descriptor( t_sl_option_list env_options, c_sl_error *error, t_md_client_string_from_reference_fn client_string_from_reference_fn, void *client_handle )
{
     int i;
     const char *name;

     name = sl_option_get_string_with_default( env_options, "be_model_name", "NO_MODEL_NAME_GIVEN" );
     this->name = sl_str_alloc_copy( name );
     this->module_list = NULL;
     this->module_hierarchy_list = NULL;
     this->error = error;
     this->types = NULL;
     error->add_text_list( 1, default_error_messages );
     this->client_string_from_reference_fn = client_string_from_reference_fn;
     this->client_handle = client_handle;
     this->statement_enumerator = 0;

     /* We can set our global signal values here. It will do.
      */
     for (i=0; i<MD_MAX_SIGNAL_BITS/MD_BITS_PER_UINT64; i++)
     {
          md_signal_value_zero.value[i] = 0;
          md_signal_value_one.value[i] = (i==0);
     }
}

/*f c_model_descriptor::~c_model_descriptor
 */
c_model_descriptor::~c_model_descriptor()
{
     if (name)
     {
          free(name);
     }

     modules_free();
}

/*a Simple interrogation functions
 */
/*f c_model_descriptor::get_name
 */
char *c_model_descriptor::get_name( void )
{
     return name;
}

/*a Model handling
 */
/*f c_model_descriptor::display_references
*/
int c_model_descriptor::display_references( t_md_output_fn output_fn, void *output_handle  )
{
    t_md_module *module;
    int result;
    result = 1;

    for (module=module_list; module; module=module->next_in_list)
    {
        result &= module_display_references( module, output_fn, output_handle );
    }
    return result;
}

/*f c_model_descriptor::display_instances
*/
int c_model_descriptor::display_instances( t_md_output_fn output_fn, void *output_handle  )
{
    t_md_module *module;
    int result;
    result = 1;

    for (module=module_list; module; module=module->next_in_list)
    {
        result &= module_display_instances( module, output_fn, output_handle );
    }
    return result;
}

/*f c_model_descriptor::analyze
*/
int c_model_descriptor::analyze( void )
{
    t_md_module *module, *last_in_hiearchy;
    t_md_module_instance *module_instance;
    int result;
    int done;

    /*b Count modules, and cross references any instantiations
     */
    result = 1;
    for (module=module_list; module; module=module->next_in_list)
    {
        result &= module_cross_reference_instantiations( module );
        module->analyzed = 0;
    }
    if (!result)
        return 0;

    /*b Now run through modules, building hierarchy list from bottom up of the hierarchy
     */
    done = 0;
    last_in_hiearchy = NULL;
    module_hierarchy_list = NULL;
    while (!done)
    {
        done = 1;
        for (module=module_list; module; module=module->next_in_list)
        {
            if (!module->analyzed)
            {
                for (module_instance = module->module_instances; module_instance; module_instance=module_instance->next_in_list)
                {
                    if ( (!module_instance->module_definition) ||
                         (!module_instance->module_definition->analyzed) )
                    {
                        break;
                    }
                }
                if (!module_instance)
                {
                    done = 0;
                    module->analyzed = 1;
                    if (last_in_hiearchy)
                    {
                        last_in_hiearchy->next_in_hierarchy = module;
                    }
                    else
                    {
                        module_hierarchy_list = module;
                    }
                    module->next_in_hierarchy = NULL;
                    last_in_hiearchy = module;
                }
            }
        }
    }

    /*b Now run through modules in bottom-up order, analyzing as we go
     */
    result = 1;
    for (module=module_hierarchy_list; module; module=module->next_in_hierarchy)
    {
        module->analyzed = 0;
        result &= module_analyze( module );
    }
    return result;
}

/*a Module handling methods
 */
/*f c_model_descriptor::module_create
 */
t_md_module *c_model_descriptor::module_create( const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *documentation )
{
     t_md_module *module;

     module = (t_md_module *)malloc(sizeof(t_md_module));
     if (!module)
     {
          error->add_error( NULL, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_module_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return NULL;
     }

     module->next_in_list = module_list;
     module_list = module;
     module_list->next_in_hierarchy = NULL;

     if (copy_name)
     {
          module->name = sl_str_alloc_copy( name );
          module->name_to_be_freed = 1;
     }
     else
     {
          module->name = (char *)name; /* copy_name==0 indicates this is freeable */
          module->name_to_be_freed = 0;
     }
     module->output_name = NULL;

     module->client_ref.base_handle = client_base_handle;
     module->client_ref.item_handle = client_item_handle;
     module->client_ref.item_reference = client_item_reference;
     module->documentation = documentation ? sl_str_alloc_copy( documentation ) : NULL;

     module->external = 0; // 1 if the module is not to be analyzed; its input and output dependencies are given explicitly, and no code is expected to be generated 
     module->analyzed = 0; // 1 after the module has been successfully analyzed
     module->combinatorial_component = 0; // 1 if combinatorial input-to-output paths are used
     module->clocks = NULL;
     module->inputs = NULL;
     module->outputs = NULL;
     module->combinatorials = NULL;
     module->nets = NULL;
     module->registers = NULL;
     module->code_blocks = NULL;
     module->last_code_block = NULL;
     module->module_instances = NULL;
     module->last_module_instance = NULL;
     module->messages = NULL;
     module->labelled_expressions = NULL;
     module->expressions = NULL;
     module->statements = NULL;
     module->switch_items = NULL;
     module->instances = NULL;
     module->lvars = NULL;
     module->port_lvars = NULL;
     module->last_statement_enumeration = 0;
     module->next_cover_case_entry=0;
     return module;
}

/*f c_model_descriptor::module_create_prototype
  Create a module prototype; it cannot contain any statements or module instantiations, just explicit ports and their explicit dependencies
 */
t_md_module *c_model_descriptor::module_create_prototype( const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference )
{
     t_md_module *module;

     module = module_create( name, copy_name, client_base_handle, client_item_handle, client_item_reference, NULL );
     if (!module)
     {
          return NULL;
     }
     module->external = 1; // 1 if the module is not to be analyzed; its input and output dependencies are given explicitly, and no code is expected to be generated 
     return module;
}

/*f c_model_descriptor::module_find
 */
t_md_module *c_model_descriptor::module_find( const char *name )
{
     t_md_module *module;

     for (module=module_list; module; module=module->next_in_list)
     {
          if (!strcmp(module->name, name))
          {
               return module;
          }
     }
     return NULL;
}

/*f c_model_descriptor::module_cross_reference_instantiations
  If the module is a prototype the instance list should be empty, so it is safe to call this for them
 */
int c_model_descriptor::module_cross_reference_instantiations( t_md_module *module )
{
    t_md_module_instance *module_instance;
    int result;

    result = 1;
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        result &= module_instance_cross_reference( module, module_instance );
    }

    return result;
}

/*f c_model_descriptor::module_analyze_invert_dependency_list
  This is given a set of instances, in an instance_iter
  Run through all the instances.
  For each instance, look at its dependencies; invert those, so that each dependency now depends on the instance
  particularly, 'child[i] depends on reference' implies 'reference has child[i] as a dependent'
  but, if child[i] depends on a clock edge, then we don't need to invert; in fact, only invert instances
 */
void c_model_descriptor::module_analyze_invert_dependency_list( t_md_type_instance_iter *instance_iter, t_md_state *state )
{
     int i;
     t_md_reference_iter iter;
     t_md_reference *reference;

     for (i=0; i<instance_iter->number_children; i++)
     {
          if (state)
          {
               reference_add( &(state->clock_ref->data.clock.dependents[ state->edge ]), instance_iter->children[i] );
               SL_DEBUG( sl_debug_level_info, "clk %s %d dependent %s", state->clock_ref->name, state->edge, instance_iter->children[i]->name );
          }
          reference_set_iterate_start( &instance_iter->children[i]->dependencies, &iter );
          while ((reference = reference_set_iterate(&iter))!=NULL)
          {
              if (reference->type==md_reference_type_instance)
              {
                  reference_add( &reference->data.instance->dependents, instance_iter->children[i] );
              }
              else if (reference->type==md_reference_type_clock_edge)
              {
                   reference_add( &(reference->data.signal->data.clock.dependents[ reference->edge ]), instance_iter->children[i] );
                   SL_DEBUG( sl_debug_level_info, "clk %s %d dependent %s", reference->data.signal->name, reference->edge, instance_iter->children[i]->name );
              }
          }
     }
}

/*f c_model_descriptor::module_analyze_dependents
  This fully extends a 'dependents' list, for both inputs and clocks
  FAQ: Does this really handle the full chain of:
    z<=1, y=z, x=y, w=x, ..., a=b, therefore 'a' has a base dependency of 'z'?
  Ans: Yes
        It does this by knowing z is a dependency of y.
        Then we have already inverted that to make y a dependent of z (the only dependent)
        Now when we analyze it here, we add the dependents of y to the dependents of z
        With the architecture of the 'iterate' function, we are guaranteed to add the new
        dependent 'x' (is dependent of y, now is also of z) so that we will pick it up in
        the loop.
 */
void c_model_descriptor::module_analyze_dependents( t_md_reference_set **dependents_ptr, void *data, int edge )
{
     t_md_reference_iter iter;
     t_md_reference *reference;
     t_md_reference_type type;
     t_md_signal *signal;
     t_md_state *state;
     int add_to_base_dependencies;
     int add_dependents;

     reference_set_iterate_start( dependents_ptr, &iter );
     while ((reference = reference_set_iterate(&iter))!=NULL)
     {
          type = reference->data.instance->reference.type;
          signal = reference->data.instance->reference.data.signal; // Only one of the two will be correct - it depends on the type
          state = reference->data.instance->reference.data.state;
          add_to_base_dependencies = 0;
          add_dependents = 0;
          //printf( "mad:type %d signal name %s\n", type, (type==md_reference_type_state)?signal->name:state->name);
          if ( (type==md_reference_type_signal) &&
               (signal->type==md_signal_type_combinatorial) )
          {
               // A comb that is dependent on the input through others has the input as a base dependency
               // And the comb is a dependent of the input
               add_to_base_dependencies = 1;
               add_dependents = 1;
          }
          else if ( (type==md_reference_type_signal) &&
                    (signal->type==md_signal_type_net) )
          {
               // A net that is dependent on the input through others has the input as a base dependency
               // And the net is a dependent of the input
               add_to_base_dependencies = 1;
               add_dependents = 1;
          }
          else if ( type==md_reference_type_state )
          {
               // State is not dependent on the input, just its clock
               // but record this as a 'base dependencie' of the state, for visibility, not for use
               add_to_base_dependencies = 1;
          }
          else
          {
              fprintf(stderr, "%s:SHOULD WE BE HERE?****************************************************************************\n", __func__ );
          }
          if (add_to_base_dependencies)
          {
               if (edge>=0)
               {
                    reference_add( &reference->data.instance->base_dependencies, (t_md_signal *)data, edge );
               }
               else
               {
                    reference_add( &reference->data.instance->base_dependencies, (t_md_type_instance *)data );
               }
          }
          if (add_dependents)
          {
               reference_union_sets( reference->data.instance->dependents, dependents_ptr );
          }
     }
}

/*f c_model_descriptor::module_analyze_dependents_of_input
  This builds the 'dependents' list of an input
  FAQ: Does this really handle the full chain of:
    z<=1, y=z, x=y, w=x, ..., a=b, therefore 'a' has a base dependency of 'z'?
  Ans: Yes
        It does this by knowing z is a dependency of y.
        Then we have already inverted that to make y a dependent of z (the only dependent)
        Now when we analyze it herem we add the dependents of y to the dependents of z
        With the architecture of the 'iterate' function, we are guaranteed to add the new
        dependent 'x' (is dependent of y, now is also of z) so that we will pick it up in
        the loop.
 */
void c_model_descriptor::module_analyze_dependents_of_input( t_md_signal *input )
{
     int i;
     SL_DEBUG( sl_debug_level_info, "analyze deps of input %s", input->name );
     for (i=0; i<input->instance_iter->number_children; i++)
     {
          module_analyze_dependents( &input->instance_iter->children[i]->dependents,
                                     input->instance_iter->children[i],
                                     -1 );
     }
}

/*f c_model_descriptor::module_analyze_dependents_of_clock_edge
  This builds the full 'dependents' list of a clock edge;
  This will be dependents of any nets that depend on the clock edge (currently on the dependents list)
  And dependents of any states clocked by the clock edge; fill those out for the state, then add that set of dependents to the clock edge
 */
void c_model_descriptor::module_analyze_dependents_of_clock_edge( t_md_module *module, t_md_signal *clock, int edge )
{
     t_md_state *state;
     int i;

     SL_DEBUG( sl_debug_level_info, "analyze deps of clock edge %s %d\n", clock->name, edge );
     module_analyze_dependents( &clock->data.clock.dependents[edge],
                                clock,
                                edge );
     for (state=module->registers; state; state=state->next_in_list)
     {
          if ( (state->clock_ref==clock) &&
               (state->edge==edge) )
          {
               for (i=0; i<state->instance_iter->number_children; i++)
               {
                    module_analyze_dependents( &state->instance_iter->children[i]->dependents,
                                               state->instance_iter->children[i],
                                               -1 );
                    reference_union_sets( state->instance_iter->children[i]->dependents, &clock->data.clock.dependents[edge] );
               }
          }
     }
     if (clock->data.clock.dependents[edge])
     {
          clock->data.clock.edges_used[edge] = 1; // Note it may also be used by asserts or module instances; so it is not just 'dependents!=NULL'
     }
}

/*f c_model_descriptor::module_analyze_generate_output_names
*/
void c_model_descriptor::module_analyze_generate_output_names(  t_md_type_instance_iter *instance_iter )
{
     int i, j, k;
     int temp_size;
     char *temp_name, *name;

     temp_name = NULL;
     temp_size = 0;
     for (i=0; i<instance_iter->number_children; i++)
     {
          name = instance_iter->children[i]->name;
          j = strlen( name );
          if ((!temp_name) || ((2*j+1)>temp_size))
          {
               if (temp_name)
                    free(temp_name);
               temp_name = (char *)malloc(2*j+1);
               temp_size=2*j+1;
               if (!temp_name)
                    return;
          }
          for (j=k=0; name[j]; j++)
          {
               if (name[j]=='.')
               {
                   int l;
                   for (l=0; output_separator[l]; l++)
                       temp_name[k++] = output_separator[l];
               }
               else
               {
                    temp_name[k++] = name[j]; 
               }
          }
          temp_name[k++] = 0;

          if (instance_iter->children[i]->output_name)
               free(instance_iter->children[i]->output_name);
          instance_iter->children[i]->output_name = sl_str_alloc_copy( temp_name );
     }
     if (temp_name)
          free(temp_name);
}

/*f c_model_descriptor::module_analyze
  Analysis goes as follows:
    All signals should have instances - check
    Analyze code blocks, so that:
      code_blocks have sets of dependencies; combs, nets and state they depend on (and so do statements within)
      code_blocks have sets of state and combs effected (and so do statements within)
    Analyze module instantiations, so that:
      nets attached to combinatorial outputs have dependencies of all combinatorial inputs to their instances
      nets attached to clocked outputs have dependencies of all 
      module instantiations have sets of dependencies; all clock edges, combs, state and nets they depend on
      module instantiations have sets of nets they effect (attached to outputs of the module instance)
    Now invert the dependency sets, so that we generate dependent graphs
      take all combinatorials, nets, and states; whatever they depend on immediately, make them dependents of.
        so all clock edges will have sets of dependents that are nets attached to outputs generated by those clock edges
          and states driven by the clock edge
        so all states, nets and combs will have sets of dependents that are nets, states and combs that depend on them immediately
    Now take the sets of dependents for clock edges, and follow graphs to make these sets complete (not just immediate) dependents, and set base dependencies of all their dependents to include the clock edge
    Now take the sets of dependents for inputs, and follow graphs to make these sets complete (not just immediate) dependents, and set base dependencies of all their dependents to include the input
 */
int c_model_descriptor::module_analyze( t_md_module *module )
{
     t_md_code_block *code_block;
     t_md_module_instance *module_instance;
     t_md_state *state;
     t_md_signal *signal;

     SL_DEBUG( sl_debug_level_info, "Analyze module %p %s", module, module->name );

     if (module->external)
     {
         module->combinatorial_component = 0;
         for (signal=module->inputs; signal; signal=signal->next_in_list)
         {
             if (signal->data.input.used_combinatorially)
             {
                 module->combinatorial_component = 1;
             }
         }
         for (signal=module->inputs; signal; signal=signal->next_in_list)
         {
             module_analyze_generate_output_names( signal->instance_iter );
         }
         for (signal=module->outputs; signal; signal=signal->next_in_list)
         {
             module_analyze_generate_output_names( signal->instance_iter );
         }
         return 1;
     }

     /*b First ensure that all the outputs either have an internal state, an internal combinatorial or an internal net - if not, add internal combinatorials (SKIP FOR NOW)
      */
     for (signal = module->outputs; signal; signal=signal->next_in_list )
     {
          if ( (!signal_find( signal->name, module->combinatorials)) &&
               (!signal_find( signal->name, module->nets)) &&
               (!state_find( signal->name, module->registers)) )
          {
//               signal_add_combinatorial( module, signal->name, 0, signal->client_ref.base_handle, signal->client_ref.item_handle, signal->client_ref.item_reference, signal->data.signal.width );
               fprintf(stderr,"Should add combinatorial %s to shadow non-registered signal\n", signal->name ); // NOTE //
          }
     }

     /*b Now ensure all inputs, combs, nets and state have instances - we cannot manage otherwise
      */
     {
         int error;
         error = 0;
         for (signal=module->combinatorials; signal; signal=signal->next_in_list)
         {
             if (!signal->instance)
                 error=1;
         }
         for (signal=module->nets; signal; signal=signal->next_in_list)
         {
             if (!signal->instance)
                 error=1;
         }
         for (state=module->registers; state; state=state->next_in_list)
         {
             if (!state->instance)
                 error=1;
         }
         for (signal=module->inputs; signal; signal=signal->next_in_list)
         {
             if (!signal->instance)
                 error=1;
         }
         for (signal=module->outputs; signal; signal=signal->next_in_list)
         {
             if (!signal->instance)
                 error=1;
         }
         if (error)
         {
             fprintf( stderr, "****************************************\n****************************************\nFatal error in analyzing module\n");
             return 0;
         }
     }

     /*b Now generate output names for all instance data
      */
     for (state=module->registers; state; state=state->next_in_list)
     {
          module_analyze_generate_output_names( state->instance_iter );
     }
     for (signal=module->inputs; signal; signal=signal->next_in_list)
     {
          module_analyze_generate_output_names( signal->instance_iter );
     }
     for (signal=module->outputs; signal; signal=signal->next_in_list)
     {
          module_analyze_generate_output_names( signal->instance_iter );
     }
     for (signal=module->combinatorials; signal; signal=signal->next_in_list)
     {
          module_analyze_generate_output_names( signal->instance_iter );
     }
     for (signal=module->nets; signal; signal=signal->next_in_list)
     {
          module_analyze_generate_output_names( signal->instance_iter );
     }

     /*b Now analyze all the code blocks
      */
     this->statement_enumerator = 0;
     for (code_block=module->code_blocks; code_block; code_block=code_block->next_in_list)
     {
         code_block_analyze( code_block, md_usage_type_rtl );
     }
     module->last_statement_enumeration = statement_enumerator;

     /*b Now analyze all the module instantiations
       This should:
         set the combinatorial dependencies of a module instance to all combs/nets/states used in expressions for combinatorial inputs to the instance
         add the instance's combinatorial dependencies to the dependencies of combinatorial output nets 
         add the clock edges used by the instance to the dependencies of clocked output nets
      */
     for (module_instance = module->module_instances; module_instance; module_instance=module_instance->next_in_list)
     {
         module_instance_analyze( module, module_instance );
     }

     /*b Now check that all inputs/outputs are NOT arrays of bit vectors
      */
     for (signal=module->inputs; signal; signal=signal->next_in_list)
     {
         int i;
         t_md_type_instance_iter *instance_iter;

         instance_iter = signal->instance_iter;
         for (i=0; i<instance_iter->number_children; i++)
         {
             if ((instance_iter->children[i]->type==md_type_instance_type_array_of_bit_vectors) && (instance_iter->children[i]->size>0))
             {
                 error->add_error( module, error_level_fatal, error_number_be_port_cannot_be_array, error_id_be_c_model_descriptor_module_instance_analyze,
                                   error_arg_type_malloc_string, instance_iter->children[i]->output_name,
                                   error_arg_type_none );
             }
         }
     }
     for (signal=module->outputs; signal; signal=signal->next_in_list)
     {
         int i;
         t_md_type_instance_iter *instance_iter;

         instance_iter = NULL;
         if (signal->data.output.combinatorial_ref) { instance_iter=signal->data.output.combinatorial_ref->instance_iter; }
         if (signal->data.output.net_ref)           { instance_iter=signal->data.output.net_ref->instance_iter; }
         if (signal->data.output.register_ref)      { instance_iter=signal->data.output.register_ref->instance_iter; }
         for (i=0; i<instance_iter->number_children; i++)
         {
             if ((instance_iter->children[i]->type==md_type_instance_type_array_of_bit_vectors) && (instance_iter->children[i]->size>0))
             {
                 error->add_error( module, error_level_fatal, error_number_be_port_cannot_be_array, error_id_be_c_model_descriptor_module_instance_analyze,
                                   error_arg_type_malloc_string, instance_iter->children[i]->output_name,
                                   error_arg_type_none );
             }
         }
     }

     /*b Check all nets, combs, and clocked state is actually assigned - else this breaks verilog at the end of the day
      */
     for (signal=module->combinatorials; signal; signal=signal->next_in_list)
     {
         int i;
         for (i=0; i<signal->instance_iter->number_children; i++)
         {
             if (!signal->instance_iter->children[i]->code_block)
             {
                 error->add_error( module, error_level_warning, error_number_be_never_assigned, error_id_be_c_model_descriptor_module_instance_analyze,
                                   error_arg_type_malloc_string, signal->instance_iter->children[i]->output_name,
                                   error_arg_type_none );
             }
         }
     }
     for (signal=module->nets; signal; signal=signal->next_in_list)
     {
         int i;
         for (i=0; i<signal->instance_iter->number_children; i++)
         {
             if (signal->instance_iter->children[i]->number_statements==0)
             {
                 error->add_error( module, error_level_warning, error_number_be_never_assigned, error_id_be_c_model_descriptor_module_instance_analyze,
                                   error_arg_type_malloc_string, signal->instance_iter->children[i]->output_name,
                                   error_arg_type_none );
             }
         }
     }
     for (state=module->registers; state; state=state->next_in_list)
     {
         int i;
         for (i=0; i<state->instance_iter->number_children; i++)
         {
             if (!state->instance_iter->children[i]->code_block)
             {
                 error->add_error( module, error_level_warning, error_number_be_never_assigned, error_id_be_c_model_descriptor_module_instance_analyze,
                                   error_arg_type_malloc_string, state->instance_iter->children[i]->output_name,
                                   error_arg_type_none );
             }
         }
     }

     /*b Now generate all the dependents and base_dependencies lists
       First - we know what each comb instance, net instance and state instance depend directly on.
       Invert those lists so that we know all the direct dependent instances of everything
         Run through all combinatorials' instances and add them to the dependents of everything they depend on
         Then do the same thing for states' instances
         There will then be chains from inputs/clocks through dependents lists to each net/combinatorial/state
      */
     for (signal=module->combinatorials; signal; signal=signal->next_in_list)
     {
          module_analyze_invert_dependency_list( signal->instance_iter, NULL );
     }
     for (signal=module->nets; signal; signal=signal->next_in_list)
     {
          module_analyze_invert_dependency_list( signal->instance_iter, NULL );
     }
     for (state=module->registers; state; state=state->next_in_list)
     {
          module_analyze_invert_dependency_list( state->instance_iter, state );
     }

     /*b Now run through all clocks and inputs and determine the complete list of direct and indirect dependents, and add to each such dependent this as a base dependent
      */
     for (signal=module->clocks; signal; signal=signal->next_in_list)
     {
          module_analyze_dependents_of_clock_edge( module, signal, 0 );
          module_analyze_dependents_of_clock_edge( module, signal, 1 );
     }
     for (signal=module->inputs; signal; signal=signal->next_in_list)
     {
          module_analyze_dependents_of_input( signal );
     }

     /*b Now determine whether inputs are used combinatorially for outputs at all, and if outputs are combinatorial on inputs
       For each output that is combinatorial look at its base dependents; if any are inputs, then those inputs are combinatorially used, and the outputs are combinatorially derived
      */
     for (signal=module->inputs; signal; signal=signal->next_in_list)
     {
         signal->data.input.used_combinatorially = 0;
     }
     for (signal=module->outputs; signal; signal=signal->next_in_list)
     {
         signal->data.output.derived_combinatorially = 0;
         signal->data.output.clocks_derived_from = NULL;
         if ( (signal->data.output.combinatorial_ref) ||
              (signal->data.output.net_ref) )
         {
             int i;
             t_md_reference *reference;
             t_md_reference_iter iter;
             t_md_reference_type type;
             t_md_signal *internal_signal, *signal_2;
             t_md_state *state;

             internal_signal = signal->data.output.combinatorial_ref;
             if (signal->data.output.net_ref)
                 internal_signal = signal->data.output.net_ref;
             for (i=0; i<internal_signal->instance_iter->number_children; i++)
             {
                 reference_set_iterate_start( &internal_signal->instance_iter->children[i]->base_dependencies, &iter );
                 while ((reference = reference_set_iterate(&iter))!=NULL)
                 {
                     if (reference->type==md_reference_type_instance)
                     {
                         type = reference->data.instance->reference.type;
                         signal_2 = reference->data.instance->reference.data.signal; // Only valid if type is signal
                         state = reference->data.instance->reference.data.state; // Only valid if type is state
                         if ( (type==md_reference_type_signal) &&
                              (signal_2->type==md_signal_type_input) )
                         {
                             signal_2->data.input.used_combinatorially = 1;
                             signal->data.output.derived_combinatorially = 1;
                         }
                         else if (type==md_reference_type_state)
                         {
                             reference_add( &signal->data.output.clocks_derived_from, state->clock_ref, state->edge );
                         }
                     }
                     else if (reference->type==md_reference_type_clock_edge) 
                     {
                         reference_add( &signal->data.output.clocks_derived_from, reference->data.signal, reference->edge );
                     }
                 }
             }
         }
         else if (signal->data.output.register_ref)
         {
             reference_add( &signal->data.output.clocks_derived_from, signal->data.output.register_ref->clock_ref, signal->data.output.register_ref->edge );
         }
         else
         {
             fprintf(stderr,"c_model_descriptor::module_analyze:should we be here?****************************************************************************\n");
         }
     }

     /*b Now determine if the module has a combinatorial component
      */
     module->combinatorial_component = 0;
     for (signal=module->inputs; signal; signal=signal->next_in_list)
     {
         if (signal->data.input.used_combinatorially)
         {
             module->combinatorial_component = 1;
         }
     }

     /*b Now return okay
      */
     module->analyzed = 1;
     return 1;
}

/*f c_model_descriptor::module_display_references_instances
  Should work for prototypes and module definitions
 */
void c_model_descriptor::module_display_references_instances( t_md_module *module, t_md_output_fn output_fn, void *output_handle, t_md_type_instance_iter *instance_iter, int what )
{
     int i;
     t_md_reference_iter iter;
     t_md_reference *reference;

     if (!instance_iter)
          return;

     for (i=0; i<instance_iter->number_children; i++)
     {
          output_fn( output_handle, 2, "Base instance %s (%s):\n", instance_iter->children[i]->name, instance_iter->children[i]->output_name );
          if (what&1)
          {
               output_fn( output_handle, 3, "Immediate dependencies of %s (%s):\n", instance_iter->children[i]->name, instance_iter->children[i]->output_name );
               reference_set_iterate_start( &instance_iter->children[i]->dependencies, &iter );
               while ((reference = reference_set_iterate(&iter))!=NULL)
               {
                    output_fn( output_handle, 4, "'%s'\n", reference_text( reference ) );
               }
          }
          if (what&2)
          {
               output_fn( output_handle, 3, "Base dependencies (registers/inputs) of %s (%s):\n", instance_iter->children[i]->name, instance_iter->children[i]->output_name );
               reference_set_iterate_start( &instance_iter->children[i]->base_dependencies, &iter );
               while ((reference = reference_set_iterate(&iter))!=NULL)
               {
                    output_fn( output_handle, 4, "'%s'\n", reference_text( reference ) );
               }
          }
          if (what&4)
          {
               output_fn( output_handle, 3, "Dependents of %s (%s):\n", instance_iter->children[i]->name, instance_iter->children[i]->output_name );
               reference_set_iterate_start( &instance_iter->children[i]->dependents, &iter );
               while ((reference = reference_set_iterate(&iter))!=NULL)
               {
                    output_fn( output_handle, 4, "'%s'\n", reference_text( reference ) );
               }
          }
     }
     output_fn( output_handle, 0, "\n" );
}

/*f c_model_descriptor::module_display_references
  Should work for prototypes and module definitions
 */
int c_model_descriptor::module_display_references( t_md_module *module, t_md_output_fn output_fn, void *output_handle  )
{
     t_md_code_block *code_block;
     t_md_state *state;
     t_md_signal *signal;
     t_md_reference_iter iter;
     t_md_reference *reference;
     t_md_module_instance *module_instance;

     /*b Display title
      */
     output_fn( output_handle, 0, "****************************************************************************\n" );
     output_fn( output_handle, 0, "Module display references: module '%s'\n", module->name );
     output_fn( output_handle, 0, "****************************************************************************\n" );

     /*b Display dependencies of all the code blocks
      */
     for (code_block=module->code_blocks; code_block; code_block=code_block->next_in_list)
     {
          output_fn( output_handle, 0, "Code block '%s'\n", code_block->name );
          output_fn( output_handle, 1, "Dependencies:\n", code_block->name );
          reference_set_iterate_start( &code_block->dependencies, &iter );
          while ((reference = reference_set_iterate(&iter))!=NULL)
          {
               output_fn( output_handle, 2, "'%s'\n", reference_text( reference ) );
          }
          output_fn( output_handle, 1, "Effects:\n", code_block->name );
          reference_set_iterate_start( &code_block->effects, &iter );
          while ((reference = reference_set_iterate(&iter))!=NULL)
          {
               output_fn( output_handle, 2, "'%s'\n", reference_text( reference ) );
          }
          output_fn( output_handle, 0, "\n" );
     }

     /*b Display dependencies of all the modules
      */
     for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
     {
          output_fn( output_handle, 0, "Module instance '%s'\n", module_instance->name );
          output_fn( output_handle, 1, "Dependencies:\n", module_instance->name );
          reference_set_iterate_start( &module_instance->dependencies, &iter );
          while ((reference = reference_set_iterate(&iter))!=NULL)
          {
               output_fn( output_handle, 2, "'%s'\n", reference_text( reference ) );
          }
          output_fn( output_handle, 1, "Combinatorial dependencies:\n", module_instance->name );
          reference_set_iterate_start( &module_instance->combinatorial_dependencies, &iter );
          while ((reference = reference_set_iterate(&iter))!=NULL)
          {
               output_fn( output_handle, 2, "'%s'\n", reference_text( reference ) );
          }
          output_fn( output_handle, 1, "Effects:\n", module_instance->name );
          reference_set_iterate_start( &module_instance->effects, &iter );
          while ((reference = reference_set_iterate(&iter))!=NULL)
          {
               output_fn( output_handle, 2, "'%s'\n", reference_text( reference ) );
          }
          output_fn( output_handle, 0, "\n" );
     }

     /*b Display dependencies and dependents of all the clocks, inputs, outputs, combinatorials, nets, registers
      */
     for (signal=module->clocks; signal; signal=signal->next_in_list)
     {
          output_fn( output_handle, 0, "Clock '%s'\n", signal->name );
          if (signal->data.clock.clock_ref)
          {
              output_fn( output_handle, 1, "Parent clock '%s' gated by '%s'\n", signal->data.clock.clock_ref->name, signal->data.clock.gate_state->name );
          }
          if (signal->documentation)
          {
              output_fn( output_handle, 1, "Documentation '%s'\n", signal->documentation );
          }
          if ( (signal->data.clock.dependents[md_edge_pos]) || (signal->data.clock.dependents[md_edge_neg]) )
          {
               if (signal->data.clock.dependents[md_edge_pos])
               {
                    output_fn( output_handle, 3, "Dependents of posedge:\n" );
                    reference_set_iterate_start( &signal->data.clock.dependents[md_edge_pos], &iter );
                    while ((reference = reference_set_iterate(&iter))!=NULL)
                    {
                         output_fn( output_handle, 4, "'%s'\n", reference_text( reference ) );
                    }
               }
               if (signal->data.clock.dependents[md_edge_neg])
               {
                    output_fn( output_handle, 3, "Dependents of negedge:\n" );
                    reference_set_iterate_start( &signal->data.clock.dependents[md_edge_neg], &iter );
                    while ((reference = reference_set_iterate(&iter))!=NULL)
                    {
                         output_fn( output_handle, 4, "'%s'\n", reference_text( reference ) );
                    }
               }
          }
          output_fn( output_handle, 0, "\n" );
     }
     for (signal=module->inputs; signal; signal=signal->next_in_list)
     {
          output_fn( output_handle, 0, "Input '%s' (", signal->name );
          if (signal->data.input.used_combinatorially )
          {
              output_fn( output_handle, 0, "used combinatorially" );
          }
          else
          {
              output_fn( output_handle, 0, "not used combinatorially" );
          }
          output_fn( output_handle, 0, ")\n" );
          if (signal->documentation)
          {
              output_fn( output_handle, 1, "Documentation '%s'\n", signal->documentation );
          }
          module_display_references_instances( module, output_fn, output_handle, signal->instance_iter, 4 ); // dependents
     }
     for (signal=module->outputs; signal; signal=signal->next_in_list)
     {
          output_fn( output_handle, 0, "Output '%s' (", signal->name );
          if (signal->data.output.derived_combinatorially )
          {
              output_fn( output_handle, 0, "derived combinatorially " );
          }
          if (signal->data.output.clocks_derived_from)
          {
              reference_set_iterate_start( &signal->data.output.clocks_derived_from, &iter );
              while ((reference = reference_set_iterate(&iter))!=NULL)
              {
                  output_fn( output_handle, 0, "%s %s ", reference->edge?"negedge":"posedge", reference->data.signal->name );
              }
          }
          output_fn( output_handle, 0, ")\n" );
          if (signal->documentation)
          {
              output_fn( output_handle, 1, "Documentation '%s'\n", signal->documentation );
          }
          module_display_references_instances( module, output_fn, output_handle, signal->instance_iter, 3 );
     }
     for (signal=module->combinatorials; signal; signal=signal->next_in_list)
     {
          output_fn( output_handle, 0, "Combinatorial '%s'\n", signal->name );
          if (signal->documentation)
          {
              output_fn( output_handle, 1, "Documentation '%s'\n", signal->documentation );
          }
          module_display_references_instances( module, output_fn, output_handle, signal->instance_iter, 7 );
     }
     for (signal=module->nets; signal; signal=signal->next_in_list)
     {
          output_fn( output_handle, 0, "Net '%s'\n", signal->name );
          if (signal->documentation)
          {
              output_fn( output_handle, 1, "Documentation '%s'\n", signal->documentation );
          }
          module_display_references_instances( module, output_fn, output_handle, signal->instance_iter, 7 );
     }
     for (state=module->registers; state; state=state->next_in_list)
     {
          output_fn( output_handle, 0, "State '%s'\n", state->name );
          if (state->documentation)
          {
              output_fn( output_handle, 1, "Documentation '%s'\n", state->documentation );
          }
          module_display_references_instances( module, output_fn, output_handle, state->instance_iter, 7 ); // direct and base
     }

     /*b Now return okay
      */
     return 1;
}

/*f c_model_descriptor::module_display_instances
  Should work for prototypes and module definitions; prototypes should indicate they are
 */
int c_model_descriptor::module_display_instances( t_md_module *module, t_md_output_fn output_fn, void *output_handle  )
{
     t_md_type_instance *instance;
     int i;

     /*b Display title
      */
     if (module->external)
     {
         output_fn( output_handle, 0, "****************************************************************************\n" );
         output_fn( output_handle, 0, "Module prototype '%s' ports' instances\n", module->name );
         output_fn( output_handle, 0, "****************************************************************************\n" );
     }
     else
     {
         output_fn( output_handle, 0, "****************************************************************************\n" );
         output_fn( output_handle, 0, "Module display instances: module '%s'\n", module->name );
         output_fn( output_handle, 0, "****************************************************************************\n" );
     }

     for (instance=module->instances; instance; instance=instance->next_in_module)
     {
          output_fn( output_handle, 0, "Instance '%s' ('%s') at %p (to be freed %d)\n", instance->name, instance->output_name, instance, instance->name_to_be_freed );
          switch (instance->type)
          {
          case md_type_instance_type_bit_vector:
               output_fn( output_handle, 1, "Bit vector length %d reset expression %p\n", instance->type_def.data.width, instance->data[0].reset_value );
               break;
          case md_type_instance_type_array_of_bit_vectors:
               output_fn( output_handle, 1, "Array size %d of bit vectors of length %d\n", instance->size, instance->type_def.data.width );
               break;
          case md_type_instance_type_structure:
               output_fn( output_handle, 1, "Structure of %d elements\n", instance->size );
               for (i=0; i<instance->size; i++)
               {
                    output_fn( output_handle, 2, "%d: %s: %p\n", i, instance->data[i].element.name_ref, instance->data[i].element.instance );
               }
               break;
          default:
               output_fn( output_handle, 1, "Unprintable (as yet) type instance\n" );
               break;
          }
     }
     return 1;
}

/*f c_model_descriptor::modules_free
 */
void c_model_descriptor::modules_free( void )
{
     t_md_module *next_module;

     while (module_list)
     {
          next_module = module_list->next_in_list;
          if ((module_list->name) && (module_list->name_to_be_freed))
               free(module_list->name);
          signals_free( module_list->clocks );
          signals_free( module_list->inputs );
          signals_free( module_list->outputs );
          signals_free( module_list->combinatorials );
          states_free( module_list->registers );
          expressions_free( module_list->expressions );
          code_blocks_free( module_list->code_blocks );
          messages_free( module_list->messages );
          labelled_expressions_free( module_list->labelled_expressions );
          statements_free( module_list->statements );
          lvars_free( module_list );
          port_lvars_free( module_list );
          free(module_list);
          module_list = next_module;
     }
}

/*a Reference set methods
 */
/*f c_model_descriptor::reference_set_free
  Free the set
 */
void c_model_descriptor::reference_set_free( t_md_reference_set **set_ptr )
{
    while (*set_ptr)
    {
        t_md_reference_set *set;
        set = *set_ptr;
        *set_ptr = set->next_set;
        free(set);
    }
}

/*f c_model_descriptor::reference_set_add
  Here we add a new set at an extra level of indirection.
  If we fail to add one, just leave the pointer *set_ptr NULL (it should be NULL on entry)
 */
void c_model_descriptor::reference_set_add( t_md_reference_set **set_ptr )
{
     *set_ptr = (t_md_reference_set *)malloc(sizeof(t_md_reference_set) + sizeof(t_md_reference)*(MD_REFERENCE_ENTRIES_PER_CHUNK-1) );
     if (!(*set_ptr))
          return;
     (*set_ptr)->next_set = NULL;
     (*set_ptr)->max_entries = MD_REFERENCE_ENTRIES_PER_CHUNK;
     (*set_ptr)->number_entries = 0;
}

/*f c_model_descriptor::reference_set_find_member
 */
int c_model_descriptor::reference_set_find_member( t_md_reference_set *set, void *data, int edge )
{
     int i;

     if (!set)
          return 0;

     for (i=0; i<set->number_entries; i++)
     {
         if ((set->members[i].data.data==data) && (set->members[i].edge==edge) )
         {
             return 1;
         }
     }
     return reference_set_find_member( set->next_set, data, edge );
}

/*f c_model_descriptor::reference_internal_add
 */
int c_model_descriptor::reference_internal_add( t_md_reference_set **set_ptr, t_md_reference_type type, void *data, int edge )
{
     if (reference_set_find_member( *set_ptr, data, edge ))
          return 1;

     while ((*set_ptr) && ((*set_ptr)->number_entries == (*set_ptr)->max_entries))
     {
          set_ptr = &( (*set_ptr)->next_set );
     }
     if (!(*set_ptr))
          reference_set_add( set_ptr );
     if (!(*set_ptr))
          return 0;

     (*set_ptr)->members[ (*set_ptr)->number_entries ].type = type;
     (*set_ptr)->members[ (*set_ptr)->number_entries ].data.data = data;
     (*set_ptr)->members[ (*set_ptr)->number_entries ].edge = edge;
     (*set_ptr)->number_entries++;
     return 1;
}

/*f c_model_descriptor::reference_add( set_ptr, instance )
 */
int c_model_descriptor::reference_add( t_md_reference_set **set_ptr, t_md_type_instance *instance )
{
     return reference_internal_add( set_ptr, md_reference_type_instance, (void *)instance, 0 );
}

/*f c_model_descriptor::reference_add( set_ptr, signal )
 */
int c_model_descriptor::reference_add( t_md_reference_set **set_ptr, t_md_signal *signal )
{
     return reference_internal_add( set_ptr, md_reference_type_signal, (void *)signal, 0 );
}

/*f c_model_descriptor::reference_add( set_ptr, state )
 */
int c_model_descriptor::reference_add( t_md_reference_set **set_ptr, t_md_state *state )
{
     return reference_internal_add( set_ptr, md_reference_type_state, (void *)state, 0 );
}

/*f c_model_descriptor::reference_add( set_ptr, clock, edge )
 */
int c_model_descriptor::reference_add( t_md_reference_set **set_ptr, t_md_signal *clock, int edge )
{
     return reference_internal_add( set_ptr, md_reference_type_clock_edge, (void *)clock, edge );
}

/*f c_model_descriptor::reference_add_data( data )
int c_model_descriptor::reference_add_data( t_md_reference_set **set_ptr, void *data )
{
    return reference_internal_add( set_ptr, md_reference_type_clock_edge, data, 0 );
}
 */

/*f c_model_descriptor::reference_union_sets
 */
int c_model_descriptor::reference_union_sets( t_md_reference_set *to_add, t_md_reference_set **set_ptr )
{
     int i;

     while (to_add)
     {
          for (i=0; i<to_add->number_entries; i++)
          {
               if (!reference_internal_add( set_ptr, to_add->members[i].type, to_add->members[i].data.data, to_add->members[i].edge ))
               {
                    return 0;
               }
          }
          to_add = to_add->next_set;
     }
     return 1;
}

/*f c_model_descriptor::reference_intersect_sets
  Go through each entry of each subset
  for all subsets 'set'
    for each element 'i' of 'set'
        if 'i' is not in 'to_intersect'
            remove 'i' by pulling the entries down and decreasing the number of elements
 */
int c_model_descriptor::reference_intersect_sets( t_md_reference_set *to_intersect, t_md_reference_set **set_ptr )
{
    t_md_reference_set *set;
    int i;
    for (set=*set_ptr; set; set=set->next_set)
    {
        for (i=0; i<set->number_entries; i++)
        {
            if (!reference_set_find_member( to_intersect, set->members[i].data.data, set->members[i].edge ))
            {
                int j;
                for (j=i+1; j<set->number_entries; j++)
                {
                    set->members[j] = set->members[j+1];
                }
                i--;
                set->number_entries--;
            }
        }
    }
    return 1;
}

/*f c_model_descriptor::reference_set_iterate_start
 */
void c_model_descriptor::reference_set_iterate_start( t_md_reference_set **set_ptr, t_md_reference_iter *iter )
{
     if (!set_ptr)
     {
          iter->set = NULL;
          return;
     }
     iter->set = *set_ptr;
     iter->entry = -1;
}

/*f c_model_descriptor::reference_set_iterate
 */
t_md_reference *c_model_descriptor::reference_set_iterate( t_md_reference_iter *iter )
{
     t_md_reference *result;

     if (!iter->set)
          return NULL;

     iter->entry++;
     if (iter->entry>=iter->set->number_entries)
     {
          iter->set = iter->set->next_set;
          iter->entry = 0;
     }

     if (!iter->set)
          return NULL;
     if (iter->entry>=iter->set->number_entries)
          return NULL;

     result = &iter->set->members[iter->entry];
     return result;
}

/*f c_model_descriptor::reference_text
 */
static char reference_text_buffer[256];
const char *c_model_descriptor::reference_text( t_md_reference *reference )
{
     if (!reference)
          return "<NULL POINTER REFERENCE>";
     switch (reference->type)
     {
     case md_reference_type_instance:
          return reference->data.instance->name;
     case md_reference_type_state:
          return reference->data.state->name;
     case md_reference_type_signal:
          return reference->data.signal->name;
     case md_reference_type_clock_edge:
         sprintf( reference_text_buffer, "%s/%d", reference->data.signal->name, reference->edge );
         return reference_text_buffer;
     default:
          break;
     }
     return "<UNKNOWN REFERENCE TYPE>";
}

/*f c_model_descriptor::reference_set_includes( set_ptr, clock signal, edge )
 */
int c_model_descriptor::reference_set_includes( t_md_reference_set **set_ptr, t_md_signal *signal, int edge )
{
     t_md_reference_iter iter;
     t_md_reference *reference;

     reference_set_iterate_start( set_ptr, &iter );
     while ((reference = reference_set_iterate(&iter))!=NULL)
     {
          switch (reference->type)
          {
          case md_reference_type_state:
               if ( (reference->data.state->clock_ref == signal) &&
                    (reference->data.state->edge == edge) )
               {
                    return 1;
               }
               break;
          case md_reference_type_instance:
               if ( (reference->data.instance->reference.type == md_reference_type_state) &&
                    (reference->data.instance->reference.data.state->clock_ref == signal) &&
                    (reference->data.instance->reference.data.state->edge == edge) )
               {
                    return 1;
               }
               break;
          case md_reference_type_clock_edge:
               if ( (reference->data.signal == signal) &&
                    (reference->edge == edge) )
               {
                    return 1;
               }
               break;
          default:
               fprintf(stderr,"Unknown reference type %d\n", reference->type);
               break;
          }
     }
     return 0;
}

/*f c_model_descriptor::reference_set_includes( set_ptr, clock signal, edge, reset, reset_level )
 */
int c_model_descriptor::reference_set_includes( t_md_reference_set **set_ptr, t_md_signal *signal, int edge, t_md_signal *reset, int reset_level )
{
     t_md_reference_iter iter;
     t_md_reference *reference;

     reference_set_iterate_start( set_ptr, &iter );
     while ((reference = reference_set_iterate(&iter))!=NULL)
     {
          switch (reference->type)
          {
          case md_reference_type_state:
               if ( (reference->data.state->clock_ref == signal) &&
                    (reference->data.state->edge == edge) &&
                    (reference->data.state->reset_ref == reset) &&
                    (reference->data.state->reset_level == reset_level) )
               {
                    return 1;
               }
               break;
          case md_reference_type_instance:
               if ( (reference->data.instance->reference.type == md_reference_type_state) &&
                    (reference->data.instance->reference.data.state->clock_ref == signal) &&
                    (reference->data.instance->reference.data.state->edge == edge) &&
                    (reference->data.instance->reference.data.state->reset_ref == reset) &&
                    (reference->data.instance->reference.data.state->reset_level == reset_level) )
               {
                    return 1;
               }
               break;
          case md_reference_type_clock_edge:
               if ( (reference->data.signal == signal) &&
                    (reference->edge == edge) )
               {
                    return 1;
               }
               break;
          default:
               fprintf(stderr,"Unknown reference type %d\n", reference->type);
               break;
          }
     }
     return 0;
}

/*f c_model_descriptor::reference_set_includes( set_ptr, instance )
 */
int c_model_descriptor::reference_set_includes( t_md_reference_set **set_ptr, t_md_type_instance *instance )
{
     t_md_reference_iter iter;
     t_md_reference *reference;

     reference_set_iterate_start( set_ptr, &iter );
     while ((reference = reference_set_iterate(&iter))!=NULL)
     {
          switch (reference->type)
          {
          case md_reference_type_instance:
               if (reference->data.instance == instance)
               {
                    return 1;
               }
               break;
          default:
               break;
          }
     }
     return 0;
}

/*f c_model_descriptor::reference_set_includes( set_ptr, md_reference_type )
 */
int c_model_descriptor::reference_set_includes( t_md_reference_set **set_ptr, t_md_reference_type type )
{
     t_md_reference_iter iter;
     t_md_reference *reference;

     reference_set_iterate_start( set_ptr, &iter );
     while ((reference = reference_set_iterate(&iter))!=NULL)
     {
          if (reference->type==type)
          {
               return 1;
          }
          if ( (type!=md_reference_type_instance) &&
               (reference->type==md_reference_type_instance) &&
               (reference->data.instance->reference.type==type) )
          {
               return 1;
          }
     }
     return 0;
}

/*a Type definition methods
 */
/*t c_model_descriptor::type_definition_find
 */
t_md_type_definition_handle c_model_descriptor::type_definition_find( const char *name )
{
     t_md_type_definition *type_def;
     t_md_type_definition_handle result;

     for (type_def=types; type_def; type_def=type_def->next_in_list )
     {
          if ((type_def->name) && !strcmp(name, type_def->name))
          {
               result.type = md_type_definition_handle_type_structure;
               result.data.type_def = type_def;
               return result;
          }
     }
     result.type = md_type_definition_handle_type_none;
     return result;
}

/*t c_model_descriptor::type_definition_create
 */
t_md_type_definition *c_model_descriptor::type_definition_create( const char *name, int copy_name, t_md_type_definition_type type, int size, int number_elements )
{
     t_md_type_definition *type_def;

     type_def = (t_md_type_definition *)malloc( sizeof(t_md_type_definition)+(number_elements-1)*sizeof(t_md_type_definition_data) );
     if (!type_def)
          return type_def;

     type_def->next_in_list = types;
     types = type_def;
     type_def->size = size;
     type_def->type = type;
     if (copy_name)
     {
          type_def->name = sl_str_alloc_copy( name );
     }
     else
     {
          type_def->name = (char *)name; /* Safe, only because copy_name says so */
     }
     type_def->name_to_be_freed = copy_name;
     return type_def;
}

/*t c_model_descriptor::type_definition_bit_vector_create
 */
t_md_type_definition_handle c_model_descriptor::type_definition_bit_vector_create( int width )
{
     t_md_type_definition_handle result;

     result.type = md_type_definition_handle_type_bit_vector;
     result.data.width = width;
     return result;
}

/*t c_model_descriptor::type_definition_array_create
 */
t_md_type_definition_handle c_model_descriptor::type_definition_array_create( const char *name, int copy_name, int size, t_md_type_definition_handle type )
{
     t_md_type_definition_handle result;
     t_md_type_definition *type_def;

     result.type = md_type_definition_handle_type_none;

     if (type.type==md_type_definition_handle_type_bit_vector)
     {
          type_def = type_definition_create( name, copy_name, md_type_definition_type_array_of_bit_vectors, size, 1 );
          if (!type_def)
          {
               return result;
          }
     }
     else
     {
          type_def = type_definition_create( name, copy_name, md_type_definition_type_array_of_types, size, 1 );
          if (!type_def)
          {
               return result;
          }
     }
     type_def->data[0].type = type;

     result.type = md_type_definition_handle_type_structure;
     result.data.type_def = type_def;
     return result;
}

/*t c_model_descriptor::type_definition_structure_create
 */
t_md_type_definition_handle c_model_descriptor::type_definition_structure_create( const char *name, int copy_name, int number_elements )
{
     t_md_type_definition_handle result;
     t_md_type_definition *type_def;
     int i;

     result.type = md_type_definition_handle_type_none;

     type_def = type_definition_create( name, copy_name, md_type_definition_type_structure, number_elements, number_elements );

     if (!type_def)
     {
          return result;
     }

     for (i=0; i<number_elements; i++)
     {
          type_def->data[i].element.name = NULL;
          type_def->data[i].element.type.type = md_type_definition_handle_type_none;
     }

     result.type = md_type_definition_handle_type_structure;
     result.data.type_def = type_def;
     return result;
}

/*t c_model_descriptor::type_definition_structure_set_element
 */
int c_model_descriptor::type_definition_structure_set_element( t_md_type_definition_handle structure, const char *name, int copy_name, int element, t_md_type_definition_handle type )
{
     t_md_type_definition *type_def;

     if ( (structure.type != md_type_definition_handle_type_structure) ||
          (!structure.data.type_def) ||
          (structure.data.type_def->type!=md_type_definition_type_structure) )
     {
          return 0;
     }

     type_def = structure.data.type_def;
     if ((element<0) || (element>type_def->size))
     {
          return 0;
     }

     if (type_def->data[element].element.name)
     {
          if (type_def->data[element].element.name_to_be_freed)
          {
               free(type_def->data[element].element.name);
               type_def->data[element].element.name = NULL;
          }
     }
     type_def->data[element].element.name_to_be_freed = copy_name;
     if (copy_name)
     {
          type_def->data[element].element.name = sl_str_alloc_copy( name );
     }
     else
     {
          type_def->data[element].element.name = (char *)name; /* Safe, only because copy_name says so */
     }
     type_def->data[element].element.type = type;

     return 1;
}

/*f c_model_descriptor::type_definition_display
 */
void c_model_descriptor::type_definition_display( t_md_output_fn output_fn, void *output_handle  )
{
     t_md_type_definition *type_def;
     int i;
     const char *name;

     for (type_def=types; type_def; type_def=type_def->next_in_list )
     {
          output_fn( output_handle, 0, "Type definition %p\n", type_def );
          if (type_def->name)
          {
               output_fn( output_handle, 1, "Name %s\n", type_def->name);
          }
          switch (type_def->type)
          {
          case md_type_definition_type_array_of_bit_vectors:
               output_fn( output_handle, 1, "Array of size %d of bit vectors of length %d\n", type_def->size, type_def->data[0].type.data.width );
               break;
          case md_type_definition_type_array_of_types:
               output_fn( output_handle, 1, "Array of size %d of type %p\n", type_def->size, type_def->data[0].type.data.type_def );
               break;
          case md_type_definition_type_structure:
               for (i=0; i<type_def->size; i++)
               {
                    name = type_def->data[i].element.name;
                    if (!name)
                         name = "<undefined name>";
                    switch (type_def->data[i].element.type.type)
                    {
                    case md_type_definition_handle_type_bit_vector:
                         output_fn( output_handle, 2, "%d: %s: bit[%d]\n", i, name, type_def->data[i].element.type.data.width );
                         break;
                    case md_type_definition_handle_type_structure:
                         output_fn( output_handle, 2, "%d: %s: %p\n", i, name, type_def->data[i].element.type.data.type_def );
                         break;
                    case md_type_definition_handle_type_none:
                         output_fn( output_handle, 2, "%d: %s: No structure\n", i, name );
                         break;
                    }
               }
               break;
          }
     }
}

/*a Type instantiation methods
 */
/*f c_model_descriptor::type_instance_create
  Creates an instance of an array of bit vectors, a bit vector, or a structure. We clear the reset data here; the instance may be state, and we need to clear the reset chains.
 */
t_md_type_instance *c_model_descriptor::type_instance_create( t_md_module *module, t_md_reference *reference, const char *name, const char *sub_name, t_md_type_definition_handle type_def, t_md_type_instance_type type, int size )
{
     t_md_type_instance *instance;
     instance = (t_md_type_instance *)malloc(sizeof(t_md_type_instance)+sizeof(t_md_type_instance_data)*size);
     memset( (void *)instance, 0, sizeof(t_md_type_instance)+sizeof(t_md_type_instance_data)*size);
     if (!instance)
          return NULL;
     if (sub_name)
     {
          instance->name = (char *)malloc(strlen(name)+1+strlen(sub_name)+1);
          if (instance->name)
               sprintf( instance->name, "%s.%s", name, sub_name );
     }
     else
     {
          instance->name = sl_str_alloc_copy( name );
     }
     if (!instance->name)
     {
          free(instance);
          return NULL;
     }

     instance->next_in_module = module->instances;
     module->instances = instance;
     instance->name_to_be_freed = 1;
     instance->output_name = NULL;
     instance->type_def = type_def;
     instance->size = size;
     instance->type = type;

     instance->reference = *reference;
     instance->dependents = NULL;
     instance->dependencies = NULL;
     instance->base_dependencies = NULL;
     instance->code_block = NULL;
     instance->number_statements = 0;

     instance->derived_combinatorially = 0; // Only valid for instances of nets
     instance->driven_in_parts = 0;  // Only valid for instances of nets

     return instance;
}

/*f c_model_descriptor::type_instantiate
 */
t_md_type_instance *c_model_descriptor::type_instantiate( t_md_module *module, t_md_reference *reference, int array_size, t_md_type_definition_handle type, const char *name, const char *sub_name )
{
     t_md_type_definition *type_def;
     t_md_type_instance *instance;
     int i;

     instance = NULL;
     if (type.type == md_type_definition_handle_type_bit_vector)
     {
          if (array_size==0)
          {
               instance = type_instance_create( module, reference, name, sub_name, type, md_type_instance_type_bit_vector, 1 );
          }
          else if (type.data.width==1)
          {
               instance = type_instantiate( module, reference, 0, type_definition_bit_vector_create( array_size ) , name, sub_name );
          }
          else
          {
               instance = type_instance_create( module, reference, name, sub_name, type, md_type_instance_type_array_of_bit_vectors, array_size );
          }
     }
     else
     {
          type_def = type.data.type_def;
          switch (type_def->type)
          {
          case md_type_definition_type_array_of_bit_vectors:
              if ( type_def->data[0].type.data.width==1 ) // (array_size array of) array of 1-bit bit vectors is a (array of) bit vector
               {
                    instance = type_instantiate( module, reference, array_size, type_definition_bit_vector_create(type_def->size), name, sub_name );
               }
              else if (array_size==0) // array of bit vectors is, well, and array of bit vectors
               {
                    instance = type_instance_create( module, reference, name, sub_name, type_def->data[0].type, md_type_instance_type_array_of_bit_vectors, type_def->size );
               }
               else // Cannot instance array of array of bit vectors
               {
                   fprintf(stderr,"type_instantiate::Cannot instance an array of array of bit vectors\n");
                    return NULL;
               }
               break;
          case md_type_definition_type_array_of_types: // An array of types is instanced as an instance of the type with that types subelements arrayed with our size - i.e. the array size pushes to the bottom of the pile
               if (array_size==0)
               {
                    instance = type_instantiate( module, reference, type_def->size, type_def->data[0].type, name, sub_name );
               }
               else
               {
                    return NULL;
               }
               break;
          case md_type_definition_type_structure: // An array of structures is instanced as this structure with each element arrayed with the size - pushing the array size to the bottom of the pile
               instance = type_instance_create( module, reference, name, sub_name, type, md_type_instance_type_structure, type_def->size );

               if (!instance)
                    return NULL;
               for (i=0; i<type_def->size; i++)
               {
                    instance->data[i].element.name_ref = type_def->data[i].element.name;
                    instance->data[i].element.instance = type_instantiate( module, reference, array_size, type_def->data[i].element.type, instance->name, type_def->data[i].element.name );
                    if (!instance->data[i].element.instance)
                    {
                         return NULL;
                    }
               }
               break;
          }
     }
     return instance;
}

/*f c_model_descriptor::type_instance_free
 */
void c_model_descriptor::type_instance_free( t_md_type_instance *instance )
{
     if (!instance)
          return;

     if (instance->name_to_be_freed)
     {
          free(instance->name);
     }
}

/*f c_model_descriptor::type_instance_count_and_populate_children
 */
int c_model_descriptor::type_instance_count_and_populate_children( t_md_type_instance *instance, t_md_type_instance_iter *iter, int number_children )
{
     int i;
     switch (instance->type)
     {
     case md_type_instance_type_structure:
          for (i=0; i<instance->size; i++)
          {
               number_children = type_instance_count_and_populate_children( instance->data[i].element.instance, iter, number_children );
          }
          break;
     case md_type_instance_type_bit_vector:
     case md_type_instance_type_array_of_bit_vectors:
          if (iter)
               iter->children[number_children] = instance;
          number_children++;
          break;
     }
     return number_children;
}

/*f c_model_descriptor::type_instance_iterate_create
 */
t_md_type_instance_iter *c_model_descriptor::type_instance_iterate_create( t_md_type_instance *instance )
{
     t_md_type_instance_iter *iter;
     int number_children;

     /*b If no instance, we must have done it already :-)
      */
     if (!instance)
          return NULL;

     /*b Count the children first
      */
     number_children = type_instance_count_and_populate_children( instance, NULL, 0 );

     /*b Create iter with enough room for all the children
      */
     iter = (t_md_type_instance_iter *)malloc(sizeof(t_md_type_instance_iter)+sizeof(t_md_type_instance *)*number_children);
     if (!iter)
          return NULL;

     /*b Populate the iter
      */
     iter->number_children = number_children;

     type_instance_count_and_populate_children( instance, iter, 0 );

     /*b Ready!
      */
     return iter;
}

/*f c_model_descriptor::type_instance_iterate_free
 */
void c_model_descriptor::type_instance_iterate_free( t_md_type_instance_iter *iter )
{
     if (iter)
          free(iter);
}

/*a Lvar methods
 */
/*f c_model_descriptor::lvar_create( parent, type handle )
 */
t_md_lvar *c_model_descriptor::lvar_create( t_md_module *module, t_md_lvar *parent, t_md_type_definition_handle type )
{
     t_md_lvar *lvar;

     lvar = (t_md_lvar *)malloc(sizeof(t_md_lvar));
     if (!lvar)
          return NULL;
     if (module->lvars)
          module->lvars->prev_in_module = lvar;
     lvar->next_in_module = module->lvars;
     lvar->prev_in_module = NULL;
     module->lvars = lvar;

     lvar->parent_ref = parent;
     lvar->child_ref = NULL;
     lvar->type = type;

     if (parent)
     {
          if (parent->top_ref)
               lvar->top_ref = parent->top_ref;
          else
               lvar->top_ref = parent;
          parent->child_ref = lvar;
          lvar->instance = parent->instance;
          lvar->instance_type = parent->instance_type;
          lvar->instance_data = parent->instance_data;

          lvar->index = parent->index;
          lvar->subscript_start = parent->subscript_start;
          lvar->subscript_length = parent->subscript_length;
     }
     else
     {
          lvar->top_ref = NULL;

          lvar->instance = NULL;
          lvar->instance_data.state = NULL;

          lvar->index.type = md_lvar_data_type_none;
          lvar->subscript_start.type = md_lvar_data_type_none;
          lvar->subscript_length.type = md_lvar_data_type_none;
     }
     return lvar;
}

/*f c_model_descriptor::lvar_duplicate
 */
t_md_lvar *c_model_descriptor::lvar_duplicate( t_md_module *module, t_md_lvar *lvar )
{
    t_md_lvar *parent_lvar, *lvar_copy;

    if (!lvar)
    {
        fprintf(stderr,"c_model_descriptor::lvar_duplicate with lvar of NULL\n" );
    }
    parent_lvar = NULL;
    if (lvar->parent_ref)
    {
        parent_lvar = lvar_duplicate( module, lvar->parent_ref);
    }

    lvar_copy = lvar_create( module, parent_lvar, lvar->type );
    lvar_copy->instance = lvar->instance;
    lvar_copy->instance_type = lvar->instance_type;
    lvar_copy->instance_data = lvar->instance_data; // These are not ownership links

    lvar_copy->index = lvar->index;
    lvar_copy->subscript_start = lvar->subscript_start; // These are not ownership links - they may be expressions, but copying the pointers is probably okay
    lvar_copy->subscript_length = lvar->subscript_length;
    SL_DEBUG( sl_debug_level_info, "Copied %p to %p", lvar, lvar_copy );
    return lvar_copy;
}

/*f c_model_descriptor::lvar_reference( chain, name )
 */
t_md_lvar *c_model_descriptor::lvar_reference( t_md_module *module, t_md_lvar *chain, const char *name )
{
     t_md_lvar *lvar;
     t_md_state *state;
     t_md_signal *signal;
     int i;

     /*b If no chain, then this is a state, output, input or comb reference
       Looking in combinatorials first (before outputs) means that references to comb outputs will work
      */
     if (!chain)
     {
          signal = NULL;
          state = state_find( name, module->registers );
          if (!state)
          {
               signal = signal_find( name, module->inputs );
          }
          if (!signal)
          {
               signal = signal_find( name, module->combinatorials );
          }
          if (!signal)
          {
              signal = signal_find( name, module->nets );
          }
          if (!signal)
          {
               signal = signal_find( name, module->outputs );
          }
          if ( !signal && !state )
          {
               error->add_error( module, error_level_serious, error_number_be_unresolved_reference, error_id_be_c_model_descriptor_statement_create,
                                 error_arg_type_malloc_string, name,
                                 error_arg_type_none );
               return NULL;
          }
          if (state)
          {
              if (state->instance)
              {
                  lvar = lvar_create( module, NULL, state->instance->type_def );
                  if (lvar)
                  {
                      lvar->instance = state->instance;
                      lvar->instance_type = md_lvar_instance_type_state;
                      lvar->instance_data.state = state;
                  }
              }
              else
              {
                  error->add_error( module, error_level_serious, error_number_be_unresolved_reference, error_id_be_c_model_descriptor_statement_create,
                                    error_arg_type_malloc_string, name,
                                    error_arg_type_none );
                  
                  return NULL;
              }
          }
          else
          {
              if (signal->instance)
              {
                  lvar = lvar_create( module, NULL, signal->instance->type_def );
                  if (lvar)
                  {
                      lvar->instance = signal->instance;
                      lvar->instance_type = md_lvar_instance_type_signal;
                      lvar->instance_data.signal = signal;
                  }
              }
              else
              {
                  error->add_error( module, error_level_serious, error_number_be_unresolved_reference, error_id_be_c_model_descriptor_statement_create,
                                    error_arg_type_malloc_string, name,
                                    error_arg_type_none );
                  
                  return NULL;
              }
          }
     }
     /*b Else it is a subelement of a structure
      */
     else
     {
          if (!chain->instance)
          {
              SL_DEBUG( sl_debug_level_info, "No instance");
               return NULL;
          }
          if (chain->type.type != md_type_definition_handle_type_structure) // If an array of type of a structure, this is true
          {
              SL_DEBUG( sl_debug_level_info, "Type not a structure (a)");
               return NULL;
          }
          if ((1) &&(chain->type.data.type_def->type != md_type_definition_type_structure)) // If a structure this is true; if an array, this is not. Hm
          {
              SL_DEBUG( sl_debug_level_info, "Type not a structure (b)");
              return NULL;
          }
          for (i=0; i<chain->instance->size; i++)
          {
               if (!strcmp(chain->instance->data[i].element.name_ref, name ))
               {
                    break;
               }
          }
          if (i>=chain->instance->size)
          {
              fprintf(stderr, "Element %s not found in structure\n", name);
              return NULL;
          }
          lvar = lvar_create( module, chain, chain->type.data.type_def->data[i].element.type );
          if (!lvar)
          {
               return NULL;
          }
          lvar->instance = chain->instance->data[i].element.instance;
     }

     /*b Return result
      */
     return lvar;
}

/*f c_model_descriptor::lvar_index( lvar, lvar_data )
 */
t_md_lvar *c_model_descriptor::lvar_index( t_md_module *module, t_md_lvar *lvar, t_md_lvar_data *data )
{
     if (!lvar)
          return NULL;

     if (!lvar->instance)
     {
          fprintf(stderr,"c_model_descriptor:lvar_index:ERROR - No instance\n");
          return NULL;
     }
     if (lvar->index.type!=md_lvar_data_type_none)
     {
          fprintf(stderr,"c_model_descriptor:lvar_index:ERROR - Already indexed\n");
          return NULL;
     }
     if (lvar->instance->type == md_type_instance_type_bit_vector)
     {
         fprintf(stderr,"Unexpected indexing of a bit vector - this should be a bit select call\n");
         return NULL;
     }
     else if (lvar->instance->type == md_type_instance_type_array_of_bit_vectors)
     {
         lvar->type = lvar->instance->type_def;
     }
     else
     {
          if (lvar->type.type != md_type_definition_handle_type_structure)
          {
               fprintf(stderr,"c_model_descriptor:lvar_index:ERROR - Type %p not a structure (%d) - okay if this is an array of bit vectors?\n", lvar, lvar->type.type);
               return NULL;
          }
// Actually the lvar type for an array is always bogus. The instance does not record that it is an array; the arrayness is in essence pushed to the bottom of the instance.
// So the type of the indexed item is the type of the item itself
//          if (lvar->type.data.type_def->type != md_type_definition_type_array_of_types)
//          {
//               fprintf(stderr,"c_model_descriptor:lvar_index:ERROR - Type not an array of types - array of bit vectors will also do...\n");
//               return NULL;
//          }
//          lvar->type = lvar->type.data.type_def->data[0].type;
          lvar->type = lvar->type;
     }
     lvar->index = *data;

     /*b Return result
      */
     return lvar;
}

/*f c_model_descriptor::lvar_index( lvar, int )
 */
t_md_lvar *c_model_descriptor::lvar_index( t_md_module *module, t_md_lvar *lvar, int index )
{
    t_md_lvar_data data;
    data.type = md_lvar_data_type_integer;
    data.data.integer = (t_sl_uint64) index;
    return lvar_index( module, lvar, &data );
}

/*f c_model_descriptor::lvar_index( lvar, expression )
 */
t_md_lvar *c_model_descriptor::lvar_index( t_md_module *module, t_md_lvar *lvar, t_md_expression *index )
{
    t_md_lvar_data data;
    data.type = md_lvar_data_type_expression;
    data.data.expression = index;
    return lvar_index( module, lvar, &data );
}

/*f c_model_descriptor::lvar_bit_select( lvar, lvar_data )
 */
t_md_lvar *c_model_descriptor::lvar_bit_select( t_md_module *module, t_md_lvar *lvar, t_md_lvar_data *data )
{
    if (!lvar)
        return NULL;

    if (!lvar->instance)
    {
        fprintf(stderr,"c_model_descriptor:lvar_bit_select:ERROR - No instance\n");
        return NULL;
    }
    if (lvar->subscript_start.type!=md_lvar_data_type_none)
    {
        fprintf(stderr,"c_model_descriptor:lvar_bit_select:ERROR - Already bit selected\n");
        return NULL;
    }
    if (lvar->instance->type == md_type_instance_type_bit_vector)
    {
        lvar->type = type_definition_bit_vector_create( 1 );
    }
    else if ((lvar->instance->type == md_type_instance_type_array_of_bit_vectors) && (lvar->index.type!=md_lvar_data_type_none))
    {
        lvar->type = type_definition_bit_vector_create( 1 );
    }
    else
    {
        fprintf(stderr,"c_model_descriptor:lvar_bit_select:ERROR - Type not a bit vector...\n");
        return NULL;
    }
    lvar->subscript_start = *data;

    /*b Return result
     */
    return lvar;
}

/*f c_model_descriptor::lvar_bit_select( lvar, int )
 */
t_md_lvar *c_model_descriptor::lvar_bit_select( t_md_module *module, t_md_lvar *lvar, int bit_select )
{
    t_md_lvar_data data;
    data.type = md_lvar_data_type_integer;
    data.data.integer = (t_sl_uint64) bit_select;
    return lvar_bit_select( module, lvar, &data );
}

/*f c_model_descriptor::lvar_bit_select( lvar, expression )
 */
t_md_lvar *c_model_descriptor::lvar_bit_select( t_md_module *module, t_md_lvar *lvar, t_md_expression *bit_select )
{
    t_md_lvar_data data;
    data.type = md_lvar_data_type_expression;
    data.data.expression = bit_select;
    return lvar_bit_select( module, lvar, &data );
}

/*f c_model_descriptor::lvar_bit_range_select( lvar, expression, length )
 */
t_md_lvar *c_model_descriptor::lvar_bit_range_select( t_md_module *module, t_md_lvar *lvar, t_md_expression *expr, int length )
{
    if (!lvar)
        return NULL;

    if (!lvar->instance)
    {
        fprintf(stderr,"c_model_descriptor:lvar_bit_range_select:ERROR - No instance\n");
        return NULL;
    }
    if (lvar->subscript_start.type!=md_lvar_data_type_none)
    {
        fprintf(stderr,"c_model_descriptor:lvar_bit_range_select:ERROR - Already bit selected\n");
        return NULL;
    }
    if (lvar->instance->type == md_type_instance_type_bit_vector)
    {
        lvar->type = type_definition_bit_vector_create( length );
    }
    else if ((lvar->instance->type == md_type_instance_type_array_of_bit_vectors) && (lvar->index.type!=md_lvar_data_type_none))
    {
        lvar->type = type_definition_bit_vector_create( length );
    }
    else
    {
        fprintf(stderr,"c_model_descriptor:lvar_bit_range_select:ERROR - Type not a bit vector...\n");
        return NULL;
    }
    lvar->subscript_start.type = md_lvar_data_type_expression;
    lvar->subscript_start.data.expression = expr;
    lvar->subscript_length.type = md_lvar_data_type_integer;
    lvar->subscript_length.data.integer = (t_sl_uint64)length;
    SL_DEBUG(sl_debug_level_info, "Built lvar %p", lvar );

    /*b Return result
     */
    return lvar;
}

/*f c_model_descriptor::lvar_from_string( parent, string )
 */
t_md_lvar *c_model_descriptor::lvar_from_string( t_md_module *module, t_md_lvar *chain, const char *string )
{
     t_md_lvar *lvar;
     char *fragment;
     int i, more;

     if (string[0]==0)
          return chain;

     if (string[0]=='[')
     {
          if (sscanf( string+1, "%d", &i )!=1)
          {
               fprintf( stderr, "Bad integer in index %s\n", string );
               return NULL;
          }
          lvar = lvar_index( module, chain, i );
          if (!lvar)
               return NULL;
          for (i=1; (string[i]!=0) && (string[i]>='0') && (string[i]<='9'); i++);
          if (string[i]!=']')
          {
               fprintf( stderr, "Bad index %s\n", string );
               return NULL;
          }
          i++;
          if (string[i]=='.')
          {
               lvar = lvar_from_string( module, lvar, string+i+1 );
          }
          else if (string[i]=='[')
          {
               lvar = lvar_from_string( module, lvar, string+i );
          }
          return lvar;
     }
     fragment = sl_str_alloc_copy( string );
     for (i=0; ( (fragment[i]!=0) && (fragment[i]!='.') && (fragment[i]!='[') ); i++);
     more = fragment[i];
     fragment[i]=0;
     lvar = lvar_reference( module, chain, fragment );
     if (!lvar)
     {
          free(fragment);
          return NULL;
     }
     fragment[i] = more;
     if (fragment[i]=='.')
     {
          lvar = lvar_from_string( module, lvar, fragment+i+1 );
     }
     else if (fragment[i]=='[')
     {
          lvar = lvar_from_string( module, lvar, fragment+i );
     }
     free(fragment);
     return lvar;
}

/*f c_model_descriptor::lvar_free
 */
void c_model_descriptor::lvar_free( t_md_module *module, t_md_lvar *lvar )
{
     t_md_lvar *next, *last;

     /*b If no lvar, we're done
      */
     if (!lvar)
          return;

     /*b We need to clear the lvar chain, so let's go to the top
      */
     if (lvar->top_ref)
          lvar = lvar->top_ref;

     /*b Now free the whole chain
      */
     while (lvar)
     {
          /*b First unlink from the chain
           */
          next = lvar->next_in_module;
          last = lvar->prev_in_module;
          if (last)
               last->next_in_module = next;
          else
               module->lvars = next;
          if (next)
               next->prev_in_module = last;

          /*b Now find next one in the chain
           */
          next = lvar->child_ref;

          /*b Now free the lvar structure and associated data
           */
          free(lvar);

          /*b Next in chain
           */
          lvar = next;
     }

     /*b Done
      */
}

/*f c_model_descriptor::lvar_is_terminal
 */
int c_model_descriptor::lvar_is_terminal( t_md_lvar *lvar )
{
     if (!lvar)
          return 0;
     switch (lvar->instance->type)
     {
     case md_type_instance_type_bit_vector:
          return 1;
     case md_type_instance_type_array_of_bit_vectors:
          if (lvar->index.type==md_lvar_data_type_none)
               return 0;
          return 1;
     case md_type_instance_type_structure:
          return 0;
     }
     return 0;
}

/*f c_model_descriptor::lvar_width
 */
int c_model_descriptor::lvar_width( t_md_lvar *lvar )
{
     if (!lvar)
          return -1;
     //printf("lvar_width lvar %p length type %d start type %d index type %d\n", lvar, lvar->subscript_length.type, lvar->subscript_start.type, lvar->index.type );
     if (lvar->subscript_length.type!=md_lvar_data_type_none)
     {
         //printf("Lvar width of lvar %p is %lld\n",lvar,lvar->subscript_length.data.integer);
         return (int)(lvar->subscript_length.data.integer);
     }
     if (lvar->subscript_start.type!=md_lvar_data_type_none)
     {
         //printf("Lvar width of lvar %p is 1 as it has a subscript start, no length\n",lvar);
         return 1;
     }
     switch (lvar->instance->type)
     {
     case md_type_instance_type_bit_vector:
          return lvar->instance->type_def.data.width;
     case md_type_instance_type_array_of_bit_vectors:
          if (lvar->index.type==md_lvar_data_type_none)
               return -1;
          return lvar->instance->type_def.data.width;
     case md_type_instance_type_structure:
          return -1;
     }
     return -1;
}

/*f c_model_descriptor::push_possible_indices_to_subscripts
 */
void c_model_descriptor::push_possible_indices_to_subscripts( t_md_lvar *lvar )
{
    if (lvar->index.type==md_lvar_data_type_expression)
    {
        push_possible_indices_to_subscripts(lvar->index.data.expression);
    }
    if (lvar->subscript_start.type==md_lvar_data_type_expression)
    {
        push_possible_indices_to_subscripts(lvar->subscript_start.data.expression);
    }
    if ( (lvar->index.type!=md_lvar_data_type_none) &&
         (lvar->subscript_start.type==md_lvar_data_type_none) &&
         (lvar->instance) &&
         (lvar->instance->type==md_type_instance_type_bit_vector) )
    {
        lvar->subscript_start = lvar->index;
        lvar->index.type = md_lvar_data_type_none;
    }
}

/*f c_model_descriptor::lvars_free
 */
void c_model_descriptor::lvars_free( t_md_module *module )
{
     while (module->lvars)
     {
          lvar_free(module, module->lvars);
     }
}

/*a Port lvar methods
 */
/*f c_model_descriptor::port_lvar_create( module, parent )
  Create a port_lvar
 */
t_md_port_lvar *c_model_descriptor::port_lvar_create( t_md_module *module, t_md_port_lvar *parent )
{
     t_md_port_lvar *port_lvar;

     /*b Allocate and chain the port_lvar in the module
      */
     port_lvar = (t_md_port_lvar *)malloc(sizeof(t_md_port_lvar));
     if (!port_lvar)
          return NULL;
     if (module->port_lvars)
          module->port_lvars->prev_in_module = port_lvar;
     port_lvar->next_in_module = module->port_lvars;
     port_lvar->prev_in_module = NULL;
     module->port_lvars = port_lvar;

     /*b Chain the port_lvar with its parent
      */
     port_lvar->parent_ref = parent;
     port_lvar->child_ref = NULL;

     if (parent)
     {
          if (parent->top_ref)
               port_lvar->top_ref = parent->top_ref;
          else
               port_lvar->top_ref = parent;
          parent->child_ref = port_lvar;
     }
     else
     {
          port_lvar->top_ref = NULL;

     }

     port_lvar->name = NULL;
     port_lvar->name_to_be_freed = 0;

     return port_lvar;
}

/*f c_model_descriptor::port_lvar_duplicate
 */
t_md_port_lvar *c_model_descriptor::port_lvar_duplicate( t_md_module *module, t_md_port_lvar *port_lvar )
{
    t_md_port_lvar *parent_port_lvar, *port_lvar_copy;

    parent_port_lvar = NULL;
    if (port_lvar->parent_ref)
    {
        parent_port_lvar = port_lvar_duplicate( module, port_lvar->parent_ref );
    }

    port_lvar_copy = port_lvar_create( module, parent_port_lvar );
    port_lvar_copy->name = sl_str_alloc_copy( port_lvar->name );
    port_lvar_copy->name_to_be_freed = 1;

    return port_lvar_copy;
}

/*f c_model_descriptor::port_lvar_reference( chain, name )
 */
t_md_port_lvar *c_model_descriptor::port_lvar_reference( t_md_module *module, t_md_port_lvar *chain, const char *name )
{
     t_md_port_lvar *port_lvar;

     /*b Subelement of a structure
      */
     port_lvar = port_lvar_create( module, chain );
     if (!port_lvar)
     {
          return NULL;
     }
     port_lvar->name = sl_str_alloc_copy( name );
     port_lvar->name_to_be_freed = 1;

     /*b Return result
      */
     return port_lvar;
}

/*f c_model_descriptor::port_lvar_resolve( t_md_port_lvar *port_lvar, t_md_signal *signals );
 */
t_md_type_instance *c_model_descriptor::port_lvar_resolve( t_md_port_lvar *port_lvar, t_md_signal *signals )
{
    t_md_signal *signal;
    t_md_type_instance *instance;
    int i;

    if (port_lvar->top_ref) // Go to the top of the chain if not there already
        port_lvar=port_lvar->top_ref;

    signal = signal_find( port_lvar->name, signals );
    if (!signal)
    {
        fprintf(stderr, "Failed to resolve port_lvar %s\n", port_lvar->name );
        return NULL;
    }
    // signal->type should be md_signal_type_input or md_signal_type_output, md_signal_type_clock

    instance = signal->instance;
    port_lvar = port_lvar->child_ref;
    while (port_lvar)
    {
        if (!instance)
        {
            fprintf(stderr, "Port_lvar resolve: No instance\n");
            return NULL;
        }
        if (instance->type_def.type != md_type_definition_handle_type_structure)
        {
            fprintf(stderr, "port_lvar resolve: Type not a structure\n");
            return NULL;
        }
        for (i=0; i<instance->size; i++)
        {
            if (!strcmp(instance->data[i].element.name_ref, port_lvar->name ))
            {
                break;
            }
        }
        if (i>=instance->size)
        {
            fprintf(stderr, "port_lvar resolve:Element %s not found in structure\n", port_lvar->name);
            return NULL;
        }
        instance = instance->data[i].element.instance;
        port_lvar = port_lvar->child_ref;
    }
    return instance;
}

/*f c_model_descriptor::port_lvar_free
 */
void c_model_descriptor::port_lvar_free( t_md_module *module, t_md_port_lvar *port_lvar )
{
     t_md_port_lvar *next, *last;

     /*b If no port_lvar, we're done
      */
     if (!port_lvar)
          return;

     /*b We need to clear the port_lvar chain, so let's go to the top
      */
     if (port_lvar->top_ref)
          port_lvar = port_lvar->top_ref;

     /*b Now free the whole chain
      */
     while (port_lvar)
     {
          /*b First unlink from the chain
           */
          next = port_lvar->next_in_module;
          last = port_lvar->prev_in_module;
          if (last)
               last->next_in_module = next;
          else
               module->port_lvars = next;
          if (next)
               next->prev_in_module = last;

          /*b Now find next one in the chain
           */
          next = port_lvar->child_ref;

          /*b Now free the port_lvar structure and associated data
           */
          if ( (port_lvar->name_to_be_freed) && (port_lvar->name))
               free( port_lvar->name);
          free(port_lvar);

          /*b Next in chain
           */
          port_lvar = next;
     }

     /*b Done
      */
}

/*f c_model_descriptor::port_lvars_free
 */
void c_model_descriptor::port_lvars_free( t_md_module *module )
{
     while (module->port_lvars)
     {
          port_lvar_free(module, module->port_lvars);
     }
}

/*a Signal methods
 */
/*f c_model_descriptor::signal_internal_create
 */
t_md_signal *c_model_descriptor::signal_internal_create( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int array_size, t_md_type_definition_handle type, t_md_signal **list, t_md_signal_type signal_type, t_md_usage_type usage_type )
{
     t_md_signal *signal;
     t_md_client_reference client_ref;
     t_md_reference reference;

     WHERE_I_AM;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     signal = (t_md_signal *)malloc(sizeof(t_md_signal));
     if (!signal)
     {
          error->add_error( module, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_signal_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return NULL;
     }

     signal->next_in_list = *list;
     *list = signal;

     if (copy_name)
     {
          signal->name = sl_str_alloc_copy( name );
          signal->name_to_be_freed = 1;
     }
     else
     {
          signal->name = (char *)name; /* copy_name == 0 */
          signal->name_to_be_freed = 0;
     }

     signal->client_ref = client_ref;
     signal->documentation = NULL;

     signal->type = signal_type;
     signal->usage_type = usage_type;
     reference.type = md_reference_type_signal;
     reference.data.signal = signal;
     SL_DEBUG(sl_debug_level_info, "Instantiate signal %s in module %s",name, module->name );
     signal->instance = type_instantiate( module, &reference, array_size, type, name, NULL );
     signal->instance_iter = type_instance_iterate_create( signal->instance );
     return signal;
}

/*f c_model_descriptor::signal_find
 */
t_md_signal *c_model_descriptor::signal_find( const char *name, t_md_signal *list )
{
     t_md_signal *signal;

     for (signal=list; signal; signal=signal->next_in_list)
     {
          if (!strcmp(signal->name, name))
          {
               return signal;
          }
     }
     return NULL;
}

/*f c_model_descriptor::signal_add_clock (port or gated clock)
  Works for module definitions and prototypes
 */
int c_model_descriptor::signal_add_clock( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference )
{
     t_md_signal *signal;

     WHERE_I_AM;
     if ( signal_find( name, module->clocks) ||
          signal_find( name, module->inputs) ||
          signal_find( name, module->outputs) ||
          signal_find( name, module->combinatorials) ||
          signal_find( name, module->nets) ||
          state_find( name, module->registers) )
     {
         WHERE_I_AM;
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_signal_add_clock,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_internal_create( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type_definition_bit_vector_create( 1 ), &module->clocks, md_signal_type_input, md_usage_type_rtl );
     if (!signal)
     {
         WHERE_I_AM;
          return 0;
     }

     signal->data.clock.edges_used[0] = 0;
     signal->data.clock.edges_used[1] = 0;
     signal->data.clock.dependents[0] = NULL;
     signal->data.clock.dependents[1] = NULL;

     signal->data.clock.clock_ref = NULL;
     signal->data.clock.gate_signal = NULL;
     signal->data.clock.gate_state = NULL;
     signal->data.clock.gate_level = 0;

     return 1;
}

/*f c_model_descriptor::signal_gate_clock
  Works for module definitions only
 */
int c_model_descriptor::signal_gate_clock( t_md_module *module, const char *name, const char *clock_to_gate, const char *gate, int gate_level, void *client_base_handle, const void *client_item_handle, int client_item_reference )
{
    t_md_signal *clock_signal, *parent_clock_signal, *gate_signal;
    t_md_state *gate_state;

    WHERE_I_AM;
    clock_signal = signal_find( name, module->clocks );
    if (!clock_signal)
    {
        WHERE_I_AM;
        error->add_error( module, error_level_fatal, error_number_be_unresolved_reference, error_id_be_c_model_descriptor_signal_add_clock,
                          error_arg_type_malloc_string, name,
                          error_arg_type_none );
        return 0;
    }

    parent_clock_signal = signal_find( clock_to_gate, module->clocks );
    gate_state  = state_find( gate, module->registers);
    gate_signal = NULL;
    if (!gate_state)
    {
        gate_signal  = signal_find( gate, module->inputs );
        if (!gate_signal) gate_signal  = signal_find( gate, module->nets );
        if (!gate_signal) gate_signal  = signal_find( gate, module->combinatorials );
    }
    if (!gate_state && !gate_signal)
    {
        WHERE_I_AM;
        error->add_error( module, error_level_fatal, error_number_be_unresolved_reference, error_id_be_c_model_descriptor_signal_add_input,
                          error_arg_type_malloc_string, gate,
                          error_arg_type_none );
    }

    clock_signal->data.clock.clock_ref = parent_clock_signal;
    clock_signal->data.clock.gate_signal = gate_signal;
    clock_signal->data.clock.gate_state = gate_state;
    clock_signal->data.clock.gate_level = gate_level;

    return 1;
}

/*f c_model_descriptor::signal_add_input (bit vector width)
  Works for module definitions and prototypes
 */
int c_model_descriptor::signal_add_input( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width )
{
     t_md_signal *signal;

     WHERE_I_AM;
     if ( signal_find(name, module->clocks) ||
          signal_find(name, module->inputs) ||
          signal_find(name, module->outputs) ||
          signal_find( name, module->combinatorials) ||
          signal_find( name, module->nets) ||
          state_find( name, module->registers) )
     {
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_signal_add_input,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_internal_create( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type_definition_bit_vector_create( width ), &module->inputs, md_signal_type_input, md_usage_type_rtl );
     if (!signal)
     {
          return 0;
     }

     signal->data.input.levels_used_for_reset[0] = 0;
     signal->data.input.levels_used_for_reset[1] = 0;
     signal->data.input.used_combinatorially = 0;
     signal->data.input.clocks_used_on = NULL;
     return 1;
}

/*f c_model_descriptor::signal_add_input (type)
  Works for module definitions and prototypes
 */
int c_model_descriptor::signal_add_input( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_type_definition_handle type )
{
     t_md_signal *signal;

     WHERE_I_AM;
     if ( signal_find(name, module->clocks) ||
          signal_find(name, module->inputs) ||
          signal_find(name, module->outputs) ||
          signal_find( name, module->combinatorials) ||
          signal_find( name, module->nets) ||
          state_find( name, module->registers) )
     {
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_signal_add_input,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_internal_create( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type, &module->inputs, md_signal_type_input, md_usage_type_rtl );
     if (!signal)
     {
          return 0;
     }

     signal->data.input.levels_used_for_reset[0] = 0;
     signal->data.input.levels_used_for_reset[1] = 0;
     signal->data.input.used_combinatorially = 0;
     signal->data.input.clocks_used_on = NULL;

     return 1;
}

/*f c_model_descriptor::signal_add_output (bit vector width)
  Works for module definitions and prototypes
 */
int c_model_descriptor::signal_add_output( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width )
{
     t_md_signal *signal;

     WHERE_I_AM;
     if ( signal_find( name, module->clocks) ||
          signal_find( name, module->inputs) ||
          signal_find( name, module->outputs) ||
          signal_find( name, module->combinatorials) ||
          signal_find( name, module->nets) ||
          state_find( name, module->registers) )
     {
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_signal_add_output,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_internal_create( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type_definition_bit_vector_create( width ), &module->outputs, md_signal_type_combinatorial, md_usage_type_rtl );
     if (!signal)
     {
          return 0;
     }

     signal->data.output.register_ref = NULL;
     signal->data.output.combinatorial_ref = NULL;
     signal->data.output.net_ref = NULL;
     signal->data.output.derived_combinatorially = 0;
     signal->data.output.clocks_derived_from = NULL;
     return 1;
}

/*f c_model_descriptor::signal_add_output (type)
  Works for module definitions and prototypes
 */
int c_model_descriptor::signal_add_output( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_type_definition_handle type )
{
     t_md_signal *signal;

     WHERE_I_AM;
     if ( signal_find( name, module->clocks) ||
          signal_find( name, module->inputs) ||
          signal_find( name, module->outputs) ||
          signal_find( name, module->combinatorials) ||
          signal_find( name, module->nets) ||
          state_find( name, module->registers) )
     {
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_signal_add_output,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_internal_create( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type, &module->outputs, md_signal_type_combinatorial, md_usage_type_rtl );
     if (!signal)
     {
          return 0;
     }

     signal->data.output.register_ref = NULL;
     signal->data.output.combinatorial_ref = NULL;
     signal->data.output.net_ref = NULL;
     signal->data.output.derived_combinatorially = 0;
     signal->data.output.clocks_derived_from = NULL;
     return 1;
}

/*f c_model_descriptor::signal_add_combinatorial (bit vector width)
  Works only for module definitions
 */
int c_model_descriptor::signal_add_combinatorial( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width, t_md_usage_type usage_type )
{
     t_md_signal *signal;
     t_md_signal *output;

     WHERE_I_AM;
     if (module->external)
     {
          error->add_error( module, error_level_fatal, error_number_be_signal_in_prototype, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     if ( signal_find(name, module->clocks) ||
          signal_find(name, module->inputs) ||
          signal_find( name, module->combinatorials) ||
          signal_find( name, module->nets) ||
          state_find( name, module->registers) )
     {
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_internal_create( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type_definition_bit_vector_create( width ), &module->combinatorials, md_signal_type_combinatorial, usage_type );

     if (!signal)
     {
          return 0;
     }

     output = signal_find( name, module->outputs );
     if (output)
     {
          output->data.output.combinatorial_ref = signal;
          signal->data.combinatorial.output_ref = output;
     }
     else
     {
         signal->data.combinatorial.output_ref = NULL;
     }
     return 1;
}

/*f c_model_descriptor::signal_add_combinatorial (type)
  Works only for module definitions
 */
int c_model_descriptor::signal_add_combinatorial( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_type_definition_handle type, t_md_usage_type usage_type )
{
     t_md_signal *signal;
     t_md_signal *output;

     WHERE_I_AM;
     if (module->external)
     {
          error->add_error( module, error_level_fatal, error_number_be_signal_in_prototype, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     if ( signal_find(name, module->clocks) ||
          signal_find(name, module->inputs) ||
          signal_find( name, module->combinatorials) ||
          signal_find( name, module->nets) ||
          state_find( name, module->registers) )
     {
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_internal_create( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type, &module->combinatorials, md_signal_type_combinatorial, usage_type );

     if (!signal)
     {
          return 0;
     }

     output = signal_find( name, module->outputs );
     if (output)
     {
          output->data.output.combinatorial_ref = signal;
          signal->data.combinatorial.output_ref = output;
     }
     else
     {
         signal->data.combinatorial.output_ref = NULL;
     }
     return 1;
}

/*f c_model_descriptor::signal_add_net (bit vector width)
  Works only for module definitions
 */
int c_model_descriptor::signal_add_net( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width )
{
     t_md_signal *signal;
     t_md_signal *output;

     WHERE_I_AM;
     if (module->external)
     {
          error->add_error( module, error_level_fatal, error_number_be_signal_in_prototype, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     if ( signal_find(name, module->clocks) ||
          signal_find(name, module->inputs) ||
          signal_find( name, module->combinatorials) ||
          signal_find( name, module->nets) ||
          state_find( name, module->registers) )
     {
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_signal_add_net,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_internal_create( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type_definition_bit_vector_create( width ), &module->nets, md_signal_type_net, md_usage_type_rtl );

     if (!signal)
     {
          return 0;
     }

     output = signal_find( name, module->outputs );
     if (output)
     {
          output->data.output.net_ref = signal;
          signal->data.net.output_ref = output;
     }
     else
     {
         signal->data.net.output_ref = NULL;
     }
     return 1;
}

/*f c_model_descriptor::signal_add_net (type)
  Works only for module definitions
 */
int c_model_descriptor::signal_add_net( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_type_definition_handle type )
{
     t_md_signal *signal;
     t_md_signal *output;

     WHERE_I_AM;
     if (module->external)
     {
          error->add_error( module, error_level_fatal, error_number_be_signal_in_prototype, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     if ( signal_find(name, module->clocks) ||
          signal_find(name, module->inputs) ||
          signal_find( name, module->combinatorials) ||
          signal_find( name, module->nets) ||
          state_find( name, module->registers) )
     {
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_signal_add_net,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_internal_create( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type, &module->nets, md_signal_type_net, md_usage_type_rtl );

     if (!signal)
     {
          return 0;
     }

     output = signal_find( name, module->outputs );
     if (output)
     {
          output->data.output.net_ref = signal;
          signal->data.net.output_ref = output;
     }
     else
     {
         signal->data.net.output_ref = NULL;
     }
     return 1;
}

/*f c_model_descriptor::signal_document - document a signal if it exists; start at clocks, inputs, outputs, combs, then nets
 */
int c_model_descriptor::signal_document( t_md_module *module, const char *name, const char *documentation )
{
    t_md_signal *signal;

     WHERE_I_AM;
    signal = signal_find( name, module->clocks );
    if (!signal) signal = signal_find( name, module->inputs );
    if (!signal) signal = signal_find( name, module->outputs );
    if (signal)
        signal->documentation = sl_str_alloc_copy(documentation);
    signal = signal_find( name, module->combinatorials );
    if (!signal) signal = signal_find( name, module->nets );
    if (signal)
        signal->documentation = sl_str_alloc_copy(documentation);
    return 1;
}

/*f c_model_descriptor::signal_input_used_combinatorially
  Only for module prototypes
 */
int c_model_descriptor::signal_input_used_combinatorially( t_md_module *module, const char *name )
{
     t_md_signal *signal;

     WHERE_I_AM;
     if (!module->external)
     {
          error->add_error( module, error_level_fatal, error_number_be_signal_use_not_in_prototype, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_find(name, module->inputs);
     if (!signal)
     {
          error->add_error( module, error_level_fatal, error_number_be_unresolved_input_reference, error_id_be_c_model_descriptor_signal_add_net,
                            error_arg_type_malloc_string, name,
                            error_arg_type_malloc_string, module->name,
                            error_arg_type_none );
          return 0;
     }

     signal->data.input.used_combinatorially = 1;
     return 1;
}

/*f c_model_descriptor::signal_input_used_on_clock
  Only for module prototypes
 */
int c_model_descriptor::signal_input_used_on_clock( t_md_module *module, const char *name, const char *clock_name, int edge )
{
     t_md_signal *signal, *clock;

     WHERE_I_AM;
     if (!module->external)
     {
          error->add_error( module, error_level_fatal, error_number_be_signal_use_not_in_prototype, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_find(name, module->inputs);
     if (!signal)
     {
          error->add_error( module, error_level_fatal, error_number_be_unresolved_input_reference, error_id_be_c_model_descriptor_signal_add_net,
                            error_arg_type_malloc_string, name,
                            error_arg_type_malloc_string, module->name,
                            error_arg_type_none );
          return 0;
     }
     clock = signal_find(clock_name, module->clocks);
     if (!clock)
     {
          error->add_error( module, error_level_fatal, error_number_be_unresolved_clock_reference, error_id_be_c_model_descriptor_signal_add_net,
                            error_arg_type_malloc_string, clock_name,
                            error_arg_type_malloc_string, module->name,
                            error_arg_type_none );
          return 0;
     }

     reference_add( &signal->data.input.clocks_used_on, clock, edge );
     return 1;
}

/*f c_model_descriptor::signal_output_derived_combinatorially
  Only for module prototypes
 */
int c_model_descriptor::signal_output_derived_combinatorially( t_md_module *module, const char *name )
{
     t_md_signal *signal;

     WHERE_I_AM;
     if (!module->external)
     {
          error->add_error( module, error_level_fatal, error_number_be_signal_use_not_in_prototype, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_find(name, module->outputs);
     if (!signal)
     {
          error->add_error( module, error_level_fatal, error_number_be_unresolved_output_reference, error_id_be_c_model_descriptor_signal_add_net,
                            error_arg_type_malloc_string, name,
                            error_arg_type_malloc_string, module->name,
                            error_arg_type_none );
          return 0;
     }

     signal->data.output.derived_combinatorially = 1;
     return 1;
}

/*f c_model_descriptor::signal_output_generated_from_clock
  Only for module prototypes
 */
int c_model_descriptor::signal_output_generated_from_clock( t_md_module *module, const char *name, const char *clock_name, int edge )
{
     t_md_signal *signal, *clock;

     WHERE_I_AM;
     if (!module->external)
     {
          error->add_error( module, error_level_fatal, error_number_be_signal_use_not_in_prototype, error_id_be_c_model_descriptor_signal_add_combinatorial,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }

     signal = signal_find(name, module->outputs);
     if (!signal)
     {
          error->add_error( module, error_level_fatal, error_number_be_unresolved_output_reference, error_id_be_c_model_descriptor_signal_add_net,
                            error_arg_type_malloc_string, name,
                            error_arg_type_malloc_string, module->name,
                            error_arg_type_none );
          return 0;
     }
     clock = signal_find(clock_name, module->clocks);
     if (!clock)
     {
          error->add_error( module, error_level_fatal, error_number_be_unresolved_clock_reference, error_id_be_c_model_descriptor_signal_add_net,
                            error_arg_type_malloc_string, clock_name,
                            error_arg_type_malloc_string, module->name,
                            error_arg_type_none );
          return 0;
     }

     reference_add( &signal->data.output.clocks_derived_from, clock, edge );
     return 1;
}

/*f c_model_descriptor::signals_free
 */
void c_model_descriptor::signals_free( t_md_signal *list )
{
     t_md_signal *next_signal;

     while (list)
     {
          next_signal = list->next_in_list;
          if (list->name_to_be_freed)
               free(list->name);
          if (list->documentation)
              free(list->documentation);
          if (list->instance_iter)
               type_instance_iterate_free(list->instance_iter);
          free(list);
          list = next_signal;
     }
}

/*a State methods
 */
/*f c_model_descriptor::state_create
  Works only for module definitions
  The clock and reset must have been declared already - the clock on the 'clock' list, the input on the 'signals' list
 */
t_md_state *c_model_descriptor::state_create( t_md_module *module, const char *state_name, int copy_state_name, t_md_client_reference *client_ref, const char *clock_name, int edge, const char *reset_name, int reset_level, t_md_signal *clocks, t_md_signal *signals, t_md_state **list )
{
     t_md_state *state;
     t_md_signal *clock, *reset;

     WHERE_I_AM;
     if (module->external)
     {
         WHERE_I_AM;
          error->add_error( module, error_level_fatal, error_number_be_signal_in_prototype, error_id_be_c_model_descriptor_state_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return NULL;
     }

     if ( signal_find( state_name, module->clocks) ||
          signal_find( state_name, module->inputs) ||
          signal_find( state_name, module->combinatorials) ||
          signal_find( state_name, module->nets) ||
          state_find( state_name, module->registers) )
     {
         WHERE_I_AM;
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_state_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return NULL;
     }

     clock = signal_find( clock_name, clocks );
     reset = signal_find( reset_name, signals);
     if (!clock || !reset)
     {
         WHERE_I_AM;
          error->add_error( module, error_level_fatal, error_number_be_undeclared_clock_or_reset, error_id_be_c_model_descriptor_state_create,
                            error_arg_type_malloc_string, state_name,
                            error_arg_type_malloc_string, clock_name,
                            error_arg_type_malloc_string, reset_name,
                            error_arg_type_none );
          return NULL;
     }

     state = (t_md_state *)malloc(sizeof(t_md_state));
     if (!state)
     {
         WHERE_I_AM;
          error->add_error( module, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_state_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return NULL;
     }

     state->next_in_list = *list;
     *list = state;

     if (copy_state_name)
     {
          state->name = sl_str_alloc_copy( state_name );
          state->name_to_be_freed = 1;
     }
     else
     {
          state->name = (char *)state_name;
          state->name_to_be_freed = 0;
     }

     state->client_ref = *client_ref;

     state->instance = NULL;
     state->instance_iter = NULL;

     state->clock_ref = clock;
     state->edge = edge;
     state->reset_ref = reset;
     state->reset_level = reset_level;
     state->output_ref = NULL;
     state->documentation = NULL;

     reset->data.input.levels_used_for_reset[reset_level] = 1;

     return state;
}

/*f c_model_descriptor::state_find
 */
t_md_state *c_model_descriptor::state_find( const char *name, t_md_state *list )
{
     t_md_state *state;

     for (state=list; state; state=state->next_in_list)
     {
          if (!strcmp(state->name, name))
          {
               return state;
          }
     }
     return NULL;
}

/*f c_model_descriptor::state_internal_add
  Works only for module definitions
 */
t_md_state *c_model_descriptor::state_internal_add( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int array_size, t_md_type_definition_handle type, const char *clock, int edge, const char *reset, int level, t_md_usage_type usage_type )
{
     t_md_state *state;
     t_md_signal *output;
     t_md_client_reference client_ref;
     t_md_reference reference;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     state = state_create( module, name, copy_name, &client_ref, clock, edge, reset, level, module->clocks, module->inputs, &module->registers );
     if (!state)
     {
         fprintf(stderr, "c_model_descriptor:BUG: failed to state_create for state_internal_add\n" );;
         return NULL;
     }

     if ( (array_size>0) && (type.type!=md_type_definition_handle_type_bit_vector) )
     {
          fprintf(stderr, "c_model_descriptor:NYI: An array of non-bit-vectors needs a new type created and used %d\n", type.type);
          exit(1);
     }

     state->usage_type = usage_type;
     reference.type = md_reference_type_state;
     reference.data.state = state;

     state->instance = type_instantiate( module, &reference, array_size, type, name, NULL );
//fprintf(stderr,"state_internal_add array size %d name '%s' instance %p\n", array_size, name, state->instance ); fflush(stderr);
     if (!state->instance)
     {
          return NULL;
     }
     state->instance_iter = type_instance_iterate_create( state->instance );

     output = signal_find( name, module->outputs );
     if (output)
     {
          output->data.output.register_ref = state;
          state->output_ref = output;
     }
     return state;
}

/*f c_model_descriptor::state_add_reset_value - append a reset expression with possible subscripts to an instance reset value
 */
void c_model_descriptor::state_add_reset_value( t_md_type_instance_data *reset_value_id, int subscript_start, int subscript_end, t_md_expression * expression )
{
    t_md_type_instance_data *ptr;
    WHERE_I_AM;
    ptr = reset_value_id;
    if (ptr->reset_value.expression)
    {
        while (ptr->reset_value.next_in_list) ptr=ptr->reset_value.next_in_list;
        ptr->reset_value.next_in_list = (t_md_type_instance_data *)malloc(sizeof(t_md_type_instance_data));
        if (!ptr->reset_value.next_in_list)
            return;
        ptr = ptr->reset_value.next_in_list;
    }
    ptr->reset_value.subscript_start = subscript_start;
    ptr->reset_value.subscript_end = subscript_end;
    push_possible_indices_to_subscripts(expression);
    ptr->reset_value.expression = expression;
    ptr->reset_value.next_in_list = NULL;
}

/*f c_model_descriptor::state_add - type instance
  Works only for module definitions
 */
int c_model_descriptor::state_add( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int array_size, t_md_type_definition_handle type, t_md_usage_type usage_type, const char *clock, int edge, const char *reset, int level )
{
    WHERE_I_AM;
    return state_internal_add( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, array_size, type, clock, edge, reset, level, usage_type)!=NULL;
}

/*f c_model_descriptor::state_add - simple bit vector
  Works only for module definitions
 */
int c_model_descriptor::state_add( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, int width, t_md_usage_type usage_type, const char *clock, int edge, const char *reset, int level, t_md_signal_value *reset_value )
{
     t_md_state *state;

     WHERE_I_AM;
     state = state_internal_add( module, name, copy_name, client_base_handle, client_item_handle, client_item_reference, 0, type_definition_bit_vector_create( width ), clock, edge, reset, level, usage_type);

     if (!state)
     {
          return 0;
     }

     state_add_reset_value( &state->instance->data[0], -1, -1, expression( module, width, reset_value ) );
     return 1;
}

/*f c_model_descriptor::state_document - document a state if it exists
 */
int c_model_descriptor::state_document( t_md_module *module, const char *name, const char *documentation )
{
    t_md_state *state;
    WHERE_I_AM;
    state = state_find( name, module->registers );
    if (state)
        state->documentation = sl_str_alloc_copy(documentation);
    return 1;
}

/*f c_model_descriptor::state_reset - set reset value of a state from a string and value
 */
int c_model_descriptor::state_reset( t_md_module *module, const char *name, t_md_signal_value *reset_value )
{
     t_md_lvar *lvar;
     t_md_expression *expr;

     lvar = lvar_from_string( module, NULL, name );
     expr = expression( module, reset_value->width, reset_value );
     return state_reset( module, lvar, expr );
}

/*f c_model_descriptor::state_reset( module, lvar, expression ) - set reset value of a state from a string and value
 */
int c_model_descriptor::state_reset( t_md_module *module, t_md_lvar *lvar, t_md_expression *expression )
{
    if (!lvar || !expression)
    {
        return 0;
    }

    if (lvar->instance_type!=md_lvar_instance_type_state)
    {
        error->add_error( module, error_level_fatal, error_number_sl_message, error_id_be_c_model_descriptor_state_create,
                          error_arg_type_malloc_string,   "Lvar for reset is not a state",
                          error_arg_type_malloc_filename, lvar->instance->name,
                          error_arg_type_line_number,     lvar->instance_type,
                          error_arg_type_none );
          return 0;
     }
     switch (lvar->instance->type)
     {
     case md_type_instance_type_bit_vector:
          if (lvar->index.type!=md_lvar_data_type_none)
          {
              if (lvar->index.type!=md_lvar_data_type_integer)
              {
                  fprintf( stderr, "c_model_descriptor::state_reset:Unexpected index in a bit vector for state reset\n");
                  lvar_free( module, lvar );
                  return 0;
              }
              state_add_reset_value( &lvar->instance->data[0], lvar->index.data.integer, -1, expression );
          }
          else
          {
              state_add_reset_value( &lvar->instance->data[0], -1, -1, expression );
          }
          break;
     case md_type_instance_type_array_of_bit_vectors:
          if (lvar->index.type!=md_lvar_data_type_integer)
          {
               fprintf( stderr, "c_model_descriptor::state_reset:Unexpected index in a bit vector array for state reset\n");
               lvar_free( module, lvar );
               return 0;
          }
          //if ( (lvar->index.data.integer<0) || (lvar->index.data.integer>=lvar->instance->size) )// <0 Not needed now, as integers are unsigned
          if ( lvar->index.data.integer>=(t_sl_uint64)lvar->instance->size )
          {
               fprintf( stderr, "c_model_descriptor::state_reset:Index in a bit vector array for state reset out of range (%lld)\n", lvar->index.data.integer);
               lvar_free( module, lvar );
               return 0;
          }
          state_add_reset_value( &lvar->instance->data[lvar->index.data.integer], -1, -1, expression );
          break;
     case md_type_instance_type_structure:
          fprintf( stderr, "c_model_descriptor::state_reset:Attempt to reset a non-endpoint of a structure\n");
          lvar_free( module, lvar );
          return 0;
     }

     lvar_free( module, lvar );
     return 1;
}

/*f c_model_descriptor::states_free
 */
void c_model_descriptor::states_free( t_md_state *list )
{
     t_md_state *next_state;

     while (list)
     {
          next_state = list->next_in_list;
          if (list->name_to_be_freed)
               free(list->name);
          if (list->documentation)
              free(list->documentation);
          if (list->instance_iter)
               type_instance_iterate_free( list->instance_iter );
          free(list);
          list = next_state;
     }
}

/*a Expression methods
 */
/*f c_model_descriptor::expression_create
 */
t_md_expression *c_model_descriptor::expression_create( t_md_module *module, t_md_expr_type type )
{
     t_md_expression *expr;

     expr = (t_md_expression *)malloc(sizeof(t_md_expression));
     if (!expr)
     {
          error->add_error( module, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_expression_create,
                            error_arg_type_malloc_string, module->name,
                            error_arg_type_none );
          return NULL;
     }

     expr->next_in_list = module->expressions;
     module->expressions = expr;
     expr->type = type;
     expr->width = 0;
     expr->next_in_chain = NULL;
     return expr;
}

/*f c_model_descriptor::expression_chain
 */
t_md_expression *c_model_descriptor::expression_chain( t_md_expression *chain, t_md_expression *new_element )
{
    t_md_expression **expr_ptr;
    if (!chain)
        return new_element;

    if (!new_element)
        return chain;
    for (expr_ptr = &chain; (*expr_ptr)->next_in_chain; expr_ptr=&((*expr_ptr)->next_in_chain) );
    (*expr_ptr)->next_in_chain = new_element;
    return chain;
}

/*f c_model_descriptor::expression (value)
 */
t_md_expression *c_model_descriptor::expression( t_md_module *module, int width, t_md_signal_value *value )
{
     t_md_expression *expr;
     module=module;

     expr = expression_create( module, md_expr_type_value );
     if (!expr)
     {
          return NULL;
     }

     expr->data.value.value = *value;
     expr->data.value.value.width = width;
     //expr->width = width; - moved to push_possible_indices_to_subscripts
     return expr;
}

/*f c_model_descriptor::expression (lvar)
 */
t_md_expression *c_model_descriptor::expression( t_md_module *module, t_md_lvar *lvar )
{
     t_md_expression *expr;

     if (!lvar)
          return NULL;
     if (!lvar_is_terminal(lvar))
     {
          lvar_free( module, lvar );
          fprintf( stderr, "c_model_descriptor::lvar is not a terminal in expression\n");
          return NULL;
     }

     expr = expression_create( module, md_expr_type_lvar );
     if (!expr)
     {
          lvar_free( module, lvar);
          return NULL;
     }
     expr->data.lvar = lvar;
     // expr->width = lvar_width( lvar ); - moved to push_possible_indices_to_subscripts
     //printf("c_model_descriptor::expression_create:Created expression %p from model lvar %p (instance %p)\n", expr, lvar, lvar->instance );
     SL_DEBUG( sl_debug_level_info, "expression %p has width to be determined at push_possible_indices_to_subscripts %d (lvar %p)", expr, expr->width, lvar );
     return expr;
}

/*f c_model_descriptor::expression (string reference)
 */
t_md_expression *c_model_descriptor::expression( t_md_module *module, const char *reference)
{
     t_md_lvar *lvar;

     lvar = lvar_from_string( module, NULL, reference );
     return expression( module, lvar );
}

/*f c_model_descriptor::expression_cast( to_width, signed, expression )
 */
t_md_expression *c_model_descriptor::expression_cast( t_md_module *module, int to_width, int signed_cast, t_md_expression *expression )
{
    t_md_expression *expr;

    expr = expression_create( module, md_expr_type_cast );
    if (!expr)
    {
        return NULL;
    }

    expr->data.cast.signed_cast = signed_cast;
    expr->data.cast.expression = expression;
    expr->width = to_width; // Required here as this is a cast...
    return expr;
}

/*f c_model_descriptor::expression_bundle( expression, expression )
 */
t_md_expression *c_model_descriptor::expression_bundle( t_md_module *module, t_md_expression *a, t_md_expression *b )
{
    t_md_expression *expr;

    expr = expression_create( module, md_expr_type_bundle );
    if (!expr)
    {
        return NULL;
    }

    expr->data.bundle.a = a;
    expr->data.bundle.b = b;
    //expr->width = a->width + b->width; moved to push_possible_indices_to_subscripts
    return expr;
}

/*f c_model_descriptor::expression (function)
 */
t_md_expression *c_model_descriptor::expression( t_md_module *module, t_md_expr_fn fn, t_md_expression *args_0, t_md_expression *args_1, t_md_expression *args_2 )
{
     t_md_expression *expr;
     int width;
     int argc;

     expr = expression_create( module, md_expr_type_fn );
     if (!expr)
     {
          return NULL;
     }

     width = -1;
     argc = -1;
     switch (fn)
     {
     case md_expr_fn_not:
     case md_expr_fn_neg:
          width = args_0->width;
          argc = 1;
          break;
     case md_expr_fn_add:
     case md_expr_fn_sub:
     case md_expr_fn_mult:
     case md_expr_fn_and:
     case md_expr_fn_or:
     case md_expr_fn_bic:
     case md_expr_fn_xor:
          width = args_0->width;
          argc = 2;
          break;
     case md_expr_fn_logical_not:
          width = 1;
          argc = 1;
          break;
     case md_expr_fn_logical_and:
     case md_expr_fn_logical_or:
          width = 1;
          argc = 2;
          break;
     case md_expr_fn_eq:
     case md_expr_fn_neq:
     case md_expr_fn_ge:
     case md_expr_fn_gt:
     case md_expr_fn_le:
     case md_expr_fn_lt:
          width = 1;
          argc = 2;
          break;
     case md_expr_fn_query:
          width = args_1->width;
          argc = 3;
          break;
     default:
          fprintf(stderr,"Attempt to add an expression of unknown function %d\n", fn);
               //ERROR//
          break;
     }
     //expr->width = width; moved to - push_possible_indices_to_subscripts
     expr->data.fn.fn = fn;
     expr->data.fn.args[0] = args_0;
     expr->data.fn.args[1] = NULL;
     expr->data.fn.args[2] = NULL;
     if (argc>=2)
          expr->data.fn.args[1] = args_1;
     if (argc>=3)
          expr->data.fn.args[2] = args_2;
     SL_DEBUG( sl_debug_level_info, "expression %p has width not known until push_possible_indices_to_subscripts %d (arg 0 %p / %d)", expr, expr->width, args_0, args_0->width );
     return expr;
}

/*f c_model_descriptor::expression_find_dependencies
 */
int c_model_descriptor::expression_find_dependencies( t_md_module *module, t_md_code_block *code_block, t_md_reference_set **set_ptr, t_md_usage_type usage_type, t_md_expression *expr )
{
    int result;

    if (!expr)
    {
        fprintf(stderr, "BUG: reached expression_find_dependencies with NULL expression\n");
        return 0;
    }
    expr->code_block = code_block;
    switch (expr->type)
    {
    case md_expr_type_value:
        // No dependencies on an integer!
        return 1;
    case md_expr_type_lvar:
        switch (expr->data.lvar->index.type)
        {
        case md_lvar_data_type_expression:
            result = expression_find_dependencies( module, code_block, set_ptr, usage_type, expr->data.lvar->index.data.expression );
            break;
        default:
            result = 1;
        }
        switch (expr->data.lvar->subscript_start.type)
        {
        case md_lvar_data_type_expression:
            result = expression_find_dependencies( module, code_block, set_ptr, usage_type, expr->data.lvar->subscript_start.data.expression );
            break;
        default:
            result = 1;
        }
        if ( (usage_type==md_usage_type_rtl) &&
             ( ((expr->data.lvar->instance->reference.type==md_reference_type_signal) && (expr->data.lvar->instance->reference.data.signal->usage_type!=usage_type)) ||
               ((expr->data.lvar->instance->reference.type==md_reference_type_state) && (expr->data.lvar->instance->reference.data.state->usage_type!=usage_type)) ) )
        {
            error->add_error( NULL, error_level_serious, error_number_be_incorrect_usage_evalution, error_id_be_c_model_descriptor_statement_analyze,
                              error_arg_type_malloc_string, expr->data.lvar->instance->output_name,
                              error_arg_type_none );
        }
        result &= reference_add( set_ptr, expr->data.lvar->instance );
        return result;
    case md_expr_type_fn:
        result = expression_find_dependencies( module, code_block, set_ptr, usage_type, expr->data.fn.args[0] );
        if (expr->data.fn.args[1] )
            result &= expression_find_dependencies( module, code_block, set_ptr, usage_type, expr->data.fn.args[1] );
        if (expr->data.fn.args[2] )
            result &= expression_find_dependencies( module, code_block, set_ptr, usage_type, expr->data.fn.args[2] );
        return result;
    case md_expr_type_cast:
        result = expression_find_dependencies( module, code_block, set_ptr, usage_type, expr->data.cast.expression );
        return result;
    case md_expr_type_bundle:
        result = expression_find_dependencies( module, code_block, set_ptr, usage_type, expr->data.bundle.a );
        result &= expression_find_dependencies( module, code_block, set_ptr, usage_type, expr->data.bundle.b );
        return result;
    default:
        fprintf(stderr, "Expression type %d reference finding not implemented yet\n", expr->type);
        return 1;
    }
}

/*f c_model_descriptor::push_possible_indices_to_subscripts
 */
void c_model_descriptor::push_possible_indices_to_subscripts( t_md_expression *expr )
{

     if (!expr)
     {
          fprintf(stderr, "BUG: reached push_possible_indices_to_subscripts with NULL expression\n");
          return;
     }
     switch (expr->type)
     {
     case md_expr_type_value:
         expr->width = expr->data.value.value.width;
         return;
     case md_expr_type_lvar:
        push_possible_indices_to_subscripts( expr->data.lvar );
        expr->width = lvar_width( expr->data.lvar );
        return;
     case md_expr_type_fn:
          push_possible_indices_to_subscripts( expr->data.fn.args[0] );
          if (expr->data.fn.args[1] )
              push_possible_indices_to_subscripts( expr->data.fn.args[1] );
          if (expr->data.fn.args[2] )
              push_possible_indices_to_subscripts( expr->data.fn.args[2] );
          switch (expr->data.fn.fn)
          {
          case md_expr_fn_not:
          case md_expr_fn_neg:
          case md_expr_fn_add:
          case md_expr_fn_sub:
          case md_expr_fn_mult:
          case md_expr_fn_and:
          case md_expr_fn_or:
          case md_expr_fn_bic:
          case md_expr_fn_xor:
              expr->width = expr->data.fn.args[0]->width;
              break;
          case md_expr_fn_logical_not:
          case md_expr_fn_logical_and:
          case md_expr_fn_logical_or:
          case md_expr_fn_eq:
          case md_expr_fn_neq:
          case md_expr_fn_ge:
          case md_expr_fn_gt:
          case md_expr_fn_le:
          case md_expr_fn_lt:
              expr->width = 1;
              break;
          case md_expr_fn_query:
              expr->width = expr->data.fn.args[1]->width;
              break;
          default:
              break;
          }
          return;
     case md_expr_type_cast:
          push_possible_indices_to_subscripts( expr->data.cast.expression );
          expr->width = expr->width; // The cast is an explicit setting of the width
          return;
     case md_expr_type_bundle:
         push_possible_indices_to_subscripts( expr->data.bundle.a );
         push_possible_indices_to_subscripts( expr->data.bundle.b );
         expr->width = expr->data.bundle.a->width + expr->data.bundle.b->width;
         return;
     default:
          fprintf(stderr, "Expression type %d index pushing not implemented yet\n", expr->type);
          return;
     }
}

/*f c_model_descriptor::expression_check_liveness
 */
void c_model_descriptor::expression_check_liveness( t_md_module *module, t_md_code_block *code_block, t_md_expression *expr, t_instance_set *parent_makes_live, t_instance_set *being_made_live )
{
    if (!expr)
    {
        fprintf(stderr, "BUG: reached expression_check_liveness with NULL expression\n");
        return;
    }
    switch (expr->type)
    {
    case md_expr_type_value:
        return;
    case md_expr_type_lvar:
    {
        t_md_type_instance *instance;
        instance = expr->data.lvar->instance;
        if (expr->data.lvar->index.type==md_lvar_data_type_expression)
        {
            expression_check_liveness( module, code_block, expr->data.lvar->index.data.expression, parent_makes_live, being_made_live );
        }
        if (expr->data.lvar->subscript_start.type==md_lvar_data_type_expression)
        {
            expression_check_liveness( module, code_block, expr->data.lvar->subscript_start.data.expression, parent_makes_live, being_made_live );
        }
        if ( (instance->reference.type==md_reference_type_signal) &&
             (instance->code_block==code_block) &&
             !(reference_set_includes( &parent_makes_live, instance) || reference_set_includes( &being_made_live, instance)) )
        {
            //error->add_error( NULL, error_level_serious, error_number_be_used_while_not_live, error_id_be_c_model_descriptor_statement_analyze,
            error->add_error( NULL, error_level_warning, error_number_be_used_while_not_live, error_id_be_c_model_descriptor_statement_analyze,
                              error_arg_type_malloc_string, instance->output_name,
                              error_arg_type_malloc_string, code_block->name, 
                              error_arg_type_none );
            fprintf(stderr,"****************************************************************************\nSERIOUS WARNING: combinatorial '%s' is used while not live in code block %s - must set the value of a combinatorial before it is used within a block else Verilog sims will not match synthesis\n****************************************************************************\n",instance->output_name,code_block->name);
        }
        return;
    }
    case md_expr_type_fn:
        expression_check_liveness( module, code_block, expr->data.fn.args[0], parent_makes_live, being_made_live );
        if (expr->data.fn.args[1] )
            expression_check_liveness( module, code_block, expr->data.fn.args[1], parent_makes_live, being_made_live );
        if (expr->data.fn.args[2] )
            expression_check_liveness( module, code_block, expr->data.fn.args[2], parent_makes_live, being_made_live );
        return;
    case md_expr_type_cast:
        expression_check_liveness( module, code_block, expr->data.cast.expression, parent_makes_live, being_made_live );
        return;
    case md_expr_type_bundle:
        expression_check_liveness( module, code_block, expr->data.bundle.a, parent_makes_live, being_made_live );
        expression_check_liveness( module, code_block, expr->data.bundle.b, parent_makes_live, being_made_live );
        return;
    default:
        fprintf(stderr, "Expression type %d reference finding not implemented yet\n", expr->type);
        return;
    }
    return;
}

/*f c_model_descriptor::expressions_free
 */
void c_model_descriptor::expressions_free( t_md_expression *list )
{
     t_md_expression *next_expr;

     while (list)
     {
          next_expr = list->next_in_list;
          free(list);
          list = next_expr;
     }
}

/*a Switch_Item methods
 */
/*f c_model_descriptor::switch_item_create
 */
t_md_switch_item *c_model_descriptor::switch_item_create( t_md_module *module, t_md_client_reference *client_ref, t_md_switch_item_type type )
{
     t_md_switch_item *switch_item;

     switch_item = (t_md_switch_item *)malloc(sizeof(t_md_switch_item));
     if (!switch_item)
     {
          error->add_error( NULL, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_switch_item_create,
                            error_arg_type_malloc_string, module->name,
                            error_arg_type_none );
          return NULL;
     }

     switch_item->next_in_module = module->switch_items;
     module->switch_items = switch_item;
     switch_item->next_in_list = NULL;

     switch_item->client_ref = *client_ref;

     switch_item->type = type;
     switch_item->statement = NULL;
     return switch_item;
}

/*f c_model_descriptor::switch_item_create( static value )
 */
t_md_switch_item *c_model_descriptor::switch_item_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_signal_value *value, t_md_statement *statement )
{
     t_md_switch_item *switch_item;
     t_md_client_reference client_ref;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     switch_item = switch_item_create( module, &client_ref, md_switch_item_type_static );

     switch_item->data.value.value = *value;
     switch_item->statement = statement;

     return switch_item;
}

/*f c_model_descriptor::switch_item_create( static value, static mask )
 */
t_md_switch_item *c_model_descriptor::switch_item_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_signal_value *value, t_md_signal_value *mask, t_md_statement *statement )
{
     t_md_switch_item *switch_item;
     t_md_client_reference client_ref;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     switch_item = switch_item_create( module, &client_ref, md_switch_item_type_static_masked );

     switch_item->data.value.value = *value;
     switch_item->data.value.mask = *mask;
     switch_item->statement = statement;

     return switch_item;
}

/*f c_model_descriptor::switch_item_create( dynamic value  - expression )
 */
t_md_switch_item *c_model_descriptor::switch_item_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_expression *expr, t_md_statement *statement )
{
     t_md_switch_item *switch_item;
     t_md_client_reference client_ref;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     switch_item = switch_item_create( module, &client_ref, md_switch_item_type_static );

     switch_item->data.expr = expr;
     switch_item->statement = statement;

     return switch_item;
}

/*f c_model_descriptor::switch_item_create( default )
 */
t_md_switch_item *c_model_descriptor::switch_item_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_statement *statement )
{
     t_md_switch_item *switch_item;
     t_md_client_reference client_ref;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     switch_item = switch_item_create( module, &client_ref, md_switch_item_type_default );

     switch_item->statement = statement;

     return switch_item;
}

/*f c_model_descriptor::switch_item_chain
 */
t_md_switch_item *c_model_descriptor::switch_item_chain( t_md_switch_item *list, t_md_switch_item *item )
{
     t_md_switch_item *ptr;

     if (!list)
     {
          return item;
     }
     for (ptr=list; ptr->next_in_list; ptr=ptr->next_in_list);
     ptr->next_in_list = item;
     return list;
}

/*a Statement methods
 */
/*f c_model_descriptor::statement_create
 */
t_md_statement *c_model_descriptor::statement_create( t_md_module *module, t_md_client_reference *client_ref, t_md_statement_type type )
{
     t_md_statement *statement;

     statement = (t_md_statement *)malloc(sizeof(t_md_statement));
     if (!statement)
     {
          error->add_error( NULL, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_statement_create,
                            error_arg_type_malloc_string, module->name,
                            error_arg_type_none );
          return NULL;
     }

     statement->next_in_module = module->statements;
     module->statements = statement;
     statement->next_in_code = NULL;

     statement->client_ref = *client_ref;

     statement->type = type;
     statement->dependencies = NULL;
     statement->effects = NULL;

     statement->enumeration = -1;
     return statement;
}

/*f c_model_descriptor::statement_create_state_assignment
 */
t_md_statement *c_model_descriptor::statement_create_state_assignment( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_lvar *lvar, t_md_expression *expr, const char *documentation )
{
     t_md_statement *statement;
     t_md_client_reference client_ref;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     if (!lvar)
          return NULL;

     if (!lvar_is_terminal( lvar ))
     {
          error->add_error( module, error_level_serious, error_number_be_unresolved_reference, error_id_be_c_model_descriptor_statement_create,
                            error_arg_type_malloc_string, lvar->instance->name,
                            error_arg_type_none );
          return NULL;
     }

     statement = statement_create( module, &client_ref, md_statement_type_state_assign );

     statement->data.state_assign.lvar = lvar;
     statement->data.state_assign.expr = expr;
     statement->data.state_assign.documentation = documentation ? sl_str_alloc_copy(documentation) : NULL;

     //printf("c_model_descriptor::statement_create_state_assignment:Created statement %p from lvar %p expression %p\n", statement, lvar, expr );
     return statement;
}

/*f c_model_descriptor::statement_create_combinatorial_assignment
 */
t_md_statement *c_model_descriptor::statement_create_combinatorial_assignment( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_lvar *lvar, t_md_expression *expr, const char *documentation )
{
     t_md_statement *statement;
     t_md_client_reference client_ref;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     if (!lvar)
          return NULL;

     if (!lvar_is_terminal( lvar ))
     {
          error->add_error( module, error_level_serious, error_number_be_unresolved_reference, error_id_be_c_model_descriptor_statement_create,
                            error_arg_type_malloc_string, lvar->instance->name,
                            error_arg_type_none );
          return NULL;
     }

     statement = statement_create( module, &client_ref, md_statement_type_comb_assign );

     statement->data.comb_assign.lvar = lvar;
     statement->data.comb_assign.expr = expr;
     statement->data.comb_assign.documentation = documentation ? sl_str_alloc_copy(documentation) : NULL;

     return statement;
}

/*f c_model_descriptor::statement_create_if_else
 */
t_md_statement *c_model_descriptor::statement_create_if_else( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, t_md_expression *expr, t_md_statement *if_true, t_md_statement *if_false, const char *expr_documentation )
{
     t_md_statement *statement;
     t_md_client_reference client_ref;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     statement = statement_create( module, &client_ref, md_statement_type_if_else );

     statement->data.if_else.expr = expr;
     statement->data.if_else.expr_documentation = expr_documentation ? sl_str_alloc_copy(expr_documentation) : NULL;
     statement->data.if_else.if_true = if_true;
     statement->data.if_else.if_false = if_false;

     return statement;
}

/*f c_model_descriptor::statement_create_static_switch
 */
t_md_statement *c_model_descriptor::statement_create_static_switch( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, int parallel, int full, t_md_expression *expr, t_md_switch_item *items, const char *expr_documentation )
{
    t_md_statement *statement;
    t_md_client_reference client_ref;
    t_md_statement_type type;

    client_ref.base_handle = client_base_handle;
    client_ref.item_handle = client_item_handle;
    client_ref.item_reference = client_item_reference;

    type = md_statement_type_parallel_switch;
    if (!parallel)
    {
        type = md_statement_type_priority_list;
    }

    statement = statement_create( module, &client_ref, type );

    statement->data.switch_stmt.expr = expr;
    statement->data.switch_stmt.expr_documentation = expr_documentation ? sl_str_alloc_copy(expr_documentation) : NULL;
    statement->data.switch_stmt.parallel = parallel;
    statement->data.switch_stmt.full = full;
    statement->data.switch_stmt.all_static = 0;
    statement->data.switch_stmt.all_unmasked = 0;
    statement->data.switch_stmt.items = items;

    return statement;
}

/*f c_model_descriptor::statement_create_assert_cover
 */
t_md_statement *c_model_descriptor::statement_create_assert_cover( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *clock_name, int edge, t_md_usage_type usage_type, t_md_expression *expr, t_md_expression *value_list, t_md_message *message, t_md_statement *stmt )
{
    t_md_statement *statement;
    t_md_client_reference client_ref;
    t_md_signal *clock;

    client_ref.base_handle = client_base_handle;
    client_ref.item_handle = client_item_handle;
    client_ref.item_reference = client_item_reference;

    clock = NULL;
    if (clock_name)
    {
        clock = signal_find( clock_name, module->clocks );
        if (!clock)
        {
            error->add_error( module, error_level_fatal, error_number_be_undeclared_clock, error_id_be_c_model_descriptor_statement_create,
                              error_arg_type_malloc_string, clock_name,
                              error_arg_type_none );
            return NULL;
        }
    }

    statement = statement_create( module, &client_ref, (usage_type==md_usage_type_cover) ? md_statement_type_cover : md_statement_type_print_assert );

    statement->data.print_assert_cover.expression = expr;
    statement->data.print_assert_cover.value_list = value_list;
    statement->data.print_assert_cover.message = message;
    statement->data.print_assert_cover.clock_ref = clock;
    statement->data.print_assert_cover.edge = edge;
    statement->data.print_assert_cover.statement = stmt;
    statement->data.print_assert_cover.cover_case_entry = 0;
    if ((usage_type==md_usage_type_cover) && !stmt)
    {
        statement->data.print_assert_cover.cover_case_entry = module->next_cover_case_entry++;
    }

    if (clock)
    {
        clock->data.clock.edges_used[edge] = 1;
    }

    return statement;
}

/*f c_model_descriptor::statement_create_log
 */
t_md_statement *c_model_descriptor::statement_create_log( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *clock_name, int edge, const char *message, t_md_labelled_expression *values )
{
    t_md_statement *statement;
    t_md_client_reference client_ref;
    t_md_signal *clock;

    client_ref.base_handle = client_base_handle;
    client_ref.item_handle = client_item_handle;
    client_ref.item_reference = client_item_reference;

    clock = NULL;
    if (clock_name)
    {
        clock = signal_find( clock_name, module->clocks );
        if (!clock)
        {
            error->add_error( module, error_level_fatal, error_number_be_undeclared_clock, error_id_be_c_model_descriptor_statement_create,
                              error_arg_type_malloc_string, clock_name,
                              error_arg_type_none );
            return NULL;
        }
    }

    statement = statement_create( module, &client_ref, md_statement_type_log );

    statement->data.log.message=message;
    statement->data.log.arguments = values;
    statement->data.log.clock_ref = clock;
    statement->data.log.edge = edge;

    if (clock)
    {
        clock->data.clock.edges_used[edge] = 1;
    }

    return statement;
}

/*f c_model_descriptor::statement_add_to_chain
  Add a chain of statements at the end of a current chain of statements
 */
t_md_statement *c_model_descriptor::statement_add_to_chain( t_md_statement *first_statement, t_md_statement *new_statement )
{
     t_md_statement *statement;

     if (!first_statement)
          return NULL;
     if (new_statement)
     {
          for (statement=first_statement; statement->next_in_code; statement=statement->next_in_code);
          statement->next_in_code = new_statement;
     }
     return first_statement;
}

/*f c_model_descriptor::statement_add_effects
 */
void c_model_descriptor::statement_add_effects( t_md_statement *chain, t_md_reference_set **effects_set_ptr )
{
    t_md_statement *stmt;
    for (stmt=chain; stmt; stmt=stmt->next_in_code)
        reference_union_sets( stmt->effects, effects_set_ptr ); 
}

/*f c_model_descriptor::statement_analyze
    Any variable references in expressions should already be cross-referenced, so all we need to do is generate reference sets for dependencies
    We also need to generate the list of everything this statement effects
    We may have a predicate (set of conditions on which this statement relies for it to be executed) that has dependencies
    So we inherit those dependencies
    We also update the code_block with any new dependencies
    A predicate_ptr of NULL implies no predicate
 */
int c_model_descriptor::statement_analyze( t_md_code_block *code_block, t_md_usage_type usage_type, t_md_statement *statement, t_md_reference_set **predicate_ptr )
{
    int result;
    t_md_lvar *lvar;
    t_md_expression *expr;

    SL_DEBUG( sl_debug_level_info, "Statement %p", statement );
    if (!statement)
        return 1;

    // Record the number of this statement
    statement->enumeration = this->statement_enumerator++;
    statement->code_block = code_block;

    if (predicate_ptr) // If this statement is predicated on something, add that something to this statement's dependencies
        reference_union_sets( *predicate_ptr, &statement->dependencies );

    switch (statement->type)
    {
    case md_statement_type_state_assign:
        lvar = statement->data.state_assign.lvar;
        expr = statement->data.state_assign.expr;
        SL_DEBUG( sl_debug_level_info, "State assignment statement lvar %p expr %p", lvar, expr );
        if (lvar->instance->code_block && (lvar->instance->code_block!=code_block))
        {
            error->add_error( NULL, error_level_serious, error_number_be_assignment_in_many_code_blocks, error_id_be_c_model_descriptor_statement_analyze,
                              error_arg_type_malloc_string, lvar->instance->output_name,
                              error_arg_type_malloc_string, lvar->instance->code_block->name,
                              error_arg_type_malloc_string, code_block->name,
                              error_arg_type_none );
        }
        lvar->instance->code_block = code_block;
        lvar->instance->number_statements++;
        push_possible_indices_to_subscripts(lvar);
        push_possible_indices_to_subscripts(expr);

        if ( ((lvar->instance->reference.type==md_reference_type_state) && (lvar->instance->reference.data.state->usage_type!=usage_type)) )
        {
            error->add_error( NULL, error_level_serious, error_number_be_incorrect_usage_assignment, error_id_be_c_model_descriptor_statement_analyze,
                              error_arg_type_malloc_string, lvar->instance->output_name,
                              error_arg_type_none );
        }
        result = expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, expr );
        if (lvar->subscript_start.type==md_lvar_data_type_expression)
            result &= expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, lvar->subscript_start.data.expression );
        if (lvar->index.type==md_lvar_data_type_expression)
            result &= expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, lvar->index.data.expression );
        reference_add( &statement->effects, lvar->instance ); // Add the instance to what the statement effects
        reference_union_sets( statement->dependencies, &lvar->instance->dependencies ); // Add any dependencies in the statement (such as the expression or predicate) to the lvar's dependencies
        break;

    case md_statement_type_comb_assign:
        // Assignee usage type must match the scope usage type
        // If in RTL scope, then expression must not contain assert or cover types
        lvar = statement->data.comb_assign.lvar;
        expr = statement->data.comb_assign.expr;
        SL_DEBUG( sl_debug_level_info, "Comb assignment statement lvar %p (%p) expr %p code block %p", lvar, lvar->instance, expr, code_block );
        if (lvar->instance->code_block && (lvar->instance->code_block!=code_block))
        {
            error->add_error( NULL, error_level_serious, error_number_be_assignment_in_many_code_blocks, error_id_be_c_model_descriptor_statement_analyze,
                              error_arg_type_malloc_string, lvar->instance->output_name,
                              error_arg_type_malloc_string, lvar->instance->code_block->name,
                              error_arg_type_malloc_string, code_block->name,
                              error_arg_type_none );
        }
        lvar->instance->code_block = code_block;
        lvar->instance->number_statements++;
        if ( ((lvar->instance->reference.type==md_reference_type_signal) && (lvar->instance->reference.data.signal->usage_type!=usage_type)) )
        {
            error->add_error( NULL, error_level_serious, error_number_be_incorrect_usage_assignment, error_id_be_c_model_descriptor_statement_analyze,
                              error_arg_type_malloc_string, lvar->instance->output_name,
                              error_arg_type_none );
        }

        push_possible_indices_to_subscripts(lvar);
        push_possible_indices_to_subscripts(expr);
        result = expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, expr );

        if (lvar->subscript_start.type==md_lvar_data_type_expression)
            result &= expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, lvar->subscript_start.data.expression );
        if (lvar->index.type==md_lvar_data_type_expression)
            result &= expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, lvar->index.data.expression );

        reference_add( &statement->effects, lvar->instance ); // Add the instance to what the statement effects
        reference_union_sets( statement->dependencies, &lvar->instance->dependencies ); // Add any dependencies in the statement (such as the expression or predicate) to the lvar's dependencies
        break;

    case md_statement_type_if_else:
        push_possible_indices_to_subscripts(statement->data.if_else.expr);
        result = expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, statement->data.if_else.expr ); // Add any instances used in the expression to the statement's dependencies
        if (statement->data.if_else.if_true)
        {
            result &= statement_analyze( code_block, usage_type, statement->data.if_else.if_true, &statement->dependencies ); // Analyze the 'true' branch with this statement (including expression) as the predicate
            statement_add_effects( statement->data.if_else.if_true, &statement->effects ); // Whatever the 'true' branch effects, this statement effects too
        }
        if (statement->data.if_else.if_false)
        {
            result &= statement_analyze( code_block, usage_type, statement->data.if_else.if_false, &statement->dependencies ); // Analyze the 'false' branch with this statement (including expression) as the predicate
            statement_add_effects( statement->data.if_else.if_false, &statement->effects ); // Whatever the 'false' branch effects, this statement effects too
        }
        break;

    case md_statement_type_parallel_switch:
        t_md_switch_item *switem;
        int all_unmasked;
        int all_static;

        push_possible_indices_to_subscripts(statement->data.switch_stmt.expr);
        result = expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, statement->data.switch_stmt.expr ); // Add any instances used in the expression to the statement's dependencies

        all_unmasked = 1;
        all_static = 1;
        for (switem = statement->data.switch_stmt.items; switem; switem=switem->next_in_list)
        {
            if (switem->type == md_switch_item_type_static_masked)
            {
                all_unmasked = 0;
            }
            if (switem->type == md_switch_item_type_dynamic)
            {
                all_static = 0;
                push_possible_indices_to_subscripts(switem->data.expr);
                result &= expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, switem->data.expr);
            }
            if (switem->statement) // Statement may be null if there is nothing to do, e.g. case 1:{}
            {
                result &= statement_analyze( code_block, usage_type, switem->statement, &statement->dependencies ); // analyze the case code, predicated on this statement and case expression

                statement_add_effects( switem->statement, &statement->effects ); // everything the statement chain in the case effects, the switch statement effects
            }
        }
        statement->data.switch_stmt.all_unmasked = all_unmasked;
        statement->data.switch_stmt.all_static = all_static;
        break;

    case md_statement_type_print_assert:
    case md_statement_type_cover:
        if ( (usage_type!=md_usage_type_rtl) &&
             ( (statement->data.print_assert_cover.expression) || (statement->data.print_assert_cover.statement) ) )
        {
            if ( ((usage_type==md_usage_type_assert) && (statement->type==md_statement_type_cover)) ||
                 ((usage_type==md_usage_type_cover) && (statement->type==md_statement_type_print_assert)) )
            {
                error->add_error( NULL, error_level_serious, error_number_be_mixing_of_assert_and_cover, error_id_be_c_model_descriptor_statement_analyze,
                                  error_arg_type_malloc_string, code_block->name,
                                  error_arg_type_none );
            }
        }
        result = 1;
        if (statement->data.print_assert_cover.expression)
        {
            push_possible_indices_to_subscripts(statement->data.print_assert_cover.expression);
            result = expression_find_dependencies( code_block->module, code_block, &statement->dependencies, (statement->type==md_statement_type_cover)?md_usage_type_cover:md_usage_type_assert, statement->data.print_assert_cover.expression ); // Add any instances used in the expression to the assertion
        }
        if (statement->data.print_assert_cover.value_list)
        {
            t_md_expression *expr;
            expr = statement->data.print_assert_cover.value_list;
            while (expr)
            {
                push_possible_indices_to_subscripts(expr);
                result &= expression_find_dependencies( code_block->module, code_block, &statement->dependencies, (statement->type==md_statement_type_cover)?md_usage_type_cover:md_usage_type_assert, expr ); // Add any instances used in the expression to the assertion
                expr = expr->next_in_chain;
            }
        }
        if (statement->data.print_assert_cover.message)
        {
            t_md_expression *expr;
            expr = statement->data.print_assert_cover.message->arguments;
            while (expr)
            {
                push_possible_indices_to_subscripts(expr);
                result &= expression_find_dependencies( code_block->module, code_block, &statement->dependencies, (statement->type==md_statement_type_cover)?md_usage_type_cover:md_usage_type_assert, expr ); // Add any instances used in the expression to the assertion
                expr = expr->next_in_chain;
            }
        }
        if (statement->data.print_assert_cover.statement)
        {
            result &= statement_analyze( code_block, (statement->type==md_statement_type_cover)?md_usage_type_cover:md_usage_type_assert, statement->data.print_assert_cover.statement, &statement->dependencies ); // analyze the assert code, predicated on this statement and assertion expression
            statement_add_effects( statement->data.print_assert_cover.statement, &statement->effects ); // everything the statement chain in the assertion effects, the assert statement effects
        }
        if (statement->data.print_assert_cover.clock_ref)
        {
            reference_add( &statement->effects, statement->data.print_assert_cover.clock_ref, statement->data.print_assert_cover.edge );
        }
        break;

    case md_statement_type_log:
    {
        t_md_labelled_expression *arg;
        result=1;
        for (arg=statement->data.log.arguments; arg; arg=arg->next_in_chain)
        {
            push_possible_indices_to_subscripts(arg->expression);
            result &= expression_find_dependencies( code_block->module, code_block, &statement->dependencies, usage_type, arg->expression ); // Add any instances used in the expression to the assertion
        }
        if (statement->data.log.clock_ref)
        {
            reference_add( &statement->effects, statement->data.log.clock_ref, statement->data.log.edge );
        }
        break;
    }

    default:
        fprintf(stderr, "Unexpected statement type %d in analysis\n", statement->type);
        exit(4);
        break;
     }

    reference_union_sets( statement->dependencies, &code_block->dependencies ); // Whatever this statement depends on, so does its code block
    reference_union_sets( statement->effects, &code_block->effects ); // Whatever this individual statement effects, so does the code block - we will catch the chain by chaining

    return result && statement_analyze( code_block, usage_type, statement->next_in_code, predicate_ptr );
}

/*f c_model_descriptor::statement_analyze_liveness
  Analyze statement for variable liveness, after basic analysis has been done
 */
void c_model_descriptor::statement_analyze_liveness( t_md_code_block *code_block, t_md_usage_type usage_type, t_md_statement *statement, t_instance_set *parent_makes_live, t_instance_set **makes_live )
{
    SL_DEBUG( sl_debug_level_info, "Statement %p", statement );
    if (!statement)
        return;

    switch (statement->type)
    {
    case md_statement_type_state_assign: // State assignments are always okay, as they are done on the clock edge
        break; 
    case md_statement_type_comb_assign:
    {
        t_md_lvar *lvar;
        lvar = statement->data.comb_assign.lvar;
        // Check expression (or index) for liveness - it may only use items that are live (already assigned in the code block, or assigned in a different code block)
        expression_check_liveness( code_block->module, code_block, statement->data.comb_assign.expr, parent_makes_live, *makes_live );
        if (lvar->subscript_start.type==md_lvar_data_type_expression)
            expression_check_liveness( code_block->module, code_block, lvar->subscript_start.data.expression, parent_makes_live, *makes_live );
        if (lvar->index.type==md_lvar_data_type_expression)
            expression_check_liveness( code_block->module, code_block, lvar->index.data.expression, parent_makes_live, *makes_live );
        instance_set_add( makes_live, lvar->instance );
        break;
    }
    case md_statement_type_if_else:
    {
        t_instance_set *if_makes_live;
        expression_check_liveness( code_block->module, code_block, statement->data.if_else.expr, parent_makes_live, *makes_live );

        //instance_set_display(*makes_live);

        instance_set_null(if_makes_live);
        if (statement->data.if_else.if_true)
        {
            instance_set_union(&if_makes_live,*makes_live);
            statement_analyze_liveness( code_block, usage_type, statement->data.if_else.if_true, parent_makes_live, &if_makes_live );
        }
        if (statement->data.if_else.if_false)
        {
            t_instance_set *false_makes_live;
            instance_set_null(false_makes_live);
            instance_set_union(&false_makes_live,*makes_live);
            statement_analyze_liveness( code_block, usage_type, statement->data.if_else.if_false, parent_makes_live, &false_makes_live );
            instance_set_intersect( &if_makes_live, false_makes_live );
            instance_set_free( false_makes_live );
        }
        else
        {
            instance_set_free(if_makes_live);
        }
        instance_set_union(makes_live,if_makes_live);
        instance_set_free(if_makes_live);

        break;
    }
    case md_statement_type_parallel_switch:
    {
        t_instance_set *switch_makes_live;
        t_md_switch_item *switem;
        int first;
        int has_default;

        instance_set_null(switch_makes_live);
        has_default = 0;
        instance_set_union(&switch_makes_live,*makes_live);
        expression_check_liveness( code_block->module, code_block, statement->data.switch_stmt.expr, parent_makes_live, *makes_live );
        first = 1;
        for (switem = statement->data.switch_stmt.items; switem; switem=switem->next_in_list)
        {
            if (switem->type == md_switch_item_type_dynamic)
            {
                expression_check_liveness( code_block->module, code_block, switem->data.expr, parent_makes_live, *makes_live );
            }
            if (switem->type == md_switch_item_type_default)
            {
                has_default = 1;
            }
            if (switem->statement) // Statement may be null if there is nothing to do, e.g. case 1:{}
            {
                if (first)
                {
                    statement_analyze_liveness( code_block, usage_type, switem->statement, parent_makes_live, &switch_makes_live );
                }
                else
                {
                    t_instance_set *case_makes_live;
                    instance_set_null(case_makes_live);
                    statement_analyze_liveness( code_block, usage_type, switem->statement, parent_makes_live, &case_makes_live );
                    instance_set_intersect( &switch_makes_live, case_makes_live );
                    instance_set_union(&switch_makes_live,*makes_live);
                    instance_set_free(case_makes_live);
                }
            }
        }
        if (has_default)
        {
            instance_set_union(makes_live,switch_makes_live); // full switch statements are NOT guaranteed to cover every case, but ARE guaranteed to generate an assertion should an unexpected case be hit
        }
        instance_set_free(switch_makes_live);
        break;
    }
    case md_statement_type_print_assert:
    case md_statement_type_cover:
    {
        if (statement->data.print_assert_cover.expression)
        {
            expression_check_liveness( code_block->module, code_block, statement->data.print_assert_cover.expression, parent_makes_live, *makes_live );
        }
        if (statement->data.print_assert_cover.value_list)
        {
            t_md_expression *expr;
            expr = statement->data.print_assert_cover.value_list;
            while (expr)
            {
                expression_check_liveness( code_block->module, code_block, expr, parent_makes_live, *makes_live );
                expr = expr->next_in_chain;
            }
        }
        if (statement->data.print_assert_cover.statement)
        {
            statement_analyze_liveness( code_block, usage_type, statement->data.print_assert_cover.statement, parent_makes_live, makes_live ); // Since there is no condition, can just use makes_live here
        }
        break;
    }
    case md_statement_type_log:
    {
        t_md_labelled_expression *arg;
        for (arg=statement->data.log.arguments; arg; arg=arg->next_in_chain)
        {
            expression_check_liveness( code_block->module, code_block, arg->expression, parent_makes_live, *makes_live );
        }
        break;
    }
    default:
        fprintf(stderr, "Unexpected statement type %d in analysis\n", statement->type);
        exit(4);
        break;
     }

    statement_analyze_liveness( code_block, usage_type, statement->next_in_code, parent_makes_live, makes_live );
}

/*f c_model_descriptor::statements_free
 */
void c_model_descriptor::statements_free( t_md_statement *list )
{
     t_md_statement *next_statement;

     while (list)
     {
          next_statement = list->next_in_module;
          free(list);
          list = next_statement;
     }
}

/*a Code block methods
 */
/*f c_model_descriptor::code_block_create
 */
t_md_code_block *c_model_descriptor::code_block_create( t_md_module *module, const char *name, int copy_name, t_md_client_reference *client_ref, const char *documentation )
{
     t_md_code_block *code;

     code = (t_md_code_block *)malloc(sizeof(t_md_code_block));
     if (!code)
     {
          error->add_error( NULL, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_code_block_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return NULL;
     }

     if (module->last_code_block)
     {
          module->last_code_block->next_in_list = code;
     }
     else
     {
          module->code_blocks = code;
     }
     module->last_code_block = code;
     code->next_in_list = NULL;

     code->module = module;

     if (copy_name)
     {
          code->name = sl_str_alloc_copy( name );
          code->name_to_be_freed = 1;
     }
     else
     {
          code->name = (char *)name;
          code->name_to_be_freed = 0;
     }

     code->client_ref = *client_ref;
     code->documentation = documentation ? sl_str_alloc_copy(documentation) : NULL;

     code->first_statement = NULL;
     code->last_statement = NULL;
     code->dependencies = NULL;
     code->effects = NULL;
     return code;
}

/*f c_model_descriptor::code_block_find
 */
t_md_code_block *c_model_descriptor::code_block_find( const char *name, t_md_code_block *list )
{
     t_md_code_block *code;

     for (code=list; code; code=code->next_in_list)
     {
          if (!strcmp(code->name, name))
          {
               return code;
          }
     }
     return NULL;
}

/*f c_model_descriptor::code_block
 */
int c_model_descriptor::code_block( t_md_module *module, const char *name, int copy_name, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *documentation )
{
     t_md_code_block *code;
     t_md_client_reference client_ref;

     client_ref.base_handle = client_base_handle;
     client_ref.item_handle = client_item_handle;
     client_ref.item_reference = client_item_reference;

     code = code_block_find( name, module->code_blocks );
     if (code)
     {
          error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_code_block_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return 0;
     }
     code = code_block_create( module, name, copy_name, &client_ref, documentation );
     if (!code)
     {
          return 0;
     }
     return 1;
}

/*f c_model_descriptor::code_block_add_statement
 */
int c_model_descriptor::code_block_add_statement( t_md_module *module, const char *code_block_name, t_md_statement *statement )
{
     t_md_code_block *code;

     code = code_block_find( code_block_name, module->code_blocks );
     if (!code)
     {
          error->add_error( module, error_level_serious, error_number_be_unresolved_reference, error_id_be_c_model_descriptor_code_block_add_statement,
                            error_arg_type_malloc_string, code_block_name,
                            error_arg_type_none );
          return 0;
     }

     if (code->last_statement)
     {
         code->last_statement = statement_add_to_chain( code->last_statement, statement );
         while (statement->next_in_code)
             statement=statement->next_in_code;
         code->last_statement = statement;
     }
     else
     {
          code->first_statement = statement;
          code->last_statement = statement;
     }

     return 1;
}

/*f c_model_descriptor::code_block_analyze
 */
int c_model_descriptor::code_block_analyze( t_md_code_block *code_block, t_md_usage_type usage_type )
{
    /*b Generate reference sets for each statement herein
     */
    statement_analyze( code_block, usage_type, code_block->first_statement, NULL );

    /*b Analyze the liveness of every comb assigned within the block, to ensure there are no transparent latches, and nothing is used before it is defined
     */
    {
        t_md_reference_set *parent_makes_live, *makes_live;
        t_md_signal *signal;
        instance_set_null( makes_live );
        instance_set_null( parent_makes_live );
        statement_analyze_liveness( code_block, usage_type, code_block->first_statement, parent_makes_live, &makes_live );
        for (signal=code_block->module->combinatorials; signal; signal=signal->next_in_list)
        {
            int i;
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                t_md_type_instance *instance;
                instance = signal->instance_iter->children[i];
                if (instance->code_block==code_block)
                {
                    if (!reference_set_includes( &makes_live, instance))
                    {
                        //error->add_error( NULL, error_level_serious, error_number_be_likely_transparent_latch, error_id_be_c_model_descriptor_statement_analyze,
                        error->add_error( NULL, error_level_warning, error_number_be_likely_transparent_latch, error_id_be_c_model_descriptor_statement_analyze,
                                          error_arg_type_malloc_string, instance->output_name,
                                          error_arg_type_none );
                        fprintf(stderr, "****************************************************************************\nSERIOUS ERROR:Expected '%s' to be live - likely transparent latch?\n", instance->output_name );
                    }
                }
            }
        }
    }

    /*b Return okay
     */
    return 1;
}

/*f c_model_descriptor::code_blocks_free
 */
void c_model_descriptor::code_blocks_free( t_md_code_block *list )
{
     t_md_code_block *next_code_block;

     while (list)
     {
          next_code_block = list->next_in_list; 
          if (list->name_to_be_freed)
               free(list->name);
          free(list);
          list = next_code_block;
     }
}

/*a Module instantiation and port methods
 */
/*f c_model_descriptor::module_instance_create
 */
t_md_module_instance *c_model_descriptor::module_instance_create( t_md_module *module, const char *type, int copy_type, const char *name, int copy_name, t_md_client_reference *client_ref )
{
     t_md_module_instance *module_instance;

     module_instance = (t_md_module_instance *)malloc(sizeof(t_md_module_instance));
     if (!module_instance)
     {
          error->add_error( NULL, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_module_instance_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return NULL;
     }

     if (module->last_module_instance)
     {
          module->last_module_instance->next_in_list = module_instance;
     }
     else
     {
          module->module_instances = module_instance;
     }
     module->last_module_instance = module_instance;
     module_instance->next_in_list = NULL;

     module_instance->module_definition = NULL;

     if (copy_name)
     {
          module_instance->name = sl_str_alloc_copy( name );
          module_instance->name_to_be_freed = 1;
     }
     else
     {
          module_instance->name = (char *)name;
          module_instance->name_to_be_freed = 0;
     }

     if (copy_type)
     {
          module_instance->type = sl_str_alloc_copy( type );
          module_instance->type_to_be_freed = 1;
     }
     else
     {
          module_instance->type = (char *)type;
          module_instance->type_to_be_freed = 0;
     }

     module_instance->client_ref = *client_ref;

     module_instance->clocks = NULL;
     module_instance->inputs = NULL;
     module_instance->outputs = NULL;
     module_instance->combinatorial_dependencies = NULL;
     module_instance->dependencies = NULL;
     module_instance->effects = NULL;
     return module_instance;
}

/*f c_model_descriptor::module_instance_find
 */
t_md_module_instance *c_model_descriptor::module_instance_find( const char *name, t_md_module_instance *list )
{
     t_md_module_instance *module_instance;

     for (module_instance=list; module_instance; module_instance=module_instance->next_in_list)
     {
          if (!strcmp(module_instance->name, name))
          {
               return module_instance;
          }
     }
     return NULL;
}

/*f c_model_descriptor::module_instance
 */
int c_model_descriptor::module_instance( t_md_module *module, const char *name, int copy_name, const char *type, int copy_type, void *client_base_handle, const void *client_item_handle, int client_item_reference )
{
    t_md_module_instance *module_instance;
    t_md_client_reference client_ref;

    client_ref.base_handle = client_base_handle;
    client_ref.item_handle = client_item_handle;
    client_ref.item_reference = client_item_reference;

    module_instance = module_instance_find( name, module->module_instances );
    if (module_instance)
    {
        error->add_error( module, error_level_fatal, error_number_be_duplicate_name, error_id_be_c_model_descriptor_module_instance_create,
                          error_arg_type_malloc_string, name,
                          error_arg_type_none );
        return 0;
    }
    module_instance = module_instance_create( module, name, copy_name, type, copy_type, &client_ref );
    if (!module_instance)
    {
        return 0;
    }
    return 1;
}

/*f c_model_descriptor::module_instance_add_clock
 */
int c_model_descriptor::module_instance_add_clock( t_md_module *module, const char *instance_name, const char *port_name, const char *clock_name )
{
     t_md_module_instance *module_instance;
     t_md_module_instance_clock_port *clock_port;

     module_instance = module_instance_find( instance_name, module->module_instances );
     if (!module_instance)
     {
         error->add_error( module, error_level_fatal, error_number_be_unknown_module_name, error_id_be_c_model_descriptor_module_instance_add_clock,
                           error_arg_type_malloc_string, instance_name,
                           error_arg_type_none );
         return 0;
     }

     clock_port = (t_md_module_instance_clock_port *)malloc(sizeof(t_md_module_instance_clock_port));
     if (!clock_port)
         return 0;

     clock_port->next_in_list = module_instance->clocks;
     module_instance->clocks = clock_port;
     clock_port->port_name = sl_str_alloc_copy( port_name );
     clock_port->clock_name = sl_str_alloc_copy( clock_name );
     clock_port->module_port_signal = NULL;
     clock_port->local_clock_signal = NULL;

     return 1;
}

/*f c_model_descriptor::module_instance_add_input
 */
int c_model_descriptor::module_instance_add_input( t_md_module *module, const char *instance_name, t_md_port_lvar *port_lvar, t_md_expression *expression )
{
     t_md_module_instance *module_instance;
     t_md_module_instance_input_port *input_port;

     module_instance = module_instance_find( instance_name, module->module_instances );
     if (!module_instance)
     {
         error->add_error( module, error_level_fatal, error_number_be_unknown_module_name, error_id_be_c_model_descriptor_module_instance_add_clock,
                           error_arg_type_malloc_string, instance_name,
                           error_arg_type_none );
         return 0;
     }

     input_port = (t_md_module_instance_input_port *)malloc(sizeof(t_md_module_instance_input_port));
     if (!input_port)
         return 0;

     input_port->next_in_list = module_instance->inputs;
     module_instance->inputs = input_port;
     input_port->port_lvar = port_lvar;
     input_port->expression = expression;
     input_port->module_port_instance = NULL;

     return 1;
}

/*f c_model_descriptor::module_instance_add_output
 */
int c_model_descriptor::module_instance_add_output( t_md_module *module, const char *instance_name, t_md_port_lvar *port_lvar, t_md_lvar *lvar )
{
     t_md_module_instance *module_instance;
     t_md_module_instance_output_port *output_port;

     module_instance = module_instance_find( instance_name, module->module_instances );
     if (!module_instance)
     {
         error->add_error( module, error_level_fatal, error_number_be_unknown_module_name, error_id_be_c_model_descriptor_module_instance_add_clock,
                           error_arg_type_malloc_string, instance_name,
                           error_arg_type_none );
         return 0;
     }

     output_port = (t_md_module_instance_output_port *)malloc(sizeof(t_md_module_instance_output_port));
     if (!output_port)
         return 0;

     output_port->next_in_list = module_instance->outputs;
     module_instance->outputs = output_port;
     output_port->port_lvar = port_lvar;
     push_possible_indices_to_subscripts( lvar );
     output_port->lvar = lvar;
     output_port->module_port_instance = NULL;

     return 1;
}

/*f c_model_descriptor::module_instance_cross_reference
 */
int c_model_descriptor::module_instance_cross_reference( t_md_module *module, t_md_module_instance *module_instance )
{
    t_md_module *module_definition;
    t_md_module_instance_clock_port *clock_port;
    t_md_module_instance_input_port *input_port;
    t_md_module_instance_output_port *output_port;
    int result;

    result = 1;
    module_definition=module_find(module_instance->type);
    if (!module_definition)
    {
        fprintf(stderr, "Unknown module type '%s' in instantiation inside module '%s'\n", module_instance->type, module->name );
        result = 0;
    }
    else
    {
        module_instance->module_definition=module_definition;
        for (clock_port=module_instance->clocks; clock_port; clock_port=clock_port->next_in_list)
        {
            clock_port->module_port_signal = signal_find( clock_port->port_name, module_definition->clocks );
            clock_port->local_clock_signal = signal_find( clock_port->clock_name, module->clocks );
            if (!clock_port->module_port_signal)
            {
                error->add_error( module, error_level_fatal, error_number_be_unresolved_clock_port_reference, error_id_be_c_model_descriptor_module_instance_analyze,
                                  error_arg_type_malloc_string, clock_port->port_name,
                                  error_arg_type_malloc_string, module_instance->name,
                                  error_arg_type_malloc_string, module_instance->type,
                                  error_arg_type_none );
            }
            if (!clock_port->local_clock_signal)
            {
                error->add_error( module, error_level_fatal, error_number_be_unresolved_clock_portref_reference, error_id_be_c_model_descriptor_module_instance_analyze,
                                  error_arg_type_malloc_string, clock_port->clock_name,
                                  error_arg_type_malloc_string, module_instance->name,
                                  error_arg_type_malloc_string, module_instance->type,
                                  error_arg_type_none );
            }
        }
        for (input_port=module_instance->inputs; input_port; input_port=input_port->next_in_list)
        {
            input_port->module_port_instance = port_lvar_resolve( input_port->port_lvar, module_definition->inputs );
            if (!input_port->module_port_instance)
            {
                error->add_error( module, error_level_fatal, error_number_be_unresolved_input_port_reference, error_id_be_c_model_descriptor_module_instance_analyze,
                                  error_arg_type_malloc_string, input_port->port_lvar->name,
                                  error_arg_type_malloc_string, module_instance->name,
                                  error_arg_type_malloc_string, module_instance->type,
                                  error_arg_type_none );
            }
        }
        for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
        {
            output_port->module_port_instance = port_lvar_resolve( output_port->port_lvar, module_definition->outputs );
            if (!output_port->module_port_instance)
            {
                error->add_error( module, error_level_fatal, error_number_be_unresolved_output_port_reference, error_id_be_c_model_descriptor_module_instance_analyze,
                                  error_arg_type_malloc_string, output_port->port_lvar->name,
                                  error_arg_type_malloc_string, module_instance->name,
                                  error_arg_type_malloc_string, module_instance->type,
                                  error_arg_type_none );
            }
        }
    }
    return result;
}

/*f c_model_descriptor::module_instance_analyze
  Set combinatorial_dependencies to be everything within all the combinatorial input's port expressions
  Add this set to all combinatorial outputs' dependencies
  Add relevant clock edges to all clocked outputs' dependencies
 */
void c_model_descriptor::module_instance_analyze( t_md_module *module, t_md_module_instance *module_instance )
{
    int result;
    t_md_module_instance_clock_port *clock_port;
    t_md_module_instance_input_port *input_port;
    t_md_module_instance_output_port *output_port;
    t_md_reference_iter iter;
    t_md_reference *reference;

    SL_DEBUG( sl_debug_level_info, "Type '%s' name '%s'", module_instance->type, module_instance->name );
    result = 1;

    for (input_port=module_instance->inputs; input_port; input_port=input_port->next_in_list)
    {
        if (input_port->expression)
        {
            push_possible_indices_to_subscripts(input_port->expression);
            result &= expression_find_dependencies( module, NULL, &module_instance->dependencies, md_usage_type_rtl, input_port->expression );
            if (input_port->module_port_instance)
            {
                if (input_port->module_port_instance->reference.data.signal->data.input.used_combinatorially)
                {
                    result &= expression_find_dependencies( module, NULL, &module_instance->combinatorial_dependencies, md_usage_type_rtl, input_port->expression );
                }

                reference_set_iterate_start( &input_port->module_port_instance->reference.data.signal->data.input.clocks_used_on, &iter );
                while ((reference = reference_set_iterate(&iter))!=NULL)
                {
                     for (clock_port=module_instance->clocks; clock_port; clock_port=clock_port->next_in_list)
                     {
                          if (clock_port->module_port_signal==reference->data.signal)
                          {
                               break;
                          }
                     }
                     // Mark clock edge as used, and module depends on the edge too
                     if ((clock_port) && (clock_port->local_clock_signal))
                     {
                         clock_port->local_clock_signal->data.clock.edges_used[reference->edge] = 1;
                         reference_add( &module_instance->dependencies, clock_port->local_clock_signal, reference->edge  );
                         SL_DEBUG( sl_debug_level_info, "clock %s %d used for input %s", clock_port->local_clock_signal->name, reference->edge, input_port->port_lvar->name );
                     }
                }
            }
        }
    }

    for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
    {
        if ((output_port->lvar) && (output_port->lvar->instance))
        {
            // Must check that output_port->lvar->instance is a net
            if (0)
            {
                printf("Type of module output port lvar %s : %s is %d %s %d\n",
                       output_port->port_lvar->name,
                       output_port->lvar->instance->name,
                       output_port->lvar->instance->reference.type,
                       output_port->lvar->instance->reference.data.signal->name,
                       output_port->lvar->instance->reference.data.signal->type
                    );
            }
            if ( (!output_port->lvar->instance->reference.data.signal) ||
                 (output_port->lvar->instance->reference.data.signal->type != md_signal_type_net) )
            {
                error->add_error( module, error_level_fatal, error_number_be_signal_driven_by_module_not_a_net, error_id_be_c_model_descriptor_module_instance_analyze,
                                  error_arg_type_malloc_string, output_port->port_lvar->name,
                                  error_arg_type_malloc_string, module_instance->type,
                                  error_arg_type_malloc_string, module_instance->name,
                                  error_arg_type_none );
            }
            else // Only if we have a valid net in hand to drive
            {
                reference_add( &module_instance->effects, output_port->lvar->instance ); // Add the instance to what the module effects
                //SL_DEBUG( sl_debug_level_info, "port %s lvar %p instance %p port signal %p\n", output_port->port_name , output_port->lvar, output_port->lvar->instance, output_port->module_port_signal );
                if (output_port->module_port_instance) // Only if it is valid port to drive the net
                {
                    // Check to see if this only partially drives the net
                    if ( (output_port->lvar->index.type!=md_lvar_data_type_none) ||
                         (output_port->lvar->subscript_start.type!=md_lvar_data_type_none) )
                    {
                        output_port->lvar->instance->driven_in_parts = 1;
                    }

                    // Look for combinatorially-derived outputs on the instance driving a net
                    if (output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially)
                    {
                        // Add any combinatorial dependencies of the instance to the lvar's dependencies, as the lvar is combinatorial
                        reference_union_sets( module_instance->combinatorial_dependencies, &output_port->lvar->instance->dependencies );
                        output_port->lvar->instance->derived_combinatorially = 1;
                    }
                    output_port->lvar->instance->number_statements++;

                    // Look for clock-derived outputs on the instance driving a net
                    reference_set_iterate_start( &output_port->module_port_instance->reference.data.signal->data.output.clocks_derived_from, &iter );
                    while ((reference = reference_set_iterate(&iter))!=NULL)
                    {
                        // Map module definition clock port to instance clock
                        for (clock_port=module_instance->clocks; clock_port; clock_port=clock_port->next_in_list)
                        {
                            if (clock_port->module_port_signal==reference->data.signal)
                            {
                                break;
                            }
                        }
                        // Add clock edge to the lvar's dependencies, as the lvar is clock-derived, and also to the module
                        if ((clock_port) && (clock_port->local_clock_signal))
                        {
                            reference_add( &output_port->lvar->instance->dependencies, clock_port->local_clock_signal, reference->edge  );
                            reference_add( &module_instance->dependencies, clock_port->local_clock_signal, reference->edge  );
                            SL_DEBUG( sl_debug_level_info, "output %s from clk %s %d", output_port->lvar->instance->name, reference->data.signal->name, reference->edge );
                        }
                    }
                }
            }
        }
    }
}

/*a Message methods
 */
/*f c_model_descriptor::message_create
 */
t_md_message *c_model_descriptor::message_create( t_md_module *module, const char *text, int copy_text, t_md_client_reference *client_ref )
{
     t_md_message *message;

     message = (t_md_message *)malloc(sizeof(t_md_message));
     if (!message)
     {
          error->add_error( NULL, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_message_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return NULL;
     }
     message->next_in_list = module->messages;
     module->messages = message;

     if (copy_text)
     {
          message->text = sl_str_alloc_copy( text );
          message->text_to_be_freed = 1;
     }
     else
     {
          message->text = (char *)text;
          message->text_to_be_freed = 0;
     }

     message->client_ref = *client_ref;
     message->arguments = NULL;

     return message;
}

/*f c_model_descriptor::message_create
 */
t_md_message *c_model_descriptor::message_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *text, int copy_text )
{
    t_md_message *message;
    t_md_client_reference client_ref;

    client_ref.base_handle = client_base_handle;
    client_ref.item_handle = client_item_handle;
    client_ref.item_reference = client_item_reference;

    message = message_create( module, text, copy_text, &client_ref );
    return message;
}

/*f c_model_descriptor::message_add_argument
 */
t_md_message *c_model_descriptor::message_add_argument( t_md_message *message, t_md_expression *expression )
{
    t_md_expression **chain;
    for (chain = &(message->arguments); (*chain); chain=&((*chain)->next_in_chain));
    *chain = expression;
    expression->next_in_chain = NULL;
    return message;
}

/*f c_model_descriptor::messages_free
 */
void c_model_descriptor::messages_free( t_md_message *list )
{
     t_md_message *next_message;

     while (list)
     {
          next_message = list->next_in_list; 
          if (list->text_to_be_freed)
               free(list->text);
          free(list);
          list = next_message;
     }
}

/*a Labelled expression methods
 */
/*f c_model_descriptor::labelled_expression_create
 */
t_md_labelled_expression *c_model_descriptor::labelled_expression_create( t_md_module *module, const char *text, int copy_text, t_md_client_reference *client_ref, t_md_expression *expression )
{
     t_md_labelled_expression *labelled_expression;

     labelled_expression = (t_md_labelled_expression *)malloc(sizeof(t_md_labelled_expression));
     if (!labelled_expression)
     {
         error->add_error( NULL, error_level_fatal, error_number_general_malloc_failed, error_id_be_c_model_descriptor_message_create,
                            error_arg_type_malloc_string, name,
                            error_arg_type_none );
          return NULL;
     }
     labelled_expression->next_in_list = module->labelled_expressions;
     module->labelled_expressions = labelled_expression;

     if (copy_text)
     {
          labelled_expression->text = sl_str_alloc_copy( text );
          labelled_expression->text_to_be_freed = 1;
     }
     else
     {
          labelled_expression->text = (char *)text;
          labelled_expression->text_to_be_freed = 0;
     }

     labelled_expression->client_ref = *client_ref;
     labelled_expression->expression = expression;
     labelled_expression->next_in_chain = NULL;

     return labelled_expression;
}

/*f c_model_descriptor::labelled_expression_create
 */
t_md_labelled_expression *c_model_descriptor::labelled_expression_create( t_md_module *module, void *client_base_handle, const void *client_item_handle, int client_item_reference, const char *text, int copy_text, t_md_expression *expression )
{
    t_md_labelled_expression *labelled_expression;
    t_md_client_reference client_ref;

    client_ref.base_handle = client_base_handle;
    client_ref.item_handle = client_item_handle;
    client_ref.item_reference = client_item_reference;

    labelled_expression = labelled_expression_create( module, text, copy_text, &client_ref, expression );
    return labelled_expression;
}

/*f c_model_descriptor::labelled_expression_append
 */
void c_model_descriptor::labelled_expression_append( t_md_labelled_expression **labelled_expression_list, t_md_labelled_expression *labelled_expression )
{
    while (*labelled_expression_list)  labelled_expression_list=&((*labelled_expression_list)->next_in_chain);
    if (labelled_expression_list)
    {
        *labelled_expression_list = labelled_expression;
        labelled_expression->next_in_chain = NULL;
    }
}

/*f c_model_descriptor::labelled_expressions_free
 */
void c_model_descriptor::labelled_expressions_free( t_md_labelled_expression *list )
{
    t_md_labelled_expression *next_labelled_expression;

    while (list)
    {
        next_labelled_expression = list->next_in_list;  // Ownership is through the next_in_list
        if (list->text_to_be_freed)
            free(list->text);
        free(list);
        list = next_labelled_expression;
    }
}

/*a Output generation methods
 */
/*f output_indented
 */
static void output_indented( void *handle, int indent, const char *format, ... )
{
     FILE *f;
     va_list ap;
     int i;

     va_start( ap, format );

     f = (FILE *)handle;
     if (indent>=0)
     {
          for (i=0; i<indent; i++)
               fprintf( f, indent_string );
     }
     vfprintf( f, format, ap );
     fflush(f);
     va_end( ap );
}

/*f c_model_descriptor::output_coverage_map
 */
void c_model_descriptor::output_coverage_map( const char *filename )
{
    t_md_module *module;
    FILE *f;
    int i, start_of_cut, end_of_cut, file_length, in_cut;
    char *file_buffer;
    t_md_statement *statement;
    char buffer[512];
    
    SL_DEBUG(sl_debug_level_info, "Output coverage map %s", filename);
    for (module=module_list; module; module=module->next_in_list)
    {
        if (module->external) continue; // No coverage fussing for external modules
        file_buffer = NULL;
        start_of_cut = -1;
        end_of_cut = -1;
        file_length = -1;
        in_cut = 0;
        if (sl_allocate_and_read_file( error, 1, filename, &file_buffer, "coverage_map" )==error_level_okay)
        {
            if (file_buffer)
            {
                for (i=0; file_buffer[i]; i+=(file_buffer[i])?1:0)
                {
                    if (!strncmp(file_buffer+i, "scope ", 6))
                    {
                        if (!strncmp(file_buffer+i+6, module->name, strlen(module->output_name) ))
                        {
                            if (file_buffer[i+6+strlen(module->output_name)]=='\n')
                            {
                                start_of_cut=i;
                                in_cut = 1;
                            }
                        }
                    }
                    if ((in_cut) && (!strncmp(file_buffer+i, "endscope\n", strlen("endscope\n"))))
                    {
                        end_of_cut = i+strlen("endscope\n");
                        in_cut = 0;
                    }
                    for (; (file_buffer[i]) && (file_buffer[i]!='\n'); i++);
                }
                file_length=i;
            }
        }
        printf("Coverage map file %s of length %d being cut from %d to %d for module %s\n", filename, file_length, start_of_cut, end_of_cut, module->output_name );

        f = fopen(filename, "w");
        if (!f)
        {
            fprintf(stderr, "Failed to open coverage map file '%s'\n", filename );
            return;
        }

        if (file_buffer)
        {
            if (start_of_cut>=0)
            {
                fwrite( file_buffer, 1, start_of_cut, f );
                if ((end_of_cut>0) && (file_length>end_of_cut))
                {
                    fwrite( file_buffer+end_of_cut, 1, file_length-end_of_cut, f );
                }
            }
            else
            {
                fwrite( file_buffer, 1, file_length, f );
            }
        }
        fprintf( f, "scope %s\n", module->output_name );
        for (statement=module->statements; statement; statement=statement->next_in_module)
        {
            if (statement->enumeration>=0)
            {
                client_string_from_reference_fn( client_handle,
                                                 statement->client_ref.base_handle,
                                                 statement->client_ref.item_handle,
                                                 statement->client_ref.item_reference,
                                                 buffer,
                                                 sizeof(buffer),
                                                 md_client_string_type_coverage );
                fprintf( f, "%d %s\n", statement->enumeration, buffer );
            }
        }
        fprintf( f, "endscope\n" );
        fclose(f);
    }
}

/*f c_model_descriptor::generate_output
 */
void c_model_descriptor::generate_output( t_sl_option_list env_options )
{
     const char *filename;
     FILE *f;
     int include_stmt_coverage, include_coverage, include_assertions;
     t_md_module *module;
     t_md_module_instance *mi;
     int i, j;
     const char *remap;

     /*b Remap module names and module instance types
      */
     for (module=module_list; module; module=module->next_in_list)
     {
         module->output_name = module->name;
         for (i=0; ; i++)
         {
             if (!sl_option_get_string( env_options, "be_remap_module_name", i, &remap))
             {
                 break;
             }
             for (j=0; remap[j] && (remap[j]!='='); j++);
             if (remap[j])
             {
                 if ( (!strncmp( module->name, remap, j)) &&
                      (module->name[j]==0) )
                 {
                     module->output_name = remap+j+1;
                     //fprintf(stderr, "Found module name '%s' for remapping instance type '%s', now '%s'\n", module->name, remap+j, module->output_name );
                 }
             }
         }
     }
     for (module=module_list; module; module=module->next_in_list)
     {
         for (mi=module->module_instances; mi; mi=mi->next_in_list)
         {
             if (mi->module_definition)
                 mi->output_type = mi->module_definition->output_name;
             else
                 mi->output_type = mi->type;
             //fprintf(stderr, "Module instance type %s is mapped to %s\n", mi->type, mi->output_type );
             for (i=0; ; i++)
             {
                 if (!sl_option_get_string( env_options, "be_remap_instance_type", i, &remap))
                 {
                     break;
                 }
                 for (j=0; remap[j] && (remap[j]!='.'); j++);
                 if (remap[j])
                 {
                     if ( (!strncmp(module->name, remap, j)) && (module->name[j]==0) )
                     {
                         //fprintf(stderr, "Found module name '%s' for remapping instance type '%s'\n", module->name, remap+j );
                         remap += j+1;
                         j = -1;
                     }
                 }
                 if (j<0)
                 {
                     for (j=0; remap[j] && (remap[j]!='='); j++);
                     if (remap[j])
                     {
                         if ( (!strncmp( mi->type, remap, j)) &&
                              (mi->type[j]==0) )
                         {
                             //fprintf(stderr, "Found module type '%s' for remapping instance type '%s'\n", mi->type, remap+j );
                             mi->output_type = remap+j+1;
                         }
                     }
                 }
             }
         }
     }

     include_coverage = 0;
     include_stmt_coverage = 0;
     include_assertions = 0;

     if (sl_option_get_string( env_options, "be_assertions" ))
     {
         include_assertions = 1;
     }

     if (sl_option_get_string( env_options, "be_stmt_coverage" ))
     {
         include_stmt_coverage = 1;
     }

     if (sl_option_get_string( env_options, "be_coverage" ))
     {
         include_coverage = 1;
     }

     filename = sl_option_get_string( env_options, "be_coverage_map" );
     if (filename)
     {
         if (include_coverage || include_stmt_coverage)
         {
             output_coverage_map( filename );
         }
     }

     filename = sl_option_get_string( env_options, "be_cppfile" );
     if (filename)
     {
          f = fopen(filename, "w");
          if (f)
          {
              target_c_output( this, output_indented, (void *)f, include_assertions, include_coverage, include_stmt_coverage );
              fclose(f);
          }
          else
          {
              error->add_error( NULL, error_level_fatal, error_number_general_bad_filename, error_id_be_c_model_descriptor_message_create,
                                error_arg_type_malloc_string, filename,
                                error_arg_type_none );
          }
     }

     filename = sl_option_get_string( env_options, "be_xmlfile" );
     fprintf(stderr,"xml file %s\n",filename);
     if (filename)
     {
          f = fopen(filename, "w");
          if (f)
          {
              target_xml_output( this, output_indented, (void *)f, include_assertions, include_coverage, include_stmt_coverage );
              fclose(f);
          }
          else
          {
              error->add_error( NULL, error_level_fatal, error_number_general_bad_filename, error_id_be_c_model_descriptor_message_create,
                                error_arg_type_malloc_string, filename,
                                error_arg_type_none );
          }
     }

     filename = sl_option_get_string( env_options, "be_vhdlfile" );
     if (filename)
     {
          f = fopen(filename, "w");
//          target_vhdl_output( this, output_indented, (void *)f );
          fclose(f);
     }

     filename = sl_option_get_string( env_options, "be_verilogfile" );
     if (filename)
     {
         t_md_verilog_options options;
         options.vmod_mode = (sl_option_get_string( env_options, "be_vmod" )!=NULL);
         options.clock_gate_module_instance_type         = sl_option_get_string( env_options, "be_v_clkgate_type" );
         options.clock_gate_module_instance_extra_ports  = sl_option_get_string( env_options, "be_v_clkgate_ports" );
         options.assert_delay_string                     = sl_option_get_string( env_options, "be_v_assert_delay" );
         options.verilog_comb_reg_suffix                 = sl_option_get_string( env_options, "be_v_comb_suffix" );
         options.include_displays                        = (sl_option_get_string( env_options, "be_v_displays" )!=NULL);
         options.include_assertions                      = (sl_option_get_string( env_options, "be_assertions" )!=NULL);
         options.include_coverage                        = (sl_option_get_string( env_options, "be_coverage" )!=NULL);
         f = fopen(filename, "w");
          if (f)
          {
              target_verilog_output( this, output_indented, (void *)f, &options );
              fclose(f);
          }
          else
          {
              error->add_error( NULL, error_level_fatal, error_number_general_bad_filename, error_id_be_c_model_descriptor_message_create,
                                error_arg_type_malloc_string, filename,
                                error_arg_type_none );
          }
     }
}

/*a External functions
 */
/*f be_getopt_usage
 */
extern void be_getopt_usage( void )
{
     printf( "\t--model \t\tRequired for C++ - model name that is used for naming the initialization functions\n");
     printf( "\t--cpp <file>\t\tGenerate C++ model output file\n");
     printf( "\t--verilog <file>\t\tGenerate verilog model output file\n");
     printf( "\t--include-assertions\tInclude assertions in C++ model\n");
     printf( "\t--include-coverage\tInclude code coverage statistics generation in C++ model\n");
     printf( "\t--coverage-desc-file <file>\tOutput coverage descriptor file\n");
     printf( "\t--remap-module-name <name=new_name>\tRemap module type 'name' to be another type 'new_name'\n");
     printf( "\t--remap-instance_type <module_name.instance_type=new_instance_type>\tRemap module instance types matching given type in given module to be another type 'new_name'\n");
}

/*f be_handle_getopt
  Handle the options for the backend from the command line using getopt
 */
extern int be_handle_getopt( t_sl_option_list *env_options, int c, const char *optarg )
{
     switch (c)
     {
     case option_be_model:
          *env_options = sl_option_list( *env_options, "be_model_name", optarg );
          return 1;
     case option_be_cpp:
          *env_options = sl_option_list( *env_options, "be_cppfile", optarg );
          return 1;
     case option_be_xml:
          *env_options = sl_option_list( *env_options, "be_xmlfile", optarg );
          return 1;
     case option_be_verilog:
          *env_options = sl_option_list( *env_options, "be_verilogfile", optarg );
          return 1;
     case option_be_vmod_mode:
          *env_options = sl_option_list( *env_options, "be_vmod", "yes" );
          return 1;
     case option_be_v_displays:
          *env_options = sl_option_list( *env_options, "be_v_displays", "yes" );
          return 1;
     case option_be_v_clkgate_type:
         *env_options = sl_option_list( *env_options, "be_v_clkgate_type", optarg );
         return 1;
     case option_be_v_clkgate_ports:
         *env_options = sl_option_list( *env_options, "be_v_clkgate_ports", optarg );
         return 1;
     case option_be_v_assert_delay:
         *env_options = sl_option_list( *env_options, "be_v_assert_delay", optarg );
         return 1;
     case option_be_v_comb_suffix:
         *env_options = sl_option_list( *env_options, "be_v_comb_suffix", optarg );
         return 1;
     case option_be_vhdl:
          *env_options = sl_option_list( *env_options, "be_vhdlfile", optarg );
          return 1;
     case option_be_include_coverage:
          *env_options = sl_option_list( *env_options, "be_coverage", "yes" );
          return 1;
     case option_be_include_stmt_coverage:
          *env_options = sl_option_list( *env_options, "be_stmt_coverage", "yes" );
          return 1;
     case option_be_include_assertions:
          *env_options = sl_option_list( *env_options, "be_assertions", "yes" );
          return 1;
     case option_be_coverage_desc_file:
          *env_options = sl_option_list( *env_options, "be_coverage_map", optarg );
          return 1;
     case option_be_remap_module_name:
          *env_options = sl_option_list( *env_options, "be_remap_module_name", optarg );
          return 1;
     case option_be_remap_instance_type:
          *env_options = sl_option_list( *env_options, "be_remap_instance_type", optarg );
          return 1;
     }
     return 0;
}

/*a To do
  add lsl, lsr
  add module instance dependency analysis to analyze
    instance depends on all things in input expression
    instance effects all lvars in outputs; lvars should also all be nets (checked elsewhere?)
      lvars are dependent on clocks if that module output depends on a clock;
      lvars are dependend on sets of inputs if that module output depends on sets of inputs;
 */

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


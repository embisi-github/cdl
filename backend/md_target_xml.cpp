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
Added
#ifdef INPUT_STORAGE

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


For sets of functions, we need a new simulation engine function call group for sets of submodule/clock handles:
submodule_clock_set_declare( int n )
submodule_clock_set_entry( int i, void *submodule_handle )
invoke_submodule_clock_set_function( submodule_clock_set, int *bitmask, function call type, int int_arg, void *handle_arg )
 type = prepreclock, preclock, clock, postclock, comb, propagate

These functions can then be split using pthreads by the simulation engine by providing an affinity, splitting, and doing a join

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
static const char *usage_type[] = { // Note this must match md_usage_type
    "rtl",
    "assert use only",
    "cover use only",
};

/*a Forward function declarations
 */
static void output_simulation_methods_statement( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance );
static void output_simulation_methods_code_block( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance );
static void output_simulation_methods_expression( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_expression *expr, int main_indent );

/*a Output functions
 */
/*f output_header
 */
static void output_header( c_model_descriptor *model, t_md_output_fn output, void *handle, int include_assertions, int include_coverage, int include_stmt_coverage )
{
    output( handle, 0, "<cdl asserts='%d' coverage='%d' stmt_coverage='%d'>\n", include_assertions, include_coverage, include_stmt_coverage );
}

/*f output_trailer
 */
static void output_trailer( c_model_descriptor *model, t_md_output_fn output, void *handle )
{
     output( handle, 0, "</cdl>\n");
}

/*f output_instance
 */
static void output_instance( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_type_instance *instance, int indent )
{
    output( handle, indent++, "<instance ref='%p' output_args_0='%d'>\n", instance, instance->output_args[0] );
    switch (instance->type)
    {
    case md_type_instance_type_bit_vector:
        output( handle, indent, "<bits name='%s' width='%d'/>\n", instance->output_name, instance->type_def.data.width);
        break;
    case md_type_instance_type_array_of_bit_vectors:
        output( handle, indent, "<vector name='%s' size='%d' width='%d'/>\n", instance->output_name, instance->size, instance->type_def.data.width );
        break;
    default:
        output( handle, indent, "<structure comment='NO TYPE FOR STRUCTURES'/>\n");
        break;
    }
    output( handle, --indent, "</instance>\n" );
}

/*f output_reference
 */
static void output_reference( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle, int indent, t_md_reference *reference )
{
    if (!reference)
    {
        output( handle, indent, "<null_reference/>\n" );
        return;
    }

    output( handle, indent++, "<reference>\n" );
    switch (reference->type)
    {
    case md_reference_type_instance:
        output_instance( model, output, handle, reference->data.instance, indent );
        break;
    case md_reference_type_state:
        output( handle, indent, "<state name='%s'/>\n", reference->data.state->name );
        break;
    case md_reference_type_signal:
        output( handle, indent, "<signal name='%s'/>\n", reference->data.signal->name );
        break;
    case md_reference_type_clock_edge:
        output( handle, indent, "<clock name='%s' edge='%d'/>\n", reference->data.signal->name, reference->edge );
        break;
    default:
        output( handle, indent, "<bad_reference_type type='%d'/>\n", reference->type );
        break;
    }
    output( handle, --indent, "</reference>\n" );
}

/*f output_references
 */
static void output_references( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle, int indent, const char *tag, t_md_reference_set *set )
{
    t_md_reference_iter iter;
    t_md_reference *reference;

    output( handle, indent++, "<%s>\n", tag );

    model->reference_set_iterate_start( &set, &iter );
    while ((reference = model->reference_set_iterate(&iter))!=NULL)
    {
        output_reference( model, module, output, handle, indent, reference );
    }

    output( handle, --indent, "</%s>\n", tag );
}

/*f output_ports_nets_clocks
 */
static void output_ports_nets_clocks( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle, int include_coverage, int include_stmt_coverage )
{
    int indent;
    int edge;
    t_md_signal *clk;
    t_md_signal *signal;
    t_md_state *reg;
    int i;
    t_md_type_instance *instance;
    t_md_reference_iter iter;
    t_md_reference *reference;
    t_md_code_block *code_block;

    indent = 1;

    /*b Output input ports
     */ 
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output( handle, indent++, "<input ref='%p' used_combinatorially='%d'>\n", signal->instance_iter->children[i], signal->data.input.used_combinatorially );
            output_instance( model, output, handle, signal->instance_iter->children[i], indent );
            output_references( model, module, output, handle, indent, "dependents", signal->instance_iter->children[i]->dependents );
            for (int level=0; level<2; level++)
            {
                if (signal->data.input.levels_used_for_reset[level])
                {
                    output( handle, indent, "<reset level='%s'/>\n", level_name[level] );
                }
            }
            for (clk=module->clocks; clk; clk=clk->next_in_list)
            {
                if (!clk->data.clock.clock_ref) // Not a gated clock
                {
                    for (edge=0; edge<2; edge++)
                    {
                        for (i=0; i<signal->instance_iter->number_children; i++)
                        {
                            t_md_signal *clk2;
                            instance = signal->instance_iter->children[i];
                            for (clk2=module->clocks; clk2; clk2=clk2?clk2->next_in_list:NULL)
                            {
                                if ((clk2==clk) || (clk2->data.clock.root_clock_ref==clk))
                                {
                                    if (model->reference_set_includes( &instance->dependents, clk2, edge ))
                                    {
                                        output( handle, indent, "<used_on_clock_edge name='%s' edge='%s'/>\n", clk->name, edge_name[edge] );
                                        clk2=NULL;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            output( handle, --indent, "</input>\n");
        }
    }

    /*b Output output ports
     */
    for (signal=module->outputs; signal; signal=signal->next_in_list)
    {
        if (signal->data.output.register_ref)
        {
            reg = signal->data.output.register_ref;
            for (i=0; i<reg->instance_iter->number_children; i++)
            {
                instance = reg->instance_iter->children[i];
                if (!reg->clock_ref->data.clock.clock_ref)
                {
                    output( handle, indent, "<output type='registered' ref='%p' name='%s' width='%d' clock='%s' edge='%s'/>\n",
                            instance,
                            instance->output_name,
                            instance->type_def.data.width,
                            reg->clock_ref->name,
                            edge_name[reg->edge] );
                }
                else
                {
                    output( handle, indent, "<output type='registered' ref='%p' name='%s' width='%d' clock='%s' edge='%s'/>\n",
                            instance,
                            instance->output_name,
                            instance->type_def.data.width,
                            reg->clock_ref->data.clock.clock_ref->name,
                            edge_name[reg->edge] );
                }
            }
        }
        if (signal->data.output.combinatorial_ref)
        {
            for (i=0; i<signal->data.output.combinatorial_ref->instance_iter->number_children; i++)
            {
                instance = signal->data.output.combinatorial_ref->instance_iter->children[i];
                output( handle, indent++, "<output type='comb' ref='%p' name='%s' width='%d' combinatorial_on_input='%d'>\n",
                        instance,
                        instance->output_name,
                        instance->type_def.data.width,
                        signal->data.output.derived_combinatorially );
                output_references( model, module, output, handle, indent, "dependencies", instance->dependencies );
                output_references( model, module, output, handle, indent, "base_dependencies", instance->base_dependencies );
                output_references( model, module, output, handle, indent, "dependents", instance->dependents );
                output( handle, --indent, "</output>\n" );
            }
        }
        if (signal->data.output.net_ref)
        {
            for (i=0; i<signal->data.output.net_ref->instance_iter->number_children; i++)
            {
                instance = signal->data.output.net_ref->instance_iter->children[i];
                output( handle, indent++, "<output type='net' ref='%p' name='%s' width='%d' combinatorial_on_input='%d'>\n",
                        instance,
                        instance->output_name,
                        instance->type_def.data.width,
                        signal->data.output.derived_combinatorially );

                output_references( model, module, output, handle, indent, "dependencies", instance->dependencies );
                output_references( model, module, output, handle, indent, "base_dependencies", instance->base_dependencies );
                output_references( model, module, output, handle, indent, "dependents", instance->dependents );
                output( handle, --indent, "</output>\n" );

                model->reference_set_iterate_start( &signal->data.output.clocks_derived_from, &iter ); // For every clock that the prototype says the output is derived from, map back to clock name, go to top of clock gate tree, and say that generates it
                while ((reference = model->reference_set_iterate(&iter))!=NULL)
                {
                    t_md_signal *clock;
                    clock = reference->data.signal;
                    while (clock->data.clock.clock_ref) { clock=clock->data.clock.clock_ref; }
                    output( handle, indent, "<!-- engine->register_output_generated_on_clock( engine_handle, \"%s\", \"%s\", %d );-->\n", instance->output_name, clock->name, reference->edge==md_edge_pos);
                }
            }
        }
    }
    output( handle, 0, "\n");

    /*b Output nets
     */ 
    for (signal=module->nets; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output( handle, indent++, "<net ref='%p' array_driven_in_parts='%d' vector_driven_in_parts='%d' derived_combinatorially='%d'>\n",
                    signal->instance_iter->children[i],
                    signal->instance_iter->children[i]->array_driven_in_parts,
                    signal->instance_iter->children[i]->vector_driven_in_parts,
                    signal->instance_iter->children[i]->derived_combinatorially );
            output_instance( model, output, handle, signal->instance_iter->children[i], indent );
            output_references( model, module, output, handle, indent, "dependencies", signal->instance_iter->children[i]->dependencies );
            output_references( model, module, output, handle, indent, "base_dependencies", signal->instance_iter->children[i]->base_dependencies );
            output_references( model, module, output, handle, indent, "dependents", signal->instance_iter->children[i]->dependents );
            output( handle, --indent, "</net>\n");
        }
    }

    /*b Output clocks
     */ 
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        for (edge=0; edge<2; edge++)
        {
            if (clk->data.clock.edges_used[edge])
            {
                output( handle, indent++, "<clock name='%s' edge='%s' ref='%p'>\n", clk->name, edge_name[edge], clk->data.clock.clock_ref );
                for (code_block = module->code_blocks; code_block; code_block=code_block->next_in_list)
                {
                    output_simulation_methods_code_block( model, output, handle, code_block, indent, clk, edge, NULL );
                }

                if (clk->data.clock.dependencies[edge])
                {
                    output_references( model, module, output, handle, indent, "dependencies_from_module_inputs", clk->data.clock.dependencies[edge] ); // All the signals/states/etc that effect the clock due to input ports on module instances the clock is used for
                }

                output( handle, --indent, "</clock>\n");
            }
        }
    }
}

/*f output_submodules
 */
static void output_submodules( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle, int include_coverage, int include_stmt_coverage )
{
    int indent;
    t_md_signal *clk;
    t_md_module_instance *module_instance;

    indent = 1;
    if (!module->module_instances)
        return;

    output( handle, indent++, "<submodules module='%s'>\n", module->output_name);
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        output( handle, indent++, "<submodule type='%s' name='%s'>\n", module_instance->output_type, module_instance->name );
        if (module_instance->module_definition)
        {
            t_md_module_instance_input_port *input_port;
            t_md_module_instance_output_port *output_port;

            for (clk=module_instance->module_definition->clocks; clk; clk=clk->next_in_list)
            {
                output( handle, indent, "<clock name='%s'/>\n", clk->name );
            }

            for (input_port=module_instance->inputs; input_port; input_port=input_port->next_in_list)
            {
                output( handle, indent, "<input name='%s' width='%d' used_combinatorially='%d'/>\n",
                        input_port->module_port_instance->output_name,
                        input_port->module_port_instance->type_def.data.width,
                        input_port->module_port_instance->reference.data.signal->data.input.used_combinatorially );
            }
            for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
            {
                // output_port->lvar is the lvar in this module driven by the instance; it must have constant subscripts to be here
                // output_port->module_port_instance is the leaf module port instance that drives the lvar - currently I believe this does not support subscripts
                output( handle, indent++, "<output name='%s' width='%d' used_combinatorially='%d'>\n",
                        output_port->module_port_instance->output_name,
                        output_port->module_port_instance->type_def.data.width,
                        output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially );

                output( handle, indent++, "<drives ref='%p' signal='%s' is_output='%d'>\n",
                        output_port->lvar->instance,
                        output_port->lvar->instance->output_name,
                        (output_port->lvar->instance->reference.data.signal->data.net.output_ref!=NULL) );
                if (output_port->lvar->instance->array_driven_in_parts)
                {
                    switch (output_port->lvar->index.type)
                    {
                    case md_lvar_data_type_integer:
                        output( handle, indent, "<index value='%d'/>\n", output_port->lvar->index.data.integer );
                        break;
                    default:
                        output( handle, indent, "<!-- unexpected lvar index type '%d' -->\n", output_port->lvar->subscript_start.type );
                        break;
                    }
                }
                if (output_port->lvar->instance->vector_driven_in_parts)
                {
                    switch (output_port->lvar->subscript_start.type)
                    {
                    case md_lvar_data_type_integer:
                        output( handle, indent, "<start value='%d'/>\n", output_port->lvar->subscript_start.data.integer );
                        if (output_port->lvar->subscript_length.type == md_lvar_data_type_none)
                            output( handle, indent, "<width value='%d'/>\n", output_port->lvar->subscript_length.data.integer );
                        break;
                    default:
                        output( handle, indent, "<!-- unexpected lvar subscript type '%d' -->\n", output_port->lvar->subscript_start.type );
                        break;
                    }
                }
                output( handle, --indent, "</drives>\n" );
                output( handle, --indent, "</output>\n" );
            }
            output_references( model, module, output, handle, indent, "dependencies", module_instance->dependencies );
            output_references( model, module, output, handle, indent, "combinatorial_dependencies", module_instance->combinatorial_dependencies );
        }
        output( handle, --indent, "</submodule>\n" );
    }
    output( handle, --indent, "</submodules>\n" );
}

/*f output_combinatorials
 */
static void output_combinatorials( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle, int include_coverage, int include_stmt_coverage )
{
    int indent;
    t_md_signal *signal;
    int i;

    indent = 1;

    /*b Output combinatorials storage type
     */ 
    for (signal=module->combinatorials; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output( handle, indent++, "<comb usage='%s'>\n", usage_type[signal->usage_type] );
            output_instance( model, output, handle, signal->instance_iter->children[i], indent );
            output_references( model, module, output, handle, indent, "dependencies", signal->instance_iter->children[i]->dependencies );
            output_references( model, module, output, handle, indent, "base_dependencies", signal->instance_iter->children[i]->base_dependencies );
            output_references( model, module, output, handle, indent, "dependents", signal->instance_iter->children[i]->dependents );
            output_simulation_methods_code_block( model, output, handle, signal->instance_iter->children[i]->code_block, indent, NULL, -1, signal->instance_iter->children[i] );
            output( handle, --indent, "</comb>\n");
        }
    }

}

/*f output_registers
 */
static void output_registers( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle, int include_coverage, int include_stmt_coverage )
{
    int indent;
    int edge;
    t_md_signal *clk;
    t_md_state *reg;
    int i;
    t_md_type_instance *instance;

    indent = 1;

    /*b Output clocked storage types
     */ 
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
                        for (i=0; i<reg->instance_iter->number_children; i++)
                        {
                            int j, array_size;
                            t_md_type_instance_data *ptr;

                            instance = reg->instance_iter->children[i];
                            array_size = instance->size;

                            output( handle, indent++, "<clocked edge='%s' clock='%s' usage='%s' reset='%s' reset_level='%s'>\n", edge_name[edge], clk->name, usage_type[reg->usage_type], reg->reset_ref->name, level_name[reg->reset_level] );
                            output_instance( model, output, handle, reg->instance_iter->children[i], indent );
                            output_references( model, module, output, handle, indent, "dependencies", reg->instance_iter->children[i]->dependencies );
                            output_references( model, module, output, handle, indent, "base_dependencies", reg->instance_iter->children[i]->base_dependencies );
                            output_references( model, module, output, handle, indent, "dependents", reg->instance_iter->children[i]->dependents );

                            for (j=0; j<(array_size>0?array_size:1); j++)
                            {
                                for (ptr = &instance->data[j]; ptr; ptr=ptr->reset_value.next_in_list)
                                {
                                    if (ptr->reset_value.expression)
                                    {
                                        if (ptr->reset_value.subscript_start>=0)
                                            output( handle, indent++, "<reset_value element='%d' subscript_start='%d'>\n", j, ptr->reset_value.subscript_start );
                                        else
                                            output( handle, indent++, "<reset_value element='%d'>\n", j );
                                        output_simulation_methods_expression( model, output, handle, NULL, ptr->reset_value.expression, indent );
                                        output( handle, --indent, "</reset_value>\n" );
                                    }
                                }
                            }
                            output( handle, --indent, "</clocked>\n");
                        }
                    }
                }
            }
        }
    }

}

/*f output_logging
 */
static void output_logging( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle )
{
    /*b Output logging
     */
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
                output( handle, 1, "<logging>\n" );
            }
            output( handle, 1, "<log_event number='%d', message='%d'>\n", j, stmt->data.log.message );
            for (j=0, arg=stmt->data.log.arguments; arg; arg=arg->next_in_chain, j++)
            {
                output( handle, 1, "<argument name='%s'/>\n", arg->text );
            }
            output( handle, 1, "</log_event>\n" );
            num_args+=j;
            i++;
        }
    }
    if (i!=0)
    {
        output( handle, 1, "</logging>\n" );
    }
}

/*f output_simulation_methods_lvar
  If in_expression is 0, then we are assigning so require 'next_state', and we don't insert the bit subscripting
  If in_expression is 1, then we can also add the bit subscripting
 */
static void output_simulation_methods_lvar( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_lvar *lvar, int indent, int sub_indent, int in_expression )
{
    output( handle, indent++, "<lvar id='%s' width='%d'>\n", lvar->instance->output_name, lvar->instance->type_def.data.width );
    if (lvar->index.type != md_lvar_data_type_none)
    {
        output( handle, indent++, "<index>\n" );
        switch (lvar->index.type)
        {
        case md_lvar_data_type_integer:
            output( handle, indent, "<value value='%d'/>\n", lvar->index.data.integer );
            break;
        case md_lvar_data_type_expression:
            output_simulation_methods_expression( model, output, handle, code_block, lvar->index.data.expression, indent );
            break;
        default:
            break;
        }
        output( handle, --indent, "</index>\n" );
    }
    if (lvar->subscript_start.type != md_lvar_data_type_none)
    {
        output( handle, indent++, "<subscript>\n" );
        if (lvar->subscript_start.type == md_lvar_data_type_integer)
        {
            output( handle, indent, "<start value='%d'/>\n", lvar->subscript_start.data.integer );
        }
        else if (lvar->subscript_start.type == md_lvar_data_type_expression)
        {
            output_simulation_methods_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, indent );
        }
        if (lvar->subscript_length.type != md_lvar_data_type_none)
        {
            output( handle, indent, "<width value='%d'/>\n", lvar->subscript_length.data.integer );
        }
        output( handle, --indent, "</subscript>\n" );
    }
    output( handle, --indent, "</lvar>\n" );
}

/*f output_simulation_methods_expression
 */
static void output_simulation_methods_expression( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_expression *expr, int indent )
{
    /*b If the expression is NULL then something went wrong...
     */
    if (!expr)
    {
        output( handle, indent, "<bad_null_expression/>\n" );
        return;
    }

    /*b Output code for the expression
     */
    switch (expr->type)
    {
    case md_expr_type_value:
        output( handle, indent, "<value width='%d' value='0x%llx'/>\n", expr->width, expr->data.value.value.value[0] );
        break;
    case md_expr_type_lvar:
        output_simulation_methods_lvar( model, output, handle, code_block, expr->data.lvar, indent, indent, 1 );
        break;
    case md_expr_type_bundle:
        output( handle, indent++, "<bundle width='%d'>\n", expr->width );
        output_simulation_methods_expression( model, output, handle, code_block, expr->data.bundle.a, indent );
        output_simulation_methods_expression( model, output, handle, code_block, expr->data.bundle.b, indent);
        output( handle, --indent, "</bundle>\n" );
        break;
    case md_expr_type_cast:
        if (expr->width==expr->data.cast.expression->width)
        {
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.cast.expression, indent);
        }
        else if ( (expr->data.cast.expression) &&
             (expr->data.cast.expression->width < expr->width) &&
             (expr->data.cast.signed_cast) )
        {
            output( handle, -1, "(SIGNED LENGTHENING CASTS NOT IMPLEMENTED YET: %d to %d)", expr->data.cast.expression->width, expr->width );
        }
        else
        {
            output( handle, indent++, "<cast from='%d' to='%d'>\n", expr->data.cast.expression->width, expr->width  );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.cast.expression, indent);
            output( handle, --indent, "</cast>\n" );
        }
        break;
    case md_expr_type_fn:
        switch (expr->data.fn.fn)
        {
        case md_expr_fn_logical_not:
        case md_expr_fn_not:
        case md_expr_fn_neg:
        {
            const char *type_string;
            if (expr->data.fn.fn==md_expr_fn_neg)
                type_string = "unary_minus";
            if (expr->data.fn.fn==md_expr_fn_not)
                type_string = "not";
            else
                type_string = "lnot";

            output( handle, indent++, "<fn width='%d' type='%s'>\n", expr->width, type_string );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[0], indent);
            output( handle, --indent, "</fn>\n" );
            break;
        }
        case md_expr_fn_add:
        case md_expr_fn_sub:
        case md_expr_fn_mult:
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
        {
            const char *type_string;
            if (expr->data.fn.fn==md_expr_fn_add)
                type_string = "add";
            else if (expr->data.fn.fn==md_expr_fn_sub)
                type_string = "sub";
            else if (expr->data.fn.fn==md_expr_fn_mult)
                type_string = "mult";
            else if (expr->data.fn.fn==md_expr_fn_and)
                type_string = "and";
            else if (expr->data.fn.fn==md_expr_fn_or)
                type_string = "or";
            else if (expr->data.fn.fn==md_expr_fn_xor)
                type_string = "xor";
            else if (expr->data.fn.fn==md_expr_fn_bic)
                type_string = "bic";
            else if (expr->data.fn.fn==md_expr_fn_logical_and)
                type_string = "land";
            else if (expr->data.fn.fn==md_expr_fn_logical_or)
                type_string = "lor";
            else if (expr->data.fn.fn==md_expr_fn_eq)
                type_string = "eq";
            else if (expr->data.fn.fn==md_expr_fn_ge)
                type_string = "ge";
            else if (expr->data.fn.fn==md_expr_fn_le)
                type_string = "le";
            else if (expr->data.fn.fn==md_expr_fn_gt)
                type_string = "gt";
            else if (expr->data.fn.fn==md_expr_fn_lt)
                type_string = "lt";
            else
                type_string = "neq";

            output( handle, indent++, "<fn width='%d' type='%s'>\n", expr->width, type_string );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[0], indent);
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[1], indent);
            output( handle, --indent, "</fn>\n" );
            break;
        }
        case md_expr_fn_query:
            output( handle, -1, "(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[0], indent);
            output( handle, -1, ")?(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[1], indent);
            output( handle, -1, "):(" );
            output_simulation_methods_expression( model, output, handle, code_block, expr->data.fn.args[2], indent);
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
    output( handle, indent++, "<if>\n");
    output( handle, indent++, "<predicate>\n");
    output_simulation_methods_expression( model, output, handle, code_block, statement->data.if_else.expr, indent+1 );
    output( handle, --indent, "</predicate>\n");
    if  (statement->data.if_else.if_true)
    {
        output( handle, indent++, "<if_true>\n");
        output_simulation_methods_statement( model, output, handle, code_block, statement->data.if_else.if_true, indent+1, clock, edge, instance );
        output( handle, --indent, "</if_true>\n");
    }
    if  (statement->data.if_else.if_false)
    {
        output( handle, indent++, "<if_false>\n");
        output_simulation_methods_statement( model, output, handle, code_block, statement->data.if_else.if_false, indent+1, clock, edge, instance );
        output( handle, --indent, "</if_false>\n");
    }
    output( handle, --indent, "</if>\n");
}

/*f output_simulation_methods_statement_parallel_switch
 */
static void output_simulation_methods_statement_parallel_switch( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance )
{
    if (statement->data.switch_stmt.all_static && statement->data.switch_stmt.all_unmasked)
    {
        t_md_switch_item *switem;

        output( handle, indent++, "<parallel_switch>\n");
        output( handle, indent++, "<expression>\n");
        output_simulation_methods_expression( model, output, handle, code_block, statement->data.switch_stmt.expr, indent+1 );
        output( handle, --indent, "</expression>\n");
        for (switem = statement->data.switch_stmt.items; switem; switem=switem->next_in_list)
        {
            int stmts_reqd;
            stmts_reqd = 1;
            if (switem->statement)
            {
                if (clock && !model->reference_set_includes( &switem->statement->effects, clock, edge ))
                    stmts_reqd = 0;
                if (instance && !model->reference_set_includes( &switem->statement->effects, instance ))
                    stmts_reqd = 0;
            }

            if (switem->type == md_switch_item_type_static)
            {
                if (stmts_reqd || statement->data.switch_stmt.full || 1)
                {
                    output( handle, indent++, "<case value='%d' required='%d'>\n", switem->data.value.value.value[0], stmts_reqd );
                    if (switem->statement)
                        output_simulation_methods_statement( model, output, handle, code_block, switem->statement, indent+1, clock, edge, instance );
                    output( handle, --indent, "</case>\n" );
                }
            }
            else if (switem->type == md_switch_item_type_default)
            {
                if (stmts_reqd || statement->data.switch_stmt.full)
                {
                    output( handle, indent++, "<default_case>\n" );
                    if (switem->statement)
                        output_simulation_methods_statement( model, output, handle, code_block, switem->statement, indent+1, clock, edge, instance );
                    output( handle, --indent, "</default_case>\n" );
                }
            }
            else
            {
                fprintf( stderr, "BUG - non static unmasked case item in parallel static unmasked switch C output\n" );
            }
        }
    }
    output( handle, --indent, "</parallel_switch>\n");
}

/*f output_simulation_methods_assignment
 */
static void output_simulation_methods_assignment( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, int indent, t_md_lvar *lvar, int clocked, t_md_expression *expr )
{
    output( handle, indent++, "<assign>\n" );
    switch (lvar->instance->type)
    {
    case md_type_instance_type_bit_vector:
    case md_type_instance_type_array_of_bit_vectors:
        output_simulation_methods_lvar( model, output, handle, code_block, lvar, indent, indent, 0 );
        output( handle, indent++, "<expression>\n" );
        output_simulation_methods_expression( model, output, handle, code_block, expr, indent+1 );
        output( handle, --indent, "</expression>\n" );
        break;
    case md_type_instance_type_structure:
        if (lvar->subscript_start.type != md_lvar_data_type_none)
        {
            output( handle, indent, "SUBSCRIPT INTO ASSIGNMENT DOES NOT WORK\n" );
        }
        output( handle, indent, "STRUCTURE ASSIGNMENT %p NOT IMPLEMENTED YET\n", lvar );
        break;
    }
    output( handle, --indent, "</assign>\n" );
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
            output_simulation_methods_expression( model, output, handle, code_block, expr, indent+2 );
            output( handle, -1, "\n" );
        }
        output( handle, indent+2, " );\n");
    }
}

/*f output_simulation_methods_statement_print_assert_cover
 */
static void output_simulation_methods_statement_print_assert_cover( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance )
{
    return;
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
            output_simulation_methods_expression( model, output, handle, code_block, statement->data.print_assert_cover.expression, indent+2 );
            if (expr)
            {
                output( handle, -1, "==(" );
                output_simulation_methods_expression( model, output, handle, code_block, expr, indent+2 );
                output( handle, -1, ")\n" );
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
                output_simulation_methods_expression( model, output, handle, code_block, statement->data.print_assert_cover.expression, indent+2 );
                output( handle, -1, ")==(" );
                output_simulation_methods_expression( model, output, handle, code_block, expr, indent+2 );
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
            output_simulation_methods_expression( model, output, handle, code_block, statement->data.print_assert_cover.expression, indent+2 );
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

    output( handle, indent++, "<log_event id='%s'>\n", statement->data.log.message ); 
    /*b Set data for the occurrence from expressions, then hit the event
    */
    for (arg=statement->data.log.arguments, i=0; arg; arg=arg->next_in_chain, i++)
    {
        if (arg->expression)
        {
            output( handle, indent++, "<log_data arg='%s'>\n", arg->text );
            output_simulation_methods_expression( model, output, handle, code_block, arg->expression, indent );
            output( handle, --indent, "</log_data>\n" );
        }
    }
    /*b Invoke event occurrence
    */
    output( handle, --indent, "</log_event>\n" );
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

    /*b Display the statement
     */
    switch (statement->type)
    {
    case md_statement_type_state_assign:
    {
        output_simulation_methods_assignment( model, output, handle, code_block, indent, statement->data.state_assign.lvar, 1, statement->data.state_assign.expr );
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

     /*b Display the statement
      */
     switch (statement->type)
     {
     case md_statement_type_state_assign:
          break;
     case md_statement_type_comb_assign:
        output_simulation_methods_assignment( model, output, handle, code_block, indent, statement->data.comb_assign.lvar, 0, statement->data.comb_assign.expr );
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
          output( handle, indent, "<statement type='Unknown statement type %d'/>\n", statement->type );
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
static void output_simulation_methods_code_block( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, int indent, t_md_signal *clock, int edge, t_md_type_instance *instance )
{
    if (!code_block)
    {
        fprintf(stderr,"No code block\n");
        return ;
    }
     /*b If the code block does not effect this signal/clock/edge, then return with outputting nothing
      */
     if (clock && !model->reference_set_includes( &code_block->effects, clock, edge ))
          return;
     if (instance && !model->reference_set_includes( &code_block->effects, instance ))
          return;

     /*b Insert comment for the block
      */
     if (instance)
          output( handle, indent++, "<code_block instance='%s' name='%s'>\n", instance->name, code_block->name );
     else
          output( handle, indent++, "<code_block name='%s'>\n", code_block->name );

     /*b Display each statement that effects state on the specified clock
      */
     output_simulation_methods_statement( model, output, handle, code_block, code_block->first_statement, indent, clock, edge, instance );

     /*b Close out with a blank line
      */
     output( handle, --indent, "</code_block>\n" );
}

/*a External functions
 */
/*f target_xml_output
 */
extern void target_xml_output( c_model_descriptor *model, t_md_output_fn output_fn, void *output_handle, int include_assertions, int include_coverage, int include_stmt_coverage )
{
    t_md_module *module;

    output_header( model, output_fn, output_handle, include_assertions, include_coverage, include_stmt_coverage );
    for (module=model->module_list; module; module=module->next_in_list)
    {
        output_fn( output_handle, 0, "<module name='%s' external='%d' comb_in_to_out='%d'>\n", module->output_name, module->external, module->combinatorial_component );
        // module->documentation
        if (!module->external)
        {
#if 0
    output_markers_mask_all( model, module, 0, -1 );
    output_markers_mask_clock_edge_dependents( model, module, NULL, 0, 1, 0 );
    output_markers_mask_output_dependencies( model, module, 2, 0 );
    output_markers_mask_input_dependents( model, module, 4, 0 );
    output_markers_mask_all_matching( model, module, 7, 3,   8, -1,   0, 0 ); // Everything marked as '3' -> 8, everything else stays same current
#elif 0
    output_markers_mask_all( model, module, 0, -1 );
    output_markers_mask_clock_edge_dependents( model, module, NULL, 0, 0x10, 0 );
    output_markers_mask_output_dependencies( model, module, 0x20, 0 );
    output_markers_mask_all_matching( model, module, 0x30, 0x30,   3, 0,   0, 0 ); // Everything marked as '3' must be valid; others 0
#elif 1
    output_markers_mask_all( model, module, 1, -1 );
    output_markers_mask_modules( model, module, 0, -1 );
    output_markers_mask_input_dependents( model, module, 2, -1 );
    output_markers_mask_clock_edge_dependents( model, module, NULL, 0, 4, -1 );// All clock edges
#elif 0
    output_markers_mask_all( model, module, 0, -1 );
    output_markers_mask_input_dependents( model, module, 4, 0 );
#endif
            output_ports_nets_clocks( model, module, output_fn, output_handle, include_coverage, include_stmt_coverage );
            output_submodules( model, module, output_fn, output_handle, include_coverage, include_stmt_coverage );
            output_combinatorials( model, module, output_fn, output_handle, include_coverage, include_stmt_coverage );
            output_registers( model, module, output_fn, output_handle, include_coverage, include_stmt_coverage );
            output_logging( model, module, output_fn, output_handle );
        }
        output_fn( output_handle, 0, "</module>\n" );
    }
    output_trailer( model, output_fn, output_handle );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


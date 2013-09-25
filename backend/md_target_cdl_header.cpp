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
#include "md_target_cdl_header.h"
#include "md_output_markers.h"
#include "c_model_descriptor.h"

/*a Static variables
 */

/*a Forward function declarations
 */

/*a Output functions
 */
/*f output_ports_nets_clocks
 */
static const char *string_type_name( t_md_type_instance *instance, const char *name )
{
    static char buffer[256];
    buffer[0] = 0;
    if (instance->type==md_type_instance_type_structure)
    {
        sprintf(buffer, "%s %s", instance->type_def.data.type_def->name, name);
    }
    else
    {
        if (instance->type_def.data.width>=2)
        {
            sprintf(buffer, "bit [%2d] %s", instance->type_def.data.width, name);
        }
        else
        {
            sprintf(buffer, "bit     %s", name);
        }
    }
    return buffer;
}
static void output_ports_nets_clocks( c_model_descriptor *model, t_md_module *module, t_md_output_fn output, void *handle, int include_all_clocks )
{
    int indent;
    t_md_signal *clk;
    t_md_signal *signal;
    int comma_newline;

    indent=0;
    comma_newline = 0;
    output( handle, indent, "/*m Module %s (from %s)\n", module->name, module->external?"CDL extern":"CDL module definition" );
    output( handle, indent, "*/\n" );
    output( handle, indent, "extern module %s(\n", module->name );

    /*b Output clocks
     */ 
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        if (include_all_clocks || (!clk->data.clock.clock_ref)) // Not a gated clock, i.e. a root clock
        {
            if (include_all_clocks || clk->data.clock.edges_used[0] || clk->data.clock.edges_used[1])
            {
                if (comma_newline) {output( handle, -1, ",\n" );}
                output( handle, indent, "clock %s", clk->name );
                comma_newline = 1;
            }
        }
    }

    /*b Output input ports
     */ 
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        if (comma_newline) {output( handle, -1, ",\n" );}
        output( handle, indent, "input %s", string_type_name(signal->instance,signal->name));
        comma_newline = 1;
    }

    /*b Output output ports
     */
    for (signal=module->outputs; signal; signal=signal->next_in_list)
    {
        /*b Careful with output, make sure to account for last line before the closing paren.
         *  otherwise, there will be a dangling comma.
         */
        if (comma_newline) {output( handle, -1, ",\n" );}
        output( handle, indent, "output %s", string_type_name(signal->instance,signal->name) );
        comma_newline = 1;
    }

    /*b Finish header, start body
     */
    output( handle, -1, "\n");
    output( handle, indent, ")\n" );
    output( handle, 0, "{\n");
    indent++;

    /*b Output combinatorial inputs/output ports
     */ 
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        if (signal->data.input.used_combinatorially)
        {
            output( handle, indent, "timing comb input %s;\n", signal->name);
        }
    }
    for (signal=module->outputs; signal; signal=signal->next_in_list)
    {
        if (signal->data.output.derived_combinatorially )
        {
            output( handle, indent, "timing comb output %s;\n", signal->name);
        }
    }

    /*b Output timings to/from clocks for external modules
     */ 
    if (module->external)
    {
        for (clk=module->clocks; clk; clk=clk->next_in_list)
        {
            t_md_reference_iter iter;
            t_md_reference *reference;
            for (signal=module->inputs; signal; signal=signal->next_in_list)
            {
                model->reference_set_iterate_start( &signal->data.input.clocks_used_on, &iter );
                while ((reference = model->reference_set_iterate(&iter))!=NULL)
                {
                    t_md_signal *clk2;
                    clk2 = reference->data.signal;
                    if (clk2 == clk)
                    {
                        output( handle, indent, "timing to %s clock %s %s;\n", (reference->edge)?"falling":"rising", clk->name, signal->name);
                        break;
                    }
                }
            }
            for (signal=module->outputs; signal; signal=signal->next_in_list)
            {
                model->reference_set_iterate_start( &signal->data.output.clocks_derived_from, &iter );
                while ((reference = model->reference_set_iterate(&iter))!=NULL)
                {
                    t_md_signal *clk2;
                    clk2 = reference->data.signal;
                    if (clk2 == clk)
                    {
                        output( handle, indent, "timing from %s clock %s %s;\n", (reference->edge)?"falling":"rising", clk->name, signal->name);
                        break;
                    }
                }
            }
        }
    }

    /*b Output timings to/from clocks for non-external modules
     */ 
    if (!module->external)
    {
        t_md_signal *clk;
        int edge;
        for (clk=module->clocks; clk; clk=clk->next_in_list)
        {
            if (!clk->data.clock.clock_ref) // Not a gated clock, i.e. a root clock
            {
                for (edge=0; edge<2; edge++)
                {
                    int used_on_clock_edge = 0;
                    int derived_from_clock_edge = 0;
                    t_md_type_instance *instance;

                    /*b Check all inputs for the clock and edge
                     */
                    for (signal=module->inputs; signal; signal=signal->next_in_list)
                    {
                        used_on_clock_edge = 0;
                        for (int i=0; i<signal->instance_iter->number_children; i++)
                        {
                            t_md_signal *clk2;
                            instance = signal->instance_iter->children[i];
                            for (clk2=module->clocks; clk2; clk2=clk2?clk2->next_in_list:NULL)
                            {
                                if ((clk2==clk) || (clk2->data.clock.root_clock_ref==clk))
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
                            output( handle, indent, "timing to %s clock %s %s;\n", (edge==md_edge_pos)?"rising":"falling", clk->name, signal->name);
                        }
                    }

                    /*b Check all outputs for the clock and edge
                     */
                    for (signal=module->outputs; signal; signal=signal->next_in_list)
                    {
                        derived_from_clock_edge = 0;
                        if (signal->data.output.register_ref)
                        {
                            t_md_state *reg;
                            reg = signal->data.output.register_ref;
                            if ((reg->clock_ref->data.clock.root_clock_ref == clk) && (reg->edge==edge))
                            {
                                for (int i=0; i<reg->instance_iter->number_children; i++)
                                {
                                    derived_from_clock_edge = 1;
                                    break;
                                }
                            }
                        }
                        if (signal->data.output.combinatorial_ref)
                        {
                            for (int i=0; i<signal->data.output.combinatorial_ref->instance_iter->number_children; i++)
                            {
                                t_md_reference_iter iter;
                                t_md_reference *reference;
                                instance = signal->data.output.combinatorial_ref->instance_iter->children[i];
                                model->reference_set_iterate_start( &signal->data.output.clocks_derived_from, &iter ); // For every clock that the prototype says the output is derived from, map back to clock name, go to top of clock gate tree, and say that generates it
                                while ((reference = model->reference_set_iterate(&iter))!=NULL)
                                {
                                    t_md_signal *clk2;
                                    clk2 = reference->data.signal;
                                    if ((clk2->data.clock.root_clock_ref==clk) && (edge==reference->edge))
                                    {
                                        derived_from_clock_edge=1;
                                        break;
                                    }
                                }
                            }
                        }
                        if (signal->data.output.net_ref)
                        {
                            for (int i=0; i<signal->data.output.net_ref->instance_iter->number_children; i++)
                            {
                                t_md_reference_iter iter;
                                t_md_reference *reference;
                                instance = signal->data.output.net_ref->instance_iter->children[i];
                                model->reference_set_iterate_start( &signal->data.output.clocks_derived_from, &iter ); // For every clock that the prototype says the output is derived from, map back to clock name, go to top of clock gate tree, and say that generates it
                                while ((reference = model->reference_set_iterate(&iter))!=NULL)
                                {
                                    t_md_signal *clk2;
                                    clk2 = reference->data.signal;
                                    if ((clk2->data.clock.root_clock_ref==clk) && (edge==reference->edge))
                                    {
                                        derived_from_clock_edge = 1;
                                        break;
                                    }
                                }
                            }
                        }
                        if (derived_from_clock_edge)
                        {
                            output( handle, indent, "timing from %s clock %s %s;\n", (edge==md_edge_pos)?"rising":"falling", clk->name, signal->name);
                        }
                    }
                }
            }
        }
    }

    /*b End body
     */
    indent--;
    output( handle, 0, "}\n");

    /*b All done
     */ 
}

/*a External functions
 */
/*f target_cdl_header_output
 */
extern void target_cdl_header_output( c_model_descriptor *model, t_md_output_fn output_fn, void *output_handle, t_md_cdl_header_options *options )
{
    t_md_module *module;

    for (module=model->module_list; module; module=module->next_in_list)
    {
        output_ports_nets_clocks( model, module, output_fn, output_handle, module->external );
    }
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


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

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "be_errors.h"
#include "cdl_version.h"
#include "md_output_markers.h"
#include "c_model_descriptor.h"

/*a Static variables for debug only
 */
static int debug=0;

/*a Basic output markers masking and value functions
 */
/*f output_markers_mask (module instance clock port)
 */
extern void output_markers_mask( t_md_module_instance_clock_port *clock_port, int set_mask, int clr_mask )
{
    clock_port->output_args[0] = (clock_port->output_args[0] &~ clr_mask) | set_mask;
}

/*f output_markers_value (module instance clock port)
 */
extern int output_markers_value( t_md_module_instance_clock_port *clock_port, int mask )
{
    return clock_port->output_args[0] & mask;
}

/*f output_markers_mask (type instance)
 */
extern void output_markers_mask( t_md_type_instance *instance, int set_mask, int clr_mask )
{
    instance->output_args[0] = (instance->output_args[0] &~ clr_mask) | set_mask;
}

/*f output_markers_value (type instance)
 */
extern int output_markers_value( t_md_type_instance *instance, int mask )
{
    return instance->output_args[0] & mask;
}

/*f output_markers_mask_matching (type instance)
 */
extern void output_markers_mask_matching( t_md_type_instance *instance, int mask, int value, int set_mask_eq, int clr_mask_eq, int set_mask_ne, int clr_mask_ne )
{
    if (output_markers_value(instance,mask)==value)
    {
        output_markers_mask(instance,set_mask_eq,clr_mask_eq);
    }
    else
    {
        output_markers_mask(instance,set_mask_ne,clr_mask_ne);
    }
}

/*f output_markers_mask (module instance)
 */
extern void output_markers_mask( t_md_module_instance *instance, int set_mask, int clr_mask )
{
    instance->output_args[0] = (instance->output_args[0] &~ clr_mask) | set_mask;
}

/*f output_markers_value (module instance)
 */
extern int output_markers_value( t_md_module_instance *instance, int mask )
{
    return instance->output_args[0] & mask;
}

/*f output_markers_mask_matching (module instance)
 */
extern void output_markers_mask_matching( t_md_module_instance *instance, int mask, int value, int set_mask_eq, int clr_mask_eq, int set_mask_ne, int clr_mask_ne )
{
    if (output_markers_value(instance,mask)==value)
    {
        output_markers_mask(instance,set_mask_eq,clr_mask_eq);
    }
    else
    {
        output_markers_mask(instance,set_mask_ne,clr_mask_ne);
    }
}

/*a Masking of instance, signal and state dependents/dependencies
 */
/*f output_markers_mask_all
 */
extern void output_markers_mask_all( c_model_descriptor *model, t_md_module *module, int set_mask, int clr_mask )
{
    t_md_signal *signal;
    t_md_module_instance *module_instance;
    int i;

    /*b   Combinatorials
     */
    for (signal=module->combinatorials; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output_markers_mask( signal->instance_iter->children[i], set_mask, clr_mask );
        }
    }

    /*b   Nets
     */
    for (signal=module->nets; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output_markers_mask( signal->instance_iter->children[i], set_mask, clr_mask );
        }
    }

    /*b   Module instances
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        output_markers_mask( module_instance, set_mask, clr_mask );
    }

    /*b   Finish
     */
}

/*f output_markers_mask_all_matching
 */
extern void output_markers_mask_all_matching( c_model_descriptor *model, t_md_module *module, int mask, int value, int set_mask_eq, int clr_mask_eq, int set_mask_ne, int clr_mask_ne )
{
    t_md_signal *signal;
    t_md_module_instance *module_instance;
    t_md_state *state;
    int i;

    /*b   Combinatorials
     */
    for (signal=module->combinatorials; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output_markers_mask_matching( signal->instance_iter->children[i], mask, value, set_mask_eq, clr_mask_eq, set_mask_ne, clr_mask_ne );
        }
    }

    /*b   Nets
     */
    for (signal=module->nets; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output_markers_mask_matching( signal->instance_iter->children[i], mask, value, set_mask_eq, clr_mask_eq, set_mask_ne, clr_mask_ne );
        }
    }

    /*b   Module instances
     */
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        output_markers_mask_matching( module_instance, mask, value, set_mask_eq, clr_mask_eq, set_mask_ne, clr_mask_ne );
    }

    /*b    State
     */
    for (state=module->registers; state; state=state->next_in_list)
    {
        for (i=0; i<state->instance_iter->number_children; i++)
        {
            output_markers_mask_matching( state->instance_iter->children[i], mask, value, set_mask_eq, clr_mask_eq, set_mask_ne, clr_mask_ne );
        }
    }

    /*b   Finish
     */
}

/*f output_markers_mask_dependents (instance)
  For an instance of a signal, set the output markers of every dependency
 */
extern void output_markers_mask_dependents( c_model_descriptor *model, t_md_module *module, t_md_type_instance *instance, int set_mask, int clr_mask )
{
    t_md_reference_iter iter;
    t_md_reference *reference;

    /*b Mark this
     */
    if (debug)
        fprintf(stderr,"ommdepnt inst %p %s %d %d\n", instance, instance->name, set_mask, clr_mask );
    output_markers_mask( instance, set_mask, clr_mask );

    /*b Mark all dependents (net and combinatorial) are all marked
     */
    model->reference_set_iterate_start( &instance->dependents, &iter );
    while ((reference = model->reference_set_iterate(&iter))!=NULL)
    {
        if (reference->type==md_reference_type_instance)
        {
            t_md_type_instance *dependency = reference->data.instance;
            if (debug)
                fprintf(stderr,"ommdepnt dependent %p %s %d %d\n", dependency, dependency->name, set_mask, clr_mask );
            output_markers_mask( dependency, set_mask, clr_mask );
        }
    }

    /*b   Finish
     */
}

/*f output_markers_mask_dependencies (instance)
  For an instance of a signal, set the output markers of every dependency
 */
extern void output_markers_mask_dependencies( c_model_descriptor *model, t_md_module *module, t_md_type_instance *instance, int set_mask, int clr_mask )
{
    t_md_reference_iter iter;
    t_md_reference *reference;

    /*b Mark this instance
     */
    if (debug)
        fprintf(stderr,"ommdepncy inst %p %s %d %d\n", instance, instance->name, set_mask, clr_mask );
    output_markers_mask( instance, set_mask, clr_mask );

    /*b Check dependencies (net and combinatorial) are all marked
     */
    model->reference_set_iterate_start( &instance->dependencies, &iter );
    while ((reference = model->reference_set_iterate(&iter))!=NULL)
    {
        if (reference->type==md_reference_type_instance)
        {
            t_md_type_instance *dependency = reference->data.instance;
            if (dependency->reference.type==md_reference_type_signal)
            {
                if ( output_markers_value(dependency,(set_mask|clr_mask))!=set_mask )
                {
                    if (debug)
                        fprintf(stderr,"ommdepncy dep %p %s %d %d\n", dependency, dependency->name, set_mask, clr_mask );
                    output_markers_mask_dependencies( model, module, dependency, set_mask, clr_mask );
                }
            }
        }
    }

    /*b   Finish
     */
}

/*f output_markers_mask_dependents (signal)
  For a signal, set the output markers of every dependency of every instance (submember) of that signal
 */
extern void output_markers_mask_dependents( c_model_descriptor *model, t_md_module *module, t_md_signal *signal, int set_mask, int clr_mask )
{
    t_md_type_instance *instance;
    int i;

    /*b Check dependents (net and combinatorial) are all marked
     */
    for (i=0; i<signal->instance_iter->number_children; i++)
    {
        /*b If instance has been done, skip to next
         */
        instance = signal->instance_iter->children[i];
        output_markers_mask_dependents( model, module, instance, set_mask, clr_mask );
    }

    /*b   Finish
     */
}

/*f output_markers_mask_dependents (state)
  For a state, set the output markers of every dependency of every instance (submember) of that signal
 */
extern void output_markers_mask_dependents( c_model_descriptor *model, t_md_module *module, t_md_state *state, int set_mask, int clr_mask )
{
    t_md_type_instance *instance;
    int i;

    /*b Check dependents (net and combinatorial) are all marked
     */
    for (i=0; i<state->instance_iter->number_children; i++)
    {
        /*b If instance has been done, skip to next
         */
        instance = state->instance_iter->children[i];
        output_markers_mask_dependents( model, module, instance, set_mask, clr_mask );
    }

    /*b   Finish
     */
}

/*f output_markers_mask_dependencies (signal)
  For a signal, set the output markers of every dependency of every instance (submember) of that signal
 */
extern void output_markers_mask_dependencies( c_model_descriptor *model, t_md_module *module, t_md_signal *signal, int set_mask, int clr_mask )
{
    t_md_type_instance *instance;
    int i;

    /*b Check dependencies (net and combinatorial) are all marked
     */
    for (i=0; i<signal->instance_iter->number_children; i++)
    {
        /*b If instance has been done, skip to next
         */
        instance = signal->instance_iter->children[i];
        if (debug)
            fprintf(stderr,"ommdepncy sig %p %s %d %d\n", instance, instance->name, set_mask, clr_mask );
        output_markers_mask_dependencies( model, module, instance, set_mask, clr_mask );
    }

    /*b   Finish
     */
}

/*f output_markers_mask_dependencies (state)
  For a state, set the output markers of every dependency of every instance (submember) of that signal
 */
extern void output_markers_mask_dependencies( c_model_descriptor *model, t_md_module *module, t_md_state *state, int set_mask, int clr_mask )
{
    t_md_type_instance *instance;
    int i;

    /*b Check dependencies (net and combinatorial) are all marked
     */
    for (i=0; i<state->instance_iter->number_children; i++)
    {
        /*b If instance has been done, skip to next
         */
        instance = state->instance_iter->children[i];
        output_markers_mask_dependencies( model, module, instance, set_mask, clr_mask );
    }

    /*b   Finish
     */
}

/*a More complex masking of inputs, outputs,
 */
/*f output_markers_mask_input_dependents
  For every input of the module mark all dependents
  Note that the list of dependents for an input is EVERY dependent even a long way down
 */
extern void output_markers_mask_input_dependents( c_model_descriptor *model, t_md_module *module, int set_mask, int clr_mask )
{
    t_md_signal *signal;
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        output_markers_mask_dependents( model, module, signal, set_mask, clr_mask );
    }
}

/*f output_markers_mask_comb_output_dependencies
  For every combinatorial output of the module (including nets that are combinatorially dependent on inputs), set 'mask' for it and its dependencies
 */
extern void output_markers_mask_comb_output_dependencies( c_model_descriptor *model, t_md_module *module, int set_mask, int clr_mask )
{
    t_md_signal *signal;
    for (signal=module->outputs; signal; signal=signal->next_in_list)
    {
        if (!signal->data.output.derived_combinatorially)
            continue;
        if (signal->data.output.combinatorial_ref)
        {
            output_markers_mask_dependencies( model, module, signal->data.output.combinatorial_ref, set_mask, clr_mask );
        }
        if (signal->data.output.net_ref)
        {
            output_markers_mask_dependencies( model, module, signal->data.output.net_ref, set_mask, clr_mask );
        }
    }
}

/*f output_markers_mask_output_dependencies
  For every output of the module (including nets that are dependent on inputs), set 'mask' for it and its dependencies
 */
extern void output_markers_mask_output_dependencies( c_model_descriptor *model, t_md_module *module, int set_mask, int clr_mask )
{
    t_md_signal *signal;
    //debug=1;
    for (signal=module->outputs; signal; signal=signal->next_in_list)
    {
        if (signal->data.output.combinatorial_ref)
        {
            if (debug)
                fprintf(stderr,"omod comb %p %s %d %d\n", signal, signal->name, set_mask, clr_mask );
            output_markers_mask_dependencies( model, module, signal->data.output.combinatorial_ref, set_mask, clr_mask );
        }
        if (signal->data.output.net_ref)
        {
            if (debug)
                fprintf(stderr,"omod net %p %s %d %d\n", signal, signal->name, set_mask, clr_mask );
            output_markers_mask_dependencies( model, module, signal->data.output.net_ref, set_mask, clr_mask );
        }
        // Do not output register dependencies - these refer to the clock edge that led to this output
        //if (signal->data.output.register_ref)
        //{
        //    output_markers_mask_dependencies( model, module, signal->data.output.register_ref, set_mask, clr_mask );
        //}
    }
    //debug=0;
}

/*f output_markers_mask_modules
 */
extern void output_markers_mask_modules( c_model_descriptor *model, t_md_module *module, int set_mask, int clr_mask )
{
    t_md_module_instance *module_instance;
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        if (!module_instance->module_definition) 
            continue;

        output_markers_mask( module_instance, set_mask, clr_mask );
    }
}

/*f output_markers_mask_comb_modules_with_matching_outputs
  For every combinatorial module, check if its outputs match mask/value
  If any does, then mark the module
  This is particularly to mark modules as 'invalid, need to output' if any of the module's outputs is in invalid
 */
extern void output_markers_mask_comb_modules_with_matching_outputs( c_model_descriptor *model, t_md_module *module, int mask, int value, int set_mask, int clr_mask )
{
    t_md_module_instance *module_instance;
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        int need_to_mark;
        t_md_module_instance_output_port *output_port;

        if ( (!module_instance->module_definition) ||
             (!module_instance->module_definition->combinatorial_component) )
            continue;

        need_to_mark = 0;
        for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
            if (output_port->module_port_instance->reference.data.signal->data.output.derived_combinatorially)
                if (output_markers_value(output_port->lvar->instance, mask)==value)
                    need_to_mark = 1;

        if (need_to_mark)
            output_markers_mask( module_instance, set_mask, clr_mask );
    }
}

/*f output_markers_mask_clock_edge_dependencies
  For every state that changes on a given clock edge mark its dependencies
  These are signals that are required for the 'next state value' function for the state
 */
extern void output_markers_mask_clock_edge_dependencies( c_model_descriptor *model, t_md_module *module, t_md_signal *clock, int edge, int set_mask, int clr_mask )
{
    t_md_state *state;
    for (state=module->registers; state; state=state->next_in_list)
    {
        if ( (clock==NULL) || ((state->clock_ref==clock) && (state->edge==edge)) )
        {
            output_markers_mask_dependencies( model, module, state, set_mask, clr_mask );
        }
    }
}

/*f output_markers_mask_clock_edge_dependents
  For every state that changes on a given clock edge mark its dependents
  Also each instance driven by a clocked module
 */
extern void output_markers_mask_clock_edge_dependents( c_model_descriptor *model, t_md_module *module, t_md_signal *clock, int edge, int set_mask, int clr_mask )
{
    t_md_state *state;
    t_md_reference_iter iter;
    t_md_reference *reference;
    if (clock==NULL)
    {
        for (clock=module->clocks; clock; clock=clock->next_in_list)
        {
            output_markers_mask_clock_edge_dependents( model, module, clock, 0, set_mask, clr_mask );
            output_markers_mask_clock_edge_dependents( model, module, clock, 1, set_mask, clr_mask );
        }
        return;
    }

    model->reference_set_iterate_start( &clock->data.clock.dependents[edge], &iter );
    while ((reference = model->reference_set_iterate(&iter))!=NULL)
    {
        if (reference->type==md_reference_type_instance)
        {
            t_md_type_instance *dependent = reference->data.instance;
            output_markers_mask( dependent, set_mask, clr_mask );
        }
        else
        {
            fprintf(stderr,"Unknown ref type %d\n",reference->type);
        }
    }
}

/*a Complex output marker checking
 */
/*f output_markers_check_net_driven_in_parts_modules_all_match
 */
extern int output_markers_check_net_driven_in_parts_modules_all_match( t_md_module *module, t_md_type_instance *instance, int mask, int match )
{
    t_md_module_instance *module_instance;
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        t_md_module_instance_output_port *output_port;

        if ( !module_instance->module_definition ) 
            continue;

        for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
        {
            if ( output_port->lvar->instance != instance )
                continue;
            if (output_markers_value( module_instance, mask)==match)
                break; // Yay - a match for the module, 
            return 0; // Boo - this output port drives the instance but the module markers mismatch
        }
    }
    return 1;
}

/*f output_markers_find_iter_match
 */
static int output_markers_verbose=0;
extern t_md_type_instance *output_markers_find_iter_match( c_model_descriptor *model, t_md_reference_iter *iter, void *instance, int mask, int match )
{
    t_md_reference *reference;
    t_md_type_instance *dependency;

    if (output_markers_verbose)
    {
        fprintf(stderr, "omfim %p mask %d match %d\n",signal,mask,match);
    }
    while ((reference = model->reference_set_iterate(iter))!=NULL)
    {
        dependency = reference->data.instance;
            if (output_markers_verbose)
            {
                fprintf(stderr, "%s %d\n",dependency->output_name,output_markers_value(dependency,mask));
            }
        if ( (dependency->reference.type==md_reference_type_signal) &&
             (dependency!=instance) && // This should handle self-referntial instances
             //(dependency->reference.data.signal!=signal) && // We had this originally; cyclic dependencies should probably be flagged, but that is not what this was about
             // This was about the signal name in the source CDL having elements which are self-dependent
             // So a structure with elements 'X' and 'Y', where comb a.Y=inp_a; a.X=a.Y would have a 'self dependency' - which we DO want to handle
             // So that commented out bit, which apparently is there potentially for cyclic a.X=a.X, actually messes up a.Y=inp; a.X=a.Y
             // However, we do self-referential is valid; code must be taken in programmatic order
             // In this case
             // comb_deps.dependent[0]  = comb_deps.dependency[3] ;
             // comb_deps.dependent[1]  = !comb_deps.dependent[0] ;
             // comb_deps.dependent[2]  = !comb_deps.dependent[1] ;
             // comb_deps.dependent[3]  = !comb_deps.dependent[2] ;
             // is expected to work
             ( (dependency->reference.data.signal->type==md_signal_type_combinatorial) || (dependency->reference.data.signal->type==md_signal_type_net) ) )
        {
            if (output_markers_value(dependency,mask)==match)
                return dependency;
        }
    }
    return NULL;
}

/*f output_markers_check_iter_any_match
 */
extern int output_markers_check_iter_any_match( c_model_descriptor *model, t_md_reference_iter *iter, void *instance, int mask, int match )
{
    if (output_markers_find_iter_match( model, iter, instance, mask, match)!=NULL)
        return 1;
    return 0;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


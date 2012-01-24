/*a Copyright
  
  This file 'md_target_verilog.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a To do
  Add support for component declarations
  Add support for print statements
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c_model_descriptor.h"
#include "cdl_version.h"
#include "md_target_verilog.h"

/*a Defines
 */
#define INSTANCE_IS_BIT_VECTOR_ARRAY(instance) (((instance)->size>1) && ((instance)->type==md_type_instance_type_array_of_bit_vectors) && ((instance)->type_def.data.width>1))
#define CLOCK_IS_GATED(clk) ((clk)->data.clock.clock_ref)

/*a Types
 */
enum // max of MD_OUTPUT_MAX_SIGNAL_ARGS, which is now 8
{
    output_arg_state_has_reset,
    output_arg_use_comb_reg,
    output_arg_vmod_has_been_declared,
    output_arg_vmod_is_indexed_by_runtime
};

/*a Static variables
 */
static t_md_verilog_options options;
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
static int current_output_depth = 0;
static t_md_usage_type output_usage_type;
static t_md_usage_type desired_output_usage_type;

/*a Forward function declarations
 */
static void output_module_rtl_architecture_expression( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_expression *expr, int main_indent, int sub_indent, int id );
static void output_module_rtl_architecture_statement( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *signal, int edge, t_md_signal *reset, int reset_level );

/*a Output functions
 */
/*f output_header
 */
static void output_header( c_model_descriptor *model, t_md_output_fn output, void *handle )
{

    output( handle, 0, "//a Note: created by CDL " __CDL__VERSION_STRING " - do not hand edit without recognizing it will be out of sync with the source\n");
    output( handle, 0, "// Output mode %d (VMOD=1, standard verilog=0)\n", options.vmod_mode );
    output( handle, 0, "\n");

}

/*f output_set_usage_type - Output translate off/on if required to get desired output type
 */
static void output_set_usage_type( c_model_descriptor *model, t_md_output_fn output, void *handle )
{
    if ((output_usage_type==md_usage_type_rtl) && (desired_output_usage_type!=md_usage_type_rtl))
    {
        output( handle, 1, "//pragma coverage off\n");
        output( handle, 1, "//synopsys  translate_off\n");
        output_usage_type = desired_output_usage_type;
    }
    else if ((output_usage_type!=md_usage_type_rtl) && (desired_output_usage_type==md_usage_type_rtl))
    {
        output( handle, 1, "//synopsys  translate_on\n");
        output( handle, 1, "//pragma coverage on\n");
        output_usage_type = desired_output_usage_type;
    }
}

/*f output_push_usage_type - push a new usage type on the 'stack' - cannot enable output though if it is disabled
 */
static void output_push_usage_type( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_usage_type usage_type )
{
    desired_output_usage_type = usage_type;
    output_set_usage_type( model, output, handle );
    current_output_depth = current_output_depth+1;
}

/*f output_pop_usage_type
 */
static void output_pop_usage_type( c_model_descriptor *model, t_md_output_fn output, void *handle )
{
    if (current_output_depth==1)
    {
        desired_output_usage_type = md_usage_type_rtl;
    }
    current_output_depth = (current_output_depth==0)?0:(current_output_depth-1);
}

/*f output_module_rtl_architecture_ports; output clocks as inputs, inputs, outputs
 */
static void output_module_rtl_architecture_ports( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module )
{
    t_md_signal *clk;
    t_md_signal *signal;
    t_md_type_instance *instance;
    int i, last;

    output( handle, 0, "(\n" );
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        if (!CLOCK_IS_GATED(clk))
        {
            output( handle, 1, "%s,\n", clk->name );
        }
    }
    output( handle, 0, "\n");

    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            last = (i==signal->instance_iter->number_children-1) && (!signal->next_in_list) && (!module->outputs);
            instance = signal->instance_iter->children[i];
            output( handle, 1, "%s%s", instance->output_name, last?"\n":",\n" );
        }
    }
    output( handle, 0, "\n");

    for (signal=module->outputs; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            last = (i==signal->instance_iter->number_children-1) && (!signal->next_in_list);
            instance = signal->instance_iter->children[i];
            output( handle, 1, "%s%s", instance->output_name, last?"\n":",\n" );
        }
    }

    output( handle, 0, ");\n" );
    output( handle, 0, "\n" );

    output( handle, 1, "//b Clocks\n" );
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        if (!CLOCK_IS_GATED(clk))
        {
            t_md_signal *clk2;
            output( handle, 1, "input %s;\n", clk->name );
            for (clk2=module->clocks; clk2; clk2=clk2->next_in_list)
            {
                if ((clk2!=clk) && (clk2->data.clock.clock_ref==clk))
                {
                    output( handle, 1, "wire %s; // Gated version of clock '%s' enabled by '%s'\n", clk2->name, clk->name, clk2->data.clock.gate_state?clk2->data.clock.gate_state->name:clk2->data.clock.gate_signal->name );
                }
            }
        }
    }
    output( handle, 0, "\n" );

    output( handle, 1, "//b Inputs\n" );
    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            instance = signal->instance_iter->children[i];
            if (instance->type == md_type_instance_type_bit_vector)
            {
                if (instance->type_def.data.width==1)
                {
                    output( handle, 1, "input %s;\n", instance->output_name );
                }
                else
                {
                    output( handle, 1, "input [%d:0]%s;\n", instance->type_def.data.width-1, instance->output_name );
                }
            }
            else
            {
                output( handle, 1, "ZZZ Don't know how to register an input array '%s'\n", instance->output_name );
            }
        }
    }
    output( handle, 0, "\n");

    output( handle, 1, "//b Outputs\n" );
    for (signal=module->outputs; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            instance = signal->instance_iter->children[i];
            if (instance->type == md_type_instance_type_bit_vector)
            {
                if (instance->type_def.data.width==1)
                {
                    output( handle, 1, "output %s;\n", instance->output_name );
                }
                else
                {
                    output( handle, 1, "output [%d:0]%s;\n", instance->type_def.data.width-1, instance->output_name );
                }
            }
            else
            {
                output( handle, 1, "ZZZ Don't know how to register an output array '%s'\n", instance->output_name );
            }
        }
    }
    output( handle, 0, "\n" );
}

/*f output_module_rtl_architecture_components - nothing as yet (would be component declarations - needed for VHDL)
 */
static void output_module_rtl_architecture_components( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module )
{
     output( handle, 0, "// output components here\n" );
     output( handle, 0, "\n" );
}

/*f output_signal_definition
 */
static void output_signal_definition( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module, t_md_type_instance *instance, const char *type, const char *suffix, const char *reason, const char *override_name )
{
    switch (instance->type)
    {
    case md_type_instance_type_bit_vector:
        if (instance->type_def.data.width==1)
        {
            output( handle, 1, "%s %s%s;\n", type, override_name?override_name:instance->output_name, suffix );
        }
        else
        {
            output( handle, 1, "%s [%d:0]%s%s;\n", type,instance->type_def.data.width-1, override_name?override_name:instance->output_name, suffix );
        }
        break;
    case md_type_instance_type_array_of_bit_vectors:
        if (options.vmod_mode && INSTANCE_IS_BIT_VECTOR_ARRAY(instance) && !(instance->output_args[output_arg_vmod_is_indexed_by_runtime]))
        {
            int i;
            for (i=0; i<instance->size; i++ )
            {
                output( handle, 1, "%s [%d:0]%s%s__%d;\n", type, instance->type_def.data.width-1, override_name?override_name:instance->output_name, suffix, i );
            }
        }
        else
        {
            if (instance->type_def.data.width==1)
            {
                output( handle, 1, "%s %s%s", type,override_name?override_name:instance->output_name, suffix );
            }
            else
            {
                output( handle, 1, "%s [%d:0]%s%s", type,instance->type_def.data.width-1, override_name?override_name:instance->output_name, suffix );
            }
            output( handle, -1, "[%d:0];\n", instance->size-1 );
        }
        break;
    default:
        output( handle, 1, "ZZZ Don't know how to register a %s '%s'\n", reason, override_name?override_name:instance->output_name );
        break;
    }
}

/*f output_documentation
 */
static void output_documentation( c_model_descriptor *model, t_md_output_fn output, void *handle, int indent, char *documentation, int postpend_if_possible )
{
    int i;
    unsigned int j;
    int postpend;
    char buffer[256];

    if (!documentation)
    {
        if (postpend_if_possible)
            output( handle, -1, "\n" );
        return;
    }

    postpend = postpend_if_possible;
    for (i=0; documentation[i] && postpend; i++)
    {
        if (documentation[i]=='\n')
            postpend = 0;
    }

    if (postpend)
    {
        output( handle, -1, "//   %s\n", documentation );
        return;
    }

    if (postpend_if_possible)
    {
        output( handle, -1, "\n" );
    }
    i=-1;
    do
    {
        i++;
        for (j=0; (documentation[i]) && (documentation[i]!='\n') && (j<sizeof(buffer)-1); i++,j++ )
            buffer[j] = documentation[i];
        buffer[j] = 0;
        output( handle, indent+1, "//   %s\n", buffer );
    } while (documentation[i]=='\n');
}

/*f output_module_rtl_architecture_internal_signals - (not gated clocks - in verilog we count them as 'ports') - output combs, output nets, clocked registers as regs, internal combs, internal nets
 */
static void output_module_rtl_architecture_internal_signals( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module )
{
    t_md_signal *signal;
    t_md_state *reg;
    int i;

    /*b Write out output combinatorials
     */
    output( handle, 1, "//b Output combinatorials\n" );
    for (signal=module->combinatorials; signal; signal=signal->next_in_list)
    {
        if (!signal->data.combinatorial.output_ref)
            continue;

        output_documentation( model, output, handle, 1, signal->documentation, 0 );
        output_push_usage_type( model, output, handle, signal->usage_type );
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output_signal_definition( model, output, handle, module, signal->instance_iter->children[i], "reg", "", "combinatorial signal array", NULL );
        }
        output_pop_usage_type( model, output, handle );
    }
    output_set_usage_type( model, output, handle );
    output( handle, 0, "\n" );

    /*b Write out output nets
     */
    output( handle, 1, "//b Output nets\n" );
    for (signal=module->nets; signal; signal=signal->next_in_list)
    {
        if (!signal->data.combinatorial.output_ref)
            continue;

        output_documentation( model, output, handle, 1, signal->documentation, 0 );
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            if (options.vmod_mode)
            {
                output_signal_definition( model, output, handle, module, signal->instance_iter->children[i], "reg", "", "wire signal array", NULL );
            }
            else
            {
                output_signal_definition( model, output, handle, module, signal->instance_iter->children[i], "wire", "", "wire signal array", NULL );
            }
        }
    }
    if (options.vmod_mode)
    {
        t_md_module_instance *module_instance;
        for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
        {
            t_md_module_instance_output_port *output_port;
            for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
            {
                char buffer[1024];
                snprintf( buffer, sizeof(buffer), "vmod_net__%s__%s", module_instance->name, output_port->module_port_instance->output_name );
                output_signal_definition( model, output, handle, module, output_port->module_port_instance, "wire", "", "wire signal array", buffer );
            }
        }
    }
    output( handle, 0, "\n" );

    /*b Write out input and output clocked signals
     */
    output( handle, 1, "//b Internal and output registers\n" );
    for (reg=module->registers; reg; reg=reg->next_in_list)
    {
//        if (reg->output_ref)
//            continue;

        output_documentation( model, output, handle, 1, reg->documentation, 0 );
        for (i=0; i<reg->instance_iter->number_children; i++)
        {
            output_push_usage_type( model, output, handle, reg->usage_type );
            output_signal_definition( model, output, handle, module, reg->instance_iter->children[i], "reg", "", "clocked signal array", NULL );
            output_pop_usage_type( model, output, handle );
        }
    }
    output_set_usage_type( model, output, handle );
    output( handle, 0, "\n" );

    /*b Write out internal combinatorials
     */
    output( handle, 1, "//b Internal combinatorials\n" );
    for (signal=module->combinatorials; signal; signal=signal->next_in_list)
    {
        if (signal->data.combinatorial.output_ref)
            continue;

        output_documentation( model, output, handle, 1, signal->documentation, 0 );
        output_push_usage_type( model, output, handle, signal->usage_type );
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            output_signal_definition( model, output, handle, module, signal->instance_iter->children[i], "reg", "", "internal combinatorial signal array", NULL );
        }
        output_pop_usage_type( model, output, handle );
    }
    output_set_usage_type( model, output, handle );
    output( handle, 0, "\n" );

    /*b Write out internal nets
     */
    output( handle, 1, "//b Internal nets\n" );
    for (signal=module->nets; signal; signal=signal->next_in_list)
    {
        if (signal->data.combinatorial.output_ref)
            continue;

        output_documentation( model, output, handle, 1, signal->documentation, 0 );
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            if (options.vmod_mode)
            {
                output_signal_definition( model, output, handle, module, signal->instance_iter->children[i], "reg", "", "wire signal array", NULL );
            }
            else
            {
                output_signal_definition( model, output, handle, module, signal->instance_iter->children[i], "wire", "", "net signal array", NULL );
            }
        }
    }
    output( handle, 0, "\n" );
}

/*f output_module_rtl_architecture_lvar - handled unique names for vmod
 */
enum
{
    rtl_lvar_out_ignore_none = 0,
    rtl_lvar_out_ignore_subindex = 1,
    rtl_lvar_out_ignore_index = 2,
    rtl_lvar_out_ignore_lvar = 4,
    rtl_lvar_out_ignore_uniquify = 8,
};
static void output_module_rtl_architecture_lvar( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_lvar *lvar, int main_indent, int sub_indent, int rtl_lvar_out, int id )
{
    if (lvar->output.uniquified_name && (!(rtl_lvar_out & rtl_lvar_out_ignore_uniquify)) )
    {
        output( handle, main_indent, lvar->output.uniquified_name, id );
    }
    else
    {
        if (!(rtl_lvar_out & rtl_lvar_out_ignore_lvar))
        {
            if (lvar->instance_type == md_lvar_instance_type_state)
            {
                output( handle, main_indent, "%s", lvar->instance->output_name );
            }
            else
            {
                t_md_signal *signal;
                signal = lvar->instance_data.signal;
                switch (signal->type)
                {
                case md_signal_type_input:
                    output( handle, main_indent, "%s", lvar->instance->output_name );
                    break;
                case md_signal_type_output:
                    if (signal->data.output.combinatorial_ref)
                    {
                        if (lvar->instance->output_args[output_arg_use_comb_reg])
                        {
                            output( handle, main_indent, "%s%s", lvar->instance->output_name, options.verilog_comb_reg_suffix );
                        }
                        else
                        {
                            output( handle, main_indent, "%s", lvar->instance->output_name );
                        }
                    }
                    else
                    {
                        output( handle, main_indent, "<unresolved output %s>", signal->name );
                    }
                    break;
                case md_signal_type_combinatorial:
                    if (lvar->instance->output_args[output_arg_use_comb_reg])
                    {
                        output( handle, main_indent, "%s%s", lvar->instance->output_name, options.verilog_comb_reg_suffix );
                    }
                    else
                    {
                        output( handle, main_indent, "%s", lvar->instance->output_name );
                    }
                    break;
                case md_signal_type_net:
                    output( handle, main_indent, "%s", lvar->instance->output_name );
                    break;
                default:
                    output( handle, main_indent, "<clock signal in expression %s>", signal->name );
                    break;
                }
            }
        }

        if (!(rtl_lvar_out & rtl_lvar_out_ignore_index))
        {
            if (options.vmod_mode && INSTANCE_IS_BIT_VECTOR_ARRAY(lvar->instance) && !(lvar->instance->output_args[output_arg_vmod_is_indexed_by_runtime]))
            {
                output( handle, -1, "__%d", lvar->index.data.integer );
            }
            else
            {
                if (lvar->index.type != md_lvar_data_type_none)
                {
                    output( handle, -1, "[" );
                    switch (lvar->index.type)
                    {
                    case md_lvar_data_type_integer:
                        output( handle, -1, "%d", lvar->index.data.integer );
                        break;
                    case md_lvar_data_type_expression:
                        output_module_rtl_architecture_expression( model, output, handle, code_block, lvar->index.data.expression, main_indent, sub_indent+1, id );
                        break;
                    default:
                        break;
                    }
                    output( handle, -1, "]" );
                }
            }
        }
    }

    if (!(rtl_lvar_out & rtl_lvar_out_ignore_subindex))
    {
        if (lvar->subscript_start.type != md_lvar_data_type_none)
        {
            if ( (lvar->subscript_length.type != md_lvar_data_type_none) && (lvar->subscript_start.type == md_lvar_data_type_integer) )
            {
                output( handle, -1, "[%d:%d]",
                        lvar->subscript_start.data.integer+lvar->subscript_length.data.integer-1,
                        lvar->subscript_start.data.integer );
            }
            else if ( (lvar->subscript_length.type != md_lvar_data_type_none) &&
                      ((lvar->subscript_start.type == md_lvar_data_type_expression) &&
                       (lvar->subscript_start.data.expression->type==md_expr_type_value)) )
            {
                output( handle, -1, "[%d:%d]", (int)(lvar->subscript_length.data.integer+lvar->subscript_start.data.expression->data.value.value.value[0]-1),
                        (int)(lvar->subscript_start.data.expression->data.value.value.value[0]) );
            }
            else if ( (lvar->subscript_length.type != md_lvar_data_type_none) &&
                      (lvar->subscript_start.type == md_lvar_data_type_expression))
            {
                output( handle, -1, "[(" );
                output_module_rtl_architecture_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, main_indent, sub_indent+1, id );
                output( handle, -1, ") +: %d]", lvar->subscript_length.data.integer );
            }
            else if (lvar->subscript_start.type == md_lvar_data_type_integer)
            {
                output( handle, -1, "[%d]", lvar->subscript_start.data.integer );
            }
            else if ((lvar->subscript_start.type == md_lvar_data_type_expression) &&
                     (lvar->subscript_start.data.expression->type==md_expr_type_value))
            {
                output( handle, -1, "[%d]", (int)(lvar->subscript_start.data.expression->data.value.value.value[0]) );
            }
            else if (lvar->subscript_start.type == md_lvar_data_type_expression)
            {
                output( handle, -1, "[" );
                output_module_rtl_architecture_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, main_indent, sub_indent+1, id );
                output( handle, -1, "]" );
            }
        }
    }
}

/*f output_module_rtl_architecture_expression
 */
static void output_module_rtl_architecture_expression( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_expression *expr, int main_indent, int sub_indent, int id )
{
    /*b If the expression is NULL then something went wrong...
     */
    if (!expr)
    {
        output( handle, -1, "<NULL expression>" );
        return;
    }

    /*b Output code for the expression
     */
    switch (expr->type)
    {
    case md_expr_type_value:
        output( handle, -1, "%d'h%llx", expr->data.value.value.width, expr->data.value.value.value[0] ); 
        break;
    case md_expr_type_lvar:
        output_module_rtl_architecture_lvar( model, output, handle , code_block, expr->data.lvar, -1, sub_indent, rtl_lvar_out_ignore_none, id );
        break;
    case md_expr_type_bundle:
        output( handle, -1, "{" );
        output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.bundle.a, main_indent, sub_indent+1 , id);
        output( handle, -1, "," );
        output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.bundle.b, main_indent, sub_indent+1, id );
        output( handle, -1, "}" );
        break;
    case md_expr_type_cast:
        // All casts should be implicit in VERILOG (?)
        // No. Integers size across okay, but the expression does not know it is an integer!!! So a constant will be 32'h..., which cannot be assigned to a single bit. Hm.
        if ( (expr->data.cast.expression) &&
             (expr->data.cast.expression->type==md_expr_type_value) )
        {
            output( handle, -1, "%d'h%llx", expr->width, expr->data.cast.expression->data.value.value.value[0] ); 
        }
        else
        {
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.cast.expression, main_indent, sub_indent+1, id );
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
                output( handle, -1, "-" );
                break;
            case md_expr_fn_not:
                output( handle, -1, "~" );
                break;
            case md_expr_fn_logical_not:
                output( handle, -1, "!" );
                break;
            default:
                break;
            }
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1, id );
            break;
        case md_expr_fn_add:
        case md_expr_fn_sub:
        case md_expr_fn_mult:
        case md_expr_fn_lsl:
        case md_expr_fn_lsr:
            output( handle, -1, "(" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1, id );
            if (expr->data.fn.fn==md_expr_fn_sub)
                output( handle, -1, "-" );
            else if (expr->data.fn.fn==md_expr_fn_mult)
                output( handle, -1, "*" );
            else if (expr->data.fn.fn==md_expr_fn_lsl)
                output( handle, -1, "<<" );
            else if (expr->data.fn.fn==md_expr_fn_lsr)
                output( handle, -1, ">>" );
            else
                output( handle, -1, "+" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent+1, id );
            output( handle, -1, ")" );
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
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1, id );
            if (expr->data.fn.fn==md_expr_fn_and)
                output( handle, -1, " & " );
            else if (expr->data.fn.fn==md_expr_fn_or)
                output( handle, -1, " | " );
            else if (expr->data.fn.fn==md_expr_fn_xor)
                output( handle, -1, " ^ " );
            else if (expr->data.fn.fn==md_expr_fn_bic)
                output( handle, -1, " &~ " );
            else if (expr->data.fn.fn==md_expr_fn_logical_and)
                output( handle, -1, "&&" );
            else if (expr->data.fn.fn==md_expr_fn_logical_or)
                output( handle, -1, "||" );
            else if (expr->data.fn.fn==md_expr_fn_eq)
                output( handle, -1, "==" );
            else if (expr->data.fn.fn==md_expr_fn_ge)
                output( handle, -1, ">=" );
            else if (expr->data.fn.fn==md_expr_fn_le)
                output( handle, -1, "<=" );
            else if (expr->data.fn.fn==md_expr_fn_gt)
                output( handle, -1, ">" );
            else if (expr->data.fn.fn==md_expr_fn_lt)
                output( handle, -1, "<" );
            else
                output( handle, -1, "!=" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent+1, id );
            output( handle, -1, ")" );
            break;
        case md_expr_fn_query:
            output( handle, -1, "(" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1, id );
            output( handle, -1, "?" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent+1, id );
            output( handle, -1, ":" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[2], main_indent, sub_indent+1, id );
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

/*f output_module_rtl_unique_declaration - handled unique names for vmod
 */
static void output_module_rtl_unique_declaration( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_lvar *lvar, int indent, int id )
{
    int width;
    if (lvar->instance->output_args[output_arg_vmod_has_been_declared]) return;
    lvar->instance->output_args[output_arg_vmod_has_been_declared]=1;
    width = lvar->instance->type_def.data.width;
    if (width>1)
    {
        output( handle, indent, "reg [%d:0] ", width-1 );
    }
    else
    {
        output( handle, indent, "reg ");
    }
    output( handle, -1, lvar->output.uniquified_name, id);
    output( handle, -1, ";\n");
}

/*f output_module_rtl_architecture_unique_expressions - handled unique names for vmod
  If uniquified LVARs exist, output their definitions or declarations
  declarations are output if main_indent is -2, else definitions
 */
static void output_module_rtl_architecture_unique_expressions( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_expression *expr, int main_indent, int sub_indent, int id )
{
    /*b Nothing if NULL
     */
    if (!expr)
    {
        return;
    }

    /*b Output code to uniqify LVARs if necessary
     */
    switch (expr->type)
    {
    case md_expr_type_value:
        break;
    case md_expr_type_lvar:
        if (expr->data.lvar->output.uniquified_name)
        {
            if (main_indent==-2)
            {
                output_module_rtl_unique_declaration( model, output, handle, code_block, expr->data.lvar, 1, id );
            }
            else
            {
                output( handle, main_indent, expr->data.lvar->output.uniquified_name, id);
                output( handle, -1, " = " );
                output_module_rtl_architecture_lvar( model, output, handle , code_block, expr->data.lvar, -1, sub_indent, rtl_lvar_out_ignore_uniquify | rtl_lvar_out_ignore_subindex, id );
                output( handle, -1, ";\n");
            }
        }
        break;
    case md_expr_type_bundle:
        output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.bundle.a, main_indent, sub_indent, id );
        output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.bundle.b, main_indent, sub_indent, id );
        break;
    case md_expr_type_cast:
        output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.cast.expression, main_indent, sub_indent, id );
        break;
    case md_expr_type_fn:
        switch (expr->data.fn.fn)
        {
        case md_expr_fn_logical_not:
        case md_expr_fn_not:
        case md_expr_fn_neg:
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent, id );
            break;
        case md_expr_fn_add:
        case md_expr_fn_sub:
        case md_expr_fn_lsl:
        case md_expr_fn_lsr:
        case md_expr_fn_mult:
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent, id );
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent, id );
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
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent, id );
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent, id );
            break;
        case md_expr_fn_query:
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent, id );
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent, id );
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, expr->data.fn.args[2], main_indent, sub_indent, id );
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

/*f output_module_rtl_architecture_parallel_switch
 */
static void output_module_rtl_architecture_parallel_switch( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge, t_md_signal *reset, int reset_level )
{
     if (statement->data.switch_stmt.all_static && statement->data.switch_stmt.all_unmasked)
     {
          t_md_switch_item *switem;
          int defaulted = 0;

          output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, statement->data.switch_stmt.expr, indent+1, 0, statement->output.unique_id );
          output( handle, indent+1, "case (");
          output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.switch_stmt.expr, indent+1, 4, statement->output.unique_id );
          for (switem = statement->data.switch_stmt.items; switem; switem=switem->next_in_list)
          {
              if (switem->type == md_switch_item_type_default)
              {
                  defaulted = 1;
              }
          }
          output( handle, -1, ") //synopsys parallel_case%s\n", (statement->data.switch_stmt.full && !defaulted)?"":"" ); // removed full case directive as people JUST DONT GET IT RIGHT
          output_documentation( model, output, handle, indent+1, statement->data.switch_stmt.expr_documentation, 0 );
          for (switem = statement->data.switch_stmt.items; switem; switem=switem->next_in_list)
          {
              int stmts_reqd;
              stmts_reqd = 1;
//               if (clock && !model->reference_set_includes( &switem->statement->effects, clock, edge ))
//                   stmts_reqd = 0;
//               if (instance && !model->reference_set_includes( &switem->statement->effects, instance ))
//                   stmts_reqd = 0;

               if (switem->type == md_switch_item_type_static)
               {
                   if (stmts_reqd || statement->data.switch_stmt.full || 1)
                   {
                       output( handle, indent+1, "%d'h%llx: // req %d\n", statement->data.switch_stmt.expr->width, switem->data.value.value.value[0], stmts_reqd );
                       output( handle, indent+2, "begin\n");
                       output_module_rtl_architecture_statement( model, output, handle, code_block, switem->statement, indent+1, clock, edge, reset, reset_level );
                       output( handle, indent+2, "end\n");
                   }
               }
               else if (switem->type == md_switch_item_type_default)
               {
                   if (stmts_reqd || statement->data.switch_stmt.full)
                   {
                       output( handle, indent+1, "default: // req %d\n", stmts_reqd);
                       output( handle, indent+2, "begin\n");
                       output_module_rtl_architecture_statement( model, output, handle, code_block, switem->statement, indent+1, clock, edge, reset, reset_level );
                       output( handle, indent+2, "end\n");
                   }
               }
               else
               {
                   model->error->add_error( NULL, error_level_serious, error_number_general_error_s, 0,
                                            error_arg_type_const_string, "BUG - non static unmasked case item in parallel static unmasked switch C output",
                                            error_arg_type_none );
                    fprintf( stderr, "BUG - non static unmasked case item in parallel static unmasked switch C output\n" );
               }
          }
          if (!defaulted)
          {
              if (statement->data.switch_stmt.full)
              {
                  output_push_usage_type( model, output, handle, md_usage_type_assert );
                  output( handle, indent+1, "default:\n");
                  output( handle, indent+2, "begin\n");
                  output( handle, indent+3, "%s\n", (options.assert_delay_string) ? options.assert_delay_string : "if (1)" );
                  output( handle, indent+3, "begin\n");
                  output( handle, indent+4, "$display($time,\"*********CDL ASSERTION FAILURE:%s:%s: Full switch statement did not cover all values\");\n", code_block->module->output_name, code_block->name);
                  output( handle, indent+3, "end\n");
                  output( handle, indent+2, "end\n");
                  output_pop_usage_type( model, output, handle );
                  output_set_usage_type( model, output, handle );
              }
              else
              {
                  output( handle, indent+1, "//pragma coverage off\n");
                  output( handle, indent+1, "default:\n");
                  output( handle, indent+2, "begin\n");
                  output( handle, indent+2, "//Need a default case to make Cadence Lint happy, even though this is not a full case\n");
                  output( handle, indent+2, "end\n");
                  output( handle, indent+1, "//pragma coverage on\n");
              }
          }
          output( handle, indent+1, "endcase\n");
     }
     else
     {
          output( handle, indent, "!!!PARALLEL SWITCH ONLY WORKS NOW FOR STATIC UNMASKED EXPRESSIONS\n");
     }
}

/*f output_module_rtl_architecture_message
 */
static void output_module_rtl_architecture_message( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_message *message, int indent, int level )
{
    char buffer[1024];
    int args[64];
    int i, j, k;
    i=j=k=0;
    if (!message)
        return;
    if (!options.include_displays)
        return;
    while (message->text[i])
    {
        if ( (message->text[i]=='%') &&
             (message->text[i+2]>='0') &&
             (message->text[i+2]<='9') &&
             (message->text[i+3]=='%') )
        {
            buffer[j++] = '%';
            buffer[j++] = (message->text[i+1]=='d') ? 'd' : 'x';
            args[k++] = message->text[i+2]-'0';
            i+=4;
        }
        else
        {
            buffer[j++] = message->text[i++];
        }
    }
    buffer[j] =0;
    output( handle, indent+1, "$display( \"%%d %s:%s\",\n", level?"ASSERTION FAILED":"Information", buffer );
    output( handle, indent+2, "$time" );
    for (i=0; i<k; i++)
    {
        t_md_expression *expr;
        for (j=0, expr=message->arguments; (j<args[i]) && (expr); j++, expr=expr->next_in_chain);
        if (expr)
        {
            output( handle, -1, ",\n" );
            output( handle, indent+2, "" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr, indent+2, 0, 0 );
        }
    }
    output( handle, -1, " );\n" );
}

/*f output_module_rtl_architecture_statement
 */
static void output_module_rtl_architecture_statement( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *signal, int edge, t_md_signal *reset, int reset_level )
{
    if (!statement)
        return;

    /*b If the statement does not effect this signal/clock/edge, then return with outputting nothing
     */
    if (signal)
    {
        if (!model->reference_set_includes( &statement->effects, signal, edge, reset, reset_level ))
        {
            output_module_rtl_architecture_statement( model, output, handle, code_block, statement->next_in_code, indent, signal, edge, reset, reset_level );
            return;
        }
    }
    else
    {
        if (!model->reference_set_includes( &statement->effects, md_reference_type_signal ))
        {
            output_module_rtl_architecture_statement( model, output, handle, code_block, statement->next_in_code, indent, signal, edge, reset, reset_level );
            return;
        }
    }

    /*b Display the statement
     */
    statement->output.unique_id = code_block->module->output.unique_id++;
    switch (statement->type)
    {
    case md_statement_type_state_assign:
        if (edge>=0)
        {
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, statement->data.state_assign.expr, indent+1, 0, statement->output.unique_id );
            output_module_rtl_architecture_lvar( model, output, handle, code_block, statement->data.state_assign.lvar, indent+1, indent+2, rtl_lvar_out_ignore_uniquify, statement->output.unique_id );
            output( handle, -1, " <= " );
            output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.state_assign.expr, indent+1, 0, statement->output.unique_id );
        }
        output( handle, -1, ";");
        output_documentation( model, output, handle, indent+1, statement->data.state_assign.documentation, 1 );
        break;
    case md_statement_type_comb_assign:
        if (edge<0)
        {
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, statement->data.comb_assign.expr, indent+1, 0, statement->output.unique_id );
            if (statement->data.comb_assign.lvar->output.uniquified_name)
            {
                output( handle, indent+1, statement->data.state_assign.lvar->output.uniquified_name, statement->output.unique_id );
                output( handle, -1, " = " );
                output_module_rtl_architecture_lvar( model, output, handle, code_block, statement->data.state_assign.lvar, -1, indent+2, rtl_lvar_out_ignore_uniquify | rtl_lvar_out_ignore_subindex, statement->output.unique_id );
                output( handle, -1, ";\n");

                output( handle, indent+1, statement->data.comb_assign.lvar->output.uniquified_name, statement->output.unique_id );
                output_module_rtl_architecture_lvar( model, output, handle, code_block, statement->data.state_assign.lvar, -1, indent+2, rtl_lvar_out_ignore_uniquify | rtl_lvar_out_ignore_lvar | rtl_lvar_out_ignore_index, statement->output.unique_id );
                output( handle, -1, " = " );
                output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.state_assign.expr, indent+1, 0, statement->output.unique_id );
                output( handle, -1, ";\n");
                output( handle, indent+1, "");
                output_module_rtl_architecture_lvar( model, output, handle, code_block, statement->data.state_assign.lvar, -1, indent+2, rtl_lvar_out_ignore_uniquify | rtl_lvar_out_ignore_subindex, statement->output.unique_id );
                output( handle, -1, " = " );
                output( handle, -1, statement->data.state_assign.lvar->output.uniquified_name, statement->output.unique_id );
            }
            else
            {
                output( handle, indent+1, "" );
                output_module_rtl_architecture_lvar( model, output, handle, code_block, statement->data.comb_assign.lvar, -1, indent+2, rtl_lvar_out_ignore_none, statement->output.unique_id );
                if (statement->data.comb_assign.wired_or)
                {
                    output( handle, -1, " = " );
                    output_module_rtl_architecture_lvar( model, output, handle, code_block, statement->data.comb_assign.lvar, -1, indent+2, rtl_lvar_out_ignore_none, statement->output.unique_id );
                    output( handle, -1, " | " );
                    // Note that expressions are bracketed if required already
                    output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.comb_assign.expr, indent+1, 0, statement->output.unique_id );
                    output( handle, -1, "" );
                }
                else
                {
                    output( handle, -1, " = " );
                    output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.comb_assign.expr, indent+1, 0, statement->output.unique_id );
                }
            }
        }
        else
        {
            output( handle, indent+1, "should not be here\n");
            // ERROR
        }
        output( handle, -1, ";" );
        output_documentation( model, output, handle, indent+1, statement->data.comb_assign.documentation, 1 );
        break;
    case md_statement_type_if_else:
        output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, statement->data.if_else.expr, indent+1, 4, statement->output.unique_id );
        output( handle, indent+1, "if (");
        output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.if_else.expr, indent+1, 4, statement->output.unique_id );
        output( handle, -1, ")");
        output_documentation( model, output, handle, indent+1, statement->data.if_else.expr_documentation, 1 );
        output( handle, indent+1, "begin\n");
        if (statement->data.if_else.if_true)
        {
            output_module_rtl_architecture_statement( model, output, handle, code_block, statement->data.if_else.if_true, indent+1, signal, edge, reset, reset_level );
        }
        output( handle, indent+1, "end //if\n");
        if (statement->data.if_else.if_false)
        {
            output( handle, indent+1, "else\n");
            output( handle, indent+1, "begin\n");
            output_module_rtl_architecture_statement( model, output, handle, code_block, statement->data.if_else.if_false, indent+1, signal, edge, reset, reset_level );
            output( handle, indent+1, "end //else\n");
        }
        break;
    case md_statement_type_parallel_switch:
        output_module_rtl_architecture_parallel_switch( model, output, handle, code_block, statement, indent, signal, edge, reset, reset_level );
        break;
    case md_statement_type_print_assert:
        if ( !statement->data.print_assert_cover.expression && !statement->data.print_assert_cover.statement)  // Print statement
        {
            output_module_rtl_architecture_message( model, output, handle, code_block, statement->data.print_assert_cover.message, indent, 0 );
        }
        else if (options.include_assertions)
        {
            output_push_usage_type( model, output, handle, md_usage_type_assert );
            // If value_list, then expression must be one of value_list ELSE do statement or message
            // If not value_list, then expression must be FALSE to do statement or message
            if (statement->data.print_assert_cover.expression &&
                statement->data.print_assert_cover.value_list)
            {
                t_md_expression *value;
                output( handle, indent+1, "if ( (1'b0==1'b0)" );
                for (value=statement->data.print_assert_cover.value_list; value; value=value->next_in_chain )
                {
                    output( handle, -1, "&&\n" );
                    output( handle, indent+2, "((" );
                    output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.print_assert_cover.expression, indent+2, 4, statement->output.unique_id );
                    output( handle, -1, ")!=(" );
                    output_module_rtl_architecture_expression( model, output, handle, code_block, value, indent+2, 4, statement->output.unique_id );
                    output( handle, -1, "))" );
                }
                output( handle, -1, ")\n" );
                output( handle, indent+1, "begin\n" );
            }
            else if (statement->data.print_assert_cover.expression)
            {
                output( handle, indent+1, "if ( !(");
                output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.print_assert_cover.expression, indent+2, 4, statement->output.unique_id );
                output( handle, -1, ") )\n" );
                output( handle, indent+1, "begin\n" );
            }
            else
            {
                output( handle, indent+1, "if (1'b0==1'b0)\n" );
                output( handle, indent+1, "begin\n" );
            }
            if (statement->data.print_assert_cover.message)
            {
                output_module_rtl_architecture_message( model, output, handle, code_block, statement->data.print_assert_cover.message, indent, 1 );
            }
            if (statement->data.print_assert_cover.statement)
            {
                output_module_rtl_architecture_statement( model, output, handle, code_block, statement->data.print_assert_cover.statement, indent+1, signal, edge, reset, reset_level );
            }
            output( handle, indent+1, "end //if\n" );
            output_pop_usage_type( model, output, handle );
            output_set_usage_type( model, output, handle );
        }
        break;
    case md_statement_type_cover:
        if (options.include_coverage)
        {
            output( handle, 1, "//Ignoring cover for now\n");
        }
        break;
    case md_statement_type_log: // Nothing for logs in verilog
        break;
    default:
        output( handle, 1, "Unknown statement type %d\n", statement->type );
        break;
    }
    output_module_rtl_architecture_statement( model, output, handle, code_block, statement->next_in_code, indent, signal, edge, reset, reset_level );
}

/*f output_module_rtl_architecture_statement_vmod_regs
 */
static void output_module_rtl_architecture_statement_vmod_regs( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *signal, int edge, t_md_signal *reset, int reset_level )
{
    if (!statement)
        return;

    /*b If the statement does not effect this signal/clock/edge, then return with outputting nothing
     */
    if (signal)
    {
        if (!model->reference_set_includes( &statement->effects, signal, edge, reset, reset_level ))
        {
            output_module_rtl_architecture_statement_vmod_regs( model, output, handle, code_block, statement->next_in_code, indent, signal, edge, reset, reset_level );
            return;
        }
    }
    else
    {
        if (!model->reference_set_includes( &statement->effects, md_reference_type_signal ))
        {
            output_module_rtl_architecture_statement_vmod_regs( model, output, handle, code_block, statement->next_in_code, indent, signal, edge, reset, reset_level );
            return;
        }
    }

    /*b Display any regs required by the statement
     */
    statement->output.unique_id = code_block->module->output.unique_id++;
    switch (statement->type)
    {
    case md_statement_type_state_assign:
        if (edge>=0)
        {
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, statement->data.state_assign.expr, -2, 0, statement->output.unique_id );
        }
        break;
    case md_statement_type_comb_assign:
        if (edge<0)
        {
            output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, statement->data.comb_assign.expr, -2, 0, statement->output.unique_id );
            if (statement->data.comb_assign.lvar->output.uniquified_name)
            {
                output_module_rtl_unique_declaration( model, output, handle, code_block, statement->data.comb_assign.lvar, -2, statement->output.unique_id );
            }
        }
        else
        {
            output( handle, indent+1, "should not be here\n");
            // ERROR
        }
        break;
    case md_statement_type_if_else:
        output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, statement->data.if_else.expr, -2, 4, statement->output.unique_id );
        if (statement->data.if_else.if_true)
        {
            output_module_rtl_architecture_statement_vmod_regs( model, output, handle, code_block, statement->data.if_else.if_true, indent+1, signal, edge, reset, reset_level );
        }
        if (statement->data.if_else.if_false)
        {
            output_module_rtl_architecture_statement_vmod_regs( model, output, handle, code_block, statement->data.if_else.if_false, indent+1, signal, edge, reset, reset_level );
        }
        break;
    case md_statement_type_parallel_switch:
        output_module_rtl_architecture_unique_expressions( model, output, handle, code_block, statement->data.switch_stmt.expr, -2, 0, statement->output.unique_id );
        {
            t_md_switch_item *switem;
            for (switem = statement->data.switch_stmt.items; switem; switem=switem->next_in_list)
            {
                if (switem->type == md_switch_item_type_static)
                {
                    output_module_rtl_architecture_statement_vmod_regs( model, output, handle, code_block, switem->statement, indent+1, signal, edge, reset, reset_level );
                }
                else if (switem->type == md_switch_item_type_default)
                {
                    output_module_rtl_architecture_statement_vmod_regs( model, output, handle, code_block, switem->statement, indent+1, signal, edge, reset, reset_level );
                }
            }
        }
        break;
    default:
        break;
    }
    output_module_rtl_architecture_statement_vmod_regs( model, output, handle, code_block, statement->next_in_code, indent, signal, edge, reset, reset_level );
}

/*f output_module_rtl_architecture_code_block
 */
static void output_module_rtl_architecture_code_block( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module , t_md_code_block *code_block )
{
    t_md_signal *clk, *reset;
    t_md_type_instance *instance, *instance2;
    int edge, reset_level;
    int has_comb, has_sync, has_many_sync;
    t_md_reference_iter iter, iter2;
    t_md_reference *reference, *reference2;
    int first;

    /*b Determine if code block has combinatorial, a single clock/reset, and/or many clock/reset
     */
    has_sync = 0;
    has_many_sync = 0;
    has_comb = 0;
    clk = NULL;
    edge = 0;
    reset = NULL;
    reset_level = 0;

    model->reference_set_iterate_start( &code_block->effects, &iter ); // We expect clock/edge (for assertions/prints) and instances
    while ((reference = model->reference_set_iterate(&iter))!=NULL)
    {
        /*b If its a combinatorial instance, its combinatorial; if its a state instnace its either sync or many_sync too
         */
        if (reference->type != md_reference_type_instance)
            continue;
        instance = reference->data.instance;
        switch (instance->reference.type)
        {
        case md_reference_type_signal:
            has_comb = 1;
            break;
        case md_reference_type_state:
            instance->output_handle = NULL;
            instance->output_args[output_arg_state_has_reset] = 0;
            if ( (has_sync) &&
                 ( (instance->reference.data.state->reset_ref!=reset) ||
                   (instance->reference.data.state->reset_level!=reset_level) ||
                   (instance->reference.data.state->clock_ref!=clk) ||
                   (instance->reference.data.state->edge!=edge) ))
            {
                has_many_sync = 1;
            }
            else
            {
                has_sync = 1;
                reset = instance->reference.data.state->reset_ref;
                reset_level = instance->reference.data.state->reset_level;
                clk = instance->reference.data.state->clock_ref;
                edge = instance->reference.data.state->edge;
            }
        default:
            break;
        }
    }

    /*b Output combinatorial process if required
      We could go through all the instances that this code_block effects, and generate an immediate dependency list for them
      and use that in the always list
      Each instance that is effected can then have a reg <thingy> done, and the instance can be marked as a reg thingy for output in the procedure (effectively influencing its output_name)
      Then at the end of the always block it can have name = <thingy>.
      This should guarantee a single assignment to name, and hence create no new events if <name> has multiple assignments in the code block
     */
    if (has_comb)
    {
        /*b Generate the set of dependencies that all the combinatorials effected by this code block depend upon
         */
        t_md_reference_set *dependencies;
        dependencies = NULL;
        model->reference_set_iterate_start( &code_block->effects, &iter );
        while ((reference = model->reference_set_iterate(&iter))!=NULL)
        {
            //reference can be an instance - if it is, then add all its immediate dependencies to our set of dependencies
            if (reference->type==md_reference_type_instance)
            {
                instance = reference->data.instance;
                if (instance->reference.type==md_reference_type_signal)
                {
                    model->reference_union_sets( instance->dependencies, &dependencies );
                }
            }
        }

        /*b Output the header for the combinatorial process
         */
        output( handle, 1, "//b %s%s combinatorial process\n", code_block->name, has_sync?"__comb":"" );
        output_documentation( model, output, handle, 1, code_block->documentation, 0 );

        /*b Do an always that depends on the dependencies, but skip anything generated by the comb block as it must already only be used if live.
          If there are no dependencies then use an initial block, else do an
         */
        {
            int has_dependencies=0;
            model->reference_set_iterate_start( &dependencies, &iter );
            while ((reference = model->reference_set_iterate(&iter))!=NULL)
            {
                instance = reference->data.instance;
                if ( (instance->reference.type != md_reference_type_signal) ||            // not generated by comb process unless it is a comb signal
                     !model->reference_set_includes(&code_block->effects, instance))      // generated by this comb process
                {
                    if (INSTANCE_IS_BIT_VECTOR_ARRAY(instance))
                    {
                        int i;
                        for (i=0; i<instance->size; i++)
                        {
                            if (!has_dependencies)
                            {
                                output( handle, 1, "always @( //%s%s\n", code_block->name, has_sync?"__comb":"" );
                                has_dependencies = 1;
                            }
                            else
                            {
                                output( handle, -1, " or\n" );
                                output_pop_usage_type( model, output, handle );
                            }
                            if (instance->reference.type==md_reference_type_signal)
                            {
                                output_push_usage_type( model, output, handle, instance->reference.data.signal->usage_type );
                            }
                            else
                            {
                                output_push_usage_type( model, output, handle, instance->reference.data.state->usage_type );
                            }
                            if (options.vmod_mode && !(instance->output_args[output_arg_vmod_is_indexed_by_runtime]))
                            {
                                output( handle, 2, "%s__%d", instance->output_name, i );
                            }
                            else
                            {
                                output( handle, 2, "%s[%d]", instance->output_name, i );
                            }
                            first = 0;
                        }
                        output( handle, 2, "//%s - Xilinx does not want arrays in sensitivity lists\n", instance->output_name );
                    }
                    else
                    {
                        if (!has_dependencies)
                        {
                            output( handle, 1, "always @( //%s%s\n", code_block->name, has_sync?"__comb":"" );
                            has_dependencies = 1;
                        }
                        else
                        {
                            output( handle, -1, " or\n" );
                            output_pop_usage_type( model, output, handle );
                        }
                        if (instance->reference.type==md_reference_type_signal)
                        {
                            output_push_usage_type( model, output, handle, instance->reference.data.signal->usage_type );
                        }
                        else
                        {
                            output_push_usage_type( model, output, handle, instance->reference.data.state->usage_type );
                        }
                        output( handle, 2, "%s", instance->output_name );
                        first = 0;
                    }
                }
            }
            if (has_dependencies)
            {
                output_pop_usage_type( model, output, handle );
                output( handle, -1, " )\n" );
            }
            else
            {
                output( handle, 1, "initial //%s%s\n", code_block->name, has_sync?"__comb":"" );
                model->error->add_error( NULL, error_level_serious, error_number_general_error_ss, 0,
                                         error_arg_type_const_string, "Verilog backend: initial statement required on output, not supported for synthesis, code block ",
                                         error_arg_type_malloc_string, code_block->name,
                                         error_arg_type_malloc_filename, code_block->module->name,
                                         error_arg_type_none );
            }
        }

        /*b Output registers if required, and clear set the relevant combinatorial instances of the module to require and use verilog variables in the comb process
         */
        t_md_signal *signal;
        output( handle, 1, "begin: %s__comb_code\n", code_block->name );
        for (signal=module->combinatorials; signal; signal=signal->next_in_list)
        {
            int i;
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                signal->instance_iter->children[i]->output_args[output_arg_use_comb_reg] = 0;
            }
        }
        model->reference_set_iterate_start( &code_block->effects, &iter );
        while ((reference = model->reference_set_iterate(&iter))!=NULL)
        {
            if (reference->type==md_reference_type_instance)
            {
                instance = reference->data.instance;
                if ((instance->reference.type==md_reference_type_signal) && (instance->number_statements>1))
                {
                    instance->output_args[output_arg_use_comb_reg] = 1;
                    output_signal_definition( model, output, handle, module, instance, "reg", options.verilog_comb_reg_suffix, "combinatorial signal array", NULL );
                }
            }
        }

        /*b Output VMOD unique variables if required
         */
        if (options.vmod_mode)
        {
            t_md_lvar *lvar;
            for (lvar=module->lvars; lvar; lvar=lvar->next_in_module)
            {
                lvar->instance->output_args[output_arg_vmod_has_been_declared] = 0;
            }
            output_module_rtl_architecture_statement_vmod_regs( model, output, handle, code_block, code_block->first_statement, 1, NULL, -1, NULL, -1 );
        }

        /*b Output the code for the block
         */
        if (0) // Skip this as we now have liveness checking which obviates the need for this
        {
            model->reference_set_iterate_start( &code_block->effects, &iter );
            while ((reference = model->reference_set_iterate(&iter))!=NULL)
            {
                if (reference->type==md_reference_type_instance)
                {
                    instance = reference->data.instance;
                    if ((instance->reference.type==md_reference_type_signal) && (instance->output_args[output_arg_use_comb_reg]))
                    {
                        if (instance->type==md_type_instance_type_array_of_bit_vectors)
                        {
                            if (options.vmod_mode && INSTANCE_IS_BIT_VECTOR_ARRAY(instance) && !(instance->output_args[output_arg_vmod_is_indexed_by_runtime]))
                            {
                                int i;
                                for (i=0; i<instance->size; i++)
                                {
                                    output( handle, 2, "%s%s__%d = %s__%d\n", instance->output_name, options.verilog_comb_reg_suffix, i, instance->output_name, i );
                                }
                            }
                            else
                            {
                                output( handle, 2, "begin:__read__%s__iter integer __iter; for (__iter=0; __iter<%d; __iter=__iter+1) %s%s[__iter] = %s[__iter]; end\n", instance->output_name, instance->size, instance->output_name, options.verilog_comb_reg_suffix, instance->output_name );
                            }
                        }
                        else
                        {
                            output( handle, 2, "%s%s = %s;\n", instance->output_name, options.verilog_comb_reg_suffix, instance->output_name );
                        }
                    }
                }
            }
        }
        output_module_rtl_architecture_statement( model, output, handle, code_block, code_block->first_statement, 1, NULL, -1, NULL, -1 );
        model->reference_set_iterate_start( &code_block->effects, &iter );
        while ((reference = model->reference_set_iterate(&iter))!=NULL)
        {
            if (reference->type==md_reference_type_instance)
            {
                instance = reference->data.instance;
                if ((instance->reference.type==md_reference_type_signal) && (instance->output_args[output_arg_use_comb_reg]))
                {
                    output_push_usage_type( model, output, handle, instance->reference.data.signal->usage_type );
                    if (instance->type==md_type_instance_type_array_of_bit_vectors)
                    {
                        if (options.vmod_mode && INSTANCE_IS_BIT_VECTOR_ARRAY(instance) && !(instance->output_args[output_arg_vmod_is_indexed_by_runtime]))
                        {
                            int i;
                            for (i=0; i<instance->size; i++)
                            {
                                output( handle, 2, "%s__%d = %s%s__%d;\n", instance->output_name, i, instance->output_name, options.verilog_comb_reg_suffix, i );
                            }
                        }
                        else
                        {
                            output( handle, 2, "begin:__set__%s__iter integer __iter; for (__iter=0; __iter<%d; __iter=__iter+1) %s[__iter] = %s%s[__iter]; end\n", instance->output_name, instance->size, instance->output_name, instance->output_name, options.verilog_comb_reg_suffix );
                        }
                    }
                    else
                    {
                        output( handle, 2, "%s = %s%s;\n", instance->output_name, instance->output_name, options.verilog_comb_reg_suffix );
                    }
                    output_pop_usage_type( model, output, handle );
                }
            }
        }
        for (signal=module->combinatorials; signal; signal=signal->next_in_list)
        {
            int i;
            for (i=0; i<signal->instance_iter->number_children; i++)
            {
                signal->instance_iter->children[i]->output_args[output_arg_use_comb_reg] = 0;
            }
        }
        output_set_usage_type( model, output, handle );
        output( handle, 1, "end //always\n");
        output( handle, 0, "\n" );

        /*b Free the dependencies set
         */
        model->reference_set_free( &dependencies );
    }

    /*b Output clocked processes if required
     */
    if (has_sync)
    {
        model->reference_set_iterate_start( &code_block->effects, &iter );
        while ((reference = model->reference_set_iterate(&iter))!=NULL)
        {
            if (reference->type!=md_reference_type_instance)
                continue;
            instance = reference->data.instance;
            if (instance->reference.type==md_reference_type_state)
            {
                if (instance->output_args[output_arg_state_has_reset])
                    continue;
                clk = instance->reference.data.state->clock_ref;
                edge = instance->reference.data.state->edge;
                reset = instance->reference.data.state->reset_ref;
                reset_level = instance->reference.data.state->reset_level;
                char buffer[1024];
                if (has_many_sync || has_comb)
                {
                    sprintf( buffer, "%s__%s_%s_%s_%s", code_block->name, edge_name[edge], clk->name, level_name[reset_level], reset->name );
                }
                else
                {
                    sprintf( buffer, "%s", code_block->name );
                }
                output( handle, 1, "//b %s clock process\n", buffer );
                output_documentation( model, output, handle, 1, code_block->documentation, 0 );
                output( handle, 1, "always @( %s %s or %s %s)\n", edge_name[edge], clk->name, edge_name[!reset_level], reset->name );
                output( handle, 1, "begin : %s__code\n", buffer);
                if (options.vmod_mode)
                {
                    t_md_lvar *lvar;
                    for (lvar=module->lvars; lvar; lvar=lvar->next_in_module)
                    {
                        lvar->instance->output_args[output_arg_vmod_has_been_declared] = 0;
                    }
                    output_module_rtl_architecture_statement_vmod_regs( model, output, handle, code_block, code_block->first_statement, 2, clk, edge, reset, reset_level );
                }
                output( handle, 2, "if (%s==1'b%d)\n", reset->name, reset_level );
                output( handle, 2, "begin\n");
                model->reference_set_iterate_start( &code_block->effects, &iter2 );
                while ((reference2 = model->reference_set_iterate(&iter2))!=NULL)
                {
                    if (reference2->type!=md_reference_type_instance)
                        continue;
                    instance2 = reference2->data.instance;
                    if ( (instance2->reference.type == md_reference_type_state) &&
                         (instance2->reference.data.state->clock_ref == clk) &&
                         (instance2->reference.data.state->edge == edge) &&
                         (instance2->reference.data.state->reset_ref == reset) &&
                         (instance2->reference.data.state->reset_level == reset_level) )
                    {
                        int j, array_size;
                        t_md_type_instance_data *ptr;
                        output_push_usage_type( model, output, handle, instance2->reference.data.state->usage_type );
                        array_size = instance2->size;
                        for (j=0; j<(array_size>0?array_size:1); j++)
                        {
                            for (ptr = &instance2->data[j]; ptr; ptr=ptr->reset_value.next_in_list)
                            {
                                if (ptr->reset_value.expression)
                                {
                                    if (array_size>1)
                                    {
                                        t_md_lvar lvar;
                                        lvar.instance = instance2;
                                        lvar.index.type = md_lvar_data_type_integer;
                                        lvar.index.data.integer = j;
                                        lvar.subscript_start.type = md_lvar_data_type_none;
                                        lvar.output.uniquified_name = NULL;
                                        lvar.instance_type = md_lvar_instance_type_state;
                                        lvar.instance_data.state = instance2->reference.data.state;
                                        output_module_rtl_architecture_lvar( model, output, handle, NULL /* no code_block */, &lvar, 3, 0, rtl_lvar_out_ignore_none, 0 );
                                        output( handle, -1, " <= " );
                                        output_module_rtl_architecture_expression( model, output, handle, NULL, ptr->reset_value.expression, 3, 0, 0 );
                                        output( handle, -1, ";\n" );
                                    }
                                    else
                                    {
                                        if (ptr->reset_value.subscript_start>=0)
                                        {
                                            if (ptr->reset_value.subscript_end>=0)
                                            {
                                                output( handle, 3, "%s[%d:%d] <= ", instance2->output_name,
                                                        ptr->reset_value.subscript_end-1 - ptr->reset_value.subscript_start,
                                                        ptr->reset_value.subscript_start );
                                            }
                                            else
                                            {
                                                output( handle, 3, "%s[%d] <= ", instance2->output_name,
                                                        ptr->reset_value.subscript_start );
                                            }
                                            output_module_rtl_architecture_expression( model, output, handle, NULL, ptr->reset_value.expression, 3, 0, 0 );
                                            output( handle, -1, "; // Should this be a bit vector?\n"); // %d %p %p %d %d, j, instance2, ptr, ptr->reset_value.subscript_start,ptr->reset_value.subscript_end );
                                        }
                                        else
                                        {
                                            output( handle, 3, "%s <= ", instance2->output_name );
                                            output_module_rtl_architecture_expression( model, output, handle, NULL, ptr->reset_value.expression, 3, 0, 0 );
                                            output( handle, -1, ";\n");
                                        }
                                    }
                                }
                            }
                        }
                        instance2->output_args[output_arg_state_has_reset] = 1;
                        output_pop_usage_type( model, output, handle );
                    }
                }
                output_set_usage_type( model, output, handle );
                output( handle, 2, "end\n");
                output( handle, 2, "else\n", clk->name, clk->name);
                output( handle, 2, "begin\n", clk->name, clk->name);
                output_module_rtl_architecture_statement( model, output, handle, code_block, code_block->first_statement, 2, clk, edge, reset, reset_level );
                output( handle, 2, "end //if\n" );
                output( handle, 1, "end //always\n");
                output( handle, 0, "\n" );
            }
        }
    }
}

/*f output_module_rtl_architecture_instance - output instances of modules with definitions; ports as clocks, inputs, outputs
 */
static void output_module_rtl_architecture_instance( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module, t_md_module_instance *module_instance )
{
    t_md_module_instance_clock_port *clock_port;
    t_md_module_instance_input_port *input_port;
    t_md_module_instance_output_port *output_port;
    int need_comma;

    /*b If module_instance is not defined then done
     */
    if ( (!module_instance->module_definition) )
    {
        return;
    }
    output( handle, 1, "%s %s(\n", module_instance->output_type, module_instance->name );
    need_comma = 0;

    for (clock_port=module_instance->clocks; clock_port; clock_port=clock_port->next_in_list)
    {
        if (need_comma) output( handle, -1, ",\n" );
        output( handle, 2, ".%s(%s)", clock_port->port_name, clock_port->clock_name );
        need_comma = 1;
    }

    for (input_port=module_instance->inputs; input_port; input_port=input_port->next_in_list)
    {
        if (need_comma) output( handle, -1, ",\n" );
        output( handle, 2, ".%s(", input_port->module_port_instance->output_name );
        output_module_rtl_architecture_expression( model, output, handle, NULL /*no code block*/, input_port->expression, 3, 0, 0 );
        output( handle, -1, ")" );
        need_comma = 1;
    }
    if (options.vmod_mode)
    {
        for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
        {
            if (need_comma) output( handle, -1, ",\n" );
            output( handle, 2, ".%s( vmod_net__%s__%s )", output_port->module_port_instance->output_name, module_instance->name, output_port->module_port_instance->output_name );
            need_comma = 1;
        }
    }
    else
    {
        for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
        {
            if (need_comma) output( handle, -1, ",\n" );
            output( handle, 2, ".%s(", output_port->module_port_instance->output_name );
            output_module_rtl_architecture_lvar( model, output, handle, NULL /* no code_block */, output_port->lvar, 3, 0, rtl_lvar_out_ignore_none, 0 );
            output( handle, -1, ")" );
            need_comma = 1;
        }
    }
    output( handle, 2, " );\n" );
    if (options.vmod_mode)
    {
        output( handle, -1, "\n" );
        output( handle, 1, "always @( ");
        for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
        {
            output( handle, -1, "vmod_net__%s__%s", module_instance->name, output_port->module_port_instance->output_name );
            if (output_port->next_in_list)
            {
                output( handle, -1, " or\n" );
                output( handle, 3, "" );
            }
        }
        output( handle, -1, ")\n" );
        output( handle, 1, "begin : %s__module_output_code\n", module_instance->name );
        for (output_port=module_instance->outputs; output_port; output_port=output_port->next_in_list)
        {
            output_module_rtl_architecture_lvar( model, output, handle, NULL /* no code_block */, output_port->lvar, 2, 0, rtl_lvar_out_ignore_none, 0 );
            output( handle, -1, " = vmod_net__%s__%s;\n", module_instance->name, output_port->module_port_instance->output_name );
        }
        output( handle, 1, "end\n\n" );
    }
}

/*f output_module_rtl_architecture - Output a module; ports, components, internal signals, instances, code blocks
 */
static char *md_unique_name( c_model_descriptor *model, t_md_module *module, t_md_lvar *lvar )
{
    char buffer[512];
    snprintf( buffer, sizeof(buffer), "%s__uniq", lvar->instance->output_name);
    return sl_str_alloc_copy( buffer );
}
static void output_module_rtl_architecture( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module )
{
    t_md_code_block *code_block;
    t_md_module_instance *module_instance;

    /*b Output module header and documentation
     */
    output( handle, 0, "//a Module %s\n", module->output_name );
    output_documentation( model, output, handle, 0, module->documentation, 0 );
    output( handle, 0, "module %s\n", module->output_name );

    /*b Check every type instance to see if it is indexed with a non-constant (i.e. an lvar uses instance[variable])
     */
    {
        t_md_type_instance *instance;
        for (instance=module->instances; instance; instance=instance->next_in_module)
        {
            instance->output_args[output_arg_vmod_is_indexed_by_runtime] = 0;
        }
    }
    {
        t_md_lvar *lvar;
        for (lvar=module->lvars; lvar; lvar=lvar->next_in_module)
        {
            if (lvar->index.type == md_lvar_data_type_expression)
            {
                //fprintf(stderr,"Instance %s/%s is indexed with an expression by lvar %p\n", lvar->instance->output_name, lvar->instance->name, lvar );
                lvar->instance->output_args[output_arg_vmod_is_indexed_by_runtime] = 1;
            }
        }
        t_md_type_instance *instance;
        for (instance=module->instances; instance; instance=instance->next_in_module)
        {
            if (options.vmod_mode && INSTANCE_IS_BIT_VECTOR_ARRAY(instance))
            {
                if (instance->output_args[output_arg_vmod_is_indexed_by_runtime])
                {
                    fprintf(stderr,"INFO FOR VMOD:Instance %s[%d] is indexed at runtime\n", instance->output_name, instance->size );
                }
                else
                {
                    fprintf(stderr,"INFO FOR VMOD:Instance %s[%d] is indexed only statically\n", instance->output_name, instance->size );
                }
            }
        }
    }

    /*b Assign unique names to each lvar in the module
     */
    {
        t_md_lvar *lvar;
        for (lvar=module->lvars; lvar; lvar=lvar->next_in_module)
        {
            lvar->output.uniquified_name = NULL;
            if (options.vmod_mode && (lvar->index.type != md_lvar_data_type_none) && (lvar->subscript_start.type != md_lvar_data_type_none))
            {
                char buffer[1024];
                char *unique_name;
                unique_name = md_unique_name( model, module, lvar );
                snprintf( buffer, sizeof(buffer), "%s%s", unique_name, options.verilog_comb_reg_suffix );
                lvar->output.uniquified_name = sl_str_alloc_copy( buffer );
                if (unique_name) free(unique_name);
            }
        }
    }
    module->output.unique_id=0;

    /*b Output ports, components and internal signals
     */
    output_module_rtl_architecture_ports( model, output, handle, module );
    output_module_rtl_architecture_components( model, output, handle, module );
    output_module_rtl_architecture_internal_signals( model, output, handle, module );

    /*b Output clock gate instantiations
     */
    output( handle, 1, "//b Clock gating module instances\n" );
    {
        t_md_signal *clk;
        for (clk=module->clocks; clk; clk=clk->next_in_list)
        {
            if (CLOCK_IS_GATED(clk))
            {
                output( handle, 1, "%s %s__gen( .CLK_IN(%s), .ENABLE(%s%s), .CLK_OUT(%s)%s );\n",
                        options.clock_gate_module_instance_type,
                        clk->name,
                        clk->data.clock.clock_ref->name,
                        clk->data.clock.gate_level?"":"!",
                        clk->data.clock.gate_state?clk->data.clock.gate_state->name:clk->data.clock.gate_signal->name,
                        clk->name,
                        options.clock_gate_module_instance_extra_ports );
            }
        }
    }

    /*b Output module instantiations
     */
    output( handle, 1, "//b Module instances\n" );
    for (module_instance=module->module_instances; module_instance; module_instance=module_instance->next_in_list)
    {
        output_module_rtl_architecture_instance( model, output, handle, module, module_instance );
    }

    /*b Output code blocks
     */
    for (code_block = module->code_blocks; code_block; code_block=code_block->next_in_list)
    {
        output_module_rtl_architecture_code_block( model, output, handle, module, code_block );
    }


    output( handle, 0, "endmodule // %s\n", module->output_name );
}

/*f output_modules - run thrugh the module list, and if not externally defined, call output_module_rtl_architecture
 */
static void output_modules( c_model_descriptor *model, t_md_output_fn output, void *handle )
{
    t_md_module *module;

    for (module=model->module_list; module; module=module->next_in_list)
    {
        if (module->external)
            continue;
        output_module_rtl_architecture( model, output, handle, module );
    }
}

/*a External functions
 */
extern void target_verilog_output( c_model_descriptor *model, t_md_output_fn output_fn, void *output_handle, t_md_verilog_options *options_in )
{
    options.vmod_mode=0;
    options.include_displays = 0;
    options.include_assertions = 0;
    options.include_coverage = 0;
    options.clock_gate_module_instance_type = "clock_gate_module";
    options.clock_gate_module_instance_extra_ports = "";
    options.assert_delay_string = NULL;
    options.verilog_comb_reg_suffix = "__var";

    if (options_in)
    {
        options.vmod_mode = options_in->vmod_mode;
        options.include_displays = options_in->include_displays;
        options.include_assertions = options_in->include_assertions;
        options.include_coverage = options_in->include_coverage;
        if (options_in->clock_gate_module_instance_extra_ports) { options.clock_gate_module_instance_extra_ports = options_in->clock_gate_module_instance_extra_ports; }
        if (options_in->clock_gate_module_instance_type)        { options.clock_gate_module_instance_type = options_in->clock_gate_module_instance_type; }
        if (options_in->assert_delay_string)                    { options.assert_delay_string = options_in->assert_delay_string; }
        if (options_in->verilog_comb_reg_suffix)                { options.verilog_comb_reg_suffix = options_in->verilog_comb_reg_suffix; }
    }

    current_output_depth = 0;
    output_usage_type = md_usage_type_rtl;
    desired_output_usage_type = md_usage_type_rtl;
    output_header( model, output_fn, output_handle );
    output_modules( model, output_fn, output_handle );
}

/*a To do
  Output constants - whatever they may be!
 */

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


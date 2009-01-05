/*a Copyright
  
  This file 'md_target_vhdl.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "c_model_descriptor.h"

/*a Static variables
 */
static char *edge_name[] = {
     "posedge",
     "negedge",
     "bug in use of edge!!!!!"
};
static char *level_name[] = {
     "active_low",
     "active_high",
     "bug in use of level!!!!!"
};

/*a Forward function declarations
 */
static void output_module_rtl_architecture_expression( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_expression *expr, int main_indent, int sub_indent );
static void output_module_rtl_architecture_statement( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *signal, int edge );

/*a Output functions
 */
/*f output_header
 */
static void output_header( c_model_descriptor *model, t_md_output_fn output, void *handle )
{

    output( handle, 0, "--a Note: created by cyclicity AHDL - do not hand edit\n");
    output( handle, 0, "\n");
    output( handle, 0, "--a Libraries\n");
    output( handle, 0, "LIBRARY ieee;\n");
    output( handle, 0, "USE ieee.std_logic_1164.all;\n");
    output( handle, 0, "USE ieee.std_logic_unsigned.all;\n");
    output( handle, 0, "\n");

}

/*f output_module_entity
 */
static void output_module_entity( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module )
{
    t_md_signal *clk;
    t_md_signal *signal;
    t_md_type_instance *instance;
    int i, last;

    output( handle, 0, "--a Module entity %s\n", module->name );
    output( handle, 0, "ENTITY %s IS\n", module->name );
    output( handle, 1, "PORT (\n" );
    for (clk=module->clocks; clk; clk=clk->next_in_list)
    {
        output( handle, 2, "%s : IN std_logic;\n", clk->name );
    }

    output( handle, 0, "\n");

    for (signal=module->inputs; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            last = (i==signal->instance_iter->number_children-1) && (!signal->next_in_list) && (!module->outputs);
            instance = signal->instance_iter->children[i];
            if (instance->type == md_type_instance_type_bit_vector)
            {
                if (instance->type_def.data.width==1)
                {
                    output( handle, 2, "%s : IN std_logic%s", instance->output_name, last?"\n":";\n" );
                }
                else
                {
                    output( handle, 2, "%s : IN std_logic_vector( %d DOWNTO 0 )%s", instance->output_name, instance->type_def.data.width-1, last?"\n":";\n" );
                }
            }
            else
            {
                output( handle, 1, "ZZZ Don't know how to register an input array '%s'\n", instance->output_name );
            }
        }
    }
    output( handle, 0, "\n");

    for (signal=module->outputs; signal; signal=signal->next_in_list)
    {
        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            last = (i==signal->instance_iter->number_children-1) && (!signal->next_in_list);
            instance = signal->instance_iter->children[i];
            if (instance->type == md_type_instance_type_bit_vector)
            {
                if (instance->type_def.data.width==1)
                {
                    output( handle, 2, "%s : OUT std_logic%s", instance->output_name, last?"\n":";\n" );
                }
                else
                {
                    output( handle, 2, "%s : OUT std_logic_vector( %d DOWNTO 0 )%s", instance->output_name, instance->type_def.data.width-1, last?"\n":";\n" );
                }
            }
            else
            {
                output( handle, 1, "ZZZ Don't know how to register an output array '%s'\n", instance->output_name );
            }
        }
    }

    output( handle, 1, ");\n" );
    output( handle, 0, "END %s;\n", module->name );
    output( handle, 0, "\n" );
}

/*f output_module_rtl_architecture_components
 */
static void output_module_rtl_architecture_components( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module )
{
     output( handle, 0, "-- output components here\n" );
     output( handle, 0, "\n" );
}

/*f output_module_rtl_architecture_lvar
 */
static void output_module_rtl_architecture_lvar( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_lvar *lvar, int main_indent, int sub_indent )
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
                output( handle, main_indent, "%s", lvar->instance->output_name );
            }
            else
            {
                output( handle, main_indent, "<unresolved output %s>", signal->name );
            }
            break;
        case md_signal_type_combinatorial:
            output( handle, main_indent, "%s", lvar->instance->output_name );
            break;
        default:
            output( handle, main_indent, "<clock signal in expression %s>", signal->name );
            break;
        }
    }
    if (lvar->subscript_start.type != md_lvar_data_type_none)
    {
        output( handle, -1, "[" );
        if (lvar->subscript_length.type != md_lvar_data_type_none)
        {
            if (lvar->subscript_start.type == md_lvar_data_type_integer)
            {
                output( handle, -1, "%d:", lvar->subscript_start.data.integer+lvar->subscript_length.data.integer-1 );
            }
            else if (lvar->subscript_start.type == md_lvar_data_type_expression)
            {
                output( handle, -1, "%d+(", lvar->subscript_length.data.integer-1 );
                output_module_rtl_architecture_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, main_indent, sub_indent+1 );
                output( handle, -1, "):" );
            }
        }
        if (lvar->subscript_start.type == md_lvar_data_type_integer)
        {
            output( handle, -1, "%d]", lvar->subscript_start.data.integer );
        }
        else if (lvar->subscript_start.type == md_lvar_data_type_expression)
        {
            output_module_rtl_architecture_expression( model, output, handle, code_block, lvar->subscript_start.data.expression, main_indent, sub_indent+1 );
            output( handle, -1, "]" );
        }
    }
}

/*f output_module_rtl_architecture_expression
 */
static void output_module_rtl_architecture_expression( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_expression *expr, int main_indent, int sub_indent )
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
        output( handle, -1, "16#%x#", expr->data.value.value.value[0] ); //expr->data.value.value.width, 
        break;
    case md_expr_type_lvar:
        output_module_rtl_architecture_lvar( model, output, handle , code_block, expr->data.lvar, -1, sub_indent );
        break;
    case md_expr_type_cast:
        // All casts should be implicit in VHDL (?)
        output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.cast.expression, main_indent, sub_indent+1 );
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
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1 );
            break;
        case md_expr_fn_add:
        case md_expr_fn_sub:
            output( handle, -1, "(" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1 );
            if (expr->data.fn.fn==md_expr_fn_sub)
                output( handle, -1, "-" );
            else
                output( handle, -1, "+" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent+1 );
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
            output( handle, -1, "(" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[0], main_indent, sub_indent+1 );
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
                output( handle, -1, "=" );
            else
                output( handle, -1, "<>" );
            output_module_rtl_architecture_expression( model, output, handle, code_block, expr->data.fn.args[1], main_indent, sub_indent+1 );
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

/*f output_module_rtl_architecture_internal_signals
 */
static void output_module_rtl_architecture_internal_signals( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module )
{
    t_md_signal *signal;
    t_md_state *reg;
    t_md_type_instance *instance;
    int i;

    output( handle, 1, "--a Internal registers\n" );
    for (reg=module->registers; reg; reg=reg->next_in_list)
    {
        if (reg->output_ref)
            continue;
        if (reg->instance->type!=md_type_instance_type_bit_vector)
        {
            output( handle, 1, "UNSUPPORTED TYPES;\n", reg->name);
        }
        else if (reg->instance->type_def.data.width==1)
        {
            output( handle, 1, "SIGNAL %s : std_logic;\n", reg->name);
        }
        else
        {
            output( handle, 1, "SIGNAL %s : std_logic_vector(%d DOWNTO 0);\n", reg->name, reg->instance->type_def.data.width-1);
        }
    }
    output( handle, 0, "\n" );

    output( handle, 1, "--a Internal combinatorials\n" );
    for (signal=module->combinatorials; signal; signal=signal->next_in_list)
    {
        if (signal->data.combinatorial.output_ref)
            continue;

        for (i=0; i<signal->instance_iter->number_children; i++)
        {
            instance = signal->instance_iter->children[i];
            if (instance->type == md_type_instance_type_bit_vector)
            {
                if (instance->type_def.data.width==1)
                {
                    output( handle, 2, "SIGNAL %s : std_logic;\n", instance->output_name );
                }
                else
                {
                    output( handle, 2, "SIGNAL %s : std_logic_vector( %d DOWNTO 0 );\n", instance->output_name, instance->type_def.data.width-1 );
                }
            }
            else
            {
                output( handle, 1, "ZZZ Don't know how to register a signal array '%s'\n", instance->output_name );
            }
        }
    }
    output( handle, 0, "\n" );
}

/*f output_module_rtl_architecture_parallel_switch
 */
static void output_module_rtl_architecture_parallel_switch( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *clock, int edge )
{
     if (statement->data.switch_stmt.all_static && statement->data.switch_stmt.all_unmasked)
     {
          t_md_switch_item *switem;
          int defaulted = 0;

          output( handle, indent+1, "case (");
          output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.switch_stmt.expr, indent+1, 4 );
          output( handle, -1, ") of \n");
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
                       output( handle, indent+1, "when 32h%x: // req %d\n", switem->data.value.value.value[0], stmts_reqd );
                       output_module_rtl_architecture_statement( model, output, handle, code_block, switem->statement, indent+1, clock, edge );
                   }
               }
               else if (switem->type == md_switch_item_type_default)
               {
                   if (stmts_reqd || statement->data.switch_stmt.full)
                   {
                       output( handle, indent+1, "default: // req %d\n", stmts_reqd);
                       output_module_rtl_architecture_statement( model, output, handle, code_block, switem->statement, indent+1, clock, edge );
                       defaulted = 1;
                   }
               }
               else
               {
                    fprintf( stderr, "BUG - non static unmasked case item in parallel static unmasked switch C output\n" );
               }
          }
          if (!defaulted && statement->data.switch_stmt.full)
          {
               output( handle, indent+1, "default:\n");
               output( handle, indent+2, "//ASSERT(0, \"Full switch statement did not cover all values\");\n");
          }
          output( handle, indent+1, "endcase;\n");
     }
     else
     {
          output( handle, indent, "!!!PARALLEL SWITCH ONLY WORKS NOW FOR STATIC UNMASKED EXPRESSIONS\n");
     }
}

/*f output_module_rtl_architecture_statement
 */
static void output_module_rtl_architecture_statement( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_code_block *code_block, t_md_statement *statement, int indent, t_md_signal *signal, int edge )
{
    if (!statement)
        return;

    /*b If the statement does not effect this signal/clock/edge, then return with outputting nothing
     */
    if (signal)
    {
        if (!model->reference_set_includes( &statement->effects, signal, edge ))
        {
            output_module_rtl_architecture_statement( model, output, handle, code_block, statement->next_in_code, indent, signal, edge );
            return;
        }
    }
    else
    {
        if (!model->reference_set_includes( &statement->effects, md_reference_type_signal ))
        {
            output_module_rtl_architecture_statement( model, output, handle, code_block, statement->next_in_code, indent, signal, edge );
            return;
        }
    }

    /*b Display the statement
     */
    switch (statement->type)
    {
    case md_statement_type_state_assign:
        if (edge>=0)
        {
            output_module_rtl_architecture_lvar( model, output, handle, code_block, statement->data.state_assign.lvar, indent+1, indent+2 );
            output( handle, -1, " <= " );
            output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.state_assign.expr, indent+1, 0 );
        }
        output( handle, -1, ";\n" );
        break;
    case md_statement_type_comb_assign:
        if (edge<0)
        {
            output_module_rtl_architecture_lvar( model, output, handle, code_block, statement->data.comb_assign.lvar, indent+1, indent+2 );
            output( handle, -1, " <= " );
            output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.comb_assign.expr, indent+1, 0 );
        }
        else
        {
            output( handle, indent+1, "should not be here\n");
            // ERROR
        }
        output( handle, -1, ";\n" );
        break;
    case md_statement_type_if_else:
        output( handle, indent+1, "IF (");
        output_module_rtl_architecture_expression( model, output, handle, code_block, statement->data.if_else.expr, indent+1, 4 );
        output( handle, -1, ") THEN\n");
        if (statement->data.if_else.if_true)
        {
            output_module_rtl_architecture_statement( model, output, handle, code_block, statement->data.if_else.if_true, indent+1, signal, edge );
        }
        if (statement->data.if_else.if_false)
        {
            output( handle, indent+1, "ELSE\n");
            output_module_rtl_architecture_statement( model, output, handle, code_block, statement->data.if_else.if_false, indent+1, signal, edge );
        }
        output( handle, indent+1, "END IF;\n");
        break;
    case md_statement_type_parallel_switch:
        output_module_rtl_architecture_parallel_switch( model, output, handle, code_block, statement, indent, signal, edge );
        break;
    default:
        output( handle, 1, "Unknown statement type %d\n", statement->type );
        break;
    }
    output_module_rtl_architecture_statement( model, output, handle, code_block, statement->next_in_code, indent, signal, edge );
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

    model->reference_set_iterate_start( &code_block->effects, &iter );
    while ((reference = model->reference_set_iterate(&iter))!=NULL)
    {
        instance = reference->data.instance;

        /*b If its a combinatorial, its combinatorial; if its a state its either sync or many_sync too
         */
        switch (instance->reference.type)
        {
        case md_reference_type_signal:
            has_comb = 1;
            break;
        case md_reference_type_state:
            instance->output_handle = NULL;
            instance->output_args[0] = 0;
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
     */
    if (has_comb)
    {
        output( handle, 1, "--b %s%s combinatorial process\n", code_block->name, has_sync?"__comb":"" );
        output( handle, 1, "PROCESS ( --%s%s\n", code_block->name, has_sync?"__comb":"" );
        model->reference_set_iterate_start( &code_block->dependencies, &iter );
        first = 1;
        while ((reference = model->reference_set_iterate(&iter))!=NULL)
        {
            instance = reference->data.instance;
            if (instance->reference.type==md_reference_type_signal)
            {
                if (!first)
                    output( handle, -1, ",\n" );
                output( handle, 2, "%s", instance->output_name );
                first = 0;
            }
        }
        output( handle, -1, " )\n" );
        output( handle, 1, "BEGIN\n");
        output_module_rtl_architecture_statement( model, output, handle, code_block, code_block->first_statement, 1, NULL, -1 );
        output( handle, 1, "END PROCESS;\n");
        output( handle, 0, "\n" );
    }

    /*b Output clocked processes if required
     */
    if (has_sync)
    {
        model->reference_set_iterate_start( &code_block->effects, &iter );
        while ((reference = model->reference_set_iterate(&iter))!=NULL)
        {
            instance = reference->data.instance;
            if (instance->reference.type==md_reference_type_state)
            {
                if (instance->output_args[0])
                {
                    continue;
                }
                clk = instance->reference.data.state->clock_ref;
                edge = instance->reference.data.state->edge;
                reset = instance->reference.data.state->reset_ref;
                reset_level = instance->reference.data.state->reset_level;
                if (has_many_sync || has_comb)
                {
                    output( handle, 1, "--b %s__%s_%s_%s_%s clock process\n", code_block->name, edge_name[edge], clk->name, level_name[reset_level], reset->name );
                    output( handle, 1, "PROCESS (%s, %s)\n", clk->name, reset->name );
                }
                else
                {
                    output( handle, 1, "--b %s clock process\n", code_block->name, clk->name, reset->name );
                    output( handle, 1, "PROCESS (%s, %s)\n", clk->name, reset->name );
                }
                output( handle, 1, "BEGIN\n");
                output( handle, 2, "IF %s='%d' THEN\n", reset->name, reset_level );
                model->reference_set_iterate_start( &code_block->effects, &iter2 );
                while ((reference2 = model->reference_set_iterate(&iter2))!=NULL)
                {
                    instance2 = reference2->data.instance;
                    if ( (instance2->reference.type == md_reference_type_state) &&
                         (instance2->reference.data.state->clock_ref == clk) &&
                         (instance2->reference.data.state->edge == edge) &&
                         (instance2->reference.data.state->reset_ref == reset) &&
                         (instance2->reference.data.state->reset_level == reset_level) )
                    {
                        if (instance2->type!=md_type_instance_type_bit_vector)
                        {
                        }
                        else
                        {
                            output( handle, 3, "%s <= ", instance2->output_name );
                            output_module_rtl_architecture_expression( model, output, handle, NULL, instance2->data[0].reset_value, 3, 0 );
                            output( handle, -1, ";\n" );
                        }
                        instance2->output_args[0] = 1;
                    }
                }
                output( handle, 2, "ELSIF (%s='1' AND %s'event) THEN\n", clk->name, clk->name);
                output_module_rtl_architecture_statement( model, output, handle, code_block, code_block->first_statement, 2, clk, edge );
                output( handle, 2, "END IF;\n");
                output( handle, 1, "END PROCESS;\n");
                output( handle, 0, "\n" );
            }
        }
    }
}

/*f output_module_rtl_architecture
 */
static void output_module_rtl_architecture( c_model_descriptor *model, t_md_output_fn output, void *handle, t_md_module *module )
{
    t_md_code_block *code_block;

    output( handle, 0, "--a Module architecture RTL %s\n", module->name );
    output( handle, 0, "ARCHITECTURE rtl OF %s IS\n", module->name );

    output_module_rtl_architecture_components( model, output, handle, module );
    output_module_rtl_architecture_internal_signals( model, output, handle, module );

    output( handle, 0, "BEGIN\n", module->name );

    for (code_block = module->code_blocks; code_block; code_block=code_block->next_in_list)
    {
        output_module_rtl_architecture_code_block( model, output, handle, module, code_block );
    }

    output( handle, 0, "END %s;\n", module->name );
}

/*f output_modules
 */
static void output_modules( c_model_descriptor *model, t_md_output_fn output, void *handle )
{
    t_md_module *tm;

    tm = model->get_toplevel_module();

    output_module_entity( model, output, handle, tm );
    output_module_rtl_architecture( model, output, handle, tm );
}

/*a External functions
 */
extern void target_vhdl_output( c_model_descriptor *model, t_md_output_fn output_fn, void *output_handle )
{
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


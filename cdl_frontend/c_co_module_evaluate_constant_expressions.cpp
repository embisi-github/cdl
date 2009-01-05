/*a Copyright
  
  This file 'c_co_module_evaluate_constant_expressions.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include <stdlib.h>
#include <stdio.h>
#include "c_type_value_pool.h"
#include "c_lexical_analyzer.h"
#include "c_cyclicity.h"
#include "cyclicity_grammar.h"
#include "sl_debug.h"

#include "c_co_case_entry.h"
#include "c_co_clock_reset_defn.h"
#include "c_co_code_label.h"
#include "c_co_constant_declaration.h"
#include "c_co_enum_identifier.h"
#include "c_co_expression.h"
#include "c_co_fsm_state.h"
#include "c_co_instantiation.h"
#include "c_co_lvar.h"
#include "c_co_module.h"
#include "c_co_module_body_entry.h"
#include "c_co_named_expression.h"
#include "c_co_nested_assignment.h"
#include "c_co_port_map.h"
#include "c_co_signal_declaration.h"
#include "c_co_sized_int_pair.h"
#include "c_co_statement.h"
#include "c_co_token_union.h"
#include "c_co_toplevel.h"
#include "c_co_type_specifier.h"

/*a c_co_case_entry
 */
/*f c_co_case_entry::evaluate_constant_expressions
 */
void c_co_case_entry::evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    //printf("c_co_case_entry::expression %p evaluate_constant_expressions %d\n", expression, reevaluate);
    if (expression)
    {
        c_co_expression *expr;
        for (expr = expression; expr; expr=(c_co_expression *)(expr->next_in_list))
        {
            expr->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
        }
    }
    if (statements)
    {
        statements->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
    }
    if (next_in_list)
    {
        ((c_co_case_entry *)next_in_list)->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
    }
}

/*a c_co_lvar
 */
/*f c_co_lvar::evaluate_constant_expressions
*/
void c_co_lvar::evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    if (lvar)
    {
        lvar->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
    }
    if (first)
    {
        first->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
    }
    // length must be a constant, and already evaluated
}

/*a c_co_module
 */
/*f c_co_module::evaluate_constant_expressions
  Evaluate constants within expressions within the module body
*/
void c_co_module::evaluate_constant_expressions( c_cyclicity *cyclicity, t_co_scope *types, int reevaluate )
{
    c_co_module_body_entry *combe;

    for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
    {
        switch (combe->type)
        {
        case module_body_entry_type_signal_declaration:
            if (combe->data.signal_declaration)
            {
                combe->data.signal_declaration->evaluate_constant_expressions( cyclicity, types, local_signals, reevaluate );
            }
            break;
        case module_body_entry_type_code_label: // cross reference statements in the module body
            if (combe->data.code_label &&
                combe->data.code_label->statements)
            {
                combe->data.code_label->statements->evaluate_constant_expressions( cyclicity, types, local_signals, reevaluate );
            }
            break;
        case module_body_entry_type_default_clock_defn: // handled in building the local signal array
        case module_body_entry_type_default_reset_defn: // handled in building the local signal array
        case module_body_entry_type_schematic_symbol:
            // Done...
            break;
        }
    }
}

/*a c_co_named_expression
 */
/*f c_co_named_expression::evaluate_constant_expressions
 */
void c_co_named_expression::evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    SL_DEBUG( sl_debug_level_verbose_info, "Evaluating nested assignment %p %d", this, reevaluate );

    if (expression)
    {
        expression->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
    }
}

/*a c_co_nested_assignment
 */
/*f c_co_nested_assignment::evaluate_constant_expressions
 */
void c_co_nested_assignment::evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    SL_DEBUG( sl_debug_level_verbose_info, "Evaluating nested assignment %p %d", this, reevaluate );

    if (nested_assignment)
    {
        nested_assignment->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
    }
    if (expression)
    {
        expression->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
    }
    if (next_in_list)
        ((c_co_nested_assignment *)next_in_list)->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
}

/*a c_co_port_map
 */
/*f c_co_port_map::evaluate_constant_expressions
 */
void c_co_port_map::evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    SL_DEBUG( sl_debug_level_info, "Evaluating constants" );
    switch (type)
    {
    case port_map_type_clock:
        break;
    case port_map_type_input:
        if (expression)
        {
            expression->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
        }
        break;
    case port_map_type_output:
        if (lvar)
        {
            lvar->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
        break;
    }
    if (next_in_list)
    {
        ((c_co_port_map *)next_in_list)->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
    }
}

/*a c_co_signal_declaration
 */
/*f c_co_signal_declaration::evaluate_constant_expressions
  Evaluate constants in type specifier and any reset value (if clocked)
*/
void c_co_signal_declaration::evaluate_constant_expressions( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    if (type_specifier)
    {
        type_specifier->evaluate( cyclicity, types, variables );
    }
    if ( (signal_declaration_type == signal_declaration_type_clocked_output) || 
         (signal_declaration_type == signal_declaration_type_clocked_local) )
    {
        if (data.clocked.reset_value)
        {
            data.clocked.reset_value->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
    }
}

/*a c_co_statement
 */
/*f c_co_statement::evaluate_constant_expressions
  Evaluate any constantes in a statement and any following statements in its list
  For an assignment, this means:
  Cross reference the lvar, and get its type
  Cross reference the expression using that type
  Check the assignment is approriately clocked
*/
void c_co_statement::evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    //printf("statement::type %d evaluate_constant_expressions %d\n", statement_type, reevaluate);
    switch (statement_type)
    {
    case statement_type_assignment:
        if (type_data.assign_stmt.lvar)
        {
            type_data.assign_stmt.lvar->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
        if (type_data.assign_stmt.nested_assignment)
        {
            type_data.assign_stmt.nested_assignment->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
        break;
    case statement_type_if_statement:
        if (type_data.if_stmt.expression)
        {
            type_data.if_stmt.expression->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
        }
        if (type_data.if_stmt.if_true)
        {
            type_data.if_stmt.if_true->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
        if (type_data.if_stmt.if_false)
        {
            type_data.if_stmt.if_false->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
        if (type_data.if_stmt.elsif)
        {
            type_data.if_stmt.elsif->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
        break;
    case statement_type_for_statement:
        // #iterations and first_value MUST be constant; hold off on everything else until the iterator has a value
        if (type_data.for_stmt.iterations)
        {
            type_data.for_stmt.iterations->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
        }
        if (type_data.for_stmt.first_value)
        {
            type_data.for_stmt.first_value->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
        }
        break;
    case statement_type_full_switch_statement:
    case statement_type_partial_switch_statement:
    case statement_type_priority_statement:
        if (type_data.switch_stmt.expression)
        {
            type_data.switch_stmt.expression->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
            if (type_data.switch_stmt.cases)
            {
                type_data.switch_stmt.cases->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
            }
        }
        break;
    case statement_type_block:
        if (type_data.block)
        {
            type_data.block->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
        break;
    case statement_type_print:
    case statement_type_cover:
    case statement_type_assert:
        if (type_data.print_assert_stmt.assertion)
        {
            type_data.print_assert_stmt.assertion->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
        }
        if (type_data.print_assert_stmt.value_list)
        {
            c_co_expression *expr;
            for (expr = type_data.print_assert_stmt.value_list; expr; expr=(c_co_expression *)(expr->next_in_list))
            {
                expr->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
            }
        }
        if (type_data.print_assert_stmt.text_args)
        {
            c_co_expression *expr;
            for (expr = type_data.print_assert_stmt.text_args; expr; expr=(c_co_expression *)(expr->next_in_list))
            {
                expr->evaluate_potential_constant( cyclicity, types, variables, reevaluate );
            }
        }
        if (type_data.print_assert_stmt.statement)
        {
            type_data.print_assert_stmt.statement->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
        break;
    case statement_type_log:
    {
        class c_co_named_expression *arg;
        for (arg = type_data.log_stmt.values; arg; arg=(c_co_named_expression *)(arg->next_in_list))
        {
            arg->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
        }
        break;
    }
    case statement_type_instantiation:
        if (type_data.instantiation)
        {
            if (type_data.instantiation->port_map)
            {
                type_data.instantiation->port_map->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
            }
            if (type_data.instantiation->index)
            {
                type_data.instantiation->index->evaluate_constant( cyclicity, types, variables, reevaluate );
            }
        }
        break;
    }
    if (next_in_list)
    {
        ((c_co_statement *)next_in_list)->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
    }
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



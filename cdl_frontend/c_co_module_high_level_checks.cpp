/*a Copyright
  
  This file 'c_co_module_high_level_checks.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "c_co_instantiation.h"
#include "c_co_lvar.h"
#include "c_co_module.h"
#include "c_co_module_prototype.h"
#include "c_co_module_body_entry.h"
#include "c_co_nested_assignment.h"
#include "c_co_port_map.h"
#include "c_co_signal_declaration.h"
#include "c_co_statement.h"
#include "c_co_token_union.h"
#include "c_co_toplevel.h"
#include "c_co_type_specifier.h"

/*a c_co_case_entry
 */
/*f c_co_case_entry::high_level_checks
 */
void c_co_case_entry::high_level_checks( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    if (statements)
    {
        statements->high_level_checks( cyclicity, types, variables );
    }
    if (next_in_list)
    {
        ((c_co_case_entry *)next_in_list)->high_level_checks( cyclicity, types, variables );
    }
}

/*a c_co_instantiation
 */
/*f c_co_instantiation::high_level_checks
 */
int c_co_instantiation::high_level_checks( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    t_co_scope *port_scope;
    port_scope = NULL;
    if (refers_to.cyc_object->co_type==co_type_module)
        port_scope = refers_to.co_module->port_scope;
    if (refers_to.cyc_object->co_type==co_type_module_prototype)
        port_scope = refers_to.co_module_prototype->port_scope;

    if (port_map)
    {
        return port_map->high_level_checks( cyclicity, types, variables, port_scope );
    }
    return 1;
}

/*a c_co_module
 */
/*f c_co_module::high_level_checks
  Do checks to ensure that local signals are only written in a single code block, and that assignments are clocked only to clocked registers, and so on
*/
void c_co_module::high_level_checks( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types )
{
    c_co_module_body_entry *combe;

    for (combe = body; combe; combe=(c_co_module_body_entry *)combe->next_in_list)
    {
        switch (combe->type)
        {
        case module_body_entry_type_signal_declaration: // no high level checks required
            if (combe->data.signal_declaration)
            {
                combe->data.signal_declaration->high_level_checks( cyclicity, types, local_signals );
            }
            break;
        case module_body_entry_type_code_label: // cross reference statements in the module body
            if (combe->data.code_label &&
                combe->data.code_label->statements)
            {
                combe->data.code_label->statements->high_level_checks( cyclicity, types, local_signals );
            }
            break;
        case module_body_entry_type_default_clock_defn: // no high level checks required
        case module_body_entry_type_default_reset_defn: // no high level checks required
        case module_body_entry_type_schematic_symbol:   // no high level checks required
            // Done...
            break;
        }
    }
}

/*a c_co_port_map
 */
/*f c_co_port_map::high_level_checks
 */
int c_co_port_map::high_level_checks( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_co_scope *port_scope )
{
    if ((port_lvar) && (!port_lvar->signal_declaration))
    {
        cyclicity->set_parse_error( this, co_compile_stage_high_level_checks, "Unresolved port on module instance" );
    }
    if (next_in_list)
    {
        return ((c_co_port_map *)next_in_list)->high_level_checks( cyclicity, types, variables, port_scope );
    }
    return 1;
}

/*a c_co_signal_declaration
 */
/*f c_co_signal_declaration::high_level_checks
 */
void c_co_signal_declaration::high_level_checks( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    /*b If this is a clocked signal, check the reset values are all constant
     */
    if ( (signal_declaration_type == signal_declaration_type_clocked_output) ||
         (signal_declaration_type == signal_declaration_type_clocked_local) )
    {
        SL_DEBUG( sl_debug_level_info, "Should check that reset values are constant\n");
    }
    if (next_in_list)
    {
        ((c_co_signal_declaration *)next_in_list)->high_level_checks( cyclicity, types, variables );
    }
}

/*a c_co_statement
 */
/*f high_level_checks
*/
void c_co_statement::high_level_checks( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    switch (statement_type)
    {
    case statement_type_assignment:
    {
        c_co_lvar *lvar;
        lvar = type_data.assign_stmt.lvar;
        if (lvar)
        {
            if (lvar->signal_declaration)
            {
                switch (lvar->signal_declaration->signal_declaration_type)
                {
                case signal_declaration_type_clocked_local:
                case signal_declaration_type_clocked_output:
                    if (!type_data.assign_stmt.clocked)
                        cyclicity->set_parse_error( this, co_compile_stage_high_level_checks, "Unclocked assignment to a clocked variable '%s'", lex_string_from_terminal( lvar->symbol ) );
                    break;
                case signal_declaration_type_comb_output:
                case signal_declaration_type_comb_local:
                    if (type_data.assign_stmt.clocked)
                        cyclicity->set_parse_error( this, co_compile_stage_high_level_checks, "Clocked assignment to an combinatorial variable '%s'", lex_string_from_terminal( lvar->symbol ) );
                    break;
                case signal_declaration_type_input:
                case signal_declaration_type_clock:
                case signal_declaration_type_parameter:
                    cyclicity->set_parse_error( this, co_compile_stage_high_level_checks, "Assignment to a non-port '%s'", lex_string_from_terminal( lvar->symbol ) );
                    break;
                case signal_declaration_type_net_local:
                case signal_declaration_type_net_output:
                    cyclicity->set_parse_error( this, co_compile_stage_high_level_checks, "Assignment to a net '%s'", lex_string_from_terminal( lvar->symbol ) );
                    break;
                }
            }
        }
        break;
    }
    case statement_type_if_statement:
        if (type_data.if_stmt.if_true)
        {
            type_data.if_stmt.if_true->high_level_checks( cyclicity, types, variables );
        }
        if (type_data.if_stmt.if_false)
        {
            type_data.if_stmt.if_false->high_level_checks( cyclicity, types, variables );
        }
        if (type_data.if_stmt.elsif)
        {
            type_data.if_stmt.elsif->high_level_checks( cyclicity, types, variables );
        }
        break;
    case statement_type_for_statement:
        if (type_data.for_stmt.statement)
        {
            type_data.for_stmt.statement->high_level_checks( cyclicity, types, type_data.for_stmt.scope );
        }
        break;
    case statement_type_full_switch_statement:
    case statement_type_partial_switch_statement:
    case statement_type_priority_statement:
        if (type_data.switch_stmt.expression)
        {
            if (type_data.switch_stmt.cases)
            {
                type_data.switch_stmt.cases->high_level_checks( cyclicity, types, variables );
            }
        }
        break;
    case statement_type_block:
        if (type_data.block)
        {
            type_data.block->high_level_checks( cyclicity, types, variables );
        }
        break;
    case statement_type_print:
    case statement_type_assert:
    case statement_type_cover:
        if (type_data.print_assert_stmt.statement)
        {
            type_data.print_assert_stmt.statement->high_level_checks( cyclicity, types, variables );
        }
        break;
    case statement_type_log: // No statements inside
        break;
    case statement_type_instantiation:
        if (type_data.instantiation)
        {
            type_data.instantiation->high_level_checks( cyclicity, types, variables );
        }
        break;
    }
    if (next_in_list)
    {
        ((c_co_statement *)next_in_list)->high_level_checks( cyclicity, types, variables );
    }
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



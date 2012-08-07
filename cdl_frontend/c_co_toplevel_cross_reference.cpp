/*a Copyright
  
  This file 'c_co_toplevel_cross_reference.cpp' copyright Gavin J Stark 2003, 2004
  
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

#include "c_co_constant_declaration.h"
#include "c_co_enum_definition.h"
#include "c_co_enum_identifier.h"
#include "c_co_expression.h"
#include "c_co_fsm_state.h"
#include "c_co_signal_declaration.h"
#include "c_co_sized_int_pair.h"
#include "c_co_token_union.h"
#include "c_co_toplevel.h"
#include "c_co_type_definition.h"
#include "c_co_type_specifier.h"
#include "c_co_type_struct.h"

/*a c_co_constant_declaration
 */
/*f c_co_constant_declaration::cross_reference
 */
void c_co_constant_declaration::cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
     if (type_specifier)
     {
          type_specifier->cross_reference( cyclicity, types, variables );
          if (expression)
          {
               expression->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_specifier->type_value );
          }
     }
}

/*a c_co_enum_definition
 */
void c_co_enum_definition::cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, c_co_type_definition *type_def )
{
     if (first_index)
     {
          first_index->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
     }
     if (second_index)
     {
          second_index->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
     }
     if (enum_identifier)
     {
          enum_identifier->cross_reference( cyclicity, types, variables, type_def );
     }
}

/*a c_co_enum_identifier
 */
/*f c_co_enum_identifier::cross_reference
 */
void c_co_enum_identifier::cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, c_co_type_definition *type_def )
{
    this->type_def = type_def;
    if (expression)
    {
        expression->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_undefined );
    }
    if (next_in_list)
    {
        ((c_co_enum_identifier *)next_in_list)->cross_reference( cyclicity, types, variables, type_def );
    }
}

/*a c_co_fsm_state
 */
/*f c_co_fsm_state::cross_reference
 */
void c_co_fsm_state::cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, c_co_type_definition *type_def )
{
    c_co_fsm_state *state;
    int has_expression;

    has_expression = 0;
    if (expression)
    {
        expression->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_undefined );
        has_expression = 1;
        fprintf(stderr, "**** WARNING - expressions given to FSM declarations are NOT checked for being suitably in range or one-hot; please do not specify values for states in FSMs unless absolutely required\nNYI:c_co_fsm_state::cross_reference:Expressions not typechecked yet\n");
    }
    this->type_def = type_def;
    for (state = (c_co_fsm_state *)next_in_list; state; )
    {
        state->type_def = type_def;
        if ((state->expression))
        {
            if (!has_expression)
            {
                cyclicity->set_parse_error( (c_cyc_object *)state, co_compile_stage_cross_reference, "Either all or no FSM states need to be assigned, and this state is, others are not" );
                return;
            }
            state->expression->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_undefined );
        }
        else if (has_expression)
        {
            cyclicity->set_parse_error( (c_cyc_object *)state, co_compile_stage_cross_reference, "Either all or no FSM states need to be assigned, and this state is not" );
            return;
        }
        state = (c_co_fsm_state *)state->next_in_list;
    }
}

/*a c_co_type_definition
 */
/*f c_co_type_definition::cross_reference
 */
void c_co_type_definition::cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    if (type_specifier)
    {
        type_specifier->cross_reference( cyclicity, types, variables );
    }
    if (type_struct)
    {
        type_struct->cross_reference( cyclicity, types, variables );
    }
    if (fsm_state)
    {
        fsm_state->cross_reference( cyclicity, types, variables, this );
    }
    if (enum_definition)
    {
        enum_definition->cross_reference( cyclicity, types, variables, this );
    }
}

/*a c_co_type_specifier
 */
/*f c_co_type_specifier::cross_reference
 Resolve cross-references in indices: only use global_scope 
*/
void c_co_type_specifier::cross_reference( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
     int i;
     if (symbol)
     {
          switch (get_symbol_type(symbol->lex_symbol))
          {
          case TOKEN_TEXT_INTEGER:
               type_value = type_value_integer;
               return;
          case TOKEN_TEXT_BIT:
               type_value = type_value_bit_array_size_1;
               return;
          case TOKEN_TEXT_STRING:
               type_value = type_value_string;
               return;
          }
          i = find_symbol_in_scope( types->co, symbol );
          if (i<0)
          {
               cyclicity->set_parse_error( this, co_compile_stage_cross_reference, "Failed to resolve type '%s'", lex_string_from_terminal( symbol ) );
               refers_to = NULL;
               type_value = type_value_error;
          }
          else
          {
               refers_to = types->co[i].co_type_definition;
               type_value = types->co[i].co_type_definition->type_value;
          }
     }
     if (type_specifier)
     {
          type_specifier->cross_reference( cyclicity, types, variables );
     }
     if (first)
     {
          first->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
     }
     if (last)
     {
          last->cross_reference_within_type_context( cyclicity, types, variables, type_value_undefined, type_value_integer );
     }
}

/*a c_co_type_struct
 */
/*f c_co_type_struct::cross_reference
 Resolve cross-references in type_specifiers on the list
*/
void c_co_type_struct::cross_reference( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
     c_co_type_struct *ts;

     for (ts=this; ts; ts = (c_co_type_struct *)ts->next_in_list)
     {
          if (ts->type_specifier)
          {
               ts->type_specifier->cross_reference( cyclicity, types, variables );
          }
     }
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



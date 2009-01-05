/*a Copyright
  
  This file 'c_co_toplevel_evaluate.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "c_co_lvar.h"
#include "c_co_sized_int_pair.h"
#include "c_co_statement.h"
#include "c_co_token_union.h"
#include "c_co_toplevel.h"
#include "c_co_type_definition.h"
#include "c_co_type_specifier.h"
#include "c_co_type_struct.h"

/*a c_co_constant_declaration
 */
/*f c_co_constant_declaration::evaluate
 */
void c_co_constant_declaration::evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    type_specifier->evaluate( cyclicity, types, variables );
 
    if (overridden)
    {
        if (cyclicity->type_value_pool->get_type(type_value) != type_specifier->type_value )
        {
            cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Type mismatch between constant and its override" );
        }
        SL_DEBUG( sl_debug_level_verbose_info, "Constant declaration '%s' has been overriden. Wanted to evaluate, but no", lex_string_from_terminal(symbol) );
        return;
    }
    SL_DEBUG( sl_debug_level_verbose_info, "Evaluating constant declaration '%s', expression %p", lex_string_from_terminal(symbol), expression );

    if ( (expression) &&
         (expression->type_check_within_type_context( cyclicity, types, variables, type_specifier->type_value ))  &&
         (expression->evaluate_constant( cyclicity, types, variables, 0 )) )
    {
        this->type_value = expression->type_value;
        SL_DEBUG( sl_debug_level_verbose_info, "Evaluated constant declaration '%s' expression %p to %d", lex_string_from_terminal(symbol), expression, this->type_value );
    }
}

/*f c_co_constant_declaration::override
 */
void c_co_constant_declaration::override( t_type_value type_value )
{
    this->overridden = 1;
    this->type_value = type_value;
}

/*a c_co_enum_definition
 */
/*f c_co_enum_definition::evaluate
 */
t_type_value c_co_enum_definition::evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    t_type_value type;
    int size=-1;

    if ( (first_index) &&
         ( (!first_index->type_check_within_type_context( cyclicity, types, variables, type_value_integer )) || (!first_index->evaluate_constant( cyclicity, types, variables, 0 ))) )
        return type_value_error;
    if ( (second_index) &&
         ( (!second_index->type_check_within_type_context( cyclicity, types, variables, type_value_integer )) || (!second_index->evaluate_constant( cyclicity, types, variables, 0 ))) )
        return type_value_error;

    SL_DEBUG( sl_debug_level_verbose_info, "first index %p second index %p", first_index, second_index );
    if (first_index)
    {
        size = cyclicity->type_value_pool->integer_value( first_index->type_value );
        if (second_index)
        {
            size = 1+size-cyclicity->type_value_pool->integer_value( second_index->type_value );
        }
        if ((size<=0) || (size>=32))
        {
            cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Size of enum must be greater than 0 and less than 32" );
            return 0;
        }
    }
    if (size<0)
    {
        cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Size of enum must be greater than 0 and less than 32" );
        type = type_value_undefined;
    }
    else
    {
        type = cyclicity->type_value_pool->add_bit_array_type( size );
    }
    SL_DEBUG( sl_debug_level_info, "created type %d (size %d)", type, size );
    if (enum_identifier)
    {
        if (!enum_identifier->evaluate( cyclicity, types, variables, type ))
            return type_value_error;
    }
    return type;
}

/*a c_co_enum_identifier
 */
/*f c_co_enum_identifier::evaluate
  type is a bit_array of the user-specified size
*/
int c_co_enum_identifier::evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type )
{
    c_co_enum_identifier *enum_id;
    int i;
    int last_int;
    int array_size;

    array_size = cyclicity->type_value_pool->get_size_of( type );
    last_int = -1;
    for (i=0, enum_id = this; enum_id; i++)
    {
        if ( (enum_id->expression) &&
             (enum_id->expression->type_check_within_type_context( cyclicity, types, variables, type )) &&
             (enum_id->expression->evaluate_constant( cyclicity, types, variables, 0 )) )
        {
            enum_id->type_value = enum_id->expression->type_value;
            last_int = cyclicity->type_value_pool->integer_value( enum_id->type_value );
        }
        else
        {
            enum_id->type_value = cyclicity->type_value_pool->new_type_value_bit_array( array_size, last_int+1, 0 );
            last_int = last_int+1;
            if ((last_int>>array_size)!=0)
            {
                printf("NNE:c_co_enum_identifier::enum implicit numbering out of range (%d bit size vector)\n", array_size );
            }
        }
        enum_id = (c_co_enum_identifier *)enum_id->next_in_list;
    }
    return 1;
}

/*a c_co_fsm_state
 */
/*f c_co_fsm_state::evaluate
 */
t_type_value c_co_fsm_state::evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    int num_states, num_bits;
    int i, j;
    c_co_fsm_state *state;
    t_type_value sub_index;

    /*b  type of an fsm_state depends on the subtype - onehot, onecold, or plain - number of states in bits for bit array or log2(number of states)
     */
    for (i=0,state=this; state; i++)
    {
        state = (c_co_fsm_state *)state->next_in_list;
    }
    num_states = i;
    switch (fsm_style)
    {
    case fsm_style_plain:
        for (j=0, i=num_states-1; i>0; i>>=1, j++);
        if (j==0)
            j=1;
        num_bits = j;
        break;
    default:
        num_bits = i;
        break;
    }
    sub_index = cyclicity->type_value_pool->add_bit_array_type( num_bits );
    SL_DEBUG( sl_debug_level_verbose_info, "Evaluate FSM states for first FSM state %s num_states %d num_bits %d sub_index %d", lex_string_from_terminal(symbol), num_states, num_bits, sub_index );
    for (i=0, state = this; state; i++)
    {
        if ( (state->expression) &&
             (state->expression->type_check_within_type_context( cyclicity, types, variables, sub_index )) &&
             (state->expression->evaluate_constant( cyclicity, types, variables, 0 )) )
        {
            state->type_value = state->expression->type_value;
        }
        else
        {
            switch (fsm_style)
            {
            case fsm_style_plain:
                state->type_value = cyclicity->type_value_pool->new_type_value_bit_array( num_bits, i, 0 );
                break;
            case fsm_style_one_hot:
                state->type_value = cyclicity->type_value_pool->new_type_value_bit_array( num_bits, 1<<i, 0 );
                break;
            case fsm_style_one_cold:
                state->type_value = cyclicity->type_value_pool->new_type_value_bit_array( num_bits, ~(1<<i), 0 );
                break;
            }
        }
        state = (c_co_fsm_state *)state->next_in_list;
    }
    return sub_index;
}

/*a c_co_type_definition
 */
/*f c_co_type_definition::evaluate
 */
void c_co_type_definition::evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    t_type_value sub_index;

    if (type_specifier)
    {
        SL_DEBUG( sl_debug_level_verbose_info, "Typedef '%s' to type specifier %p", lex_string_from_terminal(symbol), type_specifier );
        type_specifier->evaluate( cyclicity, types, variables );
        if (type_specifier->type_specifier)
            SL_DEBUG( sl_debug_level_verbose_info, "Typedef '%s' to type specifier %p type %d", lex_string_from_terminal(symbol), type_specifier, type_specifier->type_specifier->type_value );
        type_value = cyclicity->type_value_pool->add_type( types, symbol, type_specifier );
        return;
    }
    if (type_struct)
    {
        SL_DEBUG( sl_debug_level_verbose_info, "Type structure '%s' to type specifier %p", lex_string_from_terminal(symbol), type_struct );
        type_struct->evaluate( cyclicity, types, variables );
        type_value = cyclicity->type_value_pool->add_type( types, symbol, type_struct );
        SL_DEBUG( sl_debug_level_verbose_info, "Type structure '%s' to type specifier %p type %d", lex_string_from_terminal(symbol), type_struct, type_value );
        return;
    }
    if (fsm_state)
    {
        sub_index = fsm_state->evaluate( cyclicity, types, variables );
        if (sub_index!=type_value_error)
        {
            type_value = cyclicity->type_value_pool->add_type( types, symbol, sub_index );
        }
        return;
    }
    if (enum_definition)
    {
        sub_index = enum_definition->evaluate( cyclicity, types, variables );
        if (sub_index!=type_value_error)
        {
            type_value = cyclicity->type_value_pool->add_type( types, symbol, sub_index );
        }
        return;
    }
}

/*a c_co_type_specifier
 */
/*f c_co_type_specifier::evaluate
 */
void c_co_type_specifier::evaluate( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    SL_DEBUG( sl_debug_level_verbose_info, "Type specifier '%p'", this );
    if (symbol)
    {
        /*b If a user type, we have cross-referenced it to a type definition - that should already be added to the
          type pool, so we can find that.
          If a system type we can find that too.
          So we expect to find the type using the type pool...
          so set our index, then return
        */
        type_value = cyclicity->type_value_pool->find_type( this );
        if (type_value == type_value_error)
        {
            cyclicity->set_parse_error( this, co_compile_stage_evaluate_constants, "Failed to resolve type '%s' during evaluation", lex_string_from_terminal( symbol ) );
        }
        return;
    }
    if (type_specifier)
    {
        type_specifier->evaluate( cyclicity, types, variables );
    }
    if (first)
    {
        if (!first->type_check_within_type_context( cyclicity, types, variables, type_value_integer))
        {
            cyclicity->set_parse_error( this, co_compile_stage_evaluate_constants, "Array indices must be integers" );
            return;
        }
        first->evaluate_constant( cyclicity, types, variables, 0);
    }
    if (last)
    {
        if (!last->type_check_within_type_context( cyclicity, types, variables, type_value_integer))
        {
            cyclicity->set_parse_error( this, co_compile_stage_evaluate_constants, "Array indices must be integers" );
            return;
        }
        last->evaluate_constant( cyclicity, types, variables, 0);
    }
    type_value = cyclicity->type_value_pool->add_type( types, this );
}

/*a c_co_type_struct
 */
/*f c_co_type_struct::evaluate
 */
void c_co_type_struct::evaluate( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    c_co_type_struct *ts;

    for (ts=this; ts; ts = (c_co_type_struct *)ts->next_in_list)
    {
        if (ts->type_specifier)
        {
            ts->type_specifier->evaluate( cyclicity, types, variables );
        }
    }
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



/*a Copyright
  
  This file 'c_co_fsm_state.h' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a Wrapper
 */
#ifdef __INC_C_CO_FSM_STATE
#else
#define __INC_C_CO_FSM_STATE

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_fsm_style
 */
typedef enum
{
     fsm_style_plain,
     fsm_style_one_hot,
     fsm_style_one_cold
} t_fsm_style;

/*t c_co_fsm_state
 */
class c_co_fsm_state: public c_cyc_object
{
public:
     c_co_fsm_state( t_symbol *name, class c_co_expression *expression, class c_co_token_union *preceding_states, class c_co_token_union *succeeding_states, t_string *optional_documentation );
     ~c_co_fsm_state();
     c_co_fsm_state *chain_tail( c_co_fsm_state *entry );
     void set_style( t_fsm_style fsm_style );

     void cross_reference( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, c_co_type_definition *type_def );
     t_type_value evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );

     t_symbol *symbol;
     t_fsm_style fsm_style;
     class c_co_expression *expression;
     class c_co_token_union *preceding_states;
     class c_co_token_union *succeeding_states;
     t_string *documentation;

     c_co_type_definition *type_def; // filled in at cross referencing time, type definition this FSM state is part of
     t_type_value type_value; // value filled in at evaluation time, either from expression or by enumeration, with a bit vector type
};

/*a Wrapper
 */
#endif



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/




/*a Copyright
  
  This file 'c_co_nested_assignment.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_NESTED_ASSIGNMENT
#else
#define __INC_C_CO_NESTED_ASSIGNMENT

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"
#include "c_type_value_pool.h"

/*a Classes
 */
/*t c_co_nested_assignment
 */
class c_co_nested_assignment: public c_cyc_object
{
public:
     c_co_nested_assignment( class c_co_nested_assignment *nested_assignment );
     c_co_nested_assignment( t_symbol *symbol, class c_co_nested_assignment *nested_assignment );
     c_co_nested_assignment( class c_co_expression *expression );
     ~c_co_nested_assignment();
     c_co_nested_assignment *chain_tail( c_co_nested_assignment *entry );

     void cross_reference_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value parent_type, t_type_value type );
     void check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
     void evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );

     struct t_md_statement *build_model_from_statement( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module, struct t_md_lvar *model_lvar, int clocked, struct t_md_lvar *model_lvar_context, const char *documentation );
     void build_model_reset( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module, struct t_md_lvar *model_lvar, struct t_md_lvar *model_lvar_context );

     class c_co_expression *expression; // an actual value for a nested assignment. Only non-NULL if the symbol and the nesting assignment are both NULL
     t_symbol *symbol; // the symbol corresponding to an element name of a structure, whose nested_assignment defines the value
     class c_co_nested_assignment *nested_assignment; // if combined with a non-NULL symbol then this specifies the value of the element given by the symbol; if symbol is NULL, then this is an array element value

     // Possibles are:
     // expression <expr>, symbol null, nested_assignment null, chain null => expression
     // expression null, symbol 'x', nested_assignment 'v', chain null => '{ x=v }'
     // expression null, symbol 'x', nested_assignment 'v', chain '..' => '{ x=v, .. }'

     int element; // element number within a structure, if symbol is non-NULL, filled in at final cross referencing
     t_type_value type_context; // type of the element or expression, filled in at final cross referencing
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




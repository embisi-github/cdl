/*a Copyright
  
  This file 'c_co_named_expression.h' copyright Gavin J Stark 2008
  
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
#ifdef __INC_C_CO_NAMED_EXPRESSION
#else
#define __INC_C_CO_NAMED_EXPRESSION

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Classes
 */
/*t c_co_named_expression
 */
class c_co_named_expression: public c_cyc_object
{
public:
    c_co_named_expression( t_string *name, class c_co_expression *expression );
    ~c_co_named_expression();
    c_co_named_expression *chain_tail( c_co_named_expression *entry );

    void cross_reference_pre_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, class c_co_clock_reset_defn *default_clock, class c_co_clock_reset_defn *default_reset );
    void cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );
    void high_level_checks( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    struct t_md_named_expression *build_model( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module );

    t_string *name;
    class c_co_expression *expression;
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




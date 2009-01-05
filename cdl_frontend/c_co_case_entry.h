/*a Copyright
  
  This file 'c_co_case_entry.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_CASE_ENTRY
#else
#define __INC_C_CO_CASE_ENTRY

/*a Includes
 */
#include "c_cyc_object.h"

/*a Types
 */
/*t c_co_case_entry
 */
class c_co_case_entry: public c_cyc_object
{
public:
    c_co_case_entry( class c_co_expression *index, int flow_through, class c_co_statement *statements );
    ~c_co_case_entry();
    c_co_case_entry *chain_tail( c_co_case_entry *entry );

    void cross_reference_pre_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, class c_co_clock_reset_defn *default_clock, class c_co_clock_reset_defn *default_reset );
    void cross_reference_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type );
    void check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );
    void high_level_checks( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    struct t_md_switch_item *build_model( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module );

    class c_co_expression *expression; // Expression list for the case statement
    class c_co_statement *statements;  // Statement for the expression - can be NULL for {} or for flow through
    t_type_value type;                 // expected type of case expression (generally the type of parent expression)
    int flow_through;                  // If 1 then following case's statements should be used
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




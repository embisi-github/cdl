/*a Copyright
  
  This file 'c_co_port_map.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_PORT_MAP
#else
#define __INC_C_CO_PORT_MAP

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_port_map_type
 */
typedef enum
{
    port_map_type_input,
    port_map_type_output,
    port_map_type_clock
} t_port_map_type;

/*t c_co_port_map
 */
class c_co_port_map: public c_cyc_object
{
public:
    c_co_port_map( class c_co_lvar *port_lvar, class c_co_lvar *lvar ); // Build an output port
    c_co_port_map( class c_co_lvar *port_lvar, class c_co_expression *expression ); // Build an input port
    c_co_port_map( t_symbol *port_id, t_symbol *clock_id ); // Build a clock port
    c_co_port_map( t_symbol *port_id, class c_co_expression *expression ); // Build a parameter
    ~c_co_port_map();
    c_co_port_map *chain_tail( c_co_port_map *entry );

    int cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_co_scope *port_scope );
    void check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_co_scope *port_scope );
    int high_level_checks( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_co_scope *port_scope );
    void evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );
    void build_model( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module, const char *module_name );

    t_port_map_type type;
    t_symbol *port_id; // Name of the module's parameter port
    class c_co_lvar *port_lvar; // Lvar of the module's port
    t_symbol *clock_id; // Clock name to tie a clock to
    class c_co_lvar *lvar; // LVAR to tie an output to
    class c_co_expression *expression; // Expression to tie an input to - should be a nested assignment
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




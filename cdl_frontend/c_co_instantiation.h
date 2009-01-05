/*a Copyright
  
  This file 'c_co_instantiation.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_INSTANTIATION
#else
#define __INC_C_CO_INSTANTIATION

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t c_co_instantiation
 */
class c_co_instantiation: public c_cyc_object
{
public:
    c_co_instantiation( t_symbol *instance_of_id, t_symbol *instance_id, class c_co_port_map *port_map, class c_co_expression *index );
    ~c_co_instantiation();

    int cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    int high_level_checks( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );

    t_symbol *instance_of_id; // If a module instance, the module name that it is instantiating
    t_symbol *instance_id; // If a module instance, the instance name to give to this instantiation of the module
    class c_co_port_map *port_map;
    class c_co_expression *index;

    t_co_union refers_to; // Filled in at cross referencing definition of the module or bus that this is an instance of

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




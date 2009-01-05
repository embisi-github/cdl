/*a Copyright
  
  This file 'c_co_toplevel.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_TOPLEVEL
#else
#define __INC_C_CO_TOPLEVEL

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_toplevel_type
 */
typedef enum
{
    toplevel_type_none,
    toplevel_type_module,
    toplevel_type_module_prototype,
    toplevel_type_type_definition,
    toplevel_type_constant_declaration
} t_toplevel_type;

/*t c_co_toplevel
 */
class c_co_toplevel: public c_cyc_object
{
public:
    c_co_toplevel( class c_co_module *module );
    c_co_toplevel( class c_co_module_prototype *module_prototype );
    c_co_toplevel( class c_co_bus_definition *bus_definition );
    c_co_toplevel( class c_co_type_definition *type_definition );
    c_co_toplevel( class c_co_constant_declaration *constant_declaration );
    ~c_co_toplevel();
    c_co_toplevel *chain_tail( c_co_toplevel *entry );

    t_toplevel_type toplevel_type;
    union {
        class c_co_module *module;
        class c_co_module_prototype *module_prototype;
        class c_co_type_definition *type_definition;
        class c_co_constant_declaration *constant_declaration;
    } data;
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




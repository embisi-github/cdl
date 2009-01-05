/*a Copyright
  
  This file 'c_co_module.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_MODULE
#else
#define __INC_C_CO_MODULE

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t c_co_module
 */
class c_co_module: public c_cyc_object
{
public:
    c_co_module( t_symbol *id, class c_co_signal_declaration *ports, class c_co_module_body_entry *body, t_string *documentation );
    ~c_co_module();

    void index_local_signals( class c_cyclicity *cyclicity, t_co_scope *global_constants );
    void cross_reference_pre_type_resolution( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types );
    void resolve_types( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types );
    void cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types );
    void check_types( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types );
    void evaluate_constant_expressions( c_cyclicity *cyclicity, t_co_scope *types, int reevaluate );
    void high_level_checks( class c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types );

    void build_model( class c_cyclicity *cyclicity, class c_model_descriptor *model );

    t_symbol *symbol; // Name of the module
    class c_co_signal_declaration *ports; // List of signal declarations that define the ports to the module
    class c_co_module_body_entry *body; // List of body elements that make up the definition of the module
    t_string *documentation; // Documentation for the module as a whole

    t_co_scope *local_signals; // Filled in at cross reference time - null-terminated array of signal_declarations. The array must be freed if non-NULL.
    t_co_scope *port_scope; // Filled in at cross reference time - null-terminated array of signal_declarations, just the ports. The array must be freed if non-NULL.

    t_md_module *md_module; // Backend module
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




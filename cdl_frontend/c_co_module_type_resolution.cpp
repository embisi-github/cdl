/*a Copyright
  
  This file 'c_co_module_type_resolution.cpp' copyright Gavin J Stark 2003, 2004
  
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

#include "c_co_module_prototype.h"
#include "c_co_module.h"
#include "c_co_signal_declaration.h"
#include "c_co_type_specifier.h"

/*a Types
 */

/*a c_co_module_prototype
 */
/*f c_co_module_prototype::resolve_types
  Run through all the ports in the module, and resolve their types
*/
void c_co_module_prototype::resolve_types( c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types )
{
    c_co_signal_declaration *cosd;

    SL_DEBUG( sl_debug_level_verbose_info, "Module '%s'", lex_string_from_terminal( symbol ));

    for (cosd=ports; cosd; cosd=(c_co_signal_declaration *)cosd->next_in_list )
    {
        cosd->resolve_type( cyclicity, types, port_scope );
    }
}

/*a c_co_module
 */
/*f c_co_module::resolve_types
  Run through all the local signals in the module, and resolve their types
*/
void c_co_module::resolve_types( c_cyclicity *cyclicity, t_co_scope *modules, t_co_scope *types )
{
    int i;

    SL_DEBUG( sl_debug_level_verbose_info, "Module '%s'", lex_string_from_terminal( symbol ));

    for (i=0; local_signals->co[i].cyc_object; i++)
    {
        local_signals->co[i].co_signal_declaration->resolve_type( cyclicity, types, local_signals );
    }
}

/*a c_co_signal_declaration
 */
/*f c_co_signal_declaration::resolve_type
  Evaluate constants in type specifier and any reset value (if clocked)
*/
void c_co_signal_declaration::resolve_type( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables )
{
    if (type_specifier)
    {
        type_specifier->evaluate( cyclicity, types, variables );
        SL_DEBUG( sl_debug_level_verbose_info, "Resolved type for signal '%s' (%p) to %d", lex_string_from_terminal( symbol ), this, type_specifier->type_value );
    }
    if (local_clocked_signal && local_clocked_signal->type_specifier)
    {
        local_clocked_signal->type_specifier->evaluate( cyclicity, types, variables );
        SL_DEBUG( sl_debug_level_verbose_info, "Resolved type for local clocked signal '%s' (%p) to %d", lex_string_from_terminal( symbol ), local_clocked_signal, local_clocked_signal->type_specifier->type_value );
    }
}




/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



/*a Copyright
  
  This file 'c_co_lvar.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_LVAR
#else
#define __INC_C_CO_LVAR

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t c_co_lvar
  Lvar's are used in assignment statements, on the left-hand side of the assignment
  They can be any local signal or output port
  They can be a structure as a whole, an index into an array of structures, an indexed element of a structure, and so on
*/
class c_co_lvar: public c_cyc_object
{
public:
    c_co_lvar( t_symbol *symbol );
    c_co_lvar( class c_co_lvar *lvar, t_symbol *symbol );
    c_co_lvar( class c_co_lvar *lvar, class c_co_expression *first_index, class c_co_expression *length );
    ~c_co_lvar();

    void cross_reference_post_type_resolution_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type_context );
    void check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );
    struct t_md_lvar *build_model( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module, struct t_md_lvar *model_lvar_context );
    struct t_md_port_lvar *build_model_port( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module );

    t_symbol *symbol; // If an element of a structure or a variable itself - this is the terminal - it is NULL for an array, which is an array of a c_co_lvar
    class c_co_lvar *lvar; // If a structure with elements then this is the lvar for the structure; if an index into an array, this is the array
    class c_co_expression *first; // the bottom bit of the slice, if any - if a single bit, then that bit
    class c_co_expression *length; // the length of the slice of the slice

    // After cross referencing an LVAR may have:
    // lvar null, signal_declaration 'x', element=-1, symbol null, first null, non_signal_declaration null => signal 'x'
    // lvar null, signal_declaration null, element=-1, symbol null, first null, non_signal_decalaration 'p' => constant/fsm/enumid 'p'
    // lvar null, signal_declaration null, element=<n>, symbol null, first null, non_signal_decalaration null => element <n> of implied signal 'x', which comes from context
    // lvar 'x.y.z', signal_declaration 'x', element=<n>, symbol 'w', first null, non_signal_decalaration null => element 'w':<n> of lvar 'x.y.z'
    // lvar 'y.z', signal_declaration null, element=<n>, symbol 'w', first null, non_signal_decalaration null => element 'w':<n> of elements 'y.z' of implied signal 'x' which comes from context
    // lvar 'x.y.z', signal_declaration 'x', element=<n>, symbol null, first 'expr', non_signal_decalaration null => array index 'expr' of lvar 'x.y.z'
    // lvar 'y.z', signal_declaration null, element=<n>, symbol null, first 'expr', non_signal_decalaration null => array index 'expr'of elements 'y.z' of implied signal 'x' which comes from context

    int element; // If a structure element, then this is the index into the structure of the element that 'symbol' specifies for the 'lvar'
    class c_co_signal_declaration *signal_declaration; // Filled in at final cross referencing, with the local signal this is an assignment to. If the lvar is a structure element, this is the still the signal declaration
    t_co_union non_signal_declaration; // Filled in at final cross referencing, with the global this is a reference of, if NOT a local signal.
    t_type_value type; // Filled in at final cross referencing, with the type of the lvar as a whole. If the lvar is a structure, this is the structure; if the lvar is an indexed element, this is the type of the indexed element

    // For ports on a module instantiation, the LVAR
    // will haves symbol filled in, and possibly 'lvar' if it is an element of a structure

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




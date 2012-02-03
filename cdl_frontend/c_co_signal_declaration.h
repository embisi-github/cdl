/*a Copyright
  
  This file 'c_co_signal_declaration.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_SIGNAL_DECLARATION
#else
#define __INC_C_CO_SIGNAL_DECLARATION

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_signal_declaration_type
 */
typedef enum
{
    signal_declaration_type_clock, // Always a port
    signal_declaration_type_clocked_local,
    signal_declaration_type_comb_local,
    signal_declaration_type_net_local,
    signal_declaration_type_input,
    signal_declaration_type_comb_output,
    signal_declaration_type_clocked_output,
    signal_declaration_type_net_output,
    signal_declaration_type_parameter,
} t_signal_declaration_type;

/*t t_signal_usage_type
 */
typedef enum
{
    signal_usage_type_rtl,
    signal_usage_type_assert,
    signal_usage_type_cover
} t_signal_usage_type;

/*t c_co_signal_declaration
 */
class c_co_signal_declaration: public c_cyc_object
{
public:
    c_co_signal_declaration( t_symbol *id, class c_co_type_specifier *type, t_signal_usage_type usage_type, class c_co_clock_reset_defn *clock, class c_co_clock_reset_defn *reset, class c_co_nested_assignment *reset_values, class c_co_declspec *declspec, t_string *documentation ); // local clocked signal with nested reset
    c_co_signal_declaration( t_symbol *id, class c_co_type_specifier *type, t_signal_usage_type usage_type, t_string *documentation ); // local combinatorial signal
    c_co_signal_declaration( t_signal_declaration_type sd_type, t_symbol *id, class c_co_type_specifier *type, t_string *documentation ); // net declaration
    c_co_signal_declaration( t_symbol *bus_id, t_symbol *id, t_string *documentation ); // bus in a port declaration
    c_co_signal_declaration( t_symbol *id, t_symbol *dirn, class c_co_type_specifier *type, t_string *documentation ); // input/output port
    c_co_signal_declaration( t_symbol *id, t_string *documentation ); // Clock input
    c_co_signal_declaration( t_symbol *id, c_co_clock_reset_defn *clock, c_co_clock_reset_defn *clock_enable, t_string *documentation ); // gated clock
    ~c_co_signal_declaration();
    c_co_signal_declaration *chain_tail( c_co_signal_declaration *entry );
     
    void cross_reference_pre_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void resolve_type( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void check_types( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void evaluate_constant_expressions( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );
    void high_level_checks( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );

    t_symbol *symbol; // Name of the signal
    class c_co_declspec *declspec_list; // Declaration specifications
    class c_co_type_specifier *type_specifier; // Real type of the signal declaration
    t_signal_declaration_type signal_declaration_type; // Indicates if the declaration is a clock, input, clocked output, clocked local, etc
    t_string *documentation; // Documentation of the signal
    union
    {
        struct
        {
            class c_co_clock_reset_defn *clock_to_gate; // for gated clocks; NULL for port clocks
            class c_co_clock_reset_defn *gate;          // for gated clocks; NULL for port clocks
        } clock;
        struct {
            t_signal_usage_type usage_type;
            class c_co_clock_reset_defn *clock_spec;
            class c_co_clock_reset_defn *reset_spec;
            class c_co_nested_assignment *reset_value;
        } clocked;
        struct {
            t_signal_usage_type usage_type;
        } comb;
    } data;
    class c_co_signal_declaration *local_clocked_signal; // For clocked outputs, this is the cross reference from the port declaration to the original local clocked signal declaration (The port declaration is the one kept in 'local_signals' scope)
    class c_co_signal_declaration *output_port; // For clocked outputs, this is the cross reference from the original local clocked signal declaration to the port declaration
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




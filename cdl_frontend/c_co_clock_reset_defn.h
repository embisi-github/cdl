/*a Copyright
  
  This file 'c_co_clock_reset_defn.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_CLOCK_RESET_DEFN
#else
#define __INC_C_CO_CLOCK_RESET_DEFN

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_clock_reset_defn_type
 */
typedef enum
{
	 clock_reset_type_default_clock,
	 clock_reset_type_clock,
	 clock_reset_type_default_reset,
	 clock_reset_type_reset
} t_clock_reset_defn_type;

/*t t_clock_edge
 */
typedef enum 
{
     clock_edge_falling,
     clock_edge_rising,
} t_clock_edge;

/*t t_reset_active
 */
typedef enum 
{
	 reset_active_high,
	 reset_active_low
} t_reset_active;

/*t c_co_clock_reset_defn
 */
class c_co_clock_reset_defn: public c_cyc_object
{
public:
	 c_co_clock_reset_defn( t_clock_reset_defn_type clock_reset_defn_type, t_symbol *clock_symbol, t_clock_edge clock_edge );
	 c_co_clock_reset_defn( t_clock_reset_defn_type clock_reset_defn_type, t_symbol *reset_symbol, t_reset_active reset_active );
	 ~c_co_clock_reset_defn();

     void cross_reference_post_type_resolution( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );

	 t_symbol *symbol; // Name of the clock or reset
	 t_clock_edge clock_edge; // Edge for a clock
	 t_reset_active reset_active; // Level for a reset
	 t_clock_reset_defn_type clock_reset_defn_type; // Type of this definition

     class c_co_signal_declaration *signal_declaration; // Filled in at final cross referencing, refers to the signal the symbol resolves to

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




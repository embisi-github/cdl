/*a Copyright
  
  This file 'c_co_timing_spec.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_TIMING_SPEC
#else
#define __INC_C_CO_TIMING_SPEC

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_timing_spec_type
 */
typedef enum
{
    timing_spec_type_to_clock,
    timing_spec_type_from_clock,
    timing_spec_type_comb_inputs,
    timing_spec_type_comb_outputs,
} t_timing_spec_type;

/*t t_timing_spec_clock_edge
 */
typedef enum 
{
     timing_spec_clock_edge_falling,
     timing_spec_clock_edge_rising,
} t_timing_spec_clock_edge;

/*t c_co_timing_spec
 */
class c_co_timing_spec: public c_cyc_object
{
public:
    c_co_timing_spec( t_timing_spec_type type, t_timing_spec_clock_edge clock_edge, t_symbol *clock_symbol, c_co_token_union *dependents );
    c_co_timing_spec( t_timing_spec_type type, c_co_token_union *signals );
    ~c_co_timing_spec();

    void cross_reference_post_type_resolution( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );

    t_symbol *clock_symbol; // Clock for a clock symbol, if the timing is dependent on a clock
    class c_co_token_union *signals; // List of user ids that are dependents or dependencies
    t_timing_spec_type type; // Type of timing spec
    t_timing_spec_clock_edge clock_edge; // Edge for a clock

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




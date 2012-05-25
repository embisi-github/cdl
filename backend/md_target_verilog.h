/*a Copyright
  
  This file 'md_target_verilog.h' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/
#include "c_model_descriptor.h"

/*a Types
 */
/*t t_md_verilog_options
 */
typedef struct
{
    int vmod_mode;
    const char *clock_gate_module_instance_type;
    const char *clock_gate_module_instance_extra_ports;
    const char *assert_delay_string;
    const char *verilog_comb_reg_suffix;
    int include_displays;
    int include_assertions;
    int sv_assertions;
    int include_coverage;
} t_md_verilog_options;

/*a External functions
 */
extern void target_verilog_output( c_model_descriptor *model, t_md_output_fn output_fn, void *output_handle, t_md_verilog_options *options );

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


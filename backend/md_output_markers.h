/*a Copyright
  
  This file 'md_target_c.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_MD_OUTPUT_MARKERS
#else
#define __INC_MD_OUTPUT_MARKERS

/*a Includes
 */
#include "c_model_descriptor.h"

/*a External functions
 */
extern void output_markers_mask( t_md_module_instance_clock_port *clock_port, int set_mask, int clr_mask );
extern int output_markers_value( t_md_module_instance_clock_port *clock_port, int mask );
extern void output_markers_mask( t_md_type_instance *instance, int set_mask, int clr_mask );
extern int output_markers_value( t_md_type_instance *instance, int mask );
extern void output_markers_mask_matching( t_md_type_instance *instance, int mask, int value, int set_mask_eq, int clr_mask_eq, int set_mask_ne, int clr_mask_ne );
extern void output_markers_mask( t_md_module_instance *instance, int set_mask, int clr_mask );
extern int output_markers_value( t_md_module_instance *instance, int mask );
extern void output_markers_mask_matching( t_md_module_instance *instance, int mask, int value, int set_mask_eq, int clr_mask_eq, int set_mask_ne, int clr_mask_ne );
extern void output_markers_mask_all( c_model_descriptor *model, t_md_module *module, int set_mask, int clr_mask );
extern void output_markers_mask_all_matching( c_model_descriptor *model, t_md_module *module, int mask, int value, int set_mask_eq, int clr_mask_eq, int set_mask_ne, int clr_mask_ne );
extern void output_markers_mask_dependents( c_model_descriptor *model, t_md_module *module, t_md_type_instance *instance, int set_mask, int clr_mask );
extern void output_markers_mask_dependencies( c_model_descriptor *model, t_md_module *module, t_md_type_instance *instance, int set_mask, int clr_mask );
extern void output_markers_mask_dependents( c_model_descriptor *model, t_md_module *module, t_md_signal *signal, int set_mask, int clr_mask );
extern void output_markers_mask_dependents( c_model_descriptor *model, t_md_module *module, t_md_state *state, int set_mask, int clr_mask );
extern void output_markers_mask_dependencies( c_model_descriptor *model, t_md_module *module, t_md_signal *signal, int set_mask, int clr_mask );
extern void output_markers_mask_dependencies( c_model_descriptor *model, t_md_module *module, t_md_state *state, int set_mask, int clr_mask );
extern void output_markers_mask_input_dependents( c_model_descriptor *model, t_md_module *module, int set_mask, int clr_mask );
extern void output_markers_mask_comb_output_dependencies( c_model_descriptor *model, t_md_module *module, int set_mask, int clr_mask );
extern void output_markers_mask_output_dependencies( c_model_descriptor *model, t_md_module *module, int set_mask, int clr_mask );
extern void output_markers_mask_comb_modules_with_matching_outputs( c_model_descriptor *model, t_md_module *module, int mask, int value, int set_mask, int clr_mask );
extern void output_markers_mask_clock_edge_dependencies( c_model_descriptor *model, t_md_module *module, t_md_signal *clock, int edge, int set_mask, int clr_mask );
extern void output_markers_mask_clock_edge_dependents( c_model_descriptor *model, t_md_module *module, t_md_signal *clock, int edge, int set_mask, int clr_mask );
extern int output_markers_check_net_driven_in_parts_modules_all_match( t_md_module *module, t_md_type_instance *instance, int mask, int match );
extern t_md_type_instance *output_markers_find_iter_match( c_model_descriptor *model, t_md_reference_iter *iter, void *signal, int mask, int match );
extern int output_markers_check_iter_any_match( c_model_descriptor *model, t_md_reference_iter *iter, void *signal, int mask, int match );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


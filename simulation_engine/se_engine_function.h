/*a Copyright
  
  This file 'se_engine_function.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Wrapper
 */
#ifdef __INC_ENGINE__SE_ENGINE_FUNCTION
#else
#define __INC_ENGINE__SE_ENGINE_FUNCTION

/*a Includes
 */
#include "c_sl_error.h"
#include "c_se_engine__internal_types.h"
#include "c_se_engine.h"

/*a Defines
 */

/*a Types
 */

/*a External functions
 */
extern void se_engine_signal_reference_add( t_engine_signal_reference **list_ptr, t_engine_signal *entry );

extern void se_engine_function_free_functions( t_engine_function *list );
extern t_engine_function *se_engine_function_add_function( t_engine_module_instance *emi, t_engine_function **efn_list, const char *name );
extern t_engine_function *se_engine_function_find_function( t_engine_function *efn_list, const char *name );
extern void se_engine_function_references_free( t_engine_function_reference *list );
extern void se_engine_function_references_add( t_engine_function_reference **ref_list_ptr, t_engine_function *efn );
extern t_engine_function_list *se_engine_function_call_add( t_engine_function_list **list_ptr, void *handle, t_engine_callback_fn callback_fn );
extern t_engine_function_list *se_engine_function_call_add( t_engine_function_list **list_ptr, void *handle, t_engine_callback_arg_fn callback_fn );
extern t_engine_function_list *se_engine_function_call_add( t_engine_function_list **list_ptr, void *handle, t_engine_callback_argp_fn callback_fn );
extern void se_engine_function_call_add( t_engine_function_list **list_ptr, t_engine_function_reference *efr, t_engine_callback_fn callback_fn );
extern void se_engine_function_call_invoke_all( t_engine_function_list *list );
extern void se_engine_function_call_invoke_all_arg( t_engine_function_list *list, int arg );
extern void se_engine_function_call_invoke_all_argp( t_engine_function_list *list, void *arg );
extern void se_engine_function_call_display_stats_all( FILE *f, t_engine_function_list *list );
extern void se_engine_function_call_free( t_engine_function_list *list_ptr );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


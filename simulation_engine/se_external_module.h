/*a Copyright
  
  This file 'se_external_module.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_SE_EXTERNAL_MODULE
#else
#define __INC_SE_EXTERNAL_MODULE

/*a Includes
 */
#include "c_se_engine.h"

/*a Defines
 */

/*a Types
 */

/*a External functions
 */
extern void *se_external_module_find( const char *name );
extern void *se_external_module_find( const char *name, const char *implementation_name );
extern char *se_external_module_name( void *handle );
extern char *se_external_module_name( int n );
extern void se_external_module_register( int registration_version, const char *name, t_se_instantiation_fn instantiation_fn );
extern void se_external_module_register( int registration_version, const char *name, t_se_instantiation_fn instantiation_fn, const char *implementation_name );
extern t_sl_error_level se_external_module_instantiate( void *handle, class c_engine *engine, void *instantiation_handle );
extern int se_external_module_deregister( const char *name );
extern void se_external_module_deregister_all( void );
extern void se_external_module_init( void );

/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


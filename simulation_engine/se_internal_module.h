/*a Copyright
  
  This file 'se_internal_module.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_INTERNAL_MODULES
#else
#define __INC_INTERNAL_MODULES

#ifdef _WIN32
    #define DLLEXPORT __declspec(dllexport)
#else
    #define DLLEXPORT
#endif

/*a Includes
 */
#include "c_se_engine.h"

/*a External functions
 */
extern DLLEXPORT t_sl_error_level se_internal_module__logger_instantiate( c_engine *engine, void *engine_handle );
extern DLLEXPORT t_sl_error_level se_internal_module__sram_srw_instantiate( c_engine *engine, void *engine_handle );
extern DLLEXPORT t_sl_error_level se_internal_module__sram_mrw_instantiate( c_engine *engine, void *engine_handle );
extern DLLEXPORT t_sl_error_level se_internal_module__ddr_instantiate( c_engine *engine, void *engine_handle );
extern DLLEXPORT t_sl_error_level se_internal_module__test_harness_instantiate( c_engine *engine, void *engine_handle );
extern DLLEXPORT void internal_module_register_all( c_engine *engine );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


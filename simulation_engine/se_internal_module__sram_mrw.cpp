/*a Copyright
  
  This file 'se_internal_module__sram_mrw.cpp' copyright Gavin J Stark 2007
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "sl_mif.h"
#include "sl_general.h"
#include "sl_token.h"
#include "se_errors.h"
#include "se_external_module.h"
#include "se_internal_module.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"
#include "se_internal_module__sram.h"

/*a Defines
 */

/*a Types
 */
/*t c_sram_mrw
*/
class c_sram_mrw : public c_se_internal_module__sram
{
public:
    typedef c_se_internal_module__sram super;
    c_sram_mrw( class c_engine *eng, void *eng_handle );
    ~c_sram_mrw();
private:
};

/*a Extern wrapper functions for SRAM module
 */
/*f se_internal_module__sram_mrw_instantiate
 */
extern t_sl_error_level se_internal_module__sram_mrw_instantiate( c_engine *engine, void *engine_handle )
{
    c_sram_mrw *mod;
    mod = new c_sram_mrw( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*a Constructors and destructors for sram_mrw
*/
/*f c_sram_mrw::c_sram_mrw
*/
c_sram_mrw::c_sram_mrw( class c_engine *eng, void *eng_handle ) : c_se_internal_module__sram(eng, eng_handle, 1)
{
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


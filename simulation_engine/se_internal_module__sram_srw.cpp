/*a Copyright
  
  This file 'se_internal_module.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, version 2.1.
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
  for more details.
*/

/*a To do
Ensure MIF files can load greater than 32 bits wide (in hex/binary at least)
Add error messages if reads are of an unwritten location
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
#define MAX_WIDTH (8)

/*a Types
 */
/*t t_sram_srw_posedge_clock_state
*/
typedef struct t_sram_srw_posedge_clock_state
{
    int selected;
    t_sl_uint64 address;
    t_sl_uint64 read_not_write;
    t_sl_uint64 write_data[MAX_WIDTH];
    t_sl_uint64 write_enable[MAX_WIDTH];
    t_sl_uint64 data_out[MAX_WIDTH];
} t_sram_srw_posedge_clock_state;

/*t t_sram_srw_inputs
*/
typedef struct t_sram_srw_inputs
{
    t_sl_uint64 *select;
    t_sl_uint64 *address;
    t_sl_uint64 *read_not_write;
    t_sl_uint64 *write_data;
    t_sl_uint64 *write_enable;
} t_sram_srw_inputs;

/*t c_sram_srw
*/
class c_sram_srw : public c_se_internal_module__sram
{
public:
    typedef c_se_internal_module__sram super;
    c_sram_srw( class c_engine *eng, void *eng_handle );
    t_sl_error_level reset( int pass );
    t_sl_error_level preclock_posedge_clock( void );
    t_sl_error_level clock_posedge_clock( void );
    t_sl_error_level message( t_se_message *message );
private:
    c_engine *engine;
    void *engine_handle;
    const char *filename;
    int reset_type;
    int reset_value;
    int address_width;
    int data_width;
    unsigned int data_size;
    int data_byte_width;
    int data_word_width;
    int bits_per_enable;
    int number_of_enables;
    int verbose;
    t_sl_uint64 dprintf_action_address;
    t_sl_uint64 dprintf_data_address;
    t_sram_srw_inputs inputs;
    t_sram_srw_posedge_clock_state next_posedge_clock_state;
    t_sram_srw_posedge_clock_state posedge_clock_state;
    struct t_engine_log_event_array *log_event_array;
    t_sl_uint64 *memory;
};

/*a Static variables for sram_srw
*/
/*a Extern wrapper functions for SRAM module
 */
/*f se_internal_module__sram_srw_instantiate
 */
extern t_sl_error_level se_internal_module__sram_srw_instantiate( c_engine *engine, void *engine_handle )
{
    c_sram_srw *mod;
    mod = new c_sram_srw( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*a Constructors and destructors for sram_srw
*/
/*f c_sram_srw::c_sram_srw
*/
c_sram_srw::c_sram_srw( class c_engine *eng, void *eng_handle ) : c_se_internal_module__sram(eng, eng_handle, 0)
{
}

/*a Class reset/preclock/clock methods for sram_srw
*/
/*f c_sram_srw::reset
*/
t_sl_error_level c_sram_srw::reset( int pass )
{
    bzero(&posedge_clock_state, sizeof(posedge_clock_state));
    return super::reset(pass);
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


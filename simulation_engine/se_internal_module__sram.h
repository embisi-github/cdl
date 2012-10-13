/*a Copyright
  
  This file 'se_internal_module__sram.cpp' copyright Gavin J Stark 2003, 2004, 2012
  
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
#ifdef __INC_SE_INTERNAL_MODULE__SRAM
#else
#define __INC_SE_INTERNAL_MODULE__SRAM

/*a To do
Ensure MIF files can load greater than 32 bits wide (in hex/binary at least)
Add error messages if reads are of an unwritten location
 */

/*a Includes
 */
#include "c_se_engine.h"
#include "se_internal_module__sram.h"

/*a Defines
 */
#define MAX_WIDTH (8)

/*a Types
 */
/*t t_sram_posedge_clock_state
*/
typedef struct t_sram_posedge_clock_state
{
    int selected;
    t_se_signal_value address;
    t_se_signal_value read_not_write;
    t_se_signal_value write_data[MAX_WIDTH];
    t_se_signal_value write_enable[MAX_WIDTH];
    t_se_signal_value data_out[MAX_WIDTH];
} t_sram_posedge_clock_state;

/*t t_sram_inputs
*/
typedef struct t_sram_inputs
{
    t_se_signal_value *select;
    t_se_signal_value *address;
    t_se_signal_value *read_not_write;
    t_se_signal_value *write_data;
    t_se_signal_value *write_enable;
} t_sram_inputs;

/*t t_sram_clock_domain
 */
typedef struct t_sram_clock_domain
{
    class c_se_internal_module__sram *mod;
    int port;
    t_sram_inputs inputs;
    t_sram_posedge_clock_state next_posedge_clock_state;
    t_sram_posedge_clock_state posedge_clock_state;
    struct t_engine_log_event_array *log_event_array;
} t_sram_clock_domain;

/*t c_se_internal_module__sram
*/
class c_se_internal_module__sram
{
public:
    c_se_internal_module__sram( class c_engine *eng, void *eng_handle, int multiport );
    ~c_se_internal_module__sram(  );
    t_sl_error_level delete_instance( void );
    t_sl_error_level reset( int pass );
    t_sl_error_level read( unsigned int address, t_sl_uint64 *data_out, int permit_event, int error_on_out_of_range );
    t_sl_error_level write( unsigned int address, t_sl_uint64 *data_in, t_sl_uint64 *write_enable, int permit_event, int error_on_out_of_range );
    t_sl_error_level message( t_se_message *message );
    t_sl_error_level preclock_posedge_clock( t_sram_clock_domain *cd );
    t_sl_error_level clock_posedge_clock( t_sram_clock_domain *cd );
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
    int num_ports;
    t_sram_clock_domain *clock_domains; // Array of clock domain structures, one per port
    t_sl_uint64 *memory;
};

/*a Wrapper
 */
#endif


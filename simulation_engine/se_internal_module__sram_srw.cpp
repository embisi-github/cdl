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
class c_sram_srw
{
public:
    c_sram_srw( class c_engine *eng, void *eng_handle );
    ~c_sram_srw();
    t_sl_error_level delete_instance( void );
    t_sl_error_level reset( int pass );
    t_sl_error_level preclock_posedge_clock( void );
    t_sl_error_level clock_posedge_clock( void );
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
/*v log_event_descriptor
 */
static t_sram_srw_posedge_clock_state *___sram_srw_posedge_clock_state_log;
static t_engine_text_value_pair log_event_descriptor[] = 
{
    {"read", 2 },
    {"address", (t_se_signal_value *)&(___sram_srw_posedge_clock_state_log->address) - (t_se_signal_value *)___sram_srw_posedge_clock_state_log },
    {"data_out", (t_se_signal_value *)&(___sram_srw_posedge_clock_state_log->data_out) - (t_se_signal_value *)___sram_srw_posedge_clock_state_log },
    {"write", 3 },
    {"address", (t_se_signal_value *)&(___sram_srw_posedge_clock_state_log->address) - (t_se_signal_value *)___sram_srw_posedge_clock_state_log },
    {"data",    (t_se_signal_value *)&(___sram_srw_posedge_clock_state_log->data_out) - (t_se_signal_value *)___sram_srw_posedge_clock_state_log },
    {"enables", (t_se_signal_value *)&(___sram_srw_posedge_clock_state_log->write_enable) - (t_se_signal_value *)___sram_srw_posedge_clock_state_log },
    {NULL, 0}
};

/*a Static wrapper functions for SRAM module
 */
/*f sram_srw_delete - simple callback wrapper for the main method
*/
static t_sl_error_level sram_srw_delete( void *handle )
{
    c_sram_srw *mod;
    t_sl_error_level result;
    mod = (c_sram_srw *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f sram_srw_reset
 */
static t_sl_error_level sram_srw_reset( void *handle, int pass )
{
    c_sram_srw *sram;

     sram = (c_sram_srw *)handle;
     return sram->reset( pass );
}

/*f sram_srw_preclock
 */
static t_sl_error_level sram_srw_preclock( void *handle )
{
    c_sram_srw *sram;

     sram = (c_sram_srw *)handle;
     return sram->preclock_posedge_clock();
}

/*f sram_srw_clock
 */
static t_sl_error_level sram_srw_clock( void *handle )
{
    c_sram_srw *sram;

     sram = (c_sram_srw *)handle;
     return sram->clock_posedge_clock();
}

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
c_sram_srw::c_sram_srw( class c_engine *eng, void *eng_handle )
{
    engine = eng;
    engine_handle = eng_handle;

    engine->register_delete_function( engine_handle, (void *)this, sram_srw_delete );
    engine->register_reset_function( engine_handle, (void *)this, sram_srw_reset );

    memory = NULL;
    filename = engine->get_option_string( engine_handle, "filename", "" );
    reset_type = engine->get_option_int( engine_handle, "reset_type", 2 );
    reset_value = engine->get_option_int( engine_handle, "reset_value", 0 );
    data_size = engine->get_option_int( engine_handle, "size", 0 );
    data_width = engine->get_option_int( engine_handle, "width", 0 );
    bits_per_enable = engine->get_option_int( engine_handle, "bits_per_enable", 0 );
    verbose = engine->get_option_int( engine_handle, "verbose", 0 );
    dprintf_action_address = engine->get_option_int( engine_handle, "dprintf_action_address", 0 );
    dprintf_data_address = engine->get_option_int( engine_handle, "dprintf_data_address", 0 );
    address_width = sl_log2(data_size);

    if (verbose)
    {
        fprintf(stderr,"%s:Reset type %d value %x\n", engine->get_instance_name(engine_handle), reset_type, reset_value );
    }

    if ((data_width<1) || (data_width>(int)(sizeof(int)*MAX_WIDTH*8)))
    {
        engine->error->add_error( (void *)"se_internal_module__sram_srw_instantiate", error_level_serious, error_number_general_error_sd, 0,
                                  error_arg_type_malloc_string, "Data width out of range (1 to max)",
                                  error_arg_type_integer, data_width,
                                  error_arg_type_none );
    }
    if ((bits_per_enable<0) || (data_width<bits_per_enable))
    {
        engine->error->add_error( (void *)"se_internal_module__sram_srw_instantiate", error_level_serious, error_number_general_error_sd, 0,
                                  error_arg_type_malloc_string, "Bits per enable should be zero to data width",
                                  error_arg_type_integer, bits_per_enable,
                                  error_arg_type_none );
    }
    if (address_width<1)
    {
        engine->error->add_error( (void *)"se_internal_module__sram_srw_instantiate", error_level_serious, error_number_general_error_sd, 0,
                                  error_arg_type_malloc_string, "Address width out of range (1 to max)",
                                  error_arg_type_integer, address_width,
                                  error_arg_type_none );
    }
    number_of_enables = 0;
    if (bits_per_enable>0)
    {
        number_of_enables = data_width / bits_per_enable;
    }
    data_byte_width = BITS_TO_BYTES(data_width); // In 64-bit words
    data_word_width = (data_width+8*sizeof(t_sl_uint64)-1)/(sizeof(t_sl_uint64)*8); // In 64-bit words, for the signals

    engine->register_clock_fns( engine_handle, (void *)this, "sram_clock", sram_srw_preclock, sram_srw_clock );

    engine->register_input_signal( engine_handle, "select", 1, &inputs.select, 1 );
    engine->register_input_used_on_clock( engine_handle, "select", "sram_clock", 1 );
    inputs.select = NULL;

    engine->register_input_signal( engine_handle, "address", address_width, &inputs.address );
    engine->register_input_used_on_clock( engine_handle, "address", "sram_clock", 1 );
    engine->register_input_signal( engine_handle, "read_not_write", 1, &inputs.read_not_write );
    engine->register_input_used_on_clock( engine_handle, "read_not_write", "sram_clock", 1 );
    engine->register_input_signal( engine_handle, "write_data", data_width, &inputs.write_data );
    engine->register_input_used_on_clock( engine_handle, "write_data", "sram_clock", 1 );
    if (number_of_enables)
    {
        engine->register_input_signal( engine_handle, "write_enable", number_of_enables, &inputs.write_enable );
        engine->register_input_used_on_clock( engine_handle, "write_enable", "sram_clock", 1 );
    }

    engine->register_output_signal( engine_handle, "data_out", data_width, posedge_clock_state.data_out );
    engine->register_output_generated_on_clock( engine_handle, "data_out", "sram_clock", 1 );
    log_event_array = engine->log_event_register_array( engine_handle, log_event_descriptor, (t_se_signal_value *)&posedge_clock_state );

//#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
//static t_sram_srw_posedge_clock_state *___sram_srw_posedge_clock__ptr;
//static t_engine_state_desc state_desc_sram_srw_posedge_clock[] =
//{
//    {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
//};
//
//    engine->register_state_desc( engine_handle, 1, state_desc_sram_srw_posedge_int_clock, &posedge_int_clock_state, NULL );
}

/*f c_sram_srw::~c_sram_srw
*/
c_sram_srw::~c_sram_srw()
{
    delete_instance();
}

/*f c_sram_srw::delete_instance
*/
t_sl_error_level c_sram_srw::delete_instance( void )
{
    if (memory)
    {
        free(memory);
        memory = NULL;
    }
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for sram_srw
*/
/*f c_sram_srw::reset
*/
t_sl_error_level c_sram_srw::reset( int pass )
{
    int i;
    if (pass==0)
    {
        if (memory)
        {
            free(memory);
            memory = NULL;
        }
        sl_mif_allocate_and_read_mif_file( engine->error,
                                           strcmp(filename,"")?filename:NULL,
                                           "se_internal_module__sram_srw",
                                           0, // address offset
                                           data_size, // max size
                                           data_width, // bit size
                                           0, // bit start
                                           reset_type,
                                           reset_value,
                                           (int **)&memory,
                                           NULL,
                                           NULL );
    }
    posedge_clock_state.address = 0;
    posedge_clock_state.read_not_write = 0;
    for (i=0; i<MAX_WIDTH; i++)
    {
        posedge_clock_state.write_data[i] = 0;
        posedge_clock_state.write_enable[i] = 0;
        posedge_clock_state.data_out[i] = 0;
    }
    return error_level_okay;
}

/*f c_sram_srw::preclock_posedge_clock
*/
t_sl_error_level c_sram_srw::preclock_posedge_clock( void )
{
    int i;

    memcpy( &next_posedge_clock_state, &posedge_clock_state, sizeof(posedge_clock_state) );

    next_posedge_clock_state.selected = 1;
    if (inputs.select) next_posedge_clock_state.selected = (inputs.select[0]!=0);
    next_posedge_clock_state.address = inputs.address[0];
    next_posedge_clock_state.read_not_write = inputs.read_not_write[0];
    for (i=0; i<=data_word_width; i++)
    {
        next_posedge_clock_state.write_data[i] = inputs.write_data[i];
        next_posedge_clock_state.data_out[i] = 0;
    }
    if (number_of_enables)
    {
        for (i=0; i<=number_of_enables/((int)sizeof(int)*8); i++)
        {
            next_posedge_clock_state.write_enable[i] = inputs.write_enable[i];
        }
    }

    return error_level_okay;
}

/*f c_sram_srw::clock_posedge_clock
*/
t_sl_error_level c_sram_srw::clock_posedge_clock( void )
{
    memcpy( &posedge_clock_state, &next_posedge_clock_state, sizeof(posedge_clock_state) );

    if (!posedge_clock_state.selected)
        return error_level_okay;

    if (posedge_clock_state.read_not_write)
    {
        engine->log_event_occurred( engine_handle, log_event_array, 0 );
        if ((posedge_clock_state.address<data_size) && memory)
        {
            memcpy( posedge_clock_state.data_out, ((unsigned char *)memory)+data_byte_width*posedge_clock_state.address, data_byte_width );
        }
        else
        {
            engine->error->add_error( (void *)"se_internal_module__sram_srw_clock", error_level_warning, error_number_se_dated_assertion, 0,
                                      error_arg_type_integer, engine->cycle(),
                                      error_arg_type_const_string, "SRAM srw",
                                      error_arg_type_const_string, "Attempt to read a memory location outside the memory",
                                      error_arg_type_none );
            posedge_clock_state.data_out[0] = 0;
        }
        if (verbose)
        {
            fprintf(stderr,"%s:%d:Reading address %08llx data word %08llx\n", engine->get_instance_name(engine_handle), engine->cycle(), posedge_clock_state.address, posedge_clock_state.data_out[0] );
        }
    }
    else
    {
        engine->log_event_occurred( engine_handle, log_event_array, 1 );
        if ((dprintf_action_address) && (posedge_clock_state.address == dprintf_action_address))
        {
            t_sl_uint64 args[8];
            unsigned char message[256];
            char buffer[1024];
            t_sl_uint64 address;
            address = posedge_clock_state.write_data[0]; // byte address
            if (memory)
            {
                memcpy( args, memory+dprintf_data_address*data_word_width, 8*sizeof(t_sl_uint64) );
                strncpy( (char *)message, ((char*)memory)+address, sizeof(message) );
                message[sizeof(message)-1] = 0;
                sprintf( buffer, (const char *)message, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7] );
                if (strlen(buffer)>0)
                {
                    if (buffer[strlen(buffer)-1]=='\n')
                    {
                        buffer[strlen(buffer)-1] = 0;
                    }
                }
                fprintf( stderr, "%s:%d:Dprintf:'%s'\n", engine->get_instance_name(engine_handle), engine->cycle(), buffer );
            }
        }
        else if ((posedge_clock_state.address<data_size) && memory)
        {
            int i;
            for (i=0; i<data_byte_width; i++) // i for each byte
            {
                int enable_num;
                enable_num = (i*8)/(bits_per_enable==0?1:bits_per_enable);
                if ( (number_of_enables==0) ||                                  // if no enables
                     ((posedge_clock_state.write_enable[0]&(1<<enable_num))) ) // if enable set
                {
                    ((unsigned char *)memory)[data_byte_width*posedge_clock_state.address+i] = ((unsigned char *)(posedge_clock_state.write_data))[i];
                }
            }
            if ( verbose &&
                 ((number_of_enables==0) || (posedge_clock_state.write_enable[0]) ) )
            {
                fprintf(stderr,"%s:%d:Writing address %08llx data word %08llx be %08llx\n", engine->get_instance_name(engine_handle), engine->cycle(), posedge_clock_state.address, posedge_clock_state.write_data[0], posedge_clock_state.write_enable[0] );
            }
        }
        else
        {
            fprintf(stderr,"c_memory::Out of range write\n");
        }
    }
    return error_level_okay;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


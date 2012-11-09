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
#define WHERE_I_AM {}
#define WHERE_I_AM_VERBOSE {fprintf(stderr,"%s:%s:%d\n",__FILE__,__func__,__LINE__ );}

/*a Types
*/
/*a Static variables for sram_srw
*/
/*v log_event_descriptor
 */
static t_sram_posedge_clock_state *___sram_posedge_clock_state_log;
static t_engine_text_value_pair log_event_descriptor[] = 
{
    {"read", 2 },
    {"address",  (t_se_signal_value *)&(___sram_posedge_clock_state_log->address)       - (t_se_signal_value *)___sram_posedge_clock_state_log },
    {"data_out", (t_se_signal_value *)&(___sram_posedge_clock_state_log->data_out[0])      - (t_se_signal_value *)___sram_posedge_clock_state_log },
    {"write", 3 },
    {"address",  (t_se_signal_value *)&(___sram_posedge_clock_state_log->address)       - (t_se_signal_value *)___sram_posedge_clock_state_log },
    {"data",     (t_se_signal_value *)&(___sram_posedge_clock_state_log->write_data[0])    - (t_se_signal_value *)___sram_posedge_clock_state_log },
    {"enables",  (t_se_signal_value *)&(___sram_posedge_clock_state_log->write_enable[0]) - (t_se_signal_value *)___sram_posedge_clock_state_log },
    {NULL, 0}
};

/*a Static functions
 */
/*f sram_delete - simple callback wrapper for the main method
*/
static t_sl_error_level sram_delete( void *handle )
{
    c_se_internal_module__sram *sram = (c_se_internal_module__sram *)handle;
    t_sl_error_level result = sram->delete_instance();
    delete( sram );
    return result;
}

/*f sram_reset
 */
static t_sl_error_level sram_reset( void *handle, int pass )
{
    c_se_internal_module__sram *sram = (c_se_internal_module__sram *)handle;
    return sram->reset( pass );
}

/*f sram_message
 */
static t_sl_error_level sram_message( void *handle, void *arg )
{
    c_se_internal_module__sram *sram = (c_se_internal_module__sram *)handle;
    return sram->message( (t_se_message *)arg );
}

/*f sram_preclock
 */
static t_sl_error_level sram_preclock( void *handle )
{
    t_sram_clock_domain *cd = (t_sram_clock_domain *)handle;
    WHERE_I_AM;
    return cd->mod->preclock_posedge_clock(cd);
}

/*f sram_clock
 */
static t_sl_error_level sram_clock( void *handle )
{
    t_sram_clock_domain *cd = (t_sram_clock_domain *)handle;
    WHERE_I_AM;
    return cd->mod->clock_posedge_clock(cd);
}

/*a Constructors and destructors for sram_srw
*/
/*f c_se_internal_module__sram::c_se_internal_module__sram
*/
c_se_internal_module__sram::c_se_internal_module__sram( class c_engine *eng, void *eng_handle, int multiport )
{
    engine = eng;
    engine_handle = eng_handle;
    memory = NULL;
    filename = engine->get_option_string( engine_handle, "filename", "" );
    reset_type = engine->get_option_int( engine_handle, "reset_type", 2 );
    reset_value = engine->get_option_int( engine_handle, "reset_value", 0 );
    data_size = engine->get_option_int( engine_handle, "size", 0 );
    data_width = engine->get_option_int( engine_handle, "width", 0 );
    bits_per_enable = engine->get_option_int( engine_handle, "bits_per_enable", 0 );
    verbose = engine->get_option_int( engine_handle, "verbose", 0 );
    num_ports = 1;
    if (multiport) { num_ports = engine->get_option_int( engine_handle, "num_ports", 1 ); }

    // We can add event mask/match for addresses
    //dprintf_action_address = engine->get_option_int( engine_handle, "dprintf_action_address", 0 );
    //dprintf_data_address = engine->get_option_int( engine_handle, "dprintf_data_address", 0 );

    address_width = sl_log2(data_size);

    if ((data_width<1) || (data_width>(int)(sizeof(int)*MAX_WIDTH*8)))
    {
        engine->add_error( (void *)"se_internal_module__sram_srw_instantiate", error_level_serious, error_number_general_error_sd, 0,
                                  error_arg_type_malloc_string, "Data width out of range (1 to max)",
                                  error_arg_type_integer, data_width,
                                  error_arg_type_none );
    }
    if ((bits_per_enable<0) || (data_width<bits_per_enable))
    {
        engine->add_error( (void *)"se_internal_module__sram_srw_instantiate", error_level_serious, error_number_general_error_sd, 0,
                                  error_arg_type_malloc_string, "Bits per enable should be zero to data width",
                                  error_arg_type_integer, bits_per_enable,
                                  error_arg_type_none );
    }
    if (address_width<1)
    {
        engine->add_error( (void *)"se_internal_module__sram_srw_instantiate", error_level_serious, error_number_general_error_sd, 0,
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

    engine->register_delete_function( engine_handle, (void *)this,  sram_delete );
    engine->register_reset_function( engine_handle, (void *)this,   sram_reset );
    engine->register_message_function( engine_handle, (void *)this, sram_message );

    clock_domains = (t_sram_clock_domain *)malloc(sizeof(t_sram_clock_domain)*num_ports);
    for (int i=0; i<num_ports; i++)
    {
        char clk_name[256];
        char signal_name[256];

        clock_domains[i].port = i;
        clock_domains[i].mod = this;

        sprintf( clk_name, "sram_clock_%d", i );
        if (!multiport) sprintf( clk_name, "sram_clock" );
        engine->register_clock_fns( engine_handle, (void *)(&clock_domains[i]), clk_name, sram_preclock, sram_clock );

        sprintf( signal_name, "select_%d", i );
        if (!multiport) sprintf( signal_name, "select" );
        engine->register_input_signal( engine_handle, signal_name, 1, &(clock_domains[i].inputs.select), 1 );
        engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );
        clock_domains[i].inputs.select = NULL;

        sprintf( signal_name, "address_%d", i );
        if (!multiport) sprintf( signal_name, "address" );
        engine->register_input_signal( engine_handle, signal_name, address_width, &(clock_domains[i].inputs.address) );
        engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );

        sprintf( signal_name, "read_not_write_%d", i );
        if (!multiport) sprintf( signal_name, "read_not_write" );
        engine->register_input_signal( engine_handle, signal_name, 1, &(clock_domains[i].inputs.read_not_write) );
        engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );

        sprintf( signal_name, "write_data_%d", i );
        if (!multiport) sprintf( signal_name, "write_data" );
        engine->register_input_signal( engine_handle, signal_name, data_width, &(clock_domains[i].inputs.write_data) );
        engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );

        if (number_of_enables)
        {
            sprintf( signal_name, "write_enable_%d", i );
            if (!multiport) sprintf( signal_name, "write_enable" );
            engine->register_input_signal( engine_handle, signal_name, number_of_enables, &(clock_domains[i].inputs.write_enable) );
            engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );
        }

        sprintf( signal_name, "data_out_%d", i );
        if (!multiport) sprintf( signal_name, "data_out" );
        engine->register_output_signal( engine_handle, signal_name, data_width, clock_domains[i].posedge_clock_state.data_out );
        engine->register_output_generated_on_clock( engine_handle, signal_name, clk_name, 1 );
        clock_domains[i].log_event_array = engine->log_event_register_array( engine_handle, log_event_descriptor, (t_se_signal_value *)&clock_domains[i].posedge_clock_state );
    }
}

/*f c_se_internal_module__sram::~c_se_internal_module__sram
*/
c_se_internal_module__sram::~c_se_internal_module__sram()
{
    delete_instance();
}

/*f c_se_internal_module__sram::delete_instance
*/
t_sl_error_level c_se_internal_module__sram::delete_instance( void )
{
    if (memory)
    {
        free(memory);
        memory = NULL;
    }
    return error_level_okay;
}

/*a Main class methods - reset, read, write, message
*/
/*f c_se_internal_module__sram::reset
*/
t_sl_error_level c_se_internal_module__sram::reset( int pass )
{
    if (pass==0)
    {
        if (memory)
        {
            free(memory);
            memory = NULL;
        }
        sl_mif_allocate_and_read_mif_file( engine->error,
                                           strcmp(filename,"")?filename:NULL,
                                           "c_se_internal_module__sram",
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
    return error_level_okay;
}

/*f c_se_internal_module__sram::read
*/
t_sl_error_level c_se_internal_module__sram::read( unsigned int address, t_sl_uint64 *data_out, int clock_domain, int error_on_out_of_range )
{

    if ((address<data_size) && memory)
    {
        memcpy( data_out, ((unsigned char *)memory)+data_byte_width*address, data_byte_width );
    }
    else
    {
        if (error_on_out_of_range)
        {
            engine->add_error( (void *)"se_internal_module__sram_srw_clock", error_level_warning, error_number_se_dated_assertion, 0,
                                      error_arg_type_integer, engine->cycle(),
                                      error_arg_type_const_string, "SRAM srw",
                                      error_arg_type_const_string, "Attempt to read a memory location outside the memory",
                                      error_arg_type_none );
        }
        bzero( data_out, data_byte_width );
    }

    if (clock_domain>=0)
    {
        if (log_filter[0].enable &&
            ((address & log_filter[0].mask)==log_filter[0].match))
        {
            engine->log_event_occurred( engine_handle, clock_domains[clock_domain].log_event_array, 0 );
        }
        if (verbose)
        {
            fprintf(stderr,"%s:%d:Reading address %08x data word %08llx\n", engine->get_instance_name(engine_handle), engine->cycle(), address, data_out[0] );
        }
    }
    return error_level_okay;
}

/*f c_se_internal_module__sram::write
*/
t_sl_error_level c_se_internal_module__sram::write( unsigned int address, t_sl_uint64 *data_in, t_sl_uint64 *write_enable, int clock_domain, int error_on_out_of_range )
{
    if ((address<data_size) && memory)
    {
        int i;
        for (i=0; i<data_byte_width; i++) // i for each byte
        {
            int enable_num;
            enable_num = (i*8)/(bits_per_enable==0?1:bits_per_enable);
            if ( (number_of_enables==0) || (write_enable==NULL) ||
                 ((write_enable[0]&(1<<enable_num))) ) // if enable set
            {
                ((unsigned char *)memory)[data_byte_width*address+i] = ((unsigned char *)(data_in))[i];
            }
        }
    }
    else
    {
        if (error_on_out_of_range)
        {
            engine->add_error( (void *)"se_internal_module__sram_srw_clock", error_level_warning, error_number_se_dated_assertion, 0,
                                      error_arg_type_integer, engine->cycle(),
                                      error_arg_type_const_string, "SRAM srw",
                                      error_arg_type_const_string, "Attempt to write a memory location outside the memory",
                                      error_arg_type_none );
        }
    }
    if (clock_domain>=0)
    {
        if ( verbose &&
             ((number_of_enables==0) || (write_enable==NULL) || (write_enable[0]) ) )
        {
            fprintf(stderr,"%s:%d:Writing address %08x data word %08llx be %08llx\n", engine->get_instance_name(engine_handle), engine->cycle(), address, data_in[0], write_enable[0] );
        }
        if (log_filter[1].enable &&
            ((address & log_filter[1].mask)==log_filter[1].match))
        {
            engine->log_event_occurred( engine_handle, clock_domains[clock_domain].log_event_array, 1 );
        }
    }
    return error_level_okay;
}

/*f c_se_internal_module__sram::message
*/
t_sl_error_level c_se_internal_module__sram::message( t_se_message *message )
{
    switch (message->reason)
    {
    case se_message_reason_set_option:
        const char *option_name;
        option_name = (const char *)message->data.ptrs[0];
        message->response_type = se_message_response_type_int;
        message->response = 1;
        if (!strcmp(option_name,"filename"))
        {
            filename = sl_str_alloc_copy((const char *)message->data.ptrs[1]);
        }
        else if (!strcmp(option_name,"reset_type"))
        {
            reset_type = (int)(t_sl_uint64)message->data.ptrs[1];
        }
        else if (!strcmp(option_name,"reset_value"))
        {
            reset_value = (int)(t_sl_uint64)message->data.ptrs[1];
        }
        else if (!strcmp(option_name,"verbose"))
        {
            verbose = (int)(t_sl_uint64)message->data.ptrs[1];
        }
        else
        {
            message->response = -1;
        }
        break;
    case se_message_reason_force_reset:
        message->response_type = se_message_response_type_int;
        message->response = 1;
        reset( message->data.ints[0] );
        break;
    case se_message_reason_interrogate_state:
        message->response_type = se_message_response_type_ptrs;
        message->response = 1;
        message->data.ptrs[0] = NULL;
        message->data.values[1] = data_width;
        message->data.values[2] = data_size;
        break;
    case se_message_reason_log_filter:
        if (!strcmp((const char *)message->data.ptrs[0],"read"))
        {
            log_filter[0].enable = (message->data.values[1]!=0);
            log_filter[0].mask   = message->data.values[1];
            log_filter[0].match  = message->data.values[2];
        }
        if (!strcmp((const char *)message->data.ptrs[0],"write"))
        {
            log_filter[1].enable = (message->data.values[1]!=0);
            log_filter[1].mask   = message->data.values[1];
            log_filter[1].match  = message->data.values[2];
        }
    case se_message_reason_read:
        if (message->data.values[0] == 0) // subreason of 0 -> read single value - put result in values[2]
        {
            message->response_type = se_message_response_type_values;
            message->data.values[2] = 0;
            message->data.values[3] = 0;
            read( (unsigned int)message->data.values[1], &(message->data.values[2]), -1, 0 );
        }
        else // read to a block of memory; values[0] == number of values, values[1]=address, values[2]=byte size of buffer, ptrs[3] -> data buffer
        {
            t_se_signal_value *ptr;
            int num_to_read;
            unsigned int address;
            num_to_read = message->data.values[0];
            address = message->data.values[1];
            if ((num_to_read*data_word_width*sizeof(t_se_signal_value))>message->data.values[1])
            {
                num_to_read = message->data.values[2] / (data_word_width*sizeof(t_se_signal_value));
            }
            message->data.values[0] = num_to_read;
            ptr = (t_se_signal_value *)message->data.ptrs[3];
            for (int i=0; i<num_to_read; i++, ptr+=data_word_width, address++)
            {
                read( address, ptr, -1, 0 );
            }
        }
        message->response = 1;
        break;
    case se_message_reason_write:
        if (message->data.values[0] == 0) // subreason of 0 -> write single value
        {
            message->response_type = se_message_response_type_values;
            write( (unsigned int)message->data.values[1], &(message->data.values[2]), NULL, -1, 0 );
        }
        else // write from a block of memory; values[0] == number of values, values[1]=address, values[2]=byte size of buffer, ptrs[3] -> data buffer
        {
            t_se_signal_value *ptr;
            int num_to_write;
            unsigned int address;
            num_to_write = message->data.values[0];
            address = message->data.values[1];
            if ((num_to_write*data_word_width*sizeof(t_se_signal_value))>message->data.values[1])
            {
                num_to_write = message->data.values[2] / (data_word_width*sizeof(t_se_signal_value));
            }
            message->data.values[0] = num_to_write;
            ptr = (t_se_signal_value *)message->data.ptrs[3];
            for (int i=0; i<num_to_write; i++, ptr+=data_word_width, address++)
            {
                write( address, ptr, NULL, -1, 0 );
            }
        }
        message->response = 1;
        break;
    case se_message_reason_action_one:
        int address;
        message->response_type = se_message_response_type_int;
        message->response = 1;
        for (address = message->data.ints[0]; address<message->data.ints[1]; address++)
        {
            fprintf( stderr, "%08x: %016llx\n", address, memory[address] );
        }
        break;
    }
    return error_level_okay;
}

/*f c_se_internal_module__sram::preclock_posedge_clock
*/
t_sl_error_level c_se_internal_module__sram::preclock_posedge_clock( t_sram_clock_domain *cd )
{
    int i;

    WHERE_I_AM;

    memcpy( &(cd->next_posedge_clock_state), &(cd->posedge_clock_state), sizeof(t_sram_posedge_clock_state) );

    cd->next_posedge_clock_state.selected = 1;
    if (cd->inputs.select) cd->next_posedge_clock_state.selected = (cd->inputs.select[0]!=0);
    cd->next_posedge_clock_state.address = cd->inputs.address[0];
    cd->next_posedge_clock_state.read_not_write = cd->inputs.read_not_write[0];
    for (i=0; i<=data_word_width; i++) // Copy input and clear output data
    {
        cd->next_posedge_clock_state.write_data[i] = cd->inputs.write_data[i];
        cd->next_posedge_clock_state.data_out[i] = 0;
    }
    if (number_of_enables)
    {
        for (i=0; i<=number_of_enables/((int)sizeof(int)*8); i++)
        {
            cd->next_posedge_clock_state.write_enable[i] = cd->inputs.write_enable[i];
        }
    }

    return error_level_okay;
}

/*f c_se_internal_module__sram::clock_posedge_clock
*/
t_sl_error_level c_se_internal_module__sram::clock_posedge_clock( t_sram_clock_domain *cd )
{
    memcpy( &(cd->posedge_clock_state), &(cd->next_posedge_clock_state), sizeof(t_sram_posedge_clock_state) );

    if (!cd->posedge_clock_state.selected)
        return error_level_okay;

    if (cd->posedge_clock_state.read_not_write)
    {
        read( cd->posedge_clock_state.address, cd->posedge_clock_state.data_out, cd->port, 1 );
    }
    else
    {
        write( cd->posedge_clock_state.address, cd->posedge_clock_state.write_data, cd->posedge_clock_state.write_enable, cd->port, 1 );
    }
    return error_level_okay;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


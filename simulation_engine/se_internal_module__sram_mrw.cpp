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
/*t uint32, uint64
 */
typedef t_sl_uint32 uint32;
typedef t_sl_uint64 uint64;

/*t t_sram_mrw_posedge_clock_state
*/
typedef struct t_sram_mrw_posedge_clock_state
{
    int selected;
    uint64 address;
    uint64 read_not_write;
    uint64 write_data[MAX_WIDTH];
    uint64 write_enable[MAX_WIDTH];
    uint64 data_out[MAX_WIDTH];
} t_sram_mrw_posedge_clock_state;

/*t t_sram_mrw_inputs
*/
typedef struct t_sram_mrw_inputs
{
    uint64 *select;
    uint64 *address;
    uint64 *read_not_write;
    uint64 *write_data;
    uint64 *write_enable;
} t_sram_mrw_inputs;

/*t t_clock_domain
 */
typedef struct t_clock_domain
{
    class c_sram_mrw *mod;
    int port;
    t_sram_mrw_inputs inputs;
    t_sram_mrw_posedge_clock_state next_posedge_clock_state;
    t_sram_mrw_posedge_clock_state posedge_clock_state;
    struct t_engine_log_event_array *log_event_array;
} t_clock_domain;

/*t c_sram_mrw
*/
class c_sram_mrw
{
public:
    c_sram_mrw( class c_engine *eng, void *eng_handle );
    ~c_sram_mrw();
    t_sl_error_level delete_instance( void );
    t_sl_error_level reset( int pass );
    t_sl_error_level preclock_posedge_clock( t_clock_domain *cd );
    t_sl_error_level clock_posedge_clock( t_clock_domain *cd );
private:
    c_engine *engine;
    void *engine_handle;
    const char *filename;
    int num_ports;
    int reset_type;
    int reset_value;
    int address_width;
    int data_width; // in bits
    uint64 data_size;
    int data_byte_width; // in bytes - really 1, 2, 4, 8
    int data_word_width; // in 64-bit words
    int bits_per_enable; // must be 8 at present
    int number_of_enables;
    int verbose;
    t_clock_domain *clock_domains; // Array of clock domain structures, one per port
    uint64 *memory;
};

/*a Statics
 */
/*v log_event_descriptor
 */
static t_sram_mrw_posedge_clock_state *___sram_mrw_posedge_clock_state_log;
static t_engine_text_value_pair log_event_descriptor[] = 
{
    {"read", 2 },
    {"address", (t_se_signal_value *)&(___sram_mrw_posedge_clock_state_log->address) - (t_se_signal_value *)___sram_mrw_posedge_clock_state_log },
    {"data_out", (t_se_signal_value *)&(___sram_mrw_posedge_clock_state_log->data_out) - (t_se_signal_value *)___sram_mrw_posedge_clock_state_log },
    {"write", 3 },
    {"address", (t_se_signal_value *)&(___sram_mrw_posedge_clock_state_log->address) - (t_se_signal_value *)___sram_mrw_posedge_clock_state_log },
    {"data",    (t_se_signal_value *)&(___sram_mrw_posedge_clock_state_log->data_out) - (t_se_signal_value *)___sram_mrw_posedge_clock_state_log },
    {"enables", (t_se_signal_value *)&(___sram_mrw_posedge_clock_state_log->write_enable) - (t_se_signal_value *)___sram_mrw_posedge_clock_state_log },
    {NULL, 0}
};

/*a Static wrapper functions for SRAM module
 */
/*f sram_mrw_delete - simple callback wrapper for the main method
*/
static t_sl_error_level sram_mrw_delete( void *handle )
{
    c_sram_mrw *mod;
    t_sl_error_level result;
    mod = (c_sram_mrw *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f sram_mrw_reset
 */
static t_sl_error_level sram_mrw_reset( void *handle, int pass )
{
    c_sram_mrw *sram;
    sram = ((c_sram_mrw *)handle);
    return sram->reset( pass );
}

/*f sram_mrw_preclock
 */
static t_sl_error_level sram_mrw_preclock( void *handle )
{
    c_sram_mrw *sram;
    t_clock_domain *cd;
    cd = (t_clock_domain *)handle;
    sram = cd->mod;
    return sram->preclock_posedge_clock(cd);
}

/*f sram_mrw_clock
 */
static t_sl_error_level sram_mrw_clock( void *handle )
{
    c_sram_mrw *sram;
    t_clock_domain *cd;
    cd = (t_clock_domain *)handle;
    sram = cd->mod;
    return sram->clock_posedge_clock(cd);
}

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
c_sram_mrw::c_sram_mrw( class c_engine *eng, void *eng_handle )
{
    int i;

    engine = eng;
    engine_handle = eng_handle;

    engine->register_delete_function( engine_handle, (void *)this, sram_mrw_delete );
    engine->register_reset_function( engine_handle, (void *)this, sram_mrw_reset );

    memory = NULL;
    filename = engine->get_option_string( engine_handle, "filename", "" );
    num_ports = engine->get_option_int( engine_handle, "num_ports", 1 );
    reset_type = engine->get_option_int( engine_handle, "reset_type", 2 );
    reset_value = engine->get_option_int( engine_handle, "reset_value", 0 );
    data_size = engine->get_option_int( engine_handle, "size", 0 );
    data_width = engine->get_option_int( engine_handle, "width", 0 );
    bits_per_enable = engine->get_option_int( engine_handle, "bits_per_enable", 0 );
    verbose = engine->get_option_int( engine_handle, "verbose", 0 );
    address_width = sl_log2(data_size);

    if (verbose)
    {
        fprintf(stderr,"%s:Reset type %d value %x\n", engine->get_instance_name(engine_handle), reset_type, reset_value );
    }

    if ((num_ports<1) || (num_ports>10))
    {
        engine->error->add_error( (void *)"se_internal_module__sram_mrw_instantiate", error_level_serious, error_number_general_error_sd, 0,
                                  error_arg_type_malloc_string, "Multiport RAM must have 1 to 10 ports",
                                  error_arg_type_integer, num_ports,
                                  error_arg_type_none );
    }
    if ((data_width<1) || (data_width>(int)(sizeof(int)*MAX_WIDTH*8)))
    {
        engine->error->add_error( (void *)"se_internal_module__sram_mrw_instantiate", error_level_serious, error_number_general_error_sd, 0,
                                  error_arg_type_malloc_string, "Data width out of range (1 to max)",
                                  error_arg_type_integer, data_width,
                                  error_arg_type_none );
    }
    if ((bits_per_enable<0) || (data_width<bits_per_enable))
    {
        engine->error->add_error( (void *)"se_internal_module__sram_mrw_instantiate", error_level_serious, error_number_general_error_sd, 0,
                                  error_arg_type_malloc_string, "Bits per enable should be zero to data width",
                                  error_arg_type_integer, bits_per_enable,
                                  error_arg_type_none );
    }
    if (address_width<1)
    {
        engine->error->add_error( (void *)"se_internal_module__sram_mrw_instantiate", error_level_serious, error_number_general_error_sd, 0,
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
    clock_domains = (t_clock_domain *)malloc(sizeof(t_clock_domain)*num_ports);

    for (i=0; i<num_ports; i++)
    {
        char clk_name[256];
        char signal_name[256];

        clock_domains[i].port = i;
        clock_domains[i].mod = this;

        sprintf( clk_name, "sram_clock_%d", i );
        engine->register_clock_fns( engine_handle, (void *)(&clock_domains[i]), clk_name, sram_mrw_preclock, sram_mrw_clock );

        sprintf( signal_name, "select_%d", i );
        engine->register_input_signal( engine_handle, signal_name, 1, &(clock_domains[i].inputs.select), 1 );
        engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );
        clock_domains[i].inputs.select = NULL;

        sprintf( signal_name, "address_%d", i );
        engine->register_input_signal( engine_handle, signal_name, address_width, &(clock_domains[i].inputs.address) );
        engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );

        sprintf( signal_name, "read_not_write_%d", i );
        engine->register_input_signal( engine_handle, signal_name, 1, &(clock_domains[i].inputs.read_not_write) );
        engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );

        sprintf( signal_name, "write_data_%d", i );
        engine->register_input_signal( engine_handle, signal_name, data_width, &(clock_domains[i].inputs.write_data) );
        engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );

        if (number_of_enables)
        {
            sprintf( signal_name, "write_enable_%d", i );
            engine->register_input_signal( engine_handle, signal_name, number_of_enables, &(clock_domains[i].inputs.write_enable) );
            engine->register_input_used_on_clock( engine_handle, signal_name, clk_name, 1 );
        }

        sprintf( signal_name, "data_out_%d", i );
        engine->register_output_signal( engine_handle, signal_name, data_width, clock_domains[i].posedge_clock_state.data_out );
        engine->register_output_generated_on_clock( engine_handle, signal_name, clk_name, 1 );
        clock_domains[i].log_event_array = engine->log_event_register_array( engine_handle, log_event_descriptor, (t_se_signal_value *)&clock_domains[i].posedge_clock_state );
    }

}

/*f c_sram_mrw::~c_sram_mrw
*/
c_sram_mrw::~c_sram_mrw()
{
    delete_instance();
}

/*f c_sram_mrw::delete_instance
*/
t_sl_error_level c_sram_mrw::delete_instance( void )
{
    if (memory)
    {
        free(memory);
        memory = NULL;
    }
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for sram_mrw
*/
/*f c_sram_mrw::reset
*/
t_sl_error_level c_sram_mrw::reset( int pass )
{
    int i, j;
    if (pass==0)
    {
        if (memory)
        {
            free(memory);
            memory = NULL;
        }
        sl_mif_allocate_and_read_mif_file( engine->error,
                                           strcmp(filename,"")?filename:NULL,
                                           "se_internal_module__sram_mrw",
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
    for (i=0; i<num_ports; i++)
    {
        clock_domains[i].posedge_clock_state.address = 0;
        clock_domains[i].posedge_clock_state.read_not_write = 0;
        for (j=0; j<MAX_WIDTH; j++)
        {
            clock_domains[i].posedge_clock_state.write_data[j] = 0;
            clock_domains[i].posedge_clock_state.write_enable[j] = 0;
            clock_domains[i].posedge_clock_state.data_out[j] = 0;
        }
    }
    return error_level_okay;
}

/*f c_sram_mrw::preclock_posedge_clock
*/
t_sl_error_level c_sram_mrw::preclock_posedge_clock( t_clock_domain *cd )
{
    int i;

    memcpy( &(cd->next_posedge_clock_state), &(cd->posedge_clock_state), sizeof(t_sram_mrw_posedge_clock_state) );

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

/*f c_sram_mrw::clock_posedge_clock
*/
t_sl_error_level c_sram_mrw::clock_posedge_clock( t_clock_domain *cd )
{
    memcpy( &(cd->posedge_clock_state), &(cd->next_posedge_clock_state), sizeof(t_sram_mrw_posedge_clock_state) );

    if (!cd->posedge_clock_state.selected)
        return error_level_okay;

    if (cd->posedge_clock_state.read_not_write)
    {
        if ((cd->posedge_clock_state.address<data_size) && memory)
        {
            memcpy( cd->posedge_clock_state.data_out, ((unsigned char *)memory)+data_byte_width*cd->posedge_clock_state.address, data_byte_width );
        }
        else
        {
            engine->error->add_error( (void *)"se_internal_module__sram_mrw_clock", error_level_warning, error_number_se_dated_assertion, 0,
                                      error_arg_type_integer, engine->cycle(),
                                      error_arg_type_const_string, "SRAM mrw",
                                      error_arg_type_const_string, "Attempt to read a memory location outside the memory",
                                      error_arg_type_none );
            cd->posedge_clock_state.data_out[0] = 0;
        }
        engine->log_event_occurred( engine_handle, cd->log_event_array, 0 );
        if (verbose)
        {
            fprintf(stderr,"%s:%d:Port %d:Reading address %08llx data word %08llx\n",
                    engine->get_instance_name(engine_handle),
                    engine->cycle(),
                    cd->port,
                    cd->posedge_clock_state.address,
                    cd->posedge_clock_state.data_out[0] );
        }
    }
    else
    {
        if ((cd->posedge_clock_state.address<data_size) && memory)
        {
            // If bits_per_enable==16 then i==0, i==1 use 0; i==2, i==3 use 1
            // If bits_per_enable==8 then i==0 use 0; i==1 use 1
            // i.e. (i*8)/bits_per_enable
            //fprintf(stderr,"WRITE:%d %d \n",data_byte_width, cd->posedge_clock_state.address);
            int i;
            for (i=0; i<data_byte_width; i++) // i for each byte
            {
                int enable_num;
                enable_num = (i*8)/(bits_per_enable==0?1:bits_per_enable);
                if ( (number_of_enables==0) ||                                  // if no enables
                     ((cd->posedge_clock_state.write_enable[0]&(1<<enable_num))) ) // if enable set
                {
                    ((unsigned char *)memory)[data_byte_width*cd->posedge_clock_state.address+i] = ((unsigned char *)(cd->posedge_clock_state.write_data))[i];
                }
            }
            engine->log_event_occurred( engine_handle, cd->log_event_array, 1 );
            if ( verbose &&
                 ((number_of_enables==0) || (cd->posedge_clock_state.write_enable[0]) ) )
            {
                fprintf(stderr,"%s:%d:Port %d:Writing address %08llx data word %08llx be %08llx\n",
                        engine->get_instance_name(engine_handle),
                        engine->cycle(),
                        cd->port,
                        cd->posedge_clock_state.address,
                        cd->posedge_clock_state.write_data[0],
                        cd->posedge_clock_state.write_enable[0] );
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


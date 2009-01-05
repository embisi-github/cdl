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

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "sl_general.h"
#include "sl_token.h"
#include "se_errors.h"
#include "se_external_module.h"
#include "se_internal_module.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Defines
 */

/*a Types
 */
/*t t_internal_module_data
 */
typedef struct t_internal_module_data
{
    struct t_internal_module_data *next_in_list;
    c_engine *engine;
    t_sl_uint64 args[32];
    char *ptr;
    t_sl_uint64 outputs[32];
    t_sl_uint64 *inputs[32];
} t_internal_module_data;

/*t t_generic_logic_fn
*/
typedef void (*t_generic_logic_fn) ( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs );

/*t t_generic_logic
*/
typedef struct t_generic_logic
{
     const char *type;
     t_generic_logic_fn logic_fn;
} t_generic_logic;

/*a Static function declarations
 */
static void generic_logic_comb_and( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs );
static void generic_logic_comb_or( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs );
static void generic_logic_comb_xor( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs );
static void generic_logic_comb_nand( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs );
static void generic_logic_comb_nor( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs );
static void generic_logic_comb_xnor( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs );

/*a Statics
 */
/*v generic_logic_fns
 */
static t_generic_logic generic_logic_fns[] =
{
     { "and", generic_logic_comb_and },
     { "nand", generic_logic_comb_nand },
     { "or", generic_logic_comb_or },
     { "nor", generic_logic_comb_nor },
     { "xor", generic_logic_comb_xor },
     { "xnor", generic_logic_comb_xnor },
     { NULL, NULL }
};

/*a Generic logic gate functions
 */
/*f generic_logic_comb_and
 */
static void generic_logic_comb_and( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs )
{
     int i, j;

     for (i=0; i<=(width-1)/32; i++)
     {
          output[i] = inputs[0][i];
          for (j=1; j<number_inputs; j++)
          {
                output[i] &= inputs[j][i];
          }
     }
 }

/*f generic_logic_comb_nand
 */
static void generic_logic_comb_nand( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs )
{
     int i, j;

     for (i=0; i<=(width-1)/32; i++)
     {
          output[i] = inputs[0][i];
          for (j=1; j<number_inputs; j++)
          {
                output[i] &= inputs[j][i];
          }
          output[i] = ~output[i];
     }
 }

/*f generic_logic_comb_or
 */
static void generic_logic_comb_or( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs )
{
     int i, j;

     for (i=0; i<=(width-1)/32; i++)
     {
          output[i] = inputs[0][i];
          for (j=1; j<number_inputs; j++)
          {
                output[i] |= inputs[j][i];
          }
     }
 }

/*f generic_logic_comb_nor
 */
static void generic_logic_comb_nor( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs )
{
     int i, j;

     for (i=0; i<=(width-1)/32; i++)
     {
          output[i] = inputs[0][i];
          for (j=1; j<number_inputs; j++)
          {
                output[i] |= inputs[j][i];
          }
          output[i] = ~output[i];
     }
 }

/*f generic_logic_comb_xor
 */
static void generic_logic_comb_xor( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs )
{
     int i, j;

     for (i=0; i<=(width-1)/32; i++)
     {
          output[i] = inputs[0][i];
          for (j=1; j<number_inputs; j++)
          {
                output[i] ^= inputs[j][i];
          }
     }
 }

/*f generic_logic_comb_xnor
 */
static void generic_logic_comb_xnor( int width, t_sl_uint64 *output, t_sl_uint64 **inputs, int number_inputs )
{
     int i, j;

     for (i=0; i<=(width-1)/32; i++)
     {
          output[i] = inputs[0][i];
          for (j=1; j<number_inputs; j++)
          {
                output[i] ^= inputs[j][i];
          }
          output[i] = ~output[i];
     }
 }

/*a Internal logic module functions
 */
/*f create_internal_module_data
    Create a data structure for an internal module
 */
static struct t_internal_module_data *create_internal_module_data( c_engine *engine )
{
     t_internal_module_data *data;
     data = (t_internal_module_data *)malloc(sizeof(t_internal_module_data));
     data->engine = engine;
     data->ptr = NULL;
     data->next_in_list = engine->module_data;
     engine->module_data = data;
     return data;
}

/*f internal_module_delete_data
    Should unlink it?!?
 */
static t_sl_error_level internal_module_delete_data( void *handle )
{
    t_internal_module_data *data;
    if (handle)
    {
        data = (t_internal_module_data *)handle;
        if (data->ptr)
            free(data->ptr);
        free(data);
    }
    return error_level_okay;
}

/*f internal_module_generic_logic_comb
 */
static t_sl_error_level internal_module_generic_logic_comb( void *handle )
{
     t_internal_module_data *data;
     data = (t_internal_module_data *)handle;

     generic_logic_fns[ data->args[2] ].logic_fn( data->args[0], data->outputs, data->inputs, data->args[1] );

     return error_level_okay;
}

/*f internal_module_generic_logic_instantiate
 */
static t_sl_error_level internal_module_generic_logic_instantiate( c_engine *engine, void *engine_handle )
{
     const char *type;
     int width, number_inputs;
     t_internal_module_data *data;
     int i;
     char buffer[256];

     type = engine->get_option_string( engine_handle, "type", "<none>" );
     width = engine->get_option_int( engine_handle, "width", 16 );
     number_inputs = engine->get_option_int( engine_handle, "number_inputs", 2 );

     for (i=0; generic_logic_fns[i].type; i++)
     {
          if (!strcmp( generic_logic_fns[i].type, type))
          {
               break;
          }
     }
     if (!generic_logic_fns[i].type)
     {
          return engine->error->add_error( (void *)"generic logic", error_level_serious, error_number_se_internal_module_unknown, error_id_se_internal_module_generic_logic_instantitate,
                                           error_arg_type_malloc_string, type,
                                           error_arg_type_none );
     }

     data = create_internal_module_data( engine );
     data->args[0] = width;
     data->args[1] = number_inputs;
     data->args[2] = i;

     engine->register_delete_function( engine_handle, (void *)data, internal_module_delete_data );
     engine->register_output_signal( engine_handle, "bus_out", width, &data->outputs[0] );
     engine->register_comb_output( engine_handle, "bus_out" );

     for (i=0; i<number_inputs; i++)
     {
          sprintf( buffer, "bus_in__%d__", i );
          engine->register_input_signal( engine_handle, buffer, width, &data->inputs[i] );
          engine->register_comb_input( engine_handle, buffer );
     }

     engine->register_comb_fn( engine_handle, (void *)data, internal_module_generic_logic_comb );

     return error_level_okay;
}

/*f internal_module_bit_extract_comb
 */
static t_sl_error_level internal_module_bit_extract_comb( void *handle )
{
     t_internal_module_data *data;
     data = (t_internal_module_data *)handle;
     unsigned int i, j;

     data->outputs[0] = 0;
     for (i=0, j=data->args[2]; i<data->args[1]; i++, j++)
     {
          data->outputs[0] |= INPUT_BIT( (data->inputs[0]), j )<<i;
     }
     return error_level_okay;
}

/*f internal_module_bit_extract_instantiate
 */
static t_sl_error_level internal_module_bit_extract_instantiate( c_engine *engine, void *engine_handle )
{
     int width_in, width_out, start_out;
     t_internal_module_data *data;

     width_in = engine->get_option_int( engine_handle, "width_in", 16 );
     width_out = engine->get_option_int( engine_handle, "width_out", 1 );
     start_out = engine->get_option_int( engine_handle, "start_out", 1 );
     if ( (start_out+width_out>width_in) ||
          (start_out<0) ||
          (width_out<=0) )
     {
          return engine->error->add_error( (void *)"bit extract", error_level_serious, error_number_se_bad_bus_width_or_start, error_id_se_internal_module_generic_logic_instantitate,
                                           error_arg_type_integer, width_in,
                                           error_arg_type_integer, width_out,
                                           error_arg_type_integer, start_out,
                                           error_arg_type_none );
     }

     data = create_internal_module_data( engine );
     data->args[0] = width_in;
     data->args[1] = width_out;
     data->args[2] = start_out;

     engine->register_delete_function( engine_handle, (void *)data, internal_module_delete_data );
     engine->register_input_signal( engine_handle, "bus_in", width_in, &data->inputs[0] );
     engine->register_output_signal( engine_handle, "bus_out", width_out, &data->outputs[0] );
     engine->register_comb_input( engine_handle, "bus_in" );
     engine->register_comb_output( engine_handle, "bus_out" );
     engine->register_comb_fn( engine_handle, (void *)data, internal_module_bit_extract_comb );

     return error_level_okay;
}

/*f internal_module_bundle_comb
 */
static t_sl_error_level internal_module_bundle_comb( void *handle )
{
     t_internal_module_data *data;
     data = (t_internal_module_data *)handle;
     unsigned int i, j;

     data->outputs[0] = 0;
     for (i=0, j=0; i<data->args[0]; i++ )
     {
         data->outputs[0] |= (data->inputs[i][0]<<j);
         j+=data->args[i+1];
     }
     return error_level_okay;
}

/*f internal_module_bundle_instantiate
 */
static t_sl_error_level internal_module_bundle_instantiate( c_engine *engine, void *engine_handle )
{
    int number_inputs, width, width_check;
    int input_widths[32];
    t_internal_module_data *data;

    width = engine->get_option_int( engine_handle, "width", 16 );
    width_check = 0;
    number_inputs = engine->get_option_int( engine_handle, "number_inputs", 2 );
    {
        int i;
        for (i=0; i<number_inputs; i++)
        {
            char buffer[512];
            sprintf( buffer, "width%d", i );
            input_widths[i] = engine->get_option_int( engine_handle, buffer, 16 );
            width_check += input_widths[i];
        }
    }
    if (width_check!=width)
    {
        return engine->error->add_error( (void *)"bundle", error_level_serious, error_number_se_bad_bus_width_or_start, error_id_se_internal_module_generic_logic_instantitate,
                                         error_arg_type_integer, width,
                                         error_arg_type_integer, width_check,
                                         error_arg_type_integer, 0,
                                         error_arg_type_none );
    }

    data = create_internal_module_data( engine );
    data->args[0] = number_inputs;

    engine->register_delete_function( engine_handle, (void *)data, internal_module_delete_data );
    {
        int i;
        char buffer[256];
        for (i=0; i<number_inputs; i++)
        {
            data->args[i+1] = input_widths[i];
            sprintf( buffer, "bus_in__%d__", i );
            engine->register_input_signal( engine_handle, buffer, input_widths[i], &data->inputs[i] );
            engine->register_comb_input( engine_handle, buffer );
        }
    }
    engine->register_output_signal( engine_handle, "bus_out", width, &data->outputs[0] );
    engine->register_comb_output( engine_handle, "bus_out" );
    engine->register_comb_fn( engine_handle, (void *)data, internal_module_bundle_comb );

    return error_level_okay;
}

/*f internal_module_clock_phase_reset
 */
static t_sl_error_level internal_module_clock_phase_reset( void *handle, int pass )
{
    t_internal_module_data *data;
    data = (t_internal_module_data *)handle;

    data->args[2] = data->args[0]; // cycles left in delay
    data->args[3] = 0; // position in pattern
    data->outputs[0] = 0;
    return error_level_okay;
}

/*f internal_module_clock_phase_int_preclock
 */
static t_sl_error_level internal_module_clock_phase_int_preclock( void *handle )
{
    return error_level_okay;
}

/*f internal_module_clock_phase_int_clock
 */
static t_sl_error_level internal_module_clock_phase_int_clock( void *handle )
{
    t_internal_module_data *data;
    data = (t_internal_module_data *)handle;
    if (data->args[2]>0) // In initial delay
    {
        data->outputs[0] = 0;
        data->args[2]--;
    }
    else // Running in pattern
    {
        if (data->args[3]>=data->args[4])
        {
            data->outputs[0] = 0;
        }
        else
        {
            data->outputs[0] = 0;
            switch (data->ptr[data->args[3]])
            {
            case 'H':
            case 'h':
            case '*':
            case 'X':
                data->outputs[0] = 1;
            }
        }
        data->args[3]++;
        if (data->args[3]>=data->args[1])
        {
            data->args[3]=0;
        }
    }
    return error_level_okay;
}

/*f internal_module_clock_phase_instantiate
 */
static t_sl_error_level internal_module_clock_phase_instantiate( c_engine *engine, void *engine_handle )
{
    int pattern_length, delay;
    const char *pattern;
    t_internal_module_data *data;

    delay = engine->get_option_int( engine_handle, "delay", 32 );
    pattern_length = engine->get_option_int( engine_handle, "pattern_length", 32 );
    pattern = engine->get_option_string( engine_handle, "pattern", "" );

    data = create_internal_module_data( engine );
    data->args[0] = delay;
    data->args[1] = pattern_length;
    data->args[2] = delay; // cycles left in delay
    data->args[3] = 0; // position in pattern
    data->args[4] = strlen(pattern); // after this cycle in pattern output becomes 0
    data->ptr = sl_str_alloc_copy( pattern );

    engine->register_delete_function( engine_handle, (void *)data, internal_module_delete_data );
    engine->register_reset_function( engine_handle, (void *)data, internal_module_clock_phase_reset );

    engine->register_output_signal( engine_handle, "phase", 1, &data->outputs[0] );
    engine->register_clock_fns( engine_handle, (void *)data, "int_clock", internal_module_clock_phase_int_preclock, internal_module_clock_phase_int_clock );
    engine->register_output_generated_on_clock( engine_handle, "phase", "int_clock", 1 );

    {
        static t_engine_state_desc state_desc[] =
        {
            {"output",engine_state_desc_type_bits,NULL,((char *)(&(data->outputs[0])))-(char *)(data),{32,0,0,0},{NULL,NULL,NULL,NULL}},
            {"delay_left",engine_state_desc_type_bits,NULL,((char *)(&(data->args[2])))-(char *)(data),{32,0,0,0},{NULL,NULL,NULL,NULL}},
            {"position",engine_state_desc_type_bits,NULL,((char *)(&(data->args[3])))-(char *)(data),{32,0,0,0},{NULL,NULL,NULL,NULL}},
            {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
        };
        engine->register_state_desc( engine_handle, 1, state_desc, data, NULL );
    }

    data->outputs[0] = 0;
    return error_level_okay;
}

/*f internal_module_assign_reset
 */
static t_sl_error_level internal_module_assign_reset( void *handle, int pass )
{
    t_internal_module_data *data;
    data = (t_internal_module_data *)handle;

    data->outputs[0] = data->args[1];
    return error_level_okay;
}

/*f internal_module_assign_reset_preclock
 */
static t_sl_error_level internal_module_assign_reset_preclock( void *handle )
{
    return error_level_okay;
}

/*f internal_module_assign_reset_clock
 */
static t_sl_error_level internal_module_assign_reset_clock( void *handle )
{
    t_internal_module_data *data;
    data = (t_internal_module_data *)handle;
    data->outputs[0]=data->args[3];
    return error_level_okay;
}

/*f internal_module_data_assign_instantiate
 */
static t_sl_error_level internal_module_data_assign_instantiate( c_engine *engine, void *engine_handle )
{
    int bus_width, value, until_cycle, after_value;
    t_internal_module_data *data;

    bus_width = engine->get_option_int( engine_handle, "bus_width", 32 );
    value = engine->get_option_int( engine_handle, "value", 32 );
    until_cycle = engine->get_option_int( engine_handle, "until_cycle", 32 );
    after_value = engine->get_option_int( engine_handle, "after_value", 32 );

    data = create_internal_module_data( engine );
    data->args[0] = bus_width;
    data->args[1] = value;
    data->args[2] = until_cycle;
    data->args[3] = after_value;

    engine->register_delete_function( engine_handle, (void *)data, internal_module_delete_data );
    engine->register_reset_function( engine_handle, (void *)data, internal_module_assign_reset );

    engine->register_output_signal( engine_handle, "bus", bus_width, &data->outputs[0] );
    if (until_cycle>0)
    {
        static t_engine_state_desc state_desc[] =
        {
            {"output",engine_state_desc_type_bits,NULL,((char *)(&(data->outputs[0])))-(char *)(data),{32,0,0,0},{NULL,NULL,NULL,NULL}},
            {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
        };
        engine->register_clock_fns( engine_handle, (void *)data, "reset_clock", internal_module_assign_reset_preclock, internal_module_assign_reset_clock );
        engine->register_output_generated_on_clock( engine_handle, "bus", "reset_clock", 1 );
        engine->register_state_desc( engine_handle, 1, state_desc, data, NULL );
    }
    data->outputs[0] = value;
    return error_level_okay;
}

/*f internal_module_data_cmp_comb
 */
static t_sl_error_level internal_module_data_cmp_comb( void *handle )
{
     t_internal_module_data *data;
     data = (t_internal_module_data *)handle;

     data->outputs[0] = (data->inputs[0][0] == data->args[1]);

     return error_level_okay;
}

/*f internal_module_data_cmp_instantiate
 */
static t_sl_error_level internal_module_data_cmp_instantiate( c_engine *engine, void *engine_handle )
{
     int bus_width, value;
     t_internal_module_data *data;

     bus_width = engine->get_option_int( engine_handle, "bus_width", 32 );
     value = engine->get_option_int( engine_handle, "value", 32 );

     data = create_internal_module_data( engine );
     data->args[0] = bus_width;
     data->args[1] = value;

     engine->register_delete_function( engine_handle, (void *)data, internal_module_delete_data );

     engine->register_input_signal( engine_handle, "bus", bus_width, &data->inputs[0] );
     engine->register_comb_input( engine_handle, "bus" );

     engine->register_output_signal( engine_handle, "equal", 1, &data->outputs[0] );
     engine->register_comb_output( engine_handle, "equal" );
     engine->register_comb_fn( engine_handle, (void *)data, internal_module_data_cmp_comb );

     return error_level_okay;
}

/*f internal_module_data_mux_comb
 */
static t_sl_error_level internal_module_data_mux_comb( void *handle )
{
     t_internal_module_data *data;
     data = (t_internal_module_data *)handle;
     unsigned int i;

     i = data->inputs[0][0] & ((1<<data->args[2])-1);
     if (i>=data->args[1])
     {
          i = 0;// ERROR
     }
     data->outputs[0] = data->inputs[i+1][0];

     return error_level_okay;
}

/*f internal_module_data_mux_instantiate
 */
static t_sl_error_level internal_module_data_mux_instantiate( c_engine *engine, void *engine_handle )
{
     int i;
     int bus_width, number_inputs, select_width;
     t_internal_module_data *data;
     char buffer[256];

     bus_width = engine->get_option_int( engine_handle, "bus_width", 32 );
     number_inputs = engine->get_option_int( engine_handle, "number_inputs", 1 );
     select_width = engine->get_option_int( engine_handle, "select_width", 2 );

     data = create_internal_module_data( engine );
     data->args[0] = bus_width;
     data->args[1] = number_inputs;
     data->args[2] = select_width;

     engine->register_delete_function( engine_handle, (void *)data, internal_module_delete_data );
     for (i=0; i<number_inputs; i++)
     {
          sprintf( buffer, "bus_in__%d__", i );
          engine->register_input_signal( engine_handle, buffer, bus_width, &data->inputs[i+1] );
          engine->register_comb_input( engine_handle, buffer );
     }
     engine->register_output_signal( engine_handle, "bus_out", bus_width, &data->outputs[0] );
     engine->register_comb_output( engine_handle, "bus_out" );
     engine->register_input_signal( engine_handle, "select", select_width, &data->inputs[0] );
     engine->register_comb_input( engine_handle, "select" );
     engine->register_comb_fn( engine_handle, (void *)data, internal_module_data_mux_comb );

     return error_level_okay;
}

/*f internal_module_decode_comb
 */
static t_sl_error_level internal_module_decode_comb( void *handle )
{
     t_internal_module_data *data;
     data = (t_internal_module_data *)handle;

     if (data->args[0])
     {
          if ((data->inputs[1][0]&1)==0)
          {
               data->outputs[0] = 0;
               return error_level_okay;
          }
     }
     data->outputs[0] = 1<<(data->inputs[0][0]&(data->args[2]-1));

     return error_level_okay;
}

/*f internal_module_decode_instantiate
 */
static t_sl_error_level internal_module_decode_instantiate( c_engine *engine, void *engine_handle )
{
     int input_width, enabled;
     t_internal_module_data *data;

     input_width = engine->get_option_int( engine_handle, "input_width", 4 );
     enabled = engine->get_option_int( engine_handle, "enabled", 0 );

     data = create_internal_module_data( engine );
     data->args[0] = enabled;
     data->args[1] = input_width;
     data->args[2] = 1<<input_width;

     engine->register_delete_function( engine_handle, (void *)data, internal_module_delete_data );
     engine->register_input_signal( engine_handle, "bus_in", input_width, &data->inputs[0] );
     engine->register_comb_input( engine_handle, "bus_in" );
     if (enabled)
     {
          engine->register_input_signal( engine_handle, "enable", 1, &data->inputs[1] );
          engine->register_comb_input( engine_handle, "enable" );
     }
     engine->register_output_signal( engine_handle, "bus_out", data->args[2], &data->outputs[0] );
     engine->register_comb_output( engine_handle, "bus_out" );
     engine->register_comb_fn( engine_handle, (void *)data, internal_module_decode_comb );

     return error_level_okay;
}

/*a External functions
 */
extern void internal_module_register_all( c_engine *engine )
{
     se_external_module_register( 1, "__internal_module__bundle", internal_module_bundle_instantiate );
     se_external_module_register( 1, "__internal_module__bit_extract", internal_module_bit_extract_instantiate );
     se_external_module_register( 1, "__internal_module__clock_phase", internal_module_clock_phase_instantiate );
     se_external_module_register( 1, "__internal_module__assign", internal_module_data_assign_instantiate );
     se_external_module_register( 1, "__internal_module__cmp", internal_module_data_cmp_instantiate );
     se_external_module_register( 1, "__internal_module__data_mux", internal_module_data_mux_instantiate );
     se_external_module_register( 1, "__internal_module__decode", internal_module_decode_instantiate );
     se_external_module_register( 1, "__internal_module__generic_logic", internal_module_generic_logic_instantiate );
     se_external_module_register( 1, "se_test_harness", se_internal_module__test_harness_instantiate );
     se_external_module_register( 1, "se_sram_srw", se_internal_module__sram_srw_instantiate );
     se_external_module_register( 1, "se_sram_mrw", se_internal_module__sram_mrw_instantiate );
     se_external_module_register( 1, "se_logger", se_internal_module__logger_instantiate );
     se_external_module_register( 1, "se_ddr", se_internal_module__ddr_instantiate );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


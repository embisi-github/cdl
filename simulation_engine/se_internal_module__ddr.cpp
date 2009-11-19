/*a Copyright
  
This file 'se_internal_module__ddr.cpp' copyright Gavin J Stark 2003, 2004, 2007
  
This is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation, version 2.1.
  
This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
for more details.
*/

/*a Documentation
  Basically this module implements a set of DDR pins

  It takes a clock and for each ddr output signal (signal_ddr) it requires two inputs, signal_h and signal_l
  On a posedge of the clock the input data (signal_h and signal_l) is registered, and while the clock is high the registered contents of signal_h are presented on signal_ddr; when the clock falls, the registered contents of signal_h are presented on signal_ddr

  It takes the same clock and for each ddr input signal (signal_ddr) it provides two outputs, signal_at_fall and signal_at_rise
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
#define MAX_SIGNAL_WIDTH_64 (8)
#define ZERO_MAX(d) { int __j; for (__j=0; __j<MAX_SIGNAL_WIDTH_64; __j++) (d)[__j]=0; }
#define COPY_N(bits,d,s) { int __j,__n; __n = ((bits)+63)/64;for (__j=0; __j<__n; __j++) (d)[__j]=(s)[__j]; }

/*a Types
 */
/*t t_internal_module_ddr_signal_data
 */
typedef struct t_internal_module_ddr_signal_data
{
    char *name; // malloc'ed copy of the base name of the signal - actual signals are signal_h, signal_l, signal_ddr
    int width;
    union
    {
        struct
        {
            t_sl_uint64 *data_in_low;
            t_sl_uint64 *data_in_high;
            t_sl_uint64 next_data_high[MAX_SIGNAL_WIDTH_64];
            t_sl_uint64 next_data_low[MAX_SIGNAL_WIDTH_64];
            t_sl_uint64 data_low[MAX_SIGNAL_WIDTH_64]; // holding register for 1/2 clock for low output data
            t_sl_uint64 data_out[MAX_SIGNAL_WIDTH_64]; // actually where we hold the ddr output for clock high and low (high does not need a staging reg)
        } ddr_output;
        struct
        {
            t_sl_uint64 *data_in_ddr;
            t_sl_uint64 next_data[MAX_SIGNAL_WIDTH_64];
            t_sl_uint64 data_at_fall_out[MAX_SIGNAL_WIDTH_64];
            t_sl_uint64 data_at_rise_out[MAX_SIGNAL_WIDTH_64];
        } ddr_input;
    } data;
} t_internal_module_ddr_signal_data;

/*t t_internal_module_ddr_data
 */
typedef struct t_internal_module_ddr_data
{
    c_engine *engine;
    void *engine_handle;
    char *clock; // Malloc'ed copy of the name of the clock
    int number_inputs;
    int number_outputs;
    t_internal_module_ddr_signal_data signals[1]; // inputs first, then outputs
} t_internal_module_ddr_data;

/*a Statics
 */

/*a Internal ddr module functions
 */
/*f internal_module_ddr_find_signal - not used at present
static int internal_module_ddr_find_signal( t_internal_module_ddr_data *data, const char *name )
{
    int i;
    if (!data)
        return -1;
    for (i=0; i<data->number_inputs + data->number_outputs; i++)
    {
        if (!strcmp(name,data->signals[i].name))
        {
            return i;
        }
    }
    return -1;
}
 */

/*f internal_module_ddr_delete_data
 */
static t_sl_error_level internal_module_ddr_delete_data( void *handle )
{
    t_internal_module_ddr_data *data;
    int i;

    data = (t_internal_module_ddr_data *)handle;
    if (data)
    {
        free(data->clock);
        for (i=0; i<data->number_inputs + data->number_outputs; i++)
            free(data->signals[i].name);
        free(data);
    }
    return error_level_okay;
}

/*f internal_module_ddr_reset
 */
static t_sl_error_level internal_module_ddr_reset( void *handle, int pass )
{
    t_internal_module_ddr_data *data;
    int i;

    data = (t_internal_module_ddr_data *)handle;
    for (i=0; i<data->number_inputs; i++)
    {
        ZERO_MAX(data->signals[i].data.ddr_input.next_data);
        ZERO_MAX(data->signals[i].data.ddr_input.data_at_fall_out);
        ZERO_MAX(data->signals[i].data.ddr_input.data_at_rise_out);
    }
    for (i=0; i<data->number_outputs; i++)
    {
        ZERO_MAX(data->signals[i].data.ddr_output.data_low);
        ZERO_MAX(data->signals[i].data.ddr_output.data_out);
    }
    return error_level_okay;
}

/*f internal_module_ddr_preclock
 */
static t_sl_error_level internal_module_ddr_preclock( t_internal_module_ddr_data *data, int posedge )
{
    int i;

    /*b Copy inputs for use on clock function
     */
    for (i=0; i<data->number_inputs; i++)
    {
        COPY_N(data->signals[i].width, data->signals[i].data.ddr_input.next_data, data->signals[i].data.ddr_input.data_in_ddr );
    }
    if (posedge)
    {
        for (i=0; i<data->number_outputs; i++)
        {
            COPY_N(data->signals[i+data->number_inputs].width, data->signals[i+data->number_inputs].data.ddr_output.next_data_low, (t_sl_uint64 *)(data->signals[i+data->number_inputs].data.ddr_output.data_in_low) );
            COPY_N(data->signals[i+data->number_inputs].width, data->signals[i+data->number_inputs].data.ddr_output.next_data_high, (t_sl_uint64 *)(data->signals[i+data->number_inputs].data.ddr_output.data_in_high) );
        }
    }

    /*b Done
     */
    return error_level_okay;
}

/*f internal_module_ddr_clock
 */
    static t_sl_error_level internal_module_ddr_clock( t_internal_module_ddr_data *data, int posedge )
{
    int i;

    /*b Copy input data to correct register
     */
    for (i=0; i<data->number_inputs; i++)
    {
        if (posedge)
        {
            COPY_N(data->signals[i].width, data->signals[i].data.ddr_input.data_at_rise_out, data->signals[i].data.ddr_input.next_data );
        }
        else
        {
            COPY_N(data->signals[i].width, data->signals[i].data.ddr_input.data_at_fall_out, data->signals[i].data.ddr_input.next_data );
        }
    }
    if (posedge)
    {
        for (i=0; i<data->number_outputs; i++)
        {
            COPY_N(data->signals[i+data->number_inputs].width, data->signals[i+data->number_inputs].data.ddr_output.data_low, data->signals[i+data->number_inputs].data.ddr_output.next_data_low );
            COPY_N(data->signals[i+data->number_inputs].width, data->signals[i+data->number_inputs].data.ddr_output.data_out, data->signals[i+data->number_inputs].data.ddr_output.next_data_high );
        }
    }
    else
    {
        for (i=0; i<data->number_outputs; i++)
        {
            COPY_N(data->signals[i+data->number_inputs].width, data->signals[i+data->number_inputs].data.ddr_output.data_out, data->signals[i+data->number_inputs].data.ddr_output.data_low );
        }
    }

    /*b Done
     */
    return error_level_okay;
}

/*f internal_module_ddr_posedge_preclock_fn
 */
static t_sl_error_level internal_module_ddr_posedge_preclock_fn( void *handle )
{
    t_internal_module_ddr_data *data;

    data = (t_internal_module_ddr_data *)handle;
    return internal_module_ddr_preclock( data, 1 );
}

/*f internal_module_ddr_posedge_clock_fn
 */
static t_sl_error_level internal_module_ddr_posedge_clock_fn( void *handle )
{
    t_internal_module_ddr_data *data;

    data = (t_internal_module_ddr_data *)handle;
    return internal_module_ddr_clock( data, 1 );
}

/*f internal_module_ddr_negedge_preclock_fn
 */
static t_sl_error_level internal_module_ddr_negedge_preclock_fn( void *handle )
{
    t_internal_module_ddr_data *data;

    data = (t_internal_module_ddr_data *)handle;
    return internal_module_ddr_preclock( data, 0 );
}

/*f internal_module_ddr_negedge_clock_fn
 */
static t_sl_error_level internal_module_ddr_negedge_clock_fn( void *handle )
{
    t_internal_module_ddr_data *data;

    data = (t_internal_module_ddr_data *)handle;
    return internal_module_ddr_clock( data, 0 );
}

/*f internal_module__ddr_instantiate
*/
extern t_sl_error_level se_internal_module__ddr_instantiate( c_engine *engine, void *engine_handle )
{
    const char *clock;
    char *input_string, *output_string;
    char buffer[256];
    char *inputs[256], *outputs[256];
    int input_widths[256], output_widths[256];
    int num_inputs, num_outputs;
    int i, j;
    t_internal_module_ddr_data *data;

    clock = engine->get_option_string( engine_handle, "clock", "" );
    input_string = sl_str_alloc_copy(engine->get_option_string( engine_handle, "ddr_inputs", "" ));
    output_string = sl_str_alloc_copy(engine->get_option_string( engine_handle, "ddr_outputs", "" ));

    sl_tokenize_line( input_string, inputs, sizeof(inputs), &num_inputs );
    for (i=0; i<num_inputs; i++)
    {
        input_widths[i] = 1;
        for (j=0; inputs[i][j] && (inputs[i][j]!='['); j++);
        if (inputs[i][j])
        {
            inputs[i][j]=0;
            if (!sl_integer_from_token( inputs[i]+j+1, &input_widths[i]))
            {
                fprintf(stderr,"NNE:integer in width of arg '%s' to test harness was not an integer at all %s\n", inputs[i], inputs[i]+j+1);
                input_widths[i]=1;
            }
            if ((input_widths[i]>MAX_SIGNAL_WIDTH_64*64) || (input_widths[i]<1))
            {
                fprintf(stderr,"NNE:bad width of arg '%s' to test harness - needs to be between 1 and 32\n", inputs[i]);
                input_widths[i] = 1;
            }
        }
    }

    sl_tokenize_line( output_string, outputs, sizeof(outputs), &num_outputs );
    for (i=0; i<num_outputs; i++)
    {
        output_widths[i] = 1;
        for (j=0; outputs[i][j] && (outputs[i][j]!='['); j++);
        if (outputs[i][j])
        {
            outputs[i][j]=0;
            if (!sl_integer_from_token( outputs[i]+j+1, &output_widths[i]))
            {
                fprintf(stderr,"NNE:integer in width of arg '%s' to test harness was not an integer at all %s\n", outputs[i], outputs[i]+j+1);
                output_widths[i] = 1;
            }
            if ((output_widths[i]>MAX_SIGNAL_WIDTH_64*64) || (output_widths[i]<1))
            {
                fprintf(stderr,"NNE:bad width of arg '%s' to test harness - needs to be between 1 and 32\n", outputs[i]);
                output_widths[i] = 1;
            }
        }
    }

    data = (t_internal_module_ddr_data *)malloc(sizeof(t_internal_module_ddr_data)+sizeof(t_internal_module_ddr_signal_data)*(num_inputs+num_outputs));
    data->engine = engine;
    data->engine_handle = engine_handle;
    data->number_inputs = num_inputs;
    data->number_outputs = num_outputs;
    data->clock = sl_str_alloc_copy( clock );

    engine->register_clock_fns( engine_handle, (void *)data, clock, internal_module_ddr_posedge_preclock_fn, internal_module_ddr_posedge_clock_fn, internal_module_ddr_negedge_preclock_fn, internal_module_ddr_negedge_clock_fn );

    for (i=0; i<num_inputs; i++)
    {
        data->signals[i].name = sl_str_alloc_copy( inputs[i] );
        data->signals[i].width = input_widths[i];

        snprintf( buffer, sizeof(buffer), "%s_ddr", inputs[i] );
        engine->register_input_signal( engine_handle, buffer, input_widths[i], &data->signals[i].data.ddr_input.data_in_ddr );
        engine->register_input_used_on_clock( engine_handle, buffer, clock, 0 );
        engine->register_input_used_on_clock( engine_handle, buffer, clock, 1 );

        snprintf( buffer, sizeof(buffer), "%s_at_fall", inputs[i] );
        ZERO_MAX(data->signals[i].data.ddr_input.data_at_fall_out);
        engine->register_output_signal( engine_handle, buffer, input_widths[i], data->signals[i].data.ddr_input.data_at_fall_out );
        engine->register_output_generated_on_clock( engine_handle, buffer, clock, 0 );

        snprintf( buffer, sizeof(buffer), "%s_at_rise", inputs[i] );
        ZERO_MAX(data->signals[i].data.ddr_input.data_at_rise_out);
        engine->register_output_signal( engine_handle, buffer, input_widths[i], data->signals[i].data.ddr_input.data_at_rise_out );
        engine->register_output_generated_on_clock( engine_handle, buffer, clock, 1 );
    }
    for (i=0; i<num_outputs; i++)
    {
        data->signals[i+num_inputs].name = sl_str_alloc_copy( outputs[i] );
        data->signals[i+num_inputs].width = output_widths[i];

        snprintf( buffer, sizeof(buffer), "%s_l", outputs[i] );
        engine->register_input_signal( engine_handle, buffer, output_widths[i], &data->signals[i+num_inputs].data.ddr_output.data_in_low );
        engine->register_input_used_on_clock( engine_handle, buffer, clock, 1 );

        snprintf( buffer, sizeof(buffer), "%s_h", outputs[i] );
        engine->register_input_signal( engine_handle, buffer, output_widths[i], &data->signals[i+num_inputs].data.ddr_output.data_in_high );
        engine->register_input_used_on_clock( engine_handle, buffer, clock, 1 );

        snprintf( buffer, sizeof(buffer), "%s_ddr", outputs[i] );
        ZERO_MAX(data->signals[i+num_inputs].data.ddr_output.data_out);
        engine->register_output_signal( engine_handle, buffer, output_widths[i], data->signals[i+num_inputs].data.ddr_output.data_out );
        engine->register_output_generated_on_clock( engine_handle, buffer, clock, 0 ); // edge does not matter at present
        //engine->register_output_generated_on_clock( engine_handle, buffer, clock, 1 );

    }

    engine->register_reset_function( engine_handle, (void *)data, internal_module_ddr_reset );
    engine->register_delete_function( engine_handle, (void *)data, internal_module_ddr_delete_data );

    free(input_string);
    free(output_string);

    return error_level_okay;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


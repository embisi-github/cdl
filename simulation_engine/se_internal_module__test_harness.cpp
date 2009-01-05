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
#include "sl_exec_file.h"
#include "sl_general.h"
#include "sl_token.h"
#include "se_errors.h"
#include "se_external_module.h"
#include "se_internal_module.h"
#include "c_se_engine.h"

/*a Defines
 */

/*a Types
 */
/*t t_internal_module_test_harness_data
 */
typedef struct t_internal_module_test_harness_data
{
    c_engine *engine;
    void *engine_handle;
    struct t_sl_exec_file_data *exec_file_data;
    char *clock;
    int posedge;
} t_internal_module_test_harness_data;

/*a Statics
 */
/*v test_harness_file_cmds
 */
static t_sl_exec_file_cmd test_harness_file_cmds[] =
{
    {sl_exec_file_cmd_none, 0, NULL, NULL, NULL }
};

/*v test_harness_file_fns
 */
static t_sl_exec_file_fn test_harness_file_fns[] =
{
    {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },
};

/*a Internal test harness module functions
 */
/*f internal_module_test_harness_delete_data
 */
static t_sl_error_level internal_module_test_harness_delete_data( void *handle )
{
    t_internal_module_test_harness_data *data;

    data = (t_internal_module_test_harness_data *)handle;
    if (data)
    {
        free(data->clock);
        if (data->exec_file_data )
        {
            sl_exec_file_free( data->exec_file_data );
        }
        free(data);
    }
    return error_level_okay;
}

/*f internal_module_test_harness_reset
 */
static t_sl_error_level internal_module_test_harness_reset( void *handle, int pass )
{
    t_internal_module_test_harness_data *data;

    if (pass==0)
    {
        data = (t_internal_module_test_harness_data *)handle;
        if (data->exec_file_data)
        {
            sl_exec_file_reset( data->exec_file_data );
            while (sl_exec_file_despatch_next_cmd( data->exec_file_data ));
        }
    }
    return error_level_okay;
}

/*f internal_module_test_harness_preclock
 */
static t_sl_error_level internal_module_test_harness_preclock( t_internal_module_test_harness_data *data )
{
    /*b Call BFM preclock function
     */
    sl_exec_file_send_message_to_object( data->exec_file_data, "bfm_exec_file_support", "preclock", NULL );

    /*b Done
     */
    return error_level_okay;
}

/*f internal_module_test_harness_clock
 */
static t_sl_error_level internal_module_test_harness_clock( t_internal_module_test_harness_data *data )
{
    /*b Run the exec_file
     */
    if (data->exec_file_data)
    {
        while (sl_exec_file_despatch_next_cmd( data->exec_file_data ));
    }

    /*b Call BFM clock function
     */
    sl_exec_file_send_message_to_object( data->exec_file_data, "bfm_exec_file_support", "clock", NULL );

    /*b Done
     */
    return error_level_okay;
}

/*f internal_module_test_harness_posedge_preclock_fn
 */
static t_sl_error_level internal_module_test_harness_posedge_preclock_fn( void *handle )
{
    t_internal_module_test_harness_data *data;

    data = (t_internal_module_test_harness_data *)handle;
    if (data->posedge)
    {
        return internal_module_test_harness_preclock( data );
    }
    return error_level_okay;
}

/*f internal_module_test_harness_posedge_clock_fn
 */
static t_sl_error_level internal_module_test_harness_posedge_clock_fn( void *handle )
{
    t_internal_module_test_harness_data *data;

    data = (t_internal_module_test_harness_data *)handle;
    if (data->posedge)
    {
        return internal_module_test_harness_clock( data );
    }
    return error_level_okay;
}

/*f internal_module_test_harness_negedge_preclock_fn
 */
static t_sl_error_level internal_module_test_harness_negedge_preclock_fn( void *handle )
{
    t_internal_module_test_harness_data *data;

    data = (t_internal_module_test_harness_data *)handle;
    if (!data->posedge)
    {
        return internal_module_test_harness_preclock( data );
    }
    return error_level_okay;
}

/*f internal_module_test_harness_negedge_clock_fn
 */
static t_sl_error_level internal_module_test_harness_negedge_clock_fn( void *handle )
{
    t_internal_module_test_harness_data *data;

    data = (t_internal_module_test_harness_data *)handle;
    if (!data->posedge)
    {
        return internal_module_test_harness_clock( data );
    }
    return error_level_okay;
}

/*f static exec_file_cmd_handler
 */
static t_sl_error_level exec_file_cmd_handler( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    t_internal_module_test_harness_data *data;
    data = (t_internal_module_test_harness_data *)handle;

    return error_level_serious;
}

/*f internal_module_test_harness_exec_file_instantiate_callback
 */
static void internal_module_test_harness_exec_file_instantiate_callback( void *handle, struct t_sl_exec_file_data *file_data )
{
    t_internal_module_test_harness_data *data;
    t_sl_exec_file_lib_desc lib_desc;

    data = (t_internal_module_test_harness_data *)handle;

    sl_exec_file_set_environment_interrogation( file_data, (t_sl_get_environment_fn)sl_option_get_string, (void *)data->engine->get_option_list( data->engine_handle ) );

    lib_desc.version = sl_ef_lib_version_cmdcb;
    lib_desc.library_name = "cdlsim.th";
    lib_desc.handle = (void *)data;
    lib_desc.cmd_handler = exec_file_cmd_handler;
    lib_desc.file_cmds = test_harness_file_cmds;
    lib_desc.file_fns = test_harness_file_fns;
    sl_exec_file_add_library( file_data, &lib_desc );

    data->engine->bfm_add_exec_file_enhancements( file_data, data->engine_handle, data->clock, data->posedge );

}

/*f internal_module__test_harness_instantiate
  Currently requires a clock. Don't think we can avoid that. But maybe we could make it combinatorial on all inputs, if no clock is specified.
*/
extern t_sl_error_level se_internal_module__test_harness_instantiate( c_engine *engine, void *engine_handle )
{
    const char *clock, *filename;
    t_internal_module_test_harness_data *data;
    int posedge;

    clock = engine->get_option_string( engine_handle, "clock", "" );
    filename = engine->get_option_string( engine_handle, "filename", "test_harness.txt" );

    posedge=1;
    if (clock[0]=='!')
    {
        posedge=0;
        clock++;
    }

    data = (t_internal_module_test_harness_data *)malloc(sizeof(t_internal_module_test_harness_data));
    data->engine = engine;
    data->engine_handle = engine_handle;
    data->clock = sl_str_alloc_copy( clock );
    data->posedge = posedge;
    data->exec_file_data = NULL;

    engine->register_clock_fns( engine_handle, (void *)data, clock, internal_module_test_harness_posedge_preclock_fn, internal_module_test_harness_posedge_clock_fn, internal_module_test_harness_negedge_preclock_fn, internal_module_test_harness_negedge_clock_fn );

    engine->register_reset_function( engine_handle, (void *)data, internal_module_test_harness_reset );
    engine->register_delete_function( engine_handle, (void *)data, internal_module_test_harness_delete_data );

    return sl_exec_file_allocate_and_read_exec_file( engine->error, engine->message, internal_module_test_harness_exec_file_instantiate_callback, (void *)data, "exec_file", filename, &data->exec_file_data, "Test harness", NULL, NULL );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


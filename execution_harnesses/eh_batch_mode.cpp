/*a Copyright
  
  This file 'eh_batch_mode.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "sl_debug.h"
#include "c_sl_error.h"
#include "sl_option.h"
#include "sl_exec_file.h"
#include "c_se_engine.h"
#include "cdl_version.h"
#include "model_list.h"

/*a Types
 */
/*t t_string_list
 */
typedef struct t_string_list
{
     struct t_string_list *next_in_list;
     char *name;
     char *value;
} t_string_list;

/*t options
 */
enum 
{
     option_constant=option_se_start,
     option_include,
     option_cycles,
     option_define,
     option_batch,
     option_help
};

/*t cmd_*
 */
enum
{
    cmd_read_hw_file=0,
    cmd_verbose,
    cmd_reset,
    cmd_thread_pool,
    cmd_thread_map,
    cmd_step,
    cmd_setenv,
    cmd_setenv_int,
    cmd_write_profile,
    cmd_display_state
};

/*a Statics
 */
/*v verbose
 */
static int verbose=1;

/*v env_options
 */
static t_sl_option_list env_options;

/*v error thingies
 */
static c_sl_error *batch_error, *batch_message, *hw_error, *hw_message;

/*v options
 */
static option options[] = {
     SL_OPTIONS
     { "batch", required_argument, NULL, option_batch },
     { "cycles", required_argument, NULL, option_cycles },
     { "define", required_argument, NULL, option_define },
     { "help", no_argument, NULL, option_help },
     { NULL, no_argument, NULL, 0 }
};

/*v file_cmds
 */
//    option
//    force
static t_sl_exec_file_cmd file_cmds[] =
{
     {cmd_read_hw_file,  1, "read_hw_file", "s", "read_hw_file <filename>"},
     {cmd_verbose,       1, "verbose", "i", "verbose <leve> - set verbose level - 1 is default"},
     {cmd_reset,         0, "reset", "", "reset"},
     {cmd_thread_pool,   1, "thread_pool", "ssssssssssssssss", "thread_pool <thread name>+"},
     {cmd_thread_map,    2, "thread_map", "ssssssssssssssss", "thread_map <thread name> <module instances>+"},
     {cmd_step,          1, "step", "i", "step <cycle count>"},
     {cmd_setenv,        2, "setenv", "ss", "setenv <name> <value>"},
     {cmd_display_state, 0, "display_state", "display_state"},
     {cmd_write_profile, 1, "write_profile", "s", "write_profile <filename>"},
     {sl_exec_file_cmd_none, 0, NULL, NULL, NULL }
};

/*v cmd_base
 */
static int cmd_base;

/*a Main routines
 */
/*f usage
 */
static void usage( void )
{
    printf( "Usage: [options] file\n");
    printf( "\t--cycles <count> \tSet number of cycles to execute\n");
    printf( "\t--debug \t\tEnable debugging\n");
    printf( "\t--debug-level <level> \tSet level of debugging information and enable\n\t\t\t\t debugging\n");
    printf( "\t--debug-file <file> \tSet file to output debug to (else stdout)\n");
    printf( "\t--help \t\tDisplay this help message\n");
}

/*f display_state
 */
static void display_state( c_engine *eng )
{
    void *handle;
    const char *module_name, *state_prefix, *state_name;
    char buffer[256];
    int i;

    handle = NULL;
    while ((handle = eng->enumerate_instances( handle, 0 ))!=NULL)
    {
        printf("Instance %s\n", eng->get_instance_name(handle));
        for (i=0; ; i++)
        {
            if (!eng->set_state_handle( handle, i ))
                break;
            eng->get_state_name_and_type( handle, &module_name, &state_prefix, &state_name );
            eng->get_state_value_string( handle, buffer, sizeof(buffer) );

            if (state_prefix)
            {
                printf("\t%s.%s.%s : %s\n", module_name, state_prefix, state_name, buffer );
            }
            else
            {
                printf("\t%s.%s : %s\n", module_name, state_name, buffer );
            }
        }
    }
}

/*f exec_file_cmd_handler
 */
static t_sl_error_level exec_file_cmd_handler( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    c_engine *hw_engine;
    hw_engine = (c_engine *)handle;
    int i;
    switch (cmd_cb->cmd)
    {
    case cmd_read_hw_file:
    {
        const char *name;
        name = sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 );
        printf("\n****************************************************************************\nsimulation_engine (v" __CDL__VERSION_STRING "): Reading hw file '%s'\n****************************************************************************\n", name );
        fflush(stdout);
        hw_engine->delete_instances_and_signals();
        hw_engine->read_and_interpret_hw_file( name, (t_sl_get_environment_fn)sl_option_get_string, (void *)env_options );
        hw_engine->check_connectivity();
        hw_engine->build_schedule();
        break;
    }
    case cmd_reset:
        if (verbose)
            printf("se_engine: Resetting\n");
        fflush(stdout);
        hw_engine->reset_state();
        break;
    case cmd_thread_pool:
        if (verbose)
            printf("se_engine: Thread pool\n");
        hw_engine->thread_pool_init();
        for (i=0; i<sl_exec_file_get_number_of_arguments( cmd_cb->file_data, cmd_cb->args ); i++)
            hw_engine->thread_pool_add_thread( sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, i ) );
        break;
    case cmd_thread_map:
        if (verbose)
            printf("se_engine: Thread map\n");
        {
            const char *thread_name;
            thread_name = sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 );
            for (i=1; i<sl_exec_file_get_number_of_arguments( cmd_cb->file_data, cmd_cb->args ); i++)
                hw_engine->thread_pool_map_thread_to_module( thread_name, sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, i ) );
        }
        break;
    case cmd_step:
        if (verbose)
            printf("se_engine: Stepping %lld cycles (from %d)\n", sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 0 ), hw_engine->cycle() );
        fflush(stdout);
        hw_engine->step_cycles( sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 0 ) );
        break;
    case cmd_verbose:
        verbose = sl_exec_file_eval_fn_get_argument_integer( cmd_cb->file_data, cmd_cb->args, 0 );
        break;
    case cmd_setenv:
        env_options = sl_option_list( env_options,
                                      sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 ),
                                      sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 1 ) );
        break;
    case cmd_write_profile:
    {
        FILE *f;
        int append;
        const char *name;
        name = sl_exec_file_eval_fn_get_argument_string( cmd_cb->file_data, cmd_cb->args, 0 );
        append = (name[0]=='+');
        f = fopen(name+append,append?"a":"w");
        if (!f)
        {
            fprintf(stderr,"Failed to open profile file '%s' to write to\n", name );
        }
        else
        {
            hw_engine->write_profile(f);
            fclose(f);
        }
        break;
    }
    case cmd_display_state:
        display_state( hw_engine );
        break;
    default:
        break;
    }
    {
        char error_accumulator[16384];
        void *handle;
        handle = NULL;
        while (hw_engine->message->check_errors_and_reset( error_accumulator, sizeof(error_accumulator), error_level_info, error_level_info, &handle )>0)
        {
            printf("%s", error_accumulator);
        }
        handle = NULL;
        while (batch_message->check_errors_and_reset( error_accumulator, sizeof(error_accumulator), error_level_info, error_level_info, &handle )>0)
        {
            printf("%s", error_accumulator);
        }
    }
    fflush(stdout);
    if ( (hw_error->check_errors_and_reset( stderr, error_level_info, error_level_fatal)) ||
         (batch_error->check_errors_and_reset( stderr, error_level_info, error_level_fatal )) )
        return error_level_fatal;
    return error_level_okay;
}

/*f exec_file_instantiate_callback
 */
static void exec_file_instantiate_callback( void *handle, struct t_sl_exec_file_data *file_data )
{
    c_engine *hw_engine;
    t_sl_exec_file_lib_desc lib_desc;

    hw_engine = (c_engine *)handle;

    sl_exec_file_set_environment_interrogation( file_data, (t_sl_get_environment_fn)sl_option_get_string, (void *)env_options );

    lib_desc.version = sl_ef_lib_version_cmdcb;
    lib_desc.library_name = "cdlbatch";
    lib_desc.handle = (void *)hw_engine;
    lib_desc.cmd_handler = exec_file_cmd_handler;
    lib_desc.file_cmds = file_cmds;
    lib_desc.file_fns = NULL;
    cmd_base = sl_exec_file_add_library( file_data, &lib_desc );

    hw_engine->bfm_add_exec_file_enhancements( file_data, NULL );
}

/*f main
 */
extern int main( int argc, char **argv )
{
    c_engine *hw_engine;
    int c, so_far, i;
    char *option_name;
    int cycle_count;
    t_sl_exec_file_data *exec_file_data;

    se_c_engine_init();

    env_options = NULL;
    cycle_count = 100;

    so_far = 0;
    c = 0;
    optopt=1;
    while(c>=0)
    {
        c = getopt_long( argc, argv, "hvc:", options, &so_far );
        if ( !sl_option_handle_getopt( &env_options, c, optarg) )
        {
            switch (c)
            {
            case 'h':
            case option_help:
                usage();
                exit(0);
                break;
            case 'v':
                sl_debug_enable(1);
                break;
            case 'c':
            case option_cycles:
                if (sscanf(optarg,"%d", &i )==1)
                    cycle_count=i;
                break;
            case option_define:
                option_name = sl_str_alloc_copy( optarg );
                for (i=0; option_name[i] && (option_name[i]!='='); i++);
                if (option_name[i]=='=')
                {
                    option_name[i] = 0;
                    env_options = sl_option_list( env_options, option_name, option_name+i+1 );
                }
                else
                {
                    env_options = sl_option_list( env_options, option_name, "1" );
                }
                free(option_name);
                break;
            default:
                break;
            }
        }
    }
    if (optind>=argc)
    {
        usage();
        exit(4);
    }

    for (i=0; model_init_fns[i]; i++)
    {
        model_init_fns[i]();
    }

    batch_error = new c_sl_error();
    batch_message = new c_sl_error();
    hw_error = new c_sl_error();
    hw_message = new c_sl_error();
    hw_engine = new c_engine( hw_error, "main_test_engine" );
    exec_file_data = NULL;

    for (i=optind; argv[i]; i++)
    {
        sl_exec_file_allocate_and_read_exec_file( batch_error, batch_message, exec_file_instantiate_callback, (void *)hw_engine, "batch_mode_exec_file", argv[i], &exec_file_data, "batch mode cycle simulation", NULL, NULL );
        if (batch_error->check_errors( stderr, error_level_info, error_level_fatal ))
            break;
        if (!exec_file_data)
        {
            continue;
        }
        sl_exec_file_reset( exec_file_data );

        while (sl_exec_file_despatch_next_cmd( exec_file_data ));

        hw_engine->message->check_errors_and_reset( stdout, error_level_info, error_level_info );
        batch_message->check_errors_and_reset( stdout, error_level_info, error_level_info );
        fflush(stdout);
        if (hw_error->check_errors_and_reset( stderr, error_level_info, error_level_fatal ))
            break;
        if (batch_error->check_errors_and_reset( stderr, error_level_info, error_level_fatal ))
            break;
        sl_exec_file_free( exec_file_data );
    }

    batch_error->reset();
    delete(batch_message);
    delete(batch_error);
    delete(hw_engine);
    se_c_engine_exit();
    sl_option_free_list( env_options );
    return 0;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/




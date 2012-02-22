/*a Copyright
  
  This file 'c_se_engine__instantiation.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include <ctype.h>
#include "sl_debug.h"
#include "sl_option.h"
#include "sl_exec_file.h"
#include "se_internal_module.h"
#include "se_external_module.h"
#include "se_engine_function.h"
#include "se_errors.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Types
 */
/*t t_engine_module_forced_option
 */
typedef struct t_engine_module_forced_option
{
    struct t_engine_module_forced_option *next_in_list;
    char *full_name;
    t_sl_option_list option;
} t_engine_module_forced_option;

/*a Statics
 */
/*v option_list, static_env_fn, static_env_handle
 */
t_sl_option_list option_list;
static t_sl_get_environment_fn static_env_fn;
static void *static_env_handle;
/*v cmd_*
 */
enum
{
    cmd_module,
    cmd_module_force_option_int,
    cmd_module_force_option_string,
    cmd_module_force_option_object,
    cmd_option_int,
    cmd_option_string,
    cmd_option_object,
    cmd_wire,
    cmd_assign,
    cmd_cmp,
    cmd_mux,
    cmd_decode,
    cmd_extract,
    cmd_bundle,
    cmd_logic,
    cmd_clock,
    cmd_clock_divide,
    cmd_clock_phase,
    cmd_drive,
};

/*v file_cmds
 */
static t_sl_exec_file_cmd file_cmds[] =
{
     {cmd_module,         2, "module", "ss", "module <type> <instance>"},
     {cmd_module_force_option_int,  3, "module_force_option_int", "ssi", "module_force_option_int <full module instance name> <option> <value>"},
     {cmd_module_force_option_string,  3, "module_force_option_string", "sss", "module_force_option_string <full module instance name> <option> <value>"},
     {cmd_option_int,     2, "option_int", "si", "option_int <name> <value>"},
     {cmd_option_string,  2, "option_string", "ss", "option_string <name> <value>"},
     {cmd_option_object,  2, "option_object", "so", "option_object <name> <object>"},
     {cmd_wire,           1, "wire", "sssssssssssssss",    "wire <names>* - up to 15"},
     {cmd_assign,         2, "assign", "siii", "assign <output_bus> <value> [<until time> [<after value>]]"},
     {cmd_cmp,            3, "cmp", "ssi", "cmp <output_signal> <bus> <value>"},
     {cmd_mux,            3, "mux", "ssssssssss", "mux <output_bus> <select> <inputs>* - up to 8"},
     {cmd_decode,         2, "decode", "sss", "decode <output> <input> [<enable>]"},
     {cmd_extract,        4, "extract", "ssii", "extract <output> <input> <start bit> <number bits>"},
     {cmd_bundle,         2, "bundle", "sssssssssssssss", "extract <output> <inputs>*"},
     {cmd_logic,          3, "logic", "sssssssssssssss", "logic <type> <output> <inputs>* - up to 12"},
     {cmd_clock,          4, "clock", "siii", "clock <name> <delay to first high> <cycles high> <cycles low>"},
     {cmd_clock_divide,   5, "clock_divide", "ssiii", "clock_divide <name> <source clock> <delay in clocks to first high> <clock ticks high> <clock ticks low>"},
     {cmd_clock_phase,    5, "clock_phase", "ssiis", "clock_phase <output> <clock> <delay in clocks to pattern starts> <clock ticks pattern> <pattern string>"},
     {cmd_drive,          2, "drive", "ss", "drive <signal> <with signal>"},
     {sl_exec_file_cmd_none, 0, NULL, NULL, NULL }
};


/*a Hardware file and instantiation methods
 */
/*f c_engine::instantiation_exec_file_cmd_handler
 */
t_sl_error_level c_engine::instantiation_exec_file_cmd_handler( struct t_sl_exec_file_cmd_cb *cmd_cb )
{
    int i, j;
    int width;
    char *arg, *names[SL_EXEC_FILE_MAX_CMD_ARGS+1];
    //fprintf(stderr,"cmd_cb->cmd %d\n",cmd_cb->cmd);
    switch (cmd_cb->cmd)
    {
    case cmd_module:
        //fprintf(stderr,"Instantiate %s %s\n",cmd_cb->args[0].p.string, cmd_cb->args[1].p.string);
        instantiate( NULL,
                     sl_exec_file_eval_fn_get_argument_string( cmd_cb, 0), 
                     sl_exec_file_eval_fn_get_argument_string( cmd_cb, 1),
                     option_list );
        option_list = NULL;
        break;
    case cmd_module_force_option_int:
        instance_add_forced_option( sl_exec_file_eval_fn_get_argument_string( cmd_cb, 0), 
                                    sl_option_list( NULL,
                                                    sl_exec_file_eval_fn_get_argument_string( cmd_cb, 1),
                                                    sl_exec_file_eval_fn_get_argument_integer( cmd_cb, 2) ));
        break;
    case cmd_module_force_option_string:
        instance_add_forced_option( sl_exec_file_eval_fn_get_argument_string( cmd_cb, 0), 
                                    sl_option_list( NULL,
                                                    sl_exec_file_eval_fn_get_argument_string( cmd_cb, 1),
                                                    sl_exec_file_eval_fn_get_argument_string( cmd_cb, 2) ));
        break;
    case cmd_module_force_option_object:
        instance_add_forced_option( sl_exec_file_eval_fn_get_argument_string( cmd_cb, 0), 
                                    sl_option_list( NULL,
                                                    sl_exec_file_eval_fn_get_argument_string( cmd_cb, 1),
                                                    sl_exec_file_eval_fn_get_argument_pointer( cmd_cb, 2) ));
        break;
    case cmd_option_int:
        option_list = sl_option_list( option_list, 
                                      sl_exec_file_eval_fn_get_argument_string( cmd_cb, 0),
                                      sl_exec_file_eval_fn_get_argument_integer( cmd_cb, 1) );
        break;
    case cmd_option_string:
        option_list = sl_option_list( option_list, 
                                      sl_exec_file_eval_fn_get_argument_string( cmd_cb, 0),
                                      sl_exec_file_eval_fn_get_argument_string( cmd_cb, 1) );
        break;
    case cmd_option_object:
        option_list = sl_option_list( option_list, 
                                      sl_exec_file_eval_fn_get_argument_string( cmd_cb, 0),
                                      sl_exec_file_eval_fn_get_argument_pointer( cmd_cb, 1) );
        break;
    case cmd_wire:
        for (j=0; j<SL_EXEC_FILE_MAX_CMD_ARGS; j++)
        {
            if (sl_exec_file_eval_fn_get_argument_type( cmd_cb, j ) == sl_exec_file_value_type_string)
            {
                arg = sl_str_alloc_copy( sl_exec_file_eval_fn_get_argument_string( cmd_cb, j ) );
                width=1;
                for (i=0; (arg[i]!=0) && (arg[i]!='['); i++);
                if (arg[i]=='[')
                {
                    arg[i] = 0;
                    if (isalpha(arg[i+1]))
                    {
                        int k;
                        t_sl_uint64 *var;
                        for (k=i+1; arg[k] && (arg[k]!=']'); k++);
                        arg[k] = 0;
                        var = sl_exec_file_get_integer_variable( cmd_cb->file_data, arg+i+1 );
                        if (var)
                        {
                            width = var[0];
                        }
                    }
                    else
                    {
                        sscanf( arg+i+1, "%d", &width );
                    }
                }
                add_global_signal( arg, width );
                free(arg);
            }
        }
        break;
    case cmd_cmp:
        compare( sl_exec_file_filename(cmd_cb->file_data),
                 sl_exec_file_line_number( cmd_cb->file_data ),
                 sl_exec_file_eval_fn_get_argument_string( cmd_cb, 1),
                 sl_exec_file_eval_fn_get_argument_string( cmd_cb, 0), // Yes, 0 - order of args is 1, 0, 2
                 sl_exec_file_eval_fn_get_argument_integer( cmd_cb, 2) );
        break;
    case cmd_assign:
    {
        int until_time=0;
        int after_value =0;
        if (sl_exec_file_get_number_of_arguments(cmd_cb)>=3)
            until_time = cmd_cb->args[2].integer;
        if (sl_exec_file_get_number_of_arguments(cmd_cb)>=4)
            after_value = cmd_cb->args[3].integer;
        assign( sl_exec_file_filename(cmd_cb->file_data),
                sl_exec_file_line_number( cmd_cb->file_data ),
                 sl_exec_file_eval_fn_get_argument_string( cmd_cb, 0),
                 sl_exec_file_eval_fn_get_argument_integer( cmd_cb, 1),
                until_time,
                after_value );
        break;
    }
    case cmd_mux:
        for (i=0,j=0; i<SL_EXEC_FILE_MAX_CMD_ARGS; i++)
        {
            if (cmd_cb->args[i].p.ptr)
            {
                names[j] = cmd_cb->args[j].p.string;
                j++;
            }
        }
        names[j] = NULL;
        data_mux( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), j-2, names[1], names[0], (const char **)(names+2) );
        break;
    case cmd_decode:
        decode( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[1].p.string, cmd_cb->args[0].p.string, cmd_cb->args[2].p.string );
        break;
    case cmd_extract:
        bit_extract( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[1].p.string, cmd_cb->args[2].integer, cmd_cb->args[3].integer, cmd_cb->args[0].p.string );
        break;
    case cmd_bundle:
        for (i=0,j=0; i<SL_EXEC_FILE_MAX_CMD_ARGS; i++)
        {
            if (cmd_cb->args[i].p.ptr)
            {
                names[j] = cmd_cb->args[j].p.string;
                j++;
            }
        }
        names[j] = NULL;
        bundle( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), j-1, names[0], (const char **)(names+1) );
        break;
    case cmd_logic:
        for (i=0,j=0; i<SL_EXEC_FILE_MAX_CMD_ARGS; i++)
        {
            if (cmd_cb->args[i].p.ptr)
            {
                names[j] = cmd_cb->args[j].p.string;
                j++;
            }
        }
        names[j] = NULL;
        generic_logic( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), j-2, names[0], names[1], (const char **)(names+2) );
        break;
    case cmd_clock:
        //fprintf(stderr,"Create clock %s %lld %lld %lld\n",cmd_cb->args[0].p.string, cmd_cb->args[1].integer, cmd_cb->args[2].integer, cmd_cb->args[3].integer);
        create_clock( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[0].p.string, cmd_cb->args[1].integer, cmd_cb->args[2].integer, cmd_cb->args[3].integer);
        break;
    case cmd_clock_divide:
        create_clock_divide( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[0].p.string, cmd_cb->args[1].p.string, cmd_cb->args[2].integer, cmd_cb->args[3].integer, cmd_cb->args[4].integer);
        break;
    case cmd_clock_phase:
        create_clock_phase( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[0].p.string, cmd_cb->args[1].p.string, cmd_cb->args[2].integer, cmd_cb->args[3].integer, cmd_cb->args[4].p.string);
        break;
    case cmd_drive:
        names[0] = cmd_cb->args[0].p.string;
        names[1] = cmd_cb->args[1].p.string;
        if (find_global( names[0] ))
        {
            for (i=0; (names[1][i]!='.') && (names[1][i]!=0); i++);
            if (names[1][i]==0)
            {
                error->add_error( (void *)"hw_exec_file", error_level_serious, error_number_se_expected_canonical_signal_name, error_id_se_c_engine_read_and_interpret_hw_file,
                                  error_arg_type_malloc_filename, sl_exec_file_filename(cmd_cb->file_data),
                                  error_arg_type_line_number, sl_exec_file_line_number( cmd_cb->file_data ),
                                  error_arg_type_malloc_string, names[1],
                                  error_arg_type_none );
                break;
            }
            names[1][i]=0;
            name( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), names[1], names[1]+i+1, names[0] );
            break;
        }
          
        if (find_global( names[1] ))
        {
            for (i=0; (names[0][i]!='.') && (names[0][i]!=0); i++);
            if (names[0][i]==0)
            {
                error->add_error( (void *)"hw_exec_file", error_level_serious, error_number_se_expected_canonical_signal_name, error_id_se_c_engine_read_and_interpret_hw_file,
                                  error_arg_type_malloc_filename, sl_exec_file_filename(cmd_cb->file_data),
                                  error_arg_type_line_number, sl_exec_file_line_number( cmd_cb->file_data ),
                                  error_arg_type_malloc_string, names[0],
                                  error_arg_type_none );
                break;
            }
            names[0][i]=0;
            drive( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), names[0], names[0]+i+1, names[1] );
            break;
        }

        if (find_clock( names[1] ))
        {
            for (i=0; (names[0][i]!='.') && (names[0][i]!=0); i++);
            if (names[0][i]==0)
            {
                error->add_error( (void *)"hw_exec_file", error_level_serious, error_number_se_expected_canonical_signal_name, error_id_se_c_engine_read_and_interpret_hw_file,
                                  error_arg_type_malloc_filename, sl_exec_file_filename(cmd_cb->file_data),
                                  error_arg_type_line_number, sl_exec_file_line_number( cmd_cb->file_data ),
                                  error_arg_type_malloc_string, names[0],
                                  error_arg_type_none );
                break;
            }
            names[0][i]=0;
            bind_clock( sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), names[0], names[0]+i+1, names[1] );
            break;
        }
        error->add_error( (void *)"hw_exec_file", error_level_serious, error_number_se_bind_needs_global, error_id_se_c_engine_read_and_interpret_hw_file,
                          error_arg_type_malloc_string, names[0],
                          error_arg_type_malloc_string, names[1],
                          error_arg_type_malloc_filename, sl_exec_file_filename(cmd_cb->file_data),
                          error_arg_type_line_number, sl_exec_file_line_number( cmd_cb->file_data ),
                          error_arg_type_none );
        break;
    default:
        return error_level_serious;
    }
    return error_level_okay;
}

/*f static exec_file_cmd_handler_cb
 */
static t_sl_error_level exec_file_cmd_handler_cb( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    c_engine *engine;

    engine = (c_engine *)handle;
    return engine->instantiation_exec_file_cmd_handler( cmd_cb );
}

/*f exec_file_instantiate_callback
 */
static void exec_file_instantiate_callback( void *handle, struct t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_lib_desc lib_desc;

    sl_exec_file_set_environment_interrogation( file_data, static_env_fn, static_env_handle );
    lib_desc.version = sl_ef_lib_version_cmdcb;
    lib_desc.library_name = "cdlsim_instantiation";
    lib_desc.handle = handle;
    lib_desc.cmd_handler = exec_file_cmd_handler_cb;
    lib_desc.file_cmds = file_cmds;
    lib_desc.file_fns = NULL;
    sl_exec_file_add_library( file_data, &lib_desc );

}

/*f c_engine::read_and_interpret_hw_file
 */
t_sl_error_level c_engine::read_and_interpret_hw_file( const char *filename, t_sl_get_environment_fn env_fn, void *env_handle )
{
     t_sl_error_level result;
     t_sl_exec_file_data *exec_file_data;

     static_env_fn = env_fn;
     static_env_handle = env_handle;
     option_list= NULL;

     result = sl_exec_file_allocate_and_read_exec_file( error, message, exec_file_instantiate_callback, (void *)this, "hw_exec_file", filename, &exec_file_data, "hardware instantiation", NULL, NULL );

     if (result!=error_level_okay)
          return result;
     if (!exec_file_data)
     {
          return error_level_fatal;
     }

     while (sl_exec_file_despatch_next_cmd( exec_file_data));
     sl_exec_file_free( exec_file_data );

     return error_level_okay;
}

/*f c_engine::read_and_interpret_py_object
 */
t_sl_error_level c_engine::read_and_interpret_py_object( PyObject *describer, t_sl_get_environment_fn env_fn, void *env_handle )
{
     t_sl_error_level result;
     t_sl_exec_file_data *exec_file_data;

     static_env_fn = env_fn;
     static_env_handle = env_handle;
     option_list= NULL;

     result = sl_exec_file_allocate_from_python_object( error, message, exec_file_instantiate_callback, (void *)this, "hw_exec_file", describer, &exec_file_data, "hardware instantiation", 0 );

     if (result!=error_level_okay)
          return result;
     if (!exec_file_data)
     {
          return error_level_fatal;
     }

     while (sl_exec_file_despatch_next_cmd( exec_file_data));
     sl_exec_file_free( exec_file_data );

     return error_level_okay;
}

/*f c_engine::instantiate 
 */
t_sl_error_level c_engine::instantiate( void *parent_engine_handle, const char *type, const char *name, t_sl_option_list option_list )
{
     void *em;
     t_engine_module_instance *emi, *pemi;
     t_sl_option_list combined_option_list;

     SL_DEBUG(sl_debug_level_info, "c_engine::instantiate", "Instantiating '%s' of type '%s'", name, type ) ;

     em = se_external_module_find(type);
     if (!em)
     {
          return error->add_error( (void *)"instantiate", error_level_serious, error_number_se_unknown_module_type, error_id_se_c_engine_instantiate,
                                   error_arg_type_malloc_string, type,
                                   error_arg_type_malloc_string, name,
                                   error_arg_type_none );
     }

     pemi = (t_engine_module_instance *)parent_engine_handle;
     emi = (t_engine_module_instance *)find_module_instance( pemi, name );
     if (emi)
     {
          return error->add_error( (void *)"instantiate", error_level_serious, error_number_se_duplicate_name, error_id_se_c_engine_instantiate,
                                   error_arg_type_malloc_string, name,
                                   error_arg_type_none );
     }

     emi = (t_engine_module_instance *)malloc(sizeof(t_engine_module_instance));
     emi->next_instance = module_instance_list;
     module_instance_list = emi;
     emi->parent_instance = pemi;

     emi->module_handle = em;
     emi->name = sl_str_alloc_copy( name );
     emi->type = sl_str_alloc_copy( type );
     if (pemi)
     {
         int parent_length;
         parent_length = strlen(pemi->full_name);
         emi->full_name = (char *)malloc( parent_length+strlen(name)+2 );
         strcpy( emi->full_name, pemi->full_name );
         emi->full_name[parent_length] = '.';
         strcpy( emi->full_name+parent_length+1, name );
     }
     else
     {
         emi->full_name = emi->name;
     }

     /*b Find any override options for full_name, and create a new list with those options pointing to option_list at the tail;
      */
     combined_option_list = option_list;
     {
         t_engine_module_forced_option *fopt;
         for (fopt = module_forced_options; fopt; fopt=fopt->next_in_list)
         {
             if (!strcmp(fopt->full_name, emi->full_name))
             {
                 combined_option_list = sl_option_list_prepend( combined_option_list, fopt->option );
             }
         }
     }

     emi->option_list = combined_option_list;
     emi->delete_fn_list = NULL;
     emi->reset_fn_list = NULL;
     emi->clock_fn_list = NULL;
     emi->comb_fn_list = NULL;
     emi->propagate_fn_list = NULL;
     emi->prepreclock_fn_list = NULL;
     emi->input_list = NULL;
     emi->output_list = NULL;
     emi->state_desc_list = NULL;
     emi->coverage_desc = NULL;
     emi->checkpoint_fn_list = NULL;
     emi->message_fn_list = NULL;
     emi->log_event_list = NULL;

     emi->sdl_to_view = NULL;
     emi->state_to_view = 0;

     emi->checkpoint_entry = NULL;

     return se_external_module_instantiate( em, this, (void *)emi );
}

/*f c_engine::free_forced_options
 */
void c_engine::free_forced_options( void )
{
     /*b Get rid of forced options
      */
     if (module_forced_options)
     {
         t_engine_module_forced_option *fopt, *fopt_next;
         for (fopt = module_forced_options; fopt; fopt=fopt_next)
         {
             sl_option_free_list( fopt->option );
             fopt_next=fopt->next_in_list;
             free(fopt);
         }
         module_forced_options = NULL;
     }
}

/*f c_engine::instance_add_forced_option
 */
t_sl_error_level c_engine::instance_add_forced_option( const char *full_module_name, t_sl_option_list option_list )
{
    t_engine_module_forced_option *fopt;
    fopt = (t_engine_module_forced_option *)malloc(sizeof(t_engine_module_forced_option)+strlen(full_module_name)+1);
    if (!fopt) return error_level_fatal;
    fopt->next_in_list = module_forced_options;
    module_forced_options = fopt;
    fopt->full_name = ((char *)fopt)+sizeof(t_engine_module_forced_option);
    strcpy( fopt->full_name, full_module_name);
    fopt->option = option_list;
    return error_level_okay;
}

/*f c_engine::add_global_signal
 */
t_sl_error_level c_engine::add_global_signal( const char *global_name, int size )
{
     t_engine_signal *global;

     global = (t_engine_signal *)find_global( global_name );
     if (global)
     {
          return error->add_error( (void *)"global_signal", error_level_serious, error_number_se_duplicate_name, error_id_se_c_engine_global_signal,
                                   error_arg_type_malloc_string, global_name,
                                   error_arg_type_none );
     }
     create_global( global_name, size );
     return error_level_okay;
}

/*f c_engine::name
 */
t_sl_error_level c_engine::name( const char *filename, int line_number, const char *driver_module_name, const char *driver_signal_name, const char *global_name )
{
     t_engine_module_instance *emi;
     t_engine_function *output;
     t_engine_signal *global;

     emi = (t_engine_module_instance *)find_module_instance( NULL, driver_module_name );
     if (!emi)
     {
          return error->add_error( (void *)driver_module_name, error_level_serious, error_number_se_duplicate_name, error_id_se_c_engine_name,
                                   error_arg_type_malloc_string, driver_module_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     output = se_engine_function_find_function( emi->output_list, driver_signal_name );
     if (!output)
     {
          return error->add_error( (void *)driver_module_name, error_level_serious, error_number_se_unknown_output, error_id_se_c_engine_name,
                                   error_arg_type_malloc_string, driver_signal_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     if ( (find_clock(global_name)) )
     {
          return error->add_error( (void *)driver_module_name, error_level_serious, error_number_se_clock_needs_clock_driver, error_id_se_c_engine_name,
                                   error_arg_type_malloc_string, driver_signal_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     global = (t_engine_signal *)find_global( global_name );
     if (!global)
     {
          return error->add_error( (void *)driver_module_name, error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_name,
                                   error_arg_type_malloc_string, global_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }
     if (global->driven_by)
     {
          return error->add_error( (void *)driver_module_name, error_level_serious, error_number_se_multiple_global_drivers, error_id_se_c_engine_name,
                                   error_arg_type_malloc_string, driver_signal_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     se_engine_function_references_add( &global->driven_by, output );
     se_engine_signal_reference_add( &output->data.output.drives, global );
     return error_level_okay;
}

/*f c_engine::drive
 */
t_sl_error_level c_engine::drive( const char *filename, int line_number, const char *driven_module_name, const char *driven_signal_name, const char *global_name )
{
     t_engine_module_instance *emi;
     t_engine_function *input;
     t_engine_signal *global;

     emi = (t_engine_module_instance *)find_module_instance( NULL, driven_module_name );
     if (!emi)
     {
          return error->add_error( (void *)driven_module_name, error_level_serious, error_number_se_unknown_module, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, driven_module_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     input = se_engine_function_find_function( emi->input_list, driven_signal_name );
     if (!input)
     {
          return error->add_error( (void *)driven_module_name, error_level_serious, error_number_se_unknown_input, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, driven_signal_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     if (input->data.input.driven_by)
     {
          return error->add_error( (void *)driven_module_name, error_level_serious, error_number_se_multiple_port_drivers, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, driven_signal_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     if ( (find_clock(global_name)) )
     {
          return error->add_error( (void *)driven_module_name, error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, global_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     global = (t_engine_signal *)find_global( global_name );
     if (!global)
     {
          return error->add_error( (void *)driven_module_name, error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_drive,
                                   error_arg_type_malloc_string, global_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     se_engine_function_references_add( &global->drives_list, input );
     input->data.input.driven_by = global;
     return error_level_okay;
}

/*f c_engine::bind_clock
 */
t_sl_error_level c_engine::bind_clock( const char *filename, int line_number, const char *module_instance_name, const char *module_signal_name, const char *global_clock_name )
{
     t_engine_module_instance *emi;
     t_engine_function *clk;
     t_engine_clock *global_clock;

     emi = (t_engine_module_instance *)find_module_instance( NULL, module_instance_name );
     if (!emi)
     {
          return error->add_error( (void *)module_instance_name, error_level_serious, error_number_se_unknown_module, error_id_se_c_engine_bind_clock,
                                   error_arg_type_malloc_string, module_instance_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     clk = se_engine_function_find_function( emi->clock_fn_list, module_signal_name );
     if (!clk)
     {
          return error->add_error( (void *)module_instance_name, error_level_serious, error_number_se_unknown_clock, error_id_se_c_engine_bind_clock,
                                   error_arg_type_malloc_string, module_signal_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     if (clk->data.clock.driven_by)
     {
          return error->add_error( (void *)module_instance_name, error_level_serious, error_number_se_multiple_source_clocks, error_id_se_c_engine_bind_clock,
                                   error_arg_type_malloc_string, module_signal_name,
                                   error_arg_type_malloc_string, clk->data.clock.driven_by->global_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     global_clock = find_clock( global_clock_name );
     if (!global_clock)
     {
          return error->add_error( (void *)module_instance_name, error_level_serious, error_number_se_unknown_clock, error_id_se_c_engine_bind_clock,
                                   error_arg_type_malloc_string, global_clock_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     clk->data.clock.driven_by = global_clock;
     se_engine_function_references_add( &global_clock->clocks_list, clk );

     return error_level_okay;
}

/*f c_engine::bit_extract
  find bus and target signal
 */
t_sl_error_level c_engine::bit_extract( const char *filename, int line_number, const char *global_bus_name, int start, int size, const char *global_name )
{
     t_engine_signal *bus, *signal;
     t_sl_option_list option_list;
     char *instance_name;

     if ( find_clock(global_name) )
     {
          return error->add_error( (void *)"bit_extract", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_bit_extract,
                                   error_arg_type_malloc_string, global_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }
     if ( find_clock(global_bus_name) )
     {
          return error->add_error( (void *)"bit_extract", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_bit_extract,
                                   error_arg_type_malloc_string, global_bus_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     bus = (t_engine_signal *)find_global( global_bus_name );
     if (!bus)
     {
          return error->add_error( (void *)"bit_extract", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_bit_extract,
                                   error_arg_type_malloc_string, global_bus_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     signal = (t_engine_signal *)find_global( global_name );
     if (!signal)
     {
          return error->add_error( (void *)"bit_extract", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_bit_extract,
                                   error_arg_type_malloc_string, global_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }
     if (signal->size != size)
     {
          return error->add_error( (void *)"bit_extract", error_level_serious, error_number_se_bus_width_mismatch, error_id_se_c_engine_bit_extract,
                                   error_arg_type_integer, size,
                                   error_arg_type_integer, signal->size,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     option_list = sl_option_list( NULL, "width_out", signal->size );
     option_list = sl_option_list( option_list, "width_in", bus->size );
     option_list = sl_option_list( option_list, "start_out", start );

     instance_name = create_unique_instance_name( NULL, "internal", "bit_extract", global_name, NULL );
     if (instantiate( NULL, "__internal_module__bit_extract", instance_name, option_list )==error_level_okay)
     {
          drive( filename, line_number, instance_name, "bus_in", global_bus_name );
          name( filename, line_number, instance_name, "bus_out", global_name );
     }
     free(instance_name);

     return error_level_okay;
}

/*f c_engine::assign
 */
t_sl_error_level c_engine::assign( const char *filename, int line_number, const char *output_bus, int value, int until_cycle, int after_value )
{
     t_engine_signal *bus_out;
     t_sl_option_list option_list;
     char *instance_name;

     if ( find_clock(output_bus) )          
     {
          return error->add_error( (void *)"assign", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_compare,
                                   error_arg_type_malloc_string, output_bus,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     bus_out = (t_engine_signal *)find_global( output_bus );
     if (!bus_out)
     {
          return error->add_error( (void *)"assign", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_compare,
                                   error_arg_type_malloc_string, output_bus,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     option_list = sl_option_list( NULL, "bus_width", bus_out->size );
     option_list = sl_option_list( option_list, "value", value );
     option_list = sl_option_list( option_list, "until_cycle", until_cycle );
     option_list = sl_option_list( option_list, "after_value", after_value );

     instance_name = create_unique_instance_name( NULL, "internal", "assign", output_bus, NULL );
     instantiate( NULL, "__internal_module__assign", instance_name, option_list );
     name( filename, line_number, instance_name, "bus", output_bus );
     if (until_cycle>0)
     {
         create_clock( filename, line_number, instance_name, until_cycle, 1<<30, 1<<30 );
         bind_clock( filename, line_number, instance_name, "reset_clock", instance_name );
     }

     free(instance_name);

     return error_level_okay;
}

/*f c_engine::create_clock_phase
 */
t_sl_error_level c_engine::create_clock_phase( const char *filename, int line_number, const char *output_signal, const char *global_clock_name, int delay, int pattern_length, const char *pattern )
{
    t_engine_signal *signal;
    t_sl_option_list option_list;
    char *instance_name;

    if ( !find_clock(global_clock_name) )          
    {
        return error->add_error( (void *)"clock_phase", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_compare,
                                 error_arg_type_malloc_string, global_clock_name,
                                 error_arg_type_malloc_filename, filename,
                                 error_arg_type_line_number, line_number,
                                 error_arg_type_none );
    }

    if ( find_clock(output_signal) )          
    {
        return error->add_error( (void *)"clock_phase", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_compare,
                                 error_arg_type_malloc_string, output_signal,
                                 error_arg_type_malloc_filename, filename,
                                 error_arg_type_line_number, line_number,
                                 error_arg_type_none );
    }

    signal = (t_engine_signal *)find_global( output_signal );
    if (!signal)
    {
        return error->add_error( (void *)"clock_phase", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_compare,
                                 error_arg_type_malloc_string, output_signal,
                                 error_arg_type_malloc_filename, filename,
                                 error_arg_type_line_number, line_number,
                                 error_arg_type_none );
    }

    option_list = sl_option_list( NULL, "pattern", pattern );
    option_list = sl_option_list( option_list, "pattern_length", pattern_length );
    option_list = sl_option_list( option_list, "delay", delay );

    instance_name = create_unique_instance_name( NULL, "internal", "clock_phase", output_signal, NULL );
    instantiate( NULL, "__internal_module__clock_phase", instance_name, option_list );
    name( filename, line_number, instance_name, "phase", output_signal );
    bind_clock( filename, line_number, instance_name, "int_clock", global_clock_name );

    free(instance_name);

    return error_level_okay;
}

/*f c_engine::compare
 */
t_sl_error_level c_engine::compare( const char *filename, int line_number, const char *input_bus, const char *output_signal, int value )
{
     t_engine_signal *bus_in, *signal_out;
     t_sl_option_list option_list;
     char *instance_name;

     if ( find_clock(input_bus) )
     {
          return error->add_error( (void *)"compare", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_compare,
                                   error_arg_type_malloc_string, input_bus,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }
     if ( find_clock(output_signal) )          
     {
          return error->add_error( (void *)"compare", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_compare,
                                   error_arg_type_malloc_string, output_signal,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     bus_in = (t_engine_signal *)find_global( input_bus );
     if (!bus_in)
     {
          return error->add_error( (void *)"compare", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_compare,
                                   error_arg_type_malloc_string, input_bus,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     signal_out = (t_engine_signal *)find_global( output_signal );
     if (!signal_out)
     {
          return error->add_error( (void *)"compare", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_compare,
                                   error_arg_type_malloc_string, output_signal,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     option_list = sl_option_list( NULL, "bus_width", bus_in->size );
     option_list = sl_option_list( option_list, "value", value );

     instance_name = create_unique_instance_name( NULL, "internal", "compare", output_signal, NULL );
     instantiate( NULL, "__internal_module__cmp", instance_name, option_list );
     drive( filename, line_number, instance_name, "bus", input_bus );
     name( filename, line_number, instance_name, "equal", output_signal );

     free(instance_name);

     return error_level_okay;
}

/*f c_engine::data_mux
 */
t_sl_error_level c_engine::data_mux( const char *filename, int line_number, int number_inputs, const char *global_select_name, const char *global_output_bus_name, const char **global_input_bus_names )
{
     int i;
     t_engine_signal *select_in, *bus_out;
     t_sl_option_list option_list;
     char buffer[256];
     char *instance_name;

     if ( find_clock(global_select_name) )
     {
          return error->add_error( (void *)"data_mux", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_data_mux,
                                   error_arg_type_malloc_string, global_select_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }
     if ( find_clock(global_output_bus_name) )
     {
          return error->add_error( (void *)"data_mux", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_data_mux,
                                   error_arg_type_malloc_string, global_output_bus_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     for (i=0; i<number_inputs; i++)
     {
          if ( (find_clock(global_input_bus_names[i])) )
          {
               return error->add_error( (void *)"data_mux", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_data_mux,
                                        error_arg_type_malloc_string, global_input_bus_names[i],
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                        error_arg_type_none );
          }
     }
     bus_out = (t_engine_signal *)find_global( global_output_bus_name );
     if (!bus_out)
     {
          return error->add_error( (void *)"data_mux", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_data_mux,
                                   error_arg_type_malloc_string, bus_out,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }
     select_in = (t_engine_signal *)find_global( global_select_name );
     if (!select_in)
     {
          return error->add_error( (void *)"data_mux", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_data_mux,
                                   error_arg_type_malloc_string, select_in,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     option_list = sl_option_list( NULL, "bus_width", bus_out->size );
     option_list = sl_option_list( option_list, "number_inputs", number_inputs );
     option_list = sl_option_list( option_list, "select_width", select_in->size );

     instance_name = create_unique_instance_name( NULL, "internal", "data_mux", global_output_bus_name, NULL );
     instantiate( NULL, "__internal_module__data_mux", instance_name, option_list );
     drive( filename, line_number, instance_name, "select", global_select_name );
     name( filename, line_number, instance_name, "bus_out", global_output_bus_name );
     for (i=0; i<number_inputs; i++)
     {
          sprintf( buffer, "bus_in__%d__", i );
          drive( filename, line_number, instance_name, buffer, global_input_bus_names[i] );
     }

     free(instance_name);

     return error_level_okay;
}

/*f c_engine::decode
 */
t_sl_error_level c_engine::decode( const char *filename, int line_number, const char *input_bus, const char *output_bus, const char *enable )
{
     t_engine_signal *bus_in, *bus_out, *enable_in;
     t_sl_option_list option_list;
     char *instance_name;

     if ( find_clock(input_bus) )
     {
          return error->add_error( (void *)"decode", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_decode,
                                   error_arg_type_malloc_string, input_bus,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }
     if ( find_clock(output_bus) )          
     {
          return error->add_error( (void *)"decode", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_decode,
                                   error_arg_type_malloc_string, output_bus,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }
     if ( (enable) && find_clock(enable) )
     {
          return error->add_error( (void *)"decode", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_decode,
                                   error_arg_type_malloc_string, enable,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     bus_in = (t_engine_signal *)find_global( input_bus );
     if (!bus_in)
     {
          return error->add_error( (void *)"decode", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_decode,
                                   error_arg_type_malloc_string, input_bus,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     bus_out = (t_engine_signal *)find_global( output_bus );
     if (!bus_out)
     {
          return error->add_error( (void *)"decode", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_decode,
                                   error_arg_type_malloc_string, output_bus,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     if (enable)
     {
          enable_in = (t_engine_signal *)find_global( enable );
          if (!enable_in)
          {
               return error->add_error( (void *)"decode", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_decode,
                                        error_arg_type_malloc_string, enable,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                        error_arg_type_none );
          }
     }
     else
     {
          enable_in = NULL;
     }

     option_list = sl_option_list( NULL, "enabled", enable!=NULL );
     option_list = sl_option_list( option_list, "input_width", bus_in->size );

     instance_name = create_unique_instance_name( NULL, "internal", "decode", output_bus, NULL );
     instantiate( NULL, "__internal_module__decode", instance_name, option_list );
     drive( filename, line_number, instance_name, "bus_in", input_bus );
     if (enable)
     {
          drive( filename, line_number, instance_name, "enable", enable );
     }
     name( filename, line_number, instance_name, "bus_out", output_bus );

     free(instance_name);

     return error_level_okay;
}

/*f c_engine::bundle
 */
t_sl_error_level c_engine::bundle( const char *filename, int line_number, int number_inputs, const char *global_output_bus_name, const char **global_input_bus_names )
{
     int i;
     t_engine_signal *bus_out;
     t_sl_option_list option_list;
     char buffer[256];
     char *instance_name;

     if ( (find_clock(global_output_bus_name)) )
     {
          return error->add_error( (void *)"bundle", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_generic_logic,
                                   error_arg_type_malloc_string, global_output_bus_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     for (i=0; i<number_inputs; i++)
     {
          if ( (find_clock(global_input_bus_names[i])) )
          {
               return error->add_error( (void *)"bundle", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_generic_logic,
                                        error_arg_type_malloc_string, global_input_bus_names[i],
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                        error_arg_type_none );
          }
     }
     bus_out = (t_engine_signal *)find_global( global_output_bus_name );
     if (!bus_out)
     {
               return error->add_error( (void *)"bundle", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_generic_logic,
                                        error_arg_type_malloc_string, bus_out,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                        error_arg_type_none );
     }

     option_list = NULL;
     option_list = sl_option_list( option_list, "width", bus_out->size );
     for (i=0; i<number_inputs; i++)
     {
         char buffer[512];
         t_engine_signal *bus_in;
         sprintf( buffer, "width%d", i );
         bus_in = (t_engine_signal *)find_global( global_input_bus_names[i] );
         if (bus_in)
         {
             option_list = sl_option_list( option_list, buffer, bus_in->size );
         }
     }
     option_list = sl_option_list( option_list, "number_inputs", number_inputs );

     instance_name = create_unique_instance_name( NULL, "internal", "bundle", global_output_bus_name, NULL );
     instantiate( NULL, "__internal_module__bundle", instance_name, option_list );
     name( filename, line_number, instance_name, "bus_out", global_output_bus_name );
     for (i=0; i<number_inputs; i++)
     {
          sprintf( buffer, "bus_in__%d__", i );
          drive( filename, line_number, instance_name, buffer, global_input_bus_names[i] );
     }

     free(instance_name);

     return error_level_okay;
}

/*f c_engine::generic_logic
 */
t_sl_error_level c_engine::generic_logic( const char *filename, int line_number, int number_inputs, const char *type, const char *global_output_bus_name, const char **global_input_bus_names )
{
     int i;
     t_engine_signal *bus_out;
     t_sl_option_list option_list;
     char buffer[256];
     char *instance_name;

     if ( (find_clock(global_output_bus_name)) )
     {
          return error->add_error( (void *)"generic_logic", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_generic_logic,
                                   error_arg_type_malloc_string, global_output_bus_name,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                   error_arg_type_none );
     }

     for (i=0; i<number_inputs; i++)
     {
          if ( (find_clock(global_input_bus_names[i])) )
          {
               return error->add_error( (void *)"generic_logic", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_generic_logic,
                                        error_arg_type_malloc_string, global_input_bus_names[i],
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                        error_arg_type_none );
          }
     }
     bus_out = (t_engine_signal *)find_global( global_output_bus_name );
     if (!bus_out)
     {
               return error->add_error( (void *)"generic_logic", error_level_serious, error_number_se_unknown_global, error_id_se_c_engine_generic_logic,
                                        error_arg_type_malloc_string, bus_out,
                                   error_arg_type_malloc_filename, filename,
                                   error_arg_type_line_number, line_number,
                                        error_arg_type_none );
     }

     option_list = sl_option_list( NULL, "width", bus_out->size );
     option_list = sl_option_list( option_list, "number_inputs", number_inputs );
     option_list = sl_option_list( option_list, "type", type );

     instance_name = create_unique_instance_name( NULL, "internal", type, global_output_bus_name, NULL );
     instantiate( NULL, "__internal_module__generic_logic", instance_name, option_list );
     name( filename, line_number, instance_name, "bus_out", global_output_bus_name );
     for (i=0; i<number_inputs; i++)
     {
          sprintf( buffer, "bus_in__%d__", i );
          drive( filename, line_number, instance_name, buffer, global_input_bus_names[i] );
     }

     free(instance_name);

     return error_level_okay;
}

/*f c_engine::create_clock
 */
t_sl_error_level c_engine::create_clock( const char *filename, int line_number, const char *global_clock_name, int delay, int high_cycles, int low_cycles )
{
     t_engine_clock *clk;

     if ( (find_global(global_clock_name)) ||
          (find_clock(global_clock_name)) )
     {
        return error->add_error( (void *)"clock", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_generic_logic,
                                 error_arg_type_malloc_string, global_clock_name,
                                 error_arg_type_malloc_filename, filename,
                                 error_arg_type_line_number, line_number,
                                 error_arg_type_none );
     }
     clk = create_clock( global_clock_name );
     clk->delay = delay;
     clk->high_cycles = high_cycles;
     clk->low_cycles = low_cycles;
     return error_level_okay;
}

/*f c_engine::create_clock_divide
 */
t_sl_error_level c_engine::create_clock_divide( const char *filename, int line_number, const char *global_clock_name, const char *source_clock_name, int delay, int high_cycles, int low_cycles )
{
    t_engine_clock *clk, *source_clock;

    if ( (find_global(global_clock_name)) ||
         (find_clock(global_clock_name)) )
    {
        return error->add_error( (void *)"clock_divide", error_level_serious, error_number_se_misuse_of_clock, error_id_se_c_engine_generic_logic,
                                 error_arg_type_malloc_string, global_clock_name,
                                 error_arg_type_malloc_filename, filename,
                                 error_arg_type_line_number, line_number,
                                 error_arg_type_none );
    }
    source_clock = find_clock( source_clock_name );
    if (!source_clock)
    {
        return error->add_error( (void *)"clock_divide", error_level_serious, error_number_se_unknown_clock, error_id_se_c_engine_bind_clock,
                                 error_arg_type_malloc_string, global_clock_name,
                                 error_arg_type_malloc_filename, filename,
                                 error_arg_type_line_number, line_number,
                                 error_arg_type_none );
    }
    clk = create_clock( global_clock_name );
    clk->delay = source_clock->delay + (source_clock->high_cycles+source_clock->low_cycles)*delay; // Delay to first rising edge, edge is coincident with source rising
    clk->high_cycles = (source_clock->high_cycles+source_clock->low_cycles)*high_cycles;
    clk->low_cycles = (source_clock->high_cycles+source_clock->low_cycles)*low_cycles;
    return error_level_okay;
}

/*f c_engine::module_drive_input
 */
void c_engine::module_drive_input( t_engine_function *input, t_se_signal_value *value_ptr )
{
    t_engine_function_reference *efr;
    *input->data.input.value_ptr_ptr = value_ptr;
    for (efr=input->data.input.connected_to; efr; efr=efr->next_in_list)
    {
        module_drive_input( efr->signal, value_ptr );
    }
}

/*f c_engine::check_connectivity
  check all instantiated modules' inputs have a driver - and wire it up
  check all instantiated modules' outputs are driven
  check all instantiated modules' clocks are driven
  check all global signals are driven and drive
 */
t_sl_error_level c_engine::check_connectivity( void )
{
     t_engine_module_instance *emi;
     t_engine_function *emi_sig;
     t_engine_signal *sig;

     for (emi=module_instance_list; emi; emi=emi->next_instance )
     {
         if (emi->parent_instance) // Check a submodule differently
         {
             SL_DEBUG(sl_debug_level_info, "c_engine::check_connectivity", "Submodule %s", emi->full_name);
             for (emi_sig=emi->input_list;emi_sig;emi_sig=emi_sig->next_in_list)
             {
                 if ((!*emi_sig->data.input.value_ptr_ptr) && (!emi_sig->data.input.may_be_unconnected))
                 {
                     SL_DEBUG(sl_debug_level_info, "c_engine::check_connectivity", "Module %s undriven input %s", emi->full_name, emi_sig->name );
                     error->add_error( (void *)"generic_logic", error_level_serious, error_number_se_undriven_input, error_id_se_c_engine_check_connectivity,
                                       error_arg_type_malloc_string, emi->full_name,
                                       error_arg_type_malloc_string, emi_sig->name, 
                                       error_arg_type_none );
                 }
             }             
         }
         else
         {
             for (emi_sig=emi->input_list;emi_sig;emi_sig=emi_sig->next_in_list)
             {
                 sig = emi_sig->data.input.driven_by;
                 (*emi_sig->data.input.value_ptr_ptr) = dummy_input_value;
                 if (sig)
                 {
                     if (sig->size != emi_sig->data.input.size)
                     {
                         error->add_error( (void *)"generic_logic", error_level_warning, error_number_se_bus_width_mismatch_check, error_id_se_c_engine_check_connectivity,
                                           error_arg_type_malloc_string, emi->full_name,
                                           error_arg_type_malloc_string, emi_sig->name, 
                                           error_arg_type_integer, emi_sig->data.input.size,
                                           error_arg_type_malloc_string, sig->global_name,
                                           error_arg_type_integer, sig->size,
                                           error_arg_type_none );
                     }
                     if (sig->driven_by)
                     {
                         module_drive_input( emi_sig, sig->driven_by->signal->data.output.value_ptr );
                     }
                 }
                 else if (!emi_sig->data.input.may_be_unconnected)
                 {
                     error->add_error( (void *)"generic_logic", error_level_warning, error_number_se_undriven_input, error_id_se_c_engine_check_connectivity,
                                       error_arg_type_malloc_string, emi->full_name,
                                       error_arg_type_malloc_string, emi_sig->name, 
                                       error_arg_type_none );
                 }
                 else
                 {
                     (*emi_sig->data.input.value_ptr_ptr) = NULL;
                     //error->add_error( (void *)"generic_logic", error_level_info, error_number_se_undriven_input, error_id_se_c_engine_check_connectivity,
                     //                  error_arg_type_malloc_string, emi->full_name,
                     //                  error_arg_type_malloc_string, emi_sig->name, 
                     //                  error_arg_type_none );
                 }
             }
             for (emi_sig=emi->output_list;emi_sig;emi_sig=emi_sig->next_in_list)
             {
                 if (emi_sig->data.output.drives)
                 {
                     t_engine_signal_reference *sig_ref;
                     for (sig_ref = emi_sig->data.output.drives; sig_ref; sig_ref=sig_ref->next_in_list)
                     {
                         sig = sig_ref->signal;
                         if (sig->size != emi_sig->data.output.size)
                         {
                             error->add_error( (void *)"generic_logic", error_level_warning, error_number_se_bus_width_mismatch_check, error_id_se_c_engine_check_connectivity,
                                               error_arg_type_malloc_string, emi->full_name,
                                               error_arg_type_malloc_string, emi_sig->name, 
                                               error_arg_type_integer, emi_sig->data.output.size,
                                               error_arg_type_malloc_string, sig->global_name,
                                               error_arg_type_integer, sig->size,
                                               error_arg_type_none );
                         }
                     }
                 }
                 else
                 {
                     error->add_error( (void *)"generic_logic", error_level_warning, error_number_se_unloaded_output, error_id_se_c_engine_check_connectivity,
                                       error_arg_type_malloc_string, emi->full_name,
                                       error_arg_type_malloc_string, emi_sig->name, 
                                       error_arg_type_none );
                 }
             }
             for (emi_sig=emi->clock_fn_list;emi_sig;emi_sig=emi_sig->next_in_list)
             {
                 if (!emi_sig->data.clock.driven_by)
                 {
                     error->add_error( (void *)"generic_logic", error_level_warning, error_number_se_undriven_clock, error_id_se_c_engine_check_connectivity,
                                       error_arg_type_malloc_string, emi->full_name,
                                       error_arg_type_malloc_string, emi_sig->name, 
                                       error_arg_type_none );
                 }
             }
         }
     }

     for (sig=global_signal_list; sig; sig=sig->next_in_list)
     {
          if (!sig->drives_list)
          {
               error->add_error( (void *)"generic_logic", error_level_warning, error_number_se_unloaded_global, error_id_se_c_engine_check_connectivity,
                                 error_arg_type_malloc_string, sig->global_name,
                                 error_arg_type_none );
          }
          if (!sig->driven_by)
          {
               error->add_error( (void *)"generic_logic", error_level_warning, error_number_se_undriven_global, error_id_se_c_engine_check_connectivity,
                                 error_arg_type_malloc_string, sig->global_name,
                                 error_arg_type_none );
          }
     }
     return error_level_okay;
}

/*f c_engine::create_unique_instance_name
 */
char *c_engine::create_unique_instance_name( void *parent, const char *first_hint, ... )
{
     va_list ap;
     char buffer[2048];
     char *result;
     const char *hint;
     int i, base_len;

     va_start( ap, first_hint );

     buffer[0] = 0;

     hint = first_hint;
     i=0;
     while( hint )
     {
          sprintf( buffer+strlen(buffer), "%s%s", (i>0)?"__":"", hint );
          hint = (char *)va_arg( ap, char *);
          i++;
     }
     base_len = strlen(buffer);
     for (i=0; ; i++)
     {
          if (!find_module_instance( parent, buffer ))
          {
               break;
          }
          sprintf( buffer+base_len, "%d", i );
     }
     result = sl_str_alloc_copy( buffer );

     va_end (ap );

     return result;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


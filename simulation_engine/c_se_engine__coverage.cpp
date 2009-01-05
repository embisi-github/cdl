/*a Copyright
  
  This file 'c_se_engine__coverage.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "se_engine_function.h"
#include "c_se_engine.h"
#include "c_se_engine__internal_types.h"

/*a Types
 */
/*a Static function declarations
 */
//static t_sl_exec_file_eval_fn ef_fn_eval_vcd_file_size;

/*a Statics
 */
/*v cmd_*
 */
enum
{
    cmd_coverage_reset,
    cmd_coverage_save,
    cmd_coverage_load,
};

/*v fn_*
 */
enum {
    fn_coverage_size
};

/*v sim_file_cmds
 */
static t_sl_exec_file_cmd sim_file_cmds[] =
{
     {cmd_coverage_reset,       0, "coverage_reset", "ss", "coverage_reset"},
     {cmd_coverage_save,        1, "coverage_save",  "ss", "coverage_save <filename> [<module instance name>]"},
     {cmd_coverage_load,        1, "coverage_load",  "ss", "coverage_load <filename> [<module instance name>]"},
     {sl_exec_file_cmd_none, 0, NULL, NULL, NULL }
};

/*v sim_file_fns
 */
static t_sl_exec_file_fn sim_file_fns[] =
{
     {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },
};

/*a Exec file evaluation functions
 */

/*a Cycle simulation exec_file enhancement functions and methods
 */
/*f static exec_file_cmd_handler
 */
static t_sl_error_level exec_file_cmd_handler( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    c_engine *engine;
    engine = (c_engine *)handle;
    if (!engine->coverage_handle_exec_file_command( cmd_cb->file_data, cmd_cb->cmd, cmd_cb->args ))
        return error_level_serious;
    return error_level_okay;
}

/*f c_engine::coverage_add_exec_file_enhancements
 */
int c_engine::coverage_add_exec_file_enhancements( struct t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_lib_desc lib_desc;
    lib_desc.version = sl_ef_lib_version_cmdcb;
    lib_desc.library_name = "cdlsim.cover";
    lib_desc.handle = (void *) this;
    lib_desc.cmd_handler = exec_file_cmd_handler;
    lib_desc.file_cmds = sim_file_cmds;
    lib_desc.file_fns = sim_file_fns;
    return sl_exec_file_add_library( file_data, &lib_desc );
}

/*f c_engine::coverage_reset
 */
void c_engine::coverage_reset( void )
{
    t_engine_module_instance *emi;

    for (emi=module_instance_list; emi; emi=emi->next_instance )
    {
        if (emi->coverage_desc)
        {
            if (emi->coverage_desc->stmt_data)
            {
                memset( emi->coverage_desc->stmt_data, 0, emi->coverage_desc->stmt_data_size );
            }
            if (emi->coverage_desc->code_map)
            {
                memset( emi->coverage_desc->code_map, 0, emi->coverage_desc->code_map_size*sizeof(t_sl_uint64) );
            }
        } 
    }
}

/*f c_engine::coverage_save
 */
void c_engine::coverage_save( char *filename, char *entity_name )
{
    t_engine_module_instance *emi;
    FILE *f;
    int i;
    f = fopen(filename, "w");
    if (!f) return;
    for (emi=module_instance_list; emi; emi=emi->next_instance )
    {
        if (emi->coverage_desc)
        {
            fprintf( f, "%s %s %d %d\n", emi->type, emi->name, emi->coverage_desc->stmt_data_size, emi->coverage_desc->code_map_size );
            if (emi->coverage_desc->stmt_data)
            {
                for (i=0; i<emi->coverage_desc->stmt_data_size; i++)
                {
                    fprintf( f, "%3d%c", emi->coverage_desc->stmt_data[i],(((i+1)%8)!=0)?' ':'\n');
                }
                fprintf( f, "\n" );
            }
            if (emi->coverage_desc->code_map)
            {
                for (i=0; i<emi->coverage_desc->code_map_size; i++)
                {
                    fprintf( f, "%16llx%c", emi->coverage_desc->code_map[i], (((i+1)%4)!=0)?' ':'\n');
                }
            }
            fprintf( f, "\n" );
        } 
    }
    fclose(f);
}

/*f c_engine::coverage_load
 */
void c_engine::coverage_load( char *filename, char *entity_name )
{
    t_engine_module_instance *emi;
    char type[512], name[512];
    int map_size, data_size;

    FILE *f;
    int i;
    f = fopen(filename, "r");
    if (!f) return;
    while (1)
    {
        if (fscanf( f, " %510s %510s %d %d", type, name, &map_size, &data_size )!=3)
            break;
        for (emi=module_instance_list; emi; emi=emi->next_instance )
        {
            if ( (!strcmp(type,emi->type)) &&
                 (!strcmp(name,emi->name)) &&
                 emi->coverage_desc &&
                 (map_size==emi->coverage_desc->code_map_size) &&
                 (data_size==emi->coverage_desc->stmt_data_size) )
                break;
        }
        if (!emi)
            break;
        for (i=0; i<data_size; i++)
        {
            int j;
            if (fscanf( f, " %d", &j)!=1)
            {
                break;
            }
            emi->coverage_desc->stmt_data[i] = j;
        }
        if (i<data_size)
            break;
        for (i=0; i<map_size; i++)
        {
            t_sl_uint64 j;
            if (fscanf( f, " %llx", &j)!=1)
            {
                break;
            }
            emi->coverage_desc->code_map[i] = j;
        }
        if (i<map_size)
            break;
    }
    fclose(f);
}

/*f c_engine::coverage_handle_exec_file_command
  returns 0 if it did not handle
  returns 1 if it handled it
 */
int c_engine::coverage_handle_exec_file_command( struct t_sl_exec_file_data *exec_file_data, int cmd, struct t_sl_exec_file_value *args )
{
     switch (cmd)
     {
     case cmd_coverage_reset:
         coverage_reset();
         return 1;
         break;
     case cmd_coverage_load:
         coverage_load( args[0].p.string, NULL );
         return 1;
         break;
     case cmd_coverage_save:
         coverage_save( args[0].p.string, NULL );
         return 1;
         break;
     }
     return 0;
}

/*a Coverage registration
 */
/*f c_engine::coverage_stmt_register
 */
void c_engine::coverage_stmt_register( void *engine_handle, void *data, int data_size )
{
     t_engine_module_instance *emi;
     emi = (t_engine_module_instance *)engine_handle;

     SL_DEBUG(sl_debug_level_info, "c_engine::coverage_stmt_register", "Registering statement coverage" ) ;

     if (!emi)
          return;

     if (!emi->coverage_desc)
     {
         emi->coverage_desc = (t_engine_coverage_desc *)malloc(sizeof(t_engine_coverage_desc));
         if (emi->coverage_desc) memset(emi->coverage_desc,0,sizeof(t_engine_coverage_desc));
     }
     if (emi->coverage_desc)
     {
         emi->coverage_desc->stmt_data_size = data_size;
         emi->coverage_desc->stmt_data = (unsigned char *)data;
         memset( emi->coverage_desc->stmt_data, 0, emi->coverage_desc->stmt_data_size );
     }
}

/*f c_engine::coverage_code_register
 */
void c_engine::coverage_code_register( void *engine_handle, void *map, int map_size )
{
     t_engine_module_instance *emi;
     emi = (t_engine_module_instance *)engine_handle;

     SL_DEBUG(sl_debug_level_info, "c_engine::coverage_code_register", "Registering cover code coverage" ) ;

     if (!emi)
          return;

     if (!emi->coverage_desc)
     {
         emi->coverage_desc = (t_engine_coverage_desc *)malloc(sizeof(t_engine_coverage_desc));
         if (emi->coverage_desc) memset(emi->coverage_desc,0,sizeof(t_engine_coverage_desc));
     }
     if (emi->coverage_desc)
     {
         emi->coverage_desc->code_map_size = map_size;
         emi->coverage_desc->code_map = (t_sl_uint64 *)map;
         memset( emi->coverage_desc->code_map, 0, emi->coverage_desc->code_map_size*sizeof(t_sl_uint64) );
     }
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


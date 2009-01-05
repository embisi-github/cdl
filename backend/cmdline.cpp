/*a Copyright
  
  This file 'cmdline.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/
/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "c_sl_error.h"
#include "c_model_descriptor.h"
#include "md_target_c.h"
#include "md_target_vhdl.h"

/*a Defines
 */
#define MAX_INDENT (3)

/*a Statics
 */
static char *indent_string = "    ";

/*a Output functions
 */
/*f output_indented
 */
static void output_indented( void *handle, int indent, char *format, ... )
{
     FILE *f;
     va_list ap;
     int i;

     va_start( ap, format );

     f = (FILE *)handle;
     if (indent>=0)
     {
          for (i=0; i<indent; i++)
               fprintf( f, indent_string );
     }
     vfprintf( f, format, ap );
     va_end( ap );
}

/*a Main routine
 */
extern int main( int argc, char **argv )
{
     FILE *f;
     c_model_descriptor *model;
     c_sl_error *error;
     t_md_module *module;
     t_md_expression *expr1, *expr2;
     t_md_statement *stmt1, *stmt2;
     
     error = new c_sl_error();
     model = new c_model_descriptor( "sample", error );

     module = model->module_create( "toplevel", 0, NULL, NULL, 0 );
     model->module_set_toplevel( "toplevel" );

     model->signal_add_clock( module, "apb_clock", 0, NULL, NULL, 0 );
     model->signal_add_input( module, "reset", 0, NULL, NULL, 0, 1 );
     model->signal_add_input( module, "select", 0, NULL, NULL, 0, 1 );
     model->signal_add_input( module, "enable", 0, NULL, NULL, 0, 1 );
     model->signal_add_input( module, "read_not_write", 0, NULL, NULL, 0, 1 );
     model->signal_add_input( module, "write_data", 0, NULL, NULL, 0, 32 );
     model->signal_add_input( module, "address", 0, NULL, NULL, 0, 16 );
     model->signal_add_output( module, "read_data", 0, NULL, NULL, 0, 32 );
     model->signal_add_output( module, "interrupt", 0, NULL, NULL, 0, 1 );

     model->signal_add_combinatorial( module, "interrupt", 0, NULL, NULL, 0, 1 );
     model->signal_add_combinatorial( module, "write_reg", 0, NULL, NULL, 0, 1 );

     model->state_add( module, "read_data", 0, NULL, NULL, 0, 32, "apb_clock", md_edge_pos, "reset", 1, &md_signal_value_zero );

     model->code_block( module, "read_data_setting", 0, NULL, NULL, 0 );

     expr1 = model->expression( module, "select" );
     expr2 = model->expression( module, "enable" );
     expr1 = model->expression( module, md_expr_fn_and, expr1, expr2, NULL );
     model->code_block_add_statement( module, "read_data_setting", model->statement_create_full_combinatorial_assignment( module, NULL, NULL, 0, "write_reg", expr1 ) );

     expr1 = model->expression( module, 32, &md_signal_value_one );
     expr2 = model->expression( module, "read_data" );
     expr1 = model->expression( module, md_expr_fn_add, expr1, expr2, NULL );
     stmt1 = model->statement_create_full_state_assignment( module, NULL, NULL, 0, "read_data", expr1 );

     expr1 = model->expression( module, "write_reg" );
     stmt2 = model->statement_create_if_else( module, NULL, NULL, 0, expr1, stmt1, NULL );
     model->code_block_add_statement( module, "read_data_setting", stmt2 );

     expr1 = model->expression( module, 1, &md_signal_value_one );
     model->code_block( module, "interrupt_output", 0, NULL, NULL, 0 );
     model->code_block_add_statement( module, "interrupt_output", model->statement_create_full_combinatorial_assignment( module, NULL, NULL, 0, "interrupt", expr1 ) );

     if (model->error->get_error_count(error_level_info))
     {
          void *handle;
          char error_buffer[1024];
          handle = model->error->get_next_error( NULL, error_level_info );
          while (handle)
          {
               if (model->error->generate_error_message( handle, error_buffer, 1024, 1, NULL ))
               {
                    fprintf(stderr, "Error: %s\n", error_buffer );
               }
               handle = model->error->get_next_error( handle, error_level_info );
          }
          exit(4);
     }

     model->module_analyze( module );
//     model->module_display_references( module, output_indented, (void *)stdout );

     f = fopen("/tmp/a.cpp", "w");//stdout;
     target_c_output( model, output_indented, (void *)f );
     fclose(f);

     f = fopen("/tmp/a.vhd", "w");//stdout;
     target_vhdl_output( model, output_indented, (void *)f );
     fclose(f);

     delete( model );
     return 0;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


/*a Copyright
  
  This file 'cyclicity.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include <string.h>
#include <math.h>  /* For math functions, cos(), sin(), etc. */
#include <ctype.h>
#include <getopt.h>
#include "cdl_version.h"
#include "sl_debug.h"
#include "sl_option.h"
#include "c_model_descriptor.h"
#include "c_cyclicity.h"

// for debug
#include "c_type_value_pool.h"

/*a Function declarations
 */

/*a Types
 */
/*t t_string_list
 */
typedef struct string_list
{
     struct string_list *next;
     char *string;
} string_list;

/*a Statics
 */
/*t option_*
 */
enum 
{
     option_constant  = option_cdl_start,
     option_type_remap,
     option_debug,
     option_include,
     option_help,
     option_version
};

/*v options
 */
static option options[] = {
     SL_OPTIONS
     BE_OPTIONS
     { "constant", required_argument, NULL, option_constant },
     { "type-remap", required_argument, NULL, option_type_remap },
     { "debug", required_argument, NULL, option_debug },
     { "include-dir", required_argument, NULL, option_include },
     { "help", no_argument, NULL, option_help },
     { "version", no_argument, NULL, option_version },
     { NULL, no_argument, NULL, 0 }
};

/*a C code
 */
/*f display_errors
 */
static void display_errors( c_cyclicity *cyc, int include_text )
{
     int i, number_errors;
     t_lex_file_posn lex_file_posn;
     int file_number, first_line, char_offset, last_line;
     char *error_buffer;

     number_errors = cyc->get_number_of_parse_errors();
     if (number_errors)
     {
          for (i=0; i<number_errors; i++)
          {
              cyc->read_parse_error( i, &lex_file_posn, &error_buffer );
              cyc->translate_lex_file_posn( &lex_file_posn, &file_number, &first_line, &last_line, &char_offset );
              fprintf( stderr, "%s:%d:(char %d): %s\n", cyc->get_filename(file_number), 1+first_line, char_offset, error_buffer );
              if (include_text)
              {
                  char text_buffer[256];
                  cyc->parse_file_text_around( text_buffer, sizeof(text_buffer), &lex_file_posn, 1 ); // Annnotated file text around the error
                  if (text_buffer[0])
                  {
                      fprintf( stderr, "%s\n", text_buffer );
                  }
              }
          }
          delete(cyc);
          exit(4);
     }
     if (cyc->error->check_errors_and_reset( stderr, error_level_info, error_level_fatal ))
     {
         exit(4);
     }
}

/*f usage
 */
static void usage( void )
{
     printf( "Usage: [options] file\n");
     sl_option_getopt_usage();
     be_getopt_usage();
     printf( "\t--constant <defn>\tDefine a constant (overrides the value\n\t\t\t\t in the source. <defn> is \n\t\t\t\t <userid>=<sized integer>\n");
     printf( "\t--debug <level>\tTurn on debugging (level ignored as yet)\n");
     printf( "\t--help \t\tDisplay this help message\n");
     printf( "\t--version \t\tDisplay the version and copyright information\n");
     printf( "\t--include-dir <directory> \tAppend directory to search path for include files\n");
}

/*f version
 */
static void version( void )
{
    printf( "cdl version " __CDL__VERSION_STRING "\n" );
    printf( "Copyright %s\n", __CDL__VERSION_COPYRIGHT_STRING );
}

/*f main
 */
extern int main( int argc, char **argv )
{
     c_cyclicity *cyc;
     int c, so_far;
     string_list *constant_overrides, *type_remappings, *new_str, *include_dirs, *last_include_dir;
     t_sl_option_list env_options;

     sl_debug_set_level( sl_debug_level_verbose_info );
     sl_debug_enable( 0 );
     env_options = NULL;
     constant_overrides = NULL;
     type_remappings = NULL;
     include_dirs = NULL;
     last_include_dir = NULL;

     so_far = 0;
     c = 0;
     optopt=1;
     while(c>=0)
     {
          c = getopt_long( argc, argv, "hvd:", options, &so_far );
          if ( !be_handle_getopt( &env_options, c, optarg) &&
               !sl_option_handle_getopt( &env_options, c, optarg) )
          {
               switch (c)
               {
               case 'h':
               case option_help:
                    usage();
                    exit(0);
                    break;
               case 'v':
               case option_version:
                    version();
                    exit(0);
                    break;
               case 'd':
               case option_debug:
                   sl_debug_enable(1);
                    break;
               case option_constant:
                    new_str = (string_list *)malloc(sizeof(string_list));
                    new_str->next = constant_overrides;
                    new_str->string = optarg;
                    constant_overrides = new_str;
                    break;
               case option_type_remap:
                    new_str = (string_list *)malloc(sizeof(string_list));
                    new_str->next = type_remappings;
                    new_str->string = optarg;
                    type_remappings = new_str;
                    break;
               case option_include:
                    new_str = (string_list *)malloc(sizeof(string_list));
                    if (!include_dirs)
                         include_dirs = new_str;
                    else
                         last_include_dir->next = new_str;
                    new_str->next = NULL;
                    last_include_dir = new_str;
                    new_str->string = optarg;
                    break;
               case '?':
                    exit(4);
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
     cyc = new c_cyclicity();
     for (new_str = include_dirs; new_str; new_str=new_str->next)
     {
          cyc->add_include_directory( new_str->string );
     }

     for (new_str = type_remappings; new_str; new_str=new_str->next)
     {
          cyc->override_type_mapping( new_str->string );
     }

     cyc->parse_input_file( argv[optind] );

     display_errors( cyc, 1 );

     cyc->index_global_symbols(); // Generate lists of modules, bus_definitions, constants, types

     for (new_str = constant_overrides; new_str; new_str=new_str->next)
     {
          cyc->override_constant_declaration( new_str->string );
     }

     cyc->cross_reference_and_evaluate_global_symbols(); // Cross-reference and evaluate any expressions and definitions of global symbols (fsm_state, enum_identifier, types, constants) in textual order

     cyc->cross_reference_module_prototypes();

//     cyc->check_types_module_prototypes();

//     cyc->evaluate_constants_module_prototypes();

//     cyc->high_level_check_module_prototypes();

     cyc->cross_reference_modules();

     display_errors( cyc, 1 );

     cyc->check_types_modules();

     display_errors( cyc, 1 );

     cyc->evaluate_constants_modules();

     display_errors( cyc, 1 );

     cyc->high_level_check_modules();

     display_errors( cyc, 1 );

     //cyc->type_value_pool->display();

     cyc->build_model( env_options );

     display_errors( cyc, 1 );

     cyc->output_model( env_options );

     display_errors( cyc, 1 );

//     cyc->print_debug_info();

     delete(cyc);
     return 0;
}



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



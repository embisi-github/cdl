/*a Copyright
  
  This file 'c_cyclicity.h' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a Wrapper
 */
#ifdef __INC_C_CYCLICITY
#else
#define __INC_C_CYCLICITY

/*a Includes
 */
#include <stdarg.h>
#include "sl_option.h"
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t	c_cyclicity
*/
class c_cyclicity
{
public:
	c_cyclicity();
	~c_cyclicity();

    void add_include_directory( char *directory );

	int parse_input_file( FILE *f );
	int parse_input_file( char *filename );
    int parse_repeat_eof( void );
    t_lex_file_posn parse_get_current_location( void );
    void set_parse_error_va_list( t_lex_file_posn lex_file_posn, t_co_compile_stage co_compile_stage, const char *format, va_list ap);
    void set_parse_error( t_lex_file_posn lex_file_posn, t_co_compile_stage co_compile_stage, const char *format, ... );
    void set_parse_error( c_cyc_object *cyc_object, t_co_compile_stage co_compile_stage, const char *format, ... );
    void set_parse_error( t_co_compile_stage co_compile_stage, const char *format, ... );
    int get_number_of_parse_errors( void );
    int read_parse_error( int error_number, t_lex_file_posn *lex_file_posn, char **buffer );
    int parse_file_text_around( char *buffer, int buffer_size, t_lex_file_posn *lex_file_posn, int include_annotation ); // Generate string with file text around the file posn, possibly annotated with a pointer

    int get_number_of_files( void );
    char *get_filename( int file_number );
    int get_file_data( int file_number, int *file_size, int *number_lines, char **file_data );
    int get_line_data( int file_number, int line_number, char **line_start, int *line_length );
    int translate_lex_file_posn( t_lex_file_posn *lex_file_posn, int *file_number, int *first_line, int *last_line, int *char_offset );
    int translate_lex_file_posn( t_lex_file_posn *lex_file_posn, int *file_number, int *file_position, int end_not_start );

    void fill_top_handle( char *buffer );
    void fill_object_handle( char *buffer, int file_number, int file_position );

	void set_token_table( int yyntokens, const char *const *yytname, const short *yytoknum );
	int next_token( void *arg );

	void index_global_symbols( void );
    void override_constant_declaration( char *string );
	void cross_reference_and_evaluate_global_symbols( void );
	void cross_reference_module_prototypes( void );
	void cross_reference_modules( void );
    void check_types_modules( void );
    void evaluate_constants_modules( void );
    void high_level_check_modules( void );

    t_co_union cross_reference_symbol_in_scopes( t_symbol *symbol, t_co_scope *scope ); // Called by numerous submodules - here is the best place

    void build_model( t_sl_option_list env_options );
    int internal_string_from_reference( void *base_handle, const void *item_handle, int item_reference, char *buffer, int buffer_size, t_md_client_string_type type );
    void output_model( t_sl_option_list env_options );

	void print_debug_info( void );

	void set_toplevel_list( class c_co_toplevel *toplevel );

    class c_type_value_pool *type_value_pool;
    class c_sl_error *error;

    t_co_scope *modules; // Module definitions and prototypes; check the type before using each one!
    t_co_scope *type_definitions;

private:
	FILE *f;
	class c_lexical_analyzer *lexical_analyzer;
	class c_co_toplevel *toplevel_list;
    class c_model_descriptor *model;

    struct t_parse_error *parse_errors;
    struct t_parse_error *last_parse_error;
    int number_of_errors;

    int number_of_modules;
    int number_of_global_constants;
    t_co_scope *global_constants;
    int number_of_type_definitions;

};

/*a Functions
 */
/*f find_symbol_in_scope
 */
extern int find_symbol_in_scope( t_co_union *scope, t_symbol *symbol );
extern t_co_scope *create_scope( int number_of_unions );

/*a Wrapper
 */
#endif



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/




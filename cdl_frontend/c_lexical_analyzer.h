/*a Copyright
  
  This file 'c_lexical_analyzer.h' copyright Gavin J Stark 2003, 2004
  
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
#include "lexical_types.h"

/*a Types
 */
/*t	c_lexical_analyzer
 */
class c_lexical_analyzer
{
public:
    c_lexical_analyzer( class c_cyclicity *cyclicity );
    ~c_lexical_analyzer();

    void add_include_directory( char *directory );

    void reset_files( void );
    int set_file( FILE *f );
    int set_file( char *filename );
    int get_number_of_files( void );
    char *get_filename( int file_number );
    struct t_lex_file *get_file_handle( int file_number );
    int get_file_data( int file_number, int *file_size, int *number_lines, char **file_data );
    int get_line_data( int file_number, int line_number, char **line_start, int *line_length );
    int translate_lex_file_posn( t_lex_file_posn *lex_file_posn, int *file_number, int *first_line, int *last_line, int *char_offset );
    int translate_lex_file_posn( t_lex_file_posn *lex_file_posn, int *file_number, int *file_position, int end_not_start );

    class c_cyc_object *find_object_including_char( struct t_lex_file *lex_file, int file_position );

    void set_token_table( int yyntokens, const char *const *yytname, const short *yytoknum );
    int next_token( void *arg );

    int repeat_eof( void ); // Repeat TOKEN_EOF only if at end of file currently - return 1 for success
    t_lex_file_posn get_current_location( void );
    void dump_symbols( int symbol_class );
    struct t_lex_symbol *getsym( char *sym_name, int length );
    int parse_file_text_around( char *buffer, int buffer_size, t_lex_file_posn *lex_file_posn, int include_annotation );

private:
    class c_cyclicity *cyclicity;

    struct t_lex_symbol *sym_table;

    struct t_string_chain *include_directories;
    struct t_string_chain *last_include_directory;

    struct t_lex_file *current_file;
    struct t_lex_file *file_list;

    int yyntokens;
    const char *const *yytname;
    const short *yytoknum;

    struct t_lex_file *allocate_and_read_file( const char *filename, FILE *f, int length );
    struct t_lex_file *include_file( struct t_lex_file *included_by, const char *filename, int filename_length );
    struct t_lex_file *get_nth_file( int n );

    struct t_lex_symbol *putsym( const char *symname, int length, int symtype );
    int file_break_into_tokens( struct t_lex_file *file );
    int file_break_into_tokens_internal( struct t_lex_file *file, int store );
};

/*a Functions
 */
extern const char *lex_string_from_terminal( struct t_string *string );
extern const char *lex_string_from_terminal( struct t_symbol *symbol );
extern void lex_string_from_terminal( struct t_sized_int *sized_int, char *buffer, int buffer_size );
extern void lex_string_from_terminal( class c_cyclicity *cyclicity, t_type_value type_value, char *buffer, int buffer_size );
extern void lex_reset_file_posn( t_lex_file_posn *file_posn );
extern void lex_bound_file_posn( t_lex_file_posn *file_posn, t_lex_file_posn *test_posn, int max_not_min );
extern int lex_char_relative_to_posn( t_lex_file_posn *file_posn, int end_not_start, struct t_lex_file *lex_file, int file_char_position );
extern int get_symbol_type( struct t_lex_symbol *lex_symbol );


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



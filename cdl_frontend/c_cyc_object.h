/*a Copyright
  
  This file 'c_cyc_object.h' copyright Gavin J Stark 2003, 2004
  
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
#ifndef INC_C_CYC_OBJECT
#define INC_C_CYC_OBJECT

/*a Includes
 */
#include <stdarg.h>
#include "lexical_types.h"

/*a Defines
 */
#define CO_MAX_LINK (10)
#define CO_DEBUG(level,format,...) {char ___my_buffer[1024]; \
        int file_number, first_line, last_line, char_offset; \
        cyclicity->translate_lex_file_posn( &(this->start_posn), &file_number, &first_line, &last_line, &char_offset ); \
        snprintf( ___my_buffer, sizeof(___my_buffer), "%s.%d.%d:%s", cyclicity->get_filename( file_number ), first_line+1, char_offset, format ); \
        SL_DEBUG(level,___my_buffer,__VA_ARGS__);}

/*a Types
 */
/*t t_co_type
 */
typedef enum
{
	 co_type_none = 0,
	 co_type_instantiation,
	 co_type_bus_definition,
	 co_type_bus_definition_body_entry,
	 co_type_case_entry,
	 co_type_clock_reset_defn,
	 co_type_cmodel_eq,
	 co_type_code_label,
	 co_type_constant_declaration,
	 co_type_declspec,
	 co_type_enum_definition,
	 co_type_enum_identifier,
	 co_type_expression,
	 co_type_fsm_state,
	 co_type_lvar,
	 co_type_module,
	 co_type_module_body_entry,
	 co_type_module_prototype,
	 co_type_module_prototype_body_entry,
	 co_type_named_expression,
	 co_type_nested_assignment,
	 co_type_port_map,
	 co_type_schematic_symbol,
	 co_type_schematic_symbol_body_entry,
	 co_type_signal_declaration,
	 co_type_sized_int_pair,
	 co_type_statement,
     co_type_timing_spec,
	 co_type_token_union,
	 co_type_toplevel,
	 co_type_type_struct,
	 co_type_type_definition,
	 co_type_type_specifier
} t_co_type;

/*t t_co_compile_stage
 */
typedef enum
{
	 co_compile_stage_none = 0,
	 co_compile_stage_tokenize = 1,
	 co_compile_stage_parse = 2,
	 co_compile_stage_cross_reference = 3,
	 co_compile_stage_evaluate_constants = 4,
	 co_compile_stage_check_types = 5,
	 co_compile_stage_high_level_checks = 6,
} t_co_compile_stage;

/*t t_co_link_type
 */
typedef enum
{
	 co_link_type_object,
	 co_link_type_string,
	 co_link_type_symbol,
	 co_link_type_sized_int
} t_co_link_type;

/*t t_co_link
 */
typedef struct t_co_link
{
	 t_co_compile_stage compile_stage;
	 t_co_link_type link_type;
	 const char *string;
	 union
	 {
		  class c_cyc_object *object;
		  struct t_string *string;
		  struct t_sized_int *sized_int;
		  struct t_symbol *symbol;
	 } data;
} t_co_link;

/*t t_co_union
 */
typedef union
{
    class c_cyc_object *cyc_object;
    class c_co_instantiation *co_instantiation;
    class c_co_bus_definition *co_bus_definition;
    class c_co_bus_definition_body_entry *co_bus_definition_body_entry;
    class c_co_case_entry *co_case_entry;
    class c_co_clock_reset_defn *co_clock_reset_defn;
    class c_co_cmodel_eq *co_cmodel_eq;
    class c_co_code_label *co_code_label;
    class c_co_constant_declaration *co_constant_declaration;
    class c_co_declspec *co_declspec;
    class c_co_enum_definition *co_enum_definition;
    class c_co_enum_identifier *co_enum_identifier;
    class c_co_expression *co_expression;
    class c_co_fsm_state *co_fsm_state;
    class c_co_lvar *co_lvar;
    class c_co_module_prototype *co_module_prototype;
    class c_co_module_prototype_body_entry *co_module_prototype_body_entry;
    class c_co_module *co_module;
    class c_co_module_body_entry *co_module_body_entry;
    class c_co_port_map *co_port_map;
    class c_co_schematic_symbol *co_schematic_symbol;
    class c_co_schematic_symbol_body_entry *co_schematic_symbol_body_entry;
    class c_co_signal_declaration *co_signal_declaration;
    class c_co_sized_int_pair *co_sized_int_pair;
    class c_co_statement *co_statement;
    class c_co_timing_spec *co_timing_spec;
    class c_co_token_union *co_token_union;
    class c_co_toplevel *co_toplevel;
    class c_co_type_definition *co_type_definition;
    class c_co_type_specifier *co_type_specifier;
} t_co_union;

/*t t_co_scope
 */
typedef struct t_co_scope
{
     struct t_co_scope *next_scope;
     int length;
     t_co_union co[1];
} t_co_scope;

/*t t_co_string_pair
 */
typedef struct t_co_string_pair
{
    struct t_co_string_pair *next_string_pair;
    char *first;
    char *second;
} t_co_string_pair;

/*t c_cyc_object
 */
class c_cyc_object
{
public:
	 c_cyc_object();
	 ~c_cyc_object();

	 void co_init( t_co_type type, const char *string );
     void co_set_file_bound( c_cyc_object *start_object, c_cyc_object *end_object );
     void co_set_file_bound( c_cyc_object *start_object, t_lex_file_posn end_lex_file_posn );
     void co_set_file_bound( t_lex_file_posn start_lex_file_posn, c_cyc_object *end_object );
     void co_set_file_bound( t_lex_file_posn start_lex_file_posn, t_lex_file_posn end_lex_file_posn );
	 void co_link( t_co_compile_stage compile_stage, c_cyc_object *object, const char *string );
	 void co_link( t_co_compile_stage compile_stage, struct t_symbol *symbol, const char *string );
	 void co_link( t_co_compile_stage compile_stage, struct t_string *tstring, const char *string );
	 void co_link( t_co_compile_stage compile_stage, struct t_sized_int *sized_int, const char *string );
     void co_link_symbol_list( t_co_compile_stage compile_stage, ... ) ;
	 void display_links( int indent );
	 int get_number_of_links( void );
     int get_data( const char **co_textual_type, int *co_type, char *handle, char *parent_handle, t_lex_file_posn **start, t_lex_file_posn **end );
     int get_link( int link, const char **text, char *handle, const char **link_text, int *stage, int *type );
	 void get_handle( char *buffer ); // buffer must be at least 16 characters long
     t_lex_file_posn get_lex_file_posn( int end_not_start );
     c_cyc_object *find_object_from_file_posn( struct t_lex_file *lex_file, int file_char_position );
     int check_seed( int seed );

	 void (*delete_fn)( c_cyc_object *object );
	 const char *co_textual_type;
	 t_co_type co_type;
	 c_cyc_object *next_in_list;		// Ownership link
	 c_cyc_object *last_in_list;
     c_cyc_object *parent;

     // Remaining stuff is private
	 int co_next_link;
	 t_co_link co_links[CO_MAX_LINK];
     int seed;
     t_lex_file_posn start_posn;
     t_lex_file_posn end_posn;
};

/*a Function calls
 */
extern class c_cyc_object *object_from_handle( char *buffer ) ;

/*a Wrapper end
 */
#endif



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/




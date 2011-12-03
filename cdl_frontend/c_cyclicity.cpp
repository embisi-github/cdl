/*a Copyright
  
  This file 'c_cyclicity.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sl_debug.h"
#include "sl_option.h"
#include "c_model_descriptor.h"
#include "c_sl_error.h"
#include "c_type_value_pool.h"
#include "c_cyclicity.h"
#include "c_lexical_analyzer.h"
#include "c_co_toplevel.h"
#include "c_co_type_definition.h"
#include "c_co_enum_definition.h"
#include "c_co_fsm_state.h"
#include "c_co_enum_identifier.h"
#include "c_co_constant_declaration.h"
#include "c_co_type_specifier.h"
#include "c_co_signal_declaration.h"
#include "c_co_expression.h"
#include "c_co_statement.h"
#include "c_co_module.h"
#include "c_co_module_prototype.h"
extern void cyclicity_grammar_init( c_cyclicity *cyc );
extern int cyclicity_parse( c_cyclicity *grammar );

/*a Defines
 */

/*a Types
 */

/*t t_parse_error
 */
typedef struct t_parse_error
{
    struct t_parse_error *next;
    t_lex_file_posn lex_file_posn;
    t_co_compile_stage co_compile_stage;
    char string[128];
} t_parse_error;

/*a Statics
 */

/*a Static debug functions
 */
/*f debug_output
 */
static const char *indent_string = "    ";
static void debug_output( void *handle, int indent, const char *format, ... )
{
    FILE *f;
    va_list ap;
    int i;

    va_start( ap, format );

    f = (FILE *)handle;
    if (indent>=0)
    {
        for (i=0; i<indent; i++)
            fputs( indent_string, f );
    }
    vfprintf( f, format, ap );
    fflush(f);
    va_end( ap );
}

/*a Constructors and destructors
 */
/*f c_cyclicity::c_cyclicity
 */
c_cyclicity::c_cyclicity( void )
{
    lexical_analyzer = new c_lexical_analyzer( this );
    toplevel_list = NULL;
    cyclicity_grammar_init( this );
    error = new c_sl_error( sizeof(int), // private_size
                            8 ); // max_args to add error
    model = NULL;
    parse_errors = NULL;
    last_parse_error = NULL;
    number_of_errors = 0;

    modules = NULL;
    global_constants = NULL;
    type_definitions = NULL;
    type_remappings = NULL;

    type_value_pool = new c_type_value_pool( this );
}

/*f c_cyclicity::~c_cyclicity
 */
c_cyclicity::~c_cyclicity()
{
    t_parse_error *parse_error, *next_parse_error;

    //printf("Deleting c_cyclicity %p\n", this );
    if (type_value_pool)
        delete (type_value_pool);
    if (lexical_analyzer)
        delete (lexical_analyzer);

    for (parse_error=parse_errors; parse_error; parse_error=next_parse_error)
    {
        //printf("Deleting parse error %p\n", parse_error);
        next_parse_error = parse_error->next;
        free(parse_error);
    }

    if (toplevel_list)
    {
        //printf("Deleting toplevel list %p\n", toplevel_list);
        delete(toplevel_list);
    }
}

/*a Lexical analysis functions
 */
/*f c_cyclicity::add_include_directory
 */
void c_cyclicity::add_include_directory( char *directory )
{
    lexical_analyzer->add_include_directory( directory );
}

/*f c_cyclicity::get_number_of_files
 */
int c_cyclicity::get_number_of_files( void )
{
    return lexical_analyzer->get_number_of_files();
}

/*f c_cyclicity::get_filename
 */
char *c_cyclicity::get_filename( int file_number )
{
    return lexical_analyzer->get_filename( file_number );
}

/*f c_cyclicity::get_file_data
 */
int c_cyclicity::get_file_data( int file_number, int *file_size, int *number_lines, char **file_data )
{
    return lexical_analyzer->get_file_data( file_number, file_size, number_lines, file_data );
}

/*f c_cyclicity::get_line_data
 */
int c_cyclicity::get_line_data( int file_number, int line_number, char **line_start, int *line_length )
{
    return lexical_analyzer->get_line_data( file_number, line_number, line_start, line_length );
}

/*f c_cyclicity::replace_lex_symbol
 */
void c_cyclicity::replace_lex_symbol( t_symbol *symbol, const char *text, int text_length )
{
    lexical_analyzer->replacesym( symbol, text, text_length );
}

/*f c_cyclicity::translate_lex_file_posn
 */
int c_cyclicity::translate_lex_file_posn( t_lex_file_posn *lex_file_posn, int *file_number, int *first_line, int *last_line, int *char_offset )
{
    return lexical_analyzer->translate_lex_file_posn( lex_file_posn, file_number, first_line, last_line, char_offset );
}

/*f c_cyclicity::translate_lex_file_posn
 */
int c_cyclicity::translate_lex_file_posn( t_lex_file_posn *lex_file_posn, int *file_number, int *file_position, int end_not_start )
{
    return lexical_analyzer->translate_lex_file_posn( lex_file_posn, file_number, file_position, end_not_start );
}

/*a Object functions
 */
/*f c_cyclicity::fill_top_handle
 */
void c_cyclicity::fill_top_handle( char *buffer )
{
    if (toplevel_list)
        toplevel_list->get_handle( buffer );
    else
        buffer[0] = 0;
}

/*f c_cyclicity::fill_object_handle
 */
void c_cyclicity::fill_object_handle( char *buffer, int file_number, int file_position )
{
    struct t_lex_file *lex_file;
    c_cyc_object *co;
    lex_file = lexical_analyzer->get_file_handle( file_number );
    if (!lex_file)
    {
        buffer[0] = 0;
        return;
    }
    co = lexical_analyzer->find_object_including_char( lex_file, file_position );
    if (co)
        co->get_handle( buffer );
    else
        buffer[0] = 0;
}

/*a Parsing functions
 */
/*f c_cyclicity::parse_input_file
 */
int c_cyclicity::parse_input_file( FILE *f )
{
    SL_DEBUG( sl_debug_level_info, "Parse file (stream) %p", f );
    if (!lexical_analyzer->set_file( f ))
    {
        set_parse_error( co_compile_stage_tokenize, "Failed to open file" );
        return number_of_errors;
    }
    cyclicity_parse( this );
    return number_of_errors;
}

/*f c_cyclicity::parse_input_file
 */
int c_cyclicity::parse_input_file( char *filename )
{
    SL_DEBUG( sl_debug_level_info, "Parse filename %s", filename );
    if (!lexical_analyzer->set_file( filename ))
    {
        set_parse_error( co_compile_stage_tokenize, "Failed to open file '%s'", filename );
        return number_of_errors;
    }
    cyclicity_parse( this );
    return number_of_errors;
}

/*f c_cyclicity::repeat_eof
 */
int c_cyclicity::parse_repeat_eof( void )
{
    return lexical_analyzer->repeat_eof();
}

/*f c_cyclicity::parse_get_current_location
 */
t_lex_file_posn c_cyclicity::parse_get_current_location( void )
{
    return lexical_analyzer->get_current_location();
}

/*f c_cyclicity::set_parse_error_va_list ( lex_file_posn, co_compile_stage, format, va_arg )
 */
void c_cyclicity::set_parse_error_va_list( t_lex_file_posn lex_file_posn, t_co_compile_stage co_compile_stage, const char *format, va_list ap)
{
    t_parse_error *parse_error;

    parse_error = (t_parse_error *)malloc(sizeof(t_parse_error));
    if (parse_error)
    {
        vsnprintf( parse_error->string, sizeof(parse_error->string), format, ap );
        if (!parse_errors)
        {
            parse_errors = parse_error;
        }
        else
        {
            last_parse_error->next = parse_error;
        }
        parse_error->next = NULL;
        last_parse_error = parse_error;
        parse_error->co_compile_stage = co_compile_stage;
        parse_error->lex_file_posn = lex_file_posn;
    }
    number_of_errors++;
}

/*f c_cyclicity::set_parse_error ( lex_file_posn, co_compile_stage, format, ... )
 */
void c_cyclicity::set_parse_error( t_lex_file_posn lex_file_posn, t_co_compile_stage co_compile_stage, const char *format, ... )
{
    va_list ap;

    va_start( ap, format );

    set_parse_error_va_list( lex_file_posn, co_compile_stage, format, ap );

    va_end (ap );
}

/*f c_cyclicity::set_parse_error ( cyc_object, co_compile_stage, format, ... )
 */
void c_cyclicity::set_parse_error( c_cyc_object *cyc_object, t_co_compile_stage co_compile_stage, const char *format, ... )
{
    va_list ap;

    va_start( ap, format );

    set_parse_error_va_list( cyc_object->get_lex_file_posn(0), co_compile_stage, format, ap );

    va_end (ap );
}

/*f c_cyclicity::set_parse_error ( co_compile_stage, format, ... )
 */
void c_cyclicity::set_parse_error( t_co_compile_stage co_compile_stage, const char *format, ... )
{
    va_list ap;

    va_start( ap, format );

    set_parse_error_va_list( lexical_analyzer->get_current_location(), co_compile_stage, format, ap );

    va_end( ap );
}

/*f c_cyclicity::get_number_of_parse_errors
 */
int c_cyclicity::get_number_of_parse_errors( void )
{
    return number_of_errors;
}

/*f c_cyclicity::read_parse_error
 */
int c_cyclicity::read_parse_error( int error_number, t_lex_file_posn *lex_file_posn, char **buffer )
{
    t_parse_error *parse_error;

    for (parse_error=parse_errors; (error_number>0) && (parse_error); parse_error=parse_error->next, error_number--);
    if (parse_error)
    {
        *lex_file_posn = parse_error->lex_file_posn;
        *buffer = parse_error->string;
        return 1;
    }
    return 0;
}

/*f c_cyclicity::parse_file_text_around
 // Generate string with file text around the file posn, possibly annotated with a pointer
 */
int c_cyclicity::parse_file_text_around( char *buffer, int buffer_size, t_lex_file_posn *lex_file_posn, int include_annotation )
{
    return lexical_analyzer->parse_file_text_around( buffer, buffer_size, lex_file_posn, include_annotation );
}

/*f c_cyclicity::set_token_table
 */
void c_cyclicity::set_token_table( int yyntokens, const char *const *yytname, const short *yytoknum )
{
    lexical_analyzer->set_token_table( yyntokens, yytname, yytoknum );
}

/*f c_cyclicity::next_token
 */
int c_cyclicity::next_token( void *arg )
{
    return lexical_analyzer->next_token( arg );
}

/*f c_cyclicity::override_type_mapping
  Override a type specifier symbol from one to another
 */
void c_cyclicity::override_type_mapping( char *string )
{
    SL_DEBUG( sl_debug_level_info, "Overriding type with string '%s'", string );
    char *eq = strchr(string,'=');
    if (!eq)
    {
        set_parse_error( co_compile_stage_tokenize, "Bad type remap override '%s' (should be generic_type=specific_type)", string );
    }
    add_string_pair_to_list( &type_remappings, string, eq-string, eq+1, strlen(eq+1) );
}

/*f c_cyclicity::remap_type_specifier_symbol
 */
void c_cyclicity::remap_type_specifier_symbol( t_symbol *symbol )
{
    const char *type_name = lex_string_from_terminal(symbol);
    int type_name_len = strlen(type_name);
    const char *specific_type_name = find_string_from_string_pair_list( type_remappings, type_name, type_name_len );
    if (specific_type_name)
    {
        replace_lex_symbol( symbol, specific_type_name, strlen(specific_type_name) );
    }
}

/*a Cross-referencing and checking functions
  The cross referencing is the key to cyclicity frontend.

  The input to the cross referencing process is the parse tree.

  The first stage is to index all the global symbols, i.e. those in the topmost scope
  Then the global constants can be evaluated - this includes sizes of types, evaluated in lexical order
  Then constants can be overriden by externals
  Then the modules can have their signals indexed, and be cross referenced with types resolved
  Then the modules can be type checked
  Then the modules can have constants evaluated
  Then the modules can have a high level check performed

  The output from the cross referencing process is the parse tree fully cross referenced, which can be mechanically built with a backend
*/
/*f find_symbol_in_scope
 */
extern int find_symbol_in_scope( t_co_union *scope, t_symbol *symbol )
{
    int i;
    if ((!symbol) || (!scope))
        return -1;
    for (i=0; scope[i].cyc_object; i++)
    {
        switch (scope[i].cyc_object->co_type)
        {
        case co_type_type_definition:
            if (symbol->lex_symbol == scope[i].co_type_definition->symbol->lex_symbol)
            {
                return i;
            }
            break;
        case co_type_signal_declaration:
            if (symbol->lex_symbol == scope[i].co_signal_declaration->symbol->lex_symbol)
            {
                return i;
            }
            break;
        case co_type_constant_declaration:
            if (symbol->lex_symbol == scope[i].co_constant_declaration->symbol->lex_symbol)
            {
                return i;
            }
            break;
        case co_type_enum_identifier:
            if (symbol->lex_symbol == scope[i].co_enum_identifier->symbol->lex_symbol)
            {
                return i;
            }
            break;
        case co_type_fsm_state:
            if (symbol->lex_symbol == scope[i].co_fsm_state->symbol->lex_symbol)
            {
                return i;
            }
            break;
        case co_type_statement:
            if ( (scope[i].co_statement->statement_type==statement_type_for_statement) &&
                 (symbol->lex_symbol == scope[i].co_statement->type_data.for_stmt.symbol->lex_symbol) )
            {
                return i;
            }
            break;
        default:
            break;
        }
    }
    return -1;
}

/*f c_cyclicity::index_global_symbols

global level userids come from:
constants
typedefed enums
typedefed fsm_states
typedefed types

so we need to generate arrays of:
all constants/enums/fsm_state numbers (global constants)
all types (type definitions)
all module prototypes and definitions (which may be searched for instantiation)
all bus definitions (which may be searched for instantiation)

Count them, then allocate null-terminatable arrays, then set them
*/
void c_cyclicity::index_global_symbols( void )
{
    c_co_toplevel *cot;
    c_co_fsm_state *cofs;
    c_co_enum_identifier *coei;
    int pass;

    SL_DEBUG( sl_debug_level_info, "" );

    /*b Two passes - count modules, constants, type_definitions, and allocate/store
     */
    for (pass=1; pass<=2; pass++)
    {
        number_of_modules = 0;
        number_of_global_constants = 0;
        number_of_type_definitions = 0;
        for (cot = toplevel_list; cot; cot=(c_co_toplevel *)cot->next_in_list)
        {
            switch (cot->toplevel_type)
            {
            case toplevel_type_module_prototype:
                if (pass==2)
                    modules->co[ number_of_modules ].co_module_prototype = cot->data.module_prototype;
                number_of_modules++;
                break;
            case toplevel_type_module:
                if (pass==2)
                    modules->co[ number_of_modules ].co_module = cot->data.module;
                number_of_modules++;
                break;
            case toplevel_type_constant_declaration:
                if (cot->data.constant_declaration)
                {
                    if (pass==2)
                    {
                        if (find_symbol_in_scope( global_constants->co, cot->data.constant_declaration->symbol )>=0)
                        {
                            set_parse_error( cot, co_compile_stage_cross_reference, "Duplicate global-level symbol '%s' ", lex_string_from_terminal( cot->data.constant_declaration->symbol));
                        }
                        else
                        {
                            global_constants->co[ number_of_global_constants ].co_constant_declaration = cot->data.constant_declaration;
                            number_of_global_constants++;
                        }
                    }
                    else
                    {
                        number_of_global_constants++;
                    }
                }
                break;
            case toplevel_type_type_definition:
                if (pass==2)
                {
                    if (find_symbol_in_scope( type_definitions->co, cot->data.type_definition->symbol )>=0)
                    {
                        set_parse_error( cot, co_compile_stage_cross_reference, "Redefinition of type '%s'", lex_string_from_terminal( cot->data.type_definition->symbol ) );
                    }
                    type_definitions->co[number_of_type_definitions].co_type_definition = cot->data.type_definition;
                }
                number_of_type_definitions++;
                if (cot->data.type_definition->enum_definition)
                {
                    for (coei = cot->data.type_definition->enum_definition->enum_identifier; coei; coei=(c_co_enum_identifier *)coei->next_in_list)
                    {
                        if (coei->symbol)
                        {
                            if (pass==2)
                            {
                                if (find_symbol_in_scope( global_constants->co, coei->symbol)>=0)
                                {
                                    set_parse_error( coei, co_compile_stage_cross_reference, "Duplicate global-level symbol '%s' ", lex_string_from_terminal( coei->symbol));
                                }
                                else
                                {
                                    global_constants->co[ number_of_global_constants ].co_enum_identifier = coei;
                                    number_of_global_constants++;
                                }
                            }
                            else
                            {
                                number_of_global_constants++;
                            }
                        }
                    }
                }
                if (cot->data.type_definition->fsm_state)
                {
                    for (cofs = cot->data.type_definition->fsm_state; cofs; cofs=(c_co_fsm_state *)cofs->next_in_list)
                    {
                        if (cofs->symbol)
                        {
                            if (pass==2)
                            {
                                if (find_symbol_in_scope( global_constants->co, cofs->symbol)>=0)
                                {
                                    set_parse_error( cofs, co_compile_stage_cross_reference, "Duplicate global-level symbol '%s' ", lex_string_from_terminal( cofs->symbol));
                                }
                                else
                                {
                                    global_constants->co[ number_of_global_constants ].co_fsm_state = cofs;
                                    number_of_global_constants++;
                                }
                            }
                            else
                            {
                                number_of_global_constants++;
                            }
                        }
                    }
                }
                break;
            default:
                break;
            }
        }
        if (pass==1)
        {
            modules          = create_scope( number_of_modules );
            global_constants = create_scope( number_of_global_constants );
            type_definitions = create_scope( number_of_type_definitions );
        }
    }
    modules->co[number_of_modules].cyc_object = NULL;
    global_constants->co[number_of_global_constants].co_constant_declaration = NULL;
    type_definitions->co[number_of_type_definitions].co_type_definition = NULL;
}

/*f c_cyclicity::override_constant_declaration
 */
void c_cyclicity::override_constant_declaration( char *string )
{
    int i, j, k;
    struct t_lex_symbol *lex_symbol;
    t_type_value type_value;

    SL_DEBUG( sl_debug_level_info, "Overriding constant with string '%s'", string );
    for (i=0; (string[i]!=0) && (string[i]!='='); i++);
    if (string[i])
    {
        k = i+1;
        j = type_value_pool->new_type_value_integer_or_bits( string, &k, &type_value );
        if (j!=0)
        {
            SL_DEBUG( sl_debug_level_verbose_info, "Overriding failed, but sized integer (%d)", j );
            set_parse_error( (t_lex_file_posn)NULL, co_compile_stage_evaluate_constants, "Bad sized integer value in overriding constant '%s'", string );
            return;
        }
    }
    else
    {
        type_value = type_value_pool->new_type_value_integer( 1 );
    }
    lex_symbol = lexical_analyzer->getsym( string, i );

    for (i=0; global_constants->co[i].cyc_object; i++)
    {
        switch (global_constants->co[i].cyc_object->co_type)
        {
        case co_type_constant_declaration:
            if (lex_symbol == global_constants->co[i].co_constant_declaration->symbol->lex_symbol)
            {
                SL_DEBUG( sl_debug_level_verbose_info, "Overriding successful", string );
                global_constants->co[i].co_constant_declaration->override( type_value );
                return;
            }
            break;
        default:
            break;
        }
    }
    SL_DEBUG( sl_debug_level_info, "Overriding failed: could not match to a constant", string );
    set_parse_error( (t_lex_file_posn)NULL, co_compile_stage_evaluate_constants, "Could not match symbol to a global constant in overriding constant '%s'", string );
}

/*f c_cyclicity::cross_reference_and_evaluate_global_symbols
for each constant:
types mapped to the type pool
expressions are mapped to their user ids, global level only
evaluate its value if not overridden

for each enum_identifier:
expressions are mapped to their user ids, global level only
evaluate its value - if none, previous in list+1 else 0

for each fsm_state:
expressions are mapped to their user ids, global level only
assign a value appropriate to style

for each type:
index expressions are mapped to their user ids, global level only
evaluate expressions
add to type pool
*/
void c_cyclicity::cross_reference_and_evaluate_global_symbols( void )
{
    c_co_toplevel *cot;
    c_co_fsm_state *cofs;
    c_co_enum_identifier *coei;
    int next_global_constant, next_type_definition;

    SL_DEBUG( sl_debug_level_info, "" );

    /*b Evaluate constants, type_definitions indices, fsm_states, enum_identifiers
     */
    next_global_constant = 0;
    next_type_definition = 0;
    for (cot = toplevel_list; cot; cot=(c_co_toplevel *)cot->next_in_list)
    {
        switch (cot->toplevel_type)
        {
        case toplevel_type_constant_declaration:
            if (cot->data.constant_declaration == global_constants->co[ next_global_constant ].co_constant_declaration)
            {
                cot->data.constant_declaration->cross_reference( this, type_definitions, global_constants );
                cot->data.constant_declaration->evaluate( this, type_definitions, global_constants );
                next_global_constant++;
            }
            break;
        case toplevel_type_type_definition:
            if (cot->data.type_definition == type_definitions->co[ next_type_definition ].co_type_definition)
            {
                cot->data.type_definition->cross_reference( this, type_definitions, global_constants );
                cot->data.type_definition->evaluate( this, type_definitions, global_constants );
                next_type_definition++;
            }
            if ( (cot->data.type_definition) && 
                 (cot->data.type_definition->enum_definition) )
            {
                for (coei = cot->data.type_definition->enum_definition->enum_identifier; coei; coei=(c_co_enum_identifier *)coei->next_in_list)
                {
                    if (coei == global_constants->co[ next_global_constant ].co_enum_identifier)
                    {
                        next_global_constant++;
                    }
                }
            }
            if ( (cot->data.type_definition) && 
                 (cot->data.type_definition->fsm_state) )
            {
                for (cofs = cot->data.type_definition->fsm_state; cofs; cofs=(c_co_fsm_state *)cofs->next_in_list)
                {
                    if (cofs == global_constants->co[ next_global_constant ].co_fsm_state )
                    {
                        next_global_constant++;
                    }
                }
            }
            break;
        default:
            break;
        }
    }
}

/*f c_cyclicity::cross_reference_module_prototypes
 */
void c_cyclicity::cross_reference_module_prototypes( void )
{
    int i;

    SL_DEBUG( sl_debug_level_info, "" );

    /*b Now cross reference the module prototype contents
      Cross reference types of ports and local signals
      Cross reference signals and constant usages within the module bodies with ports, local signals, and global constants
    */
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module_prototype)
        {
            modules->co[i].co_module_prototype->cross_reference_pre_type_resolution( this, modules, type_definitions, global_constants );
        }
    }
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module_prototype)
        {
            modules->co[i].co_module_prototype->resolve_types( this, modules, type_definitions );
        }
    }
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module_prototype)
        {
//            modules->co[i].co_module_prototype->cross_reference_post_type_resolution( this, modules, type_definitions ); - might be needed for content of the module, i.e. timings
        }
    }
}

/*f c_cyclicity::cross_reference_modules
 */
void c_cyclicity::cross_reference_modules( void )
{
    int i;

    SL_DEBUG( sl_debug_level_info, "" );

    /*b Index local signals
     */
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module)
        {
            modules->co[i].co_module->index_local_signals( this, global_constants );
        }
    }

    /*b Now cross reference the module contents
      Cross reference bus and module instantiations within module definitions
      Build local signal declaration lists within modules
      Cross reference types of ports and local signals
      Cross reference signals and constant usages within the module bodies with ports, local signals, and global constants
    */
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module)
        {
            modules->co[i].co_module->cross_reference_pre_type_resolution( this, modules, type_definitions );
        }
    }
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module)
        {
            modules->co[i].co_module->resolve_types( this, modules, type_definitions );
        }
    }
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module)
        {
            modules->co[i].co_module->cross_reference_post_type_resolution( this, modules, type_definitions );
        }
    }
}

/*f c_cyclicity::check_types_modules
 */
void c_cyclicity::check_types_modules( void )
{
    int i;

    SL_DEBUG( sl_debug_level_info, "" );


    /*b Check types everywhere in the modules
     */
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module)
        {
            modules->co[i].co_module->check_types( this, modules, type_definitions );
        }
    }
}

/*f c_cyclicity::evaluate_constants_modules
 */
void c_cyclicity::evaluate_constants_modules( void )
{
    int i;

    SL_DEBUG( sl_debug_level_info, "" );

    /*b Evaluate constants throughout modules
     */
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module)
        {
            modules->co[i].co_module->evaluate_constant_expressions( this, type_definitions, 0 );
        }
    }
}

/*f c_cyclicity::high_level_check_modules
 */
void c_cyclicity::high_level_check_modules( void )
{
    int i;

    SL_DEBUG( sl_debug_level_info, "" );

    /*b Do high level checks throughout modules
     */
    for (i=0; i<number_of_modules; i++)
    {
        if (modules->co[i].cyc_object->co_type==co_type_module)
        {
            modules->co[i].co_module->high_level_checks( this, modules, type_definitions );
        }
    }
}

/*a Backend interaction functions
 */
/*f string_from_reference
  Our references are module, object, subitem (probably 0)
 */
static int string_from_reference( void *handle, void *base_handle, const void *item_handle, int item_reference, char *buffer, int buffer_size, t_md_client_string_type type )
{
    c_cyclicity *cyclicity;
    cyclicity = (c_cyclicity *)handle;
    return cyclicity->internal_string_from_reference( base_handle, item_handle, item_reference, buffer, buffer_size, type );
}


/*f c_cyclicity::internal_string_from_reference
  Our references are module, object, subitem (probably 0)
 */
int c_cyclicity::internal_string_from_reference( void *base_handle, const void *item_handle, int item_reference, char *buffer, int buffer_size, t_md_client_string_type type )
{
    switch (type)
    {
    case md_client_string_type_coverage:
        if (item_handle) // item_handle should be a c_cyc_object of some form;
        {
            c_cyc_object *co;
            t_lex_file_posn file_posn;
            int file_number;
            int first_line, last_line, char_offset;
            co = (c_cyc_object *)item_handle;
            file_posn = co->get_lex_file_posn( 0 );
            lexical_analyzer->translate_lex_file_posn( &file_posn, &file_number, &first_line, &last_line, &char_offset );
            snprintf( buffer, buffer_size, "%s %d %d", lexical_analyzer->get_filename( file_number ), first_line+1, char_offset );
        }
        else
        {
            snprintf( buffer, buffer_size, "%p %p %d", base_handle, item_handle, item_reference );
        }
        return 1;
    case md_client_string_type_human_readable:
        if (item_handle) // item_handle should be a c_cyc_object of some form;
        {
            c_cyc_object *co;
            t_lex_file_posn file_posn;
            int file_number;
            int first_line, last_line, char_offset;
            co = (c_cyc_object *)item_handle;
            file_posn = co->get_lex_file_posn( 0 );
            lexical_analyzer->translate_lex_file_posn( &file_posn, &file_number, &first_line, &last_line, &char_offset );
            snprintf( buffer, buffer_size, "%s:%d", lexical_analyzer->get_filename( file_number ), first_line+1 );
        }
        else
        {
            snprintf( buffer, buffer_size, "??%p??:??%p??:??%d??", base_handle, item_handle, item_reference );
        }
        return 1;
    }
    return 0;
}

/*f c_cyclicity::build_model( t_sl_option_list env_options )
 */
void c_cyclicity::build_model( t_sl_option_list env_options )
{
    int i;
    c_co_module *com;

    SL_DEBUG( sl_debug_level_info, "" );

    /*b Delete any backend model that may exist
     */
    if (model)
        delete(model);

    /*b Create a new backend model
     */
    SL_DEBUG( sl_debug_level_info, "Create backend model" );
    model = new c_model_descriptor( env_options, error, string_from_reference, (void *)this );

    /*b First create a backend type definition from each internal type
     */
    SL_DEBUG( sl_debug_level_info, "Build type definitions" );
    type_value_pool->build_type_definitions( model );

    /*b Now build each module with the backend
     */
    for (i=0; i<number_of_modules; i++)
    {
        SL_DEBUG( sl_debug_level_info, "Build module prototype %d", i );
        if (modules->co[i].cyc_object->co_type==co_type_module_prototype)
        {
            modules->co[i].co_module_prototype->build_model( this, model );
        }
    }
    for (i=0; i<number_of_modules; i++)
    {
        SL_DEBUG( sl_debug_level_info, "Build module %d", i );
        if (modules->co[i].cyc_object->co_type==co_type_module)
        {
            modules->co[i].co_module->build_model( this, model );
        }
    }

    if (error->check_errors_and_reset( stderr, error_level_info, error_level_serious ))
        exit(4);

    /*b Analyze
     */
    SL_DEBUG( sl_debug_level_info, "Analyze" );
    model->analyze();
    if (error->check_errors_and_reset( stderr, error_level_info, error_level_serious ))
    {
        exit(4);
    }

    /* Debug display
     */
    if (SL_DEBUG_TEST( sl_debug_level_info ))
    {
        for (i=0; i<number_of_modules; i++)
        {
            if (modules->co[i].cyc_object->co_type==co_type_module_prototype)
            {
                //module_analyze( modules->co[i].co_module_prototype->md_module_prototype );
                model->module_display_references( modules->co[i].co_module_prototype->md_module_prototype, debug_output, (void *)stdout );
            }
        }
        for (i=0; i<number_of_modules; i++)
        {
            if (modules->co[i].cyc_object->co_type==co_type_module)
            {
                com = modules->co[i].co_module;
                //model->type_definition_display( debug_output, (void *)stdout );
                //model->module_display_instances( com->md_module, debug_output, (void *)stdout );
                //model->module_analyze( com->md_module );
                model->module_display_references( com->md_module, debug_output, (void *)stdout );
            }
        }
    }
}

/*f c_cyclicity::output_model( t_sl_option_list env_options )
 */
void c_cyclicity::output_model( t_sl_option_list env_options )
{
    if (model)
    {
        model->generate_output( env_options );
    }
}

/*a Unsorted
 */
/*f c_cyclicity::set_toplevel( c_co_toplevel )
 */
void c_cyclicity::set_toplevel_list( class c_co_toplevel *toplevel )
{
    this->toplevel_list = toplevel;
}

/*f c_cyclicity::print_debug_info
 */
void c_cyclicity::print_debug_info( void )
{
    toplevel_list->display_links( 0 );
}

/*a Cross referencing support methods
 */
/*f add_string_pair_to_list
 */
void add_string_pair_to_list( t_co_string_pair **list_ptr, const char *first, int first_length, const char *second, int second_length )
{
    t_co_string_pair *ptr;
    ptr = (t_co_string_pair *)malloc(sizeof(t_co_string_pair) + first_length + second_length + 4 );
    ptr->first  = ((char *)(ptr))+sizeof(t_co_string_pair);
    ptr->second = ptr->first+first_length+1;
    strncpy( ptr->first, first, first_length );
    ptr->first[first_length] = 0;
    strncpy( ptr->second, second, second_length );
    ptr->second[second_length] = 0;

    ptr->next_string_pair = *list_ptr;
    *list_ptr = ptr;
}

/*f find_string_from_string_pair_list
 */
char *find_string_from_string_pair_list( t_co_string_pair *list, const char *first, int first_length )
{
    t_co_string_pair *ptr;
    for (ptr=list; ptr; ptr=ptr->next_string_pair )
    {
        if (strncmp(first,ptr->first,first_length)==0)
        {
            if (first_length==(int)strlen(ptr->first))
            {
                return ptr->second;
            }
        }
    }
    return NULL;
}

/*f create_scope
 */
t_co_scope *create_scope( int number_of_unions )
{
    t_co_scope *ptr;
    int i;

    ptr = (t_co_scope *)malloc(sizeof(t_co_scope) + sizeof(t_co_union) * (number_of_unions+1));
    ptr->next_scope = NULL;
    for (i=0; i<=number_of_unions+1; i++)
    {
        ptr->co[i].cyc_object = NULL;
    }
    return ptr;
}

/*f c_cyclicity::cross_reference_symbol_in_scopes
 */
t_co_union c_cyclicity::cross_reference_symbol_in_scopes( t_symbol *symbol, t_co_scope *scope )
{
    int i;
    t_co_union none;

    for (; scope; scope=scope->next_scope )
    {
        i = find_symbol_in_scope( scope->co, symbol );
        if (i>=0)
            return scope->co[i];
    }
    none.cyc_object = NULL;
    return none;
}

/*a To do
  use c_sl_error rather than the internal error functions

  make nested_assignments use lvars not symbols

  c_type_value_pool
  Tidy up the whole thing, documentation would help

  c_co_fsm_state::evaluate
  check for duplicated values (by casting to integer and doing a search)

  c_co_expression::internal_evaluate
  finish sizeof() - need to correctly push type symbols and use them correctly too (check state)

  add support for 'dD' in reading sized values
  add support for signed and unsigned bit vectors
  add support for enumerations to not specify the bit width
  change type checking and scoping to correctly support the language
    remove integer type
    put in proper unsized bit vector type
    change expression evaluation to not have integer functions any more, but to have scope for sized and unsized functions
  check prototypes match actual definitions of modules
    types of inputs are identical
    combinatorial paths are in fact combinatorial (or non-existent)
    clocked paths are in fact clocked (or non-existent)
  add timing to timing specs for prototypes
  add 'layers of logic' estimation and warnings for too much logic
  reset values should be checked for run-time constants at high-level in c_cyclicity and in c_model_descriptor.cpp
  inferred latches should be errored
  unassigned state should be warned about
  used state that is not assigned should be errored
  assignments to combinatorials/clocked should all be in the same code block
  indexing of arrays is not checked, or built into modules
  Need to add 'masked bit vector' type
  masked bit vectors need a new type, and need to be reimplemented
  priority statements are not checked - in fsm_machine.cdl
  Should output documentation as comments
  Should output synthesis assists as comments (full/parallel switch etc)
  Possibly need to go back to unsized values (max size 32-bits) for integers for backend
  Add sequences for assertions
    In model...
      model_sequence_create( )
      model_sequence_add_expression( sequence, expression )
      model_sequence_add_delay( sequence, min_delay, max_delay )
      model_sequence_add_op( sequence, type (not, and, or, xor) sub_sequence_0, sub_sequence_1 or NULL )
      model_assert_create ( clock edge, reset expression, sequence, message ) // performed at end of preclock
    Syntax:
      assert_sequence( posnegedge clock, reset_expression, sequence, "message" );
      sequence { (a==1); delay 1; (a==0); delay 1 to 3; (a==1) } fred;
      sequence { fred && (b==0); } jim;
      sequence: sequence { <sequence_item>;* } <user> ;
      sequence_item: delay <integer> [to <integer>]
      sequence_item: sequence_expression
      sequence_expression: <user sequence name>
      sequence_expression: ( expression )
      sequence_expression: sequence_expression && sequence_expression
      sequence_expression: sequence_expression || sequence_expression
      sequence_expression: not sequence_expression
      would be nice to have a 'many', but that is unbounded.
      all sequence expressions will be numbered during analysis (and reset guard expressions) (and sequences themselves), and a type added to store the results of those expressions
      all the maximum length of all sequences will be calculated during analysis, and a trail array of the above type of that length will be allocated at instantiation time
      all expressions will be evaluated at end of preclock for the approriate edge (add it will assert the edge function to occur, also)
      all sequences we be evaluated in appropriate order (dependency analysis - those that depend on others are skipped, repeat until all done) after expressions are evaluated
      all assertion statements (sequential) will be added after that, depending on results of sequences

  Future:
  enable as a demon

  add RPC instead of python calls, and then build the python front-end on the RPCs.
  Maybe even build a python frontend that issues calls to a class that may be either RPC connected or hard-connected to c_cyclicity
  An RPC and demon lets the project development be viewed from more than one place
  The RPC development will then tie in with dynamic execution of the C model, with waveform/diagnostic vieiwing.
  For that to work we need the full toplevel instantiation methodology...

*/


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



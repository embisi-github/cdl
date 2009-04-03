/*a Copyright
  
  This file 'c_sl_error.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_C_SL_ERROR
#else
#define __INC_C_SL_ERROR

/*a Includes
 */
#include <stdio.h>

/*a Types
 */
/*t t_sl_error_level
//d The error level runs from okay through fatal, as varying degrees of severity
//d okay means that no error has occurred
//d info, not really an error, just informational
//d warning, not an error necessarily, but potentially serious - life can continue without segmentation faults though
//d serious is an error, but execution can continue without any segmentation faults etc, but should not got to the next stage
//d fatal is an error that indicates that bus errors or segmentation faults will ensue if not stopped immediately
 */
typedef enum
{
    error_level_okay,
    error_level_info,
    error_level_warning,
    error_level_serious,
    error_level_fatal
} t_sl_error_level;

/*t t_sl_error_arg_type
//d For errors, various arguments of various types are supported
//d integer is a C 'int'
//d uint64 is a 64-bit unsigned integer, t_sl_uint64
//d sint64 is a 64-bit signed integer, t_sl_sint64
//d uint32 is a 32-bit unsigned integer, t_sl_uint32
//d sint32 is a 32-bit signed integer, t_sl_sint32
//d line_number is a C 'int', but there should be at most one specified in an error message
//d const_filename is a char *, which is statically defined or in malloced memory that is guaranteed not to be freed for the life of the error
//d malloc_filename is a char * which is copied by the error system to a newly malloced memory location, so it may be in a temporary buffer when an error is added
//d const_string is a char *, which is statically defined or in malloced memory that is guaranteed not to be freed for the life of the error
//d malloc_string is a char * which is copied by the error system to a newly malloced memory location, so it may be in a temporary buffer when an error is added
//d const_private_data is ?
//d malloc_private_data is ?
 */
typedef enum t_sl_error_arg_type
{
    error_arg_type_integer,
    error_arg_type_uint64,
    error_arg_type_sint64,
    error_arg_type_uint32,
    error_arg_type_sint32,
    error_arg_type_line_number,
    error_arg_type_const_filename,
    error_arg_type_malloc_filename,
    error_arg_type_const_string,
    error_arg_type_malloc_string,
    error_arg_type_const_private_data,
    error_arg_type_malloc_private_data,
    error_arg_type_none
};

/*t t_sl_error_fn
//d No idea what this is used for
 */
typedef t_sl_error_level (*t_sl_error_fn)( void *handle,
                                           t_sl_error_level error_level,
                                           const char *format, ... );

/*t t_sl_error_text
//d internal structure
 */
typedef struct t_sl_error_text
{
    int number;
    const char *message;
} t_sl_error_text;

/*t t_sl_error_text_list
//d internal structure
 */
typedef struct t_sl_error_text_list
{
    struct t_sl_error_text_list *next_in_list;
    int number_messages;
    t_sl_error_text *messages;
} t_sl_error_text_list;

/*a c_sl_error class
 */
/*t c_sl_error
//d The class for the errors
 */
class c_sl_error
{
public:
     /*b Constructors/destructors
      */
     c_sl_error( int private_size, int max_args );
     c_sl_error( void ); // default private_size of 4, max_args of 8
     ~c_sl_error();
     void internal_create( int private_size, int max_args );

     /*b Reset
      */
     void reset( void );

     /*b Message adding
      */
     int add_text_list( int errors, t_sl_error_text *messages );

     /*b Error adding
      */
     t_sl_error_level add_error( void *location,
                                 t_sl_error_level error_level,
                                 int error_number,
                                 int function_id,
                                 ... ); // This is the correct way to add errors
     t_sl_error_level add_error_lite( void *location, // used for tracking errors
                                      t_sl_error_level error_level,
                                      const char *format, ... ); // This is not internationalizable, and is really for simple temporary stuff

     /*b Error interrogation
      */
     t_sl_error_level get_error_level( void );
     int get_error_count( t_sl_error_level error_level );
     void *get_next_error( void *handle, t_sl_error_level error_level );
     void *get_nth_error( int n, t_sl_error_level error_level );
     int generate_error_message( void *handle, char *buffer, int buffer_size, int verbosity, void *callback );

     /*b Error display
      */
     int check_errors( FILE *f, t_sl_error_level error_level_display, t_sl_error_level error_level_count );
     int check_errors_and_reset( FILE *f, t_sl_error_level error_level_display, t_sl_error_level error_level_count );
     int check_errors_and_reset( char *str, size_t str_length, t_sl_error_level error_level_display, t_sl_error_level error_level_count, void **handle );

     /*b Private data structures
      */
private:
     int private_size;
     int max_error_args;
     struct t_error *error_list;
     struct t_error *last_error;
     t_sl_error_level worst_error;
     t_sl_error_text_list *error_message_lists;
     t_sl_error_text_list *function_message_lists;
};

/*a Error number enumerations
 */
/*t error_number_base_*
 */
enum
{
     error_number_base_general = 0,
     error_number_base_sl = 1000,
     error_number_base_se = 2000,
     error_number_base_be = 3000,
     error_number_base_cdl = 4000,
     error_number_base_ef = 5000,
     error_number_base_ae = 6000,
};

/*t error_number_general_*
 */
enum
{
     error_number_general_malloc_failed = error_number_base_general,
     error_number_general_bad_integer,
     error_number_general_bad_hexadecimal,
     error_number_general_address_out_of_range,
     error_number_general_empty_filename,
     error_number_general_bad_filename,
     error_number_general_error_s,
     error_number_general_error_ss,
     error_number_general_error_sss,
     error_number_general_error_sd,
     error_number_general_error_ssd,
     error_number_general_error_sssd,
};

/*t error_number_sl_*
 */
enum
{
     error_number_sl_unexpected_indent = error_number_base_sl,
     error_number_sl_unknown_command,
     error_number_sl_unknown_label,
     error_number_sl_unknown_variable,
     error_number_sl_unknown_memory,
     error_number_sl_unknown_event,
     error_number_sl_syntax,
     error_number_sl_exec_file_bad_memory,
     error_number_sl_exec_file_memory_address_out_of_range,
     error_number_sl_exec_file_stack_underflow,
     error_number_sl_exec_file_stack_item_mismatch,
     error_number_sl_exec_file_stack_overflow,
     error_number_sl_exec_file_end_of_file_reached,
     error_number_sl_exec_file_too_much_looping,
     error_number_sl_exec_file_bad_function_evaluation,
     error_number_sl_debug,
     error_number_sl_message,
     error_number_sl_pass,
     error_number_sl_fail,
     error_number_sl_spawn,
     error_number_sl_die,
     error_number_sl_end
};

/*a Error function id enumerations
 */
/*t error_id_base_*
 */
enum
{
     error_id_base_sl = 0,
     error_id_base_se = 1000,
     error_id_base_be = 2000,
     error_id_base_cdl = 3000,
     error_id_base_ef = 4000,
     error_id_base_ae = 5000,
};

/*t error_id_* for SL
 */
enum
{
     error_id_sl_mif_allocate_and_read_mif_file = error_id_base_sl,
     error_id_sl_indented_file_allocate_and_read_indented_file,
     error_id_sl_exec_file_allocate_and_read_exec_file,
     error_id_sl_exec_file_stack_entry_read,
     error_id_sl_exec_file_stack_entry_push,
     error_id_sl_exec_file_get_next_cmd,
     error_id_sl_general_sl_allocate_and_read_file,
     error_id_sl_lite,
};

/*a Error messages (default)
 */
/*v default error messages
 */
#define C_SL_ERROR_DEFAULT_MESSAGES \
{ error_number_general_malloc_failed,        "Malloc failed" }, \
{ error_number_general_bad_integer,          "Bad integer %s0 in line %l of file %f" },\
{ error_number_general_bad_hexadecimal,      "Bad hexadecimal %s0 in line %l of file %f" },\
{ error_number_general_address_out_of_range, "Address %d0 out of range in line %l of file %f" },\
{ error_number_general_empty_filename,       "Empty filename in line %l of file %f" },\
{ error_number_general_bad_filename,         "Bad filename (could not open?) '%s0' in line %l of file %f" },\
{ error_number_general_error_s,              "%f:%l:ERROR: %s0" },\
{ error_number_general_error_ss,             "%f:%l:ERROR: %s0 (%s1)" },\
{ error_number_general_error_sss,            "%f:%l:ERROR: %s0 (%s1) (%s2)" },\
{ error_number_general_error_sd,             "%f:%l:ERROR: %s0 (%d1)" },\
{ error_number_general_error_ssd,            "%f:%l:ERROR: %s0 (%s1) (%d2)" },\
{ error_number_general_error_sssd,           "%f:%l:ERROR: %s0 (%s1) (%s2) (%d3)" },\
{ error_number_sl_unexpected_indent,         "Bad indent line %l of file %f" },\
{ error_number_sl_unknown_command,           "Unknown command '%s0' at line %l of file %f" },\
{ error_number_sl_unknown_label,             "Unknown label '%s0' at line %l of file %f" },\
{ error_number_sl_unknown_variable,          "Unknown variable '%s0' in thread '%s1' at line %l of file %f" },\
{ error_number_sl_unknown_memory,            "Unknown memory '%s0' in thread '%s1' at line %l of file %f" },\
{ error_number_sl_unknown_event,             "Unknown event '%s0' in thread '%s1' at line %l of file %f" },\
{ error_number_sl_syntax,                    "Syntax error in line %l of file %f (syntax %s0)" },\
{ error_number_sl_exec_file_bad_memory,      "Bad memory declaration for '%s0', needs static sizes in line %l exec file '%f'" },\
{ error_number_sl_exec_file_memory_address_out_of_range, "Address %d0 out of range in thread '%s1' at line %l of file %f" },\
{ error_number_sl_exec_file_stack_underflow,  "Attempt to read from empty stack depth with '%s0' in thread '%s1' at line %l of exec file '%f'" }, \
{ error_number_sl_exec_file_stack_item_mismatch, "Stack item mismatch (%s0) in thread '%s1' at line %l of exec file '%f'" }, \
{ error_number_sl_exec_file_stack_overflow, "Stack overflow (max depth %d0) with %s1 in thread '%s2' at line %l of exec file '%f'" }, \
{ error_number_sl_exec_file_end_of_file_reached, "End of file reached without 'end' statement in execution of thread '%s0' of exec file '%f'" }, \
{ error_number_sl_exec_file_too_much_looping, "Too much looping (infinite loop?) at line %l of exec file '%f'" }, \
{ error_number_sl_exec_file_bad_function_evaluation, "Error in function evaluation at line %l of exec file '%f'" }, \
{ error_number_sl_debug,                      "%f:%l:%s2:(%d0):%s1" }, \
{ error_number_sl_message,                    "%f:%l:%s0" }, \
{ error_number_sl_pass,                       "%f:%l:%s2:Pass (%d0):%s1" }, \
{ error_number_sl_fail,                       "%f:%l:%s2:Fail (%d0):%s1" }, \
{ error_number_sl_spawn,                      "%f:%l:%s1:Spawning thread '%s0'" }, \
{ error_number_sl_die,                        "%f:%l:%s1:Dying" }, \
{ error_number_sl_end,                        "%f:%l:%s0:End of thread/exec file execution reached correctly" }, \
{ -1, NULL }

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


/*a Copyright
  
  This file 'sl_exec_file.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_EXEC_FILE
#else
#define __INC_SL_EXEC_FILE

/*a Includes
 */
#include "c_sl_error.h"
#include "sl_general.h"
#include "sl_cons.h"
#include "sl_ef_lib_event.h"
#ifdef SL_EXEC_FILE_PYTHON
#include <Python.h>
#else
typedef void PyObject;
#endif

/*a Defines
 */
#define SL_EXEC_FILE_MAX_CMD_ARGS (32)
#define SL_EXEC_FILE_MAX_FN_ARGS (32)
#define SL_EXEC_FILE_MAX_STACK_DEPTH (8)
#define SL_EXEC_FILE_MAX_OPS_PER_CMD (10000)
#define SL_EXEC_FILE_MAX_THREAD_LOOPS (100)
#define SL_EXEC_FILE_CMD_NONE {sl_exec_file_cmd_none, 0, NULL, NULL, NULL }
#define SL_EXEC_FILE_METHOD_NONE {NULL, 0, 0, NULL, NULL, NULL }
#define SL_EXEC_FILE_FN_NONE {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },

#define SL_EF_STATE_DESC_PTR(ptr,type) static type *ptr;
#define SL_EF_STATE_DESC_ENTRY(ptr,name,width,size) { #name, (((char *)&(ptr->name))-(char *)ptr), width, size }
#define SL_EF_STATE_DESC_END { NULL, 0, 0, 0 }

/*a Types
 */
/*t t_sl_exec_file_completion
 */
typedef struct t_sl_exec_file_completion
{
    t_sl_uint64 finished;
    t_sl_uint64 passed;
    t_sl_uint64 failed;
    t_sl_uint64 return_code;
} t_sl_exec_file_completion;

/*t t_sl_exec_file_callback_fn
 */
typedef void (*t_sl_exec_file_callback_fn)( void *handle, struct t_sl_exec_file_data *file_data );

/*t t_sl_exec_file_wait_cb
 */
typedef struct t_sl_exec_file_wait_cb
{
    union
    {
        t_sl_uint64 uint64;
        void *pointer;
    } args[8];
} t_sl_exec_file_wait_cb;

/*t t_sl_exec_file_wait_callback_fn
 */
typedef int (*t_sl_exec_file_wait_callback_fn)( t_sl_exec_file_wait_cb *wait_cb );

/*t t_sl_exec_file_object_cb
 */
typedef struct t_sl_exec_file_object_cb
{
    void *object_handle;
    struct t_sl_exec_file_object_desc *object_desc;
    union
    {
        struct
        {
            const char *message;  // Message
            void *client_handle;   // Handle from the sender of the message
        } message;
    } data;
} t_sl_exec_file_object_cb;

/*t t_sl_exec_file_object_callback_fn
 */
typedef t_sl_error_level t_sl_exec_file_object_callback_fn( t_sl_exec_file_object_cb *obj_cb );

/*t t_sl_exec_file_state_desc - state description declared by objects to the exec file
 */
typedef struct t_sl_exec_file_state_desc_entry
{
    const  char *name;
    int    offset;
    int    width;
    int    size;
} t_sl_exec_file_state_desc_entry;

/*t Deprecated - if ever used at all - code never existed :-) - t_sl_exec_file_state_desc - state description declared by objects to the exec file
typedef struct t_sl_exec_file_state_desc
{
    const char *state_name;
    int type;
    void *data_ptr; // Offset from external pointer to the data
    int args[4];
} t_sl_exec_file_state_desc;
extern int sl_exec_file_register_state( struct t_sl_exec_file_data *file_data, void *object_handle, void *base_ptr, t_sl_exec_file_state_desc *state_desc );
 */

/*t t_sl_exec_file_register_state_fn
 */
typedef void (*t_sl_exec_file_register_state_fn)( void *handle, const char *instance, const char *state_name, int type, int *data_ptr, int width, ... );

/*t t_sl_exec_file_poststate_callback_fn
*/
typedef void (*t_sl_exec_file_poststate_callback_fn)( void *handle );

/*t t_sl_exec_file_method_fn
 */
typedef t_sl_error_level t_sl_exec_file_method_fn( struct t_sl_exec_file_cmd_cb *cmd_cb, void *object_handle, struct t_sl_exec_file_object_desc *object_desc, struct t_sl_exec_file_method *method );

/*t t_sl_exec_file_eval_fn
 */
typedef int t_sl_exec_file_eval_fn( void *handle, struct t_sl_exec_file_data *file_data, struct t_sl_exec_file_value *args  );

/*t t_sl_exec_file_cmd_handler_fn
 */
typedef t_sl_error_level t_sl_exec_file_cmd_handler_fn( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle );

/*t sl_ef_lib_version_*
 */
typedef enum
{
    sl_ef_lib_version_initial,
    sl_ef_lib_version_cmdcb,
} t_sl_ef_lib_version;

/*t sl_ef_object_version_*
 */
typedef enum
{
    sl_ef_object_version_checkpoint_restore,
} t_sl_ef_object_version;

/*t exec_file_cmd_*
 */
enum
{
     sl_exec_file_cmd_none = -1,
     sl_exec_file_cmd_first_internal = 0,
     sl_exec_file_cmd_base_se = 1000,
     sl_exec_file_cmd_base_ef = 2000,
     sl_exec_file_cmd_base_cdl = 3000,
     sl_exec_file_cmd_first_external = 4000,
};

/*t t_sl_exec_file_cmd
 */
typedef struct t_sl_exec_file_cmd
{
     int cmd_value;
     int min_args;
     const char *cmd;
     const char *args;
     const char *syntax;
} t_sl_exec_file_cmd;

/*t sl_exec_file_fn_*
 */
enum
{
     sl_exec_file_fn_none = -1,
     sl_exec_file_fn_first_internal = 0,
     sl_exec_file_fn_base_se = 1000,
     sl_exec_file_fn_base_ef = 2000,
     sl_exec_file_fn_base_cdl = 3000,
     sl_exec_file_fn_first_external = 4000,
};

/*t t_sl_exec_file_method
 */
typedef struct t_sl_exec_file_method
{
    const char *method;
    char result;
    int min_args;
    const char *args;
    const char *syntax;
    t_sl_exec_file_method_fn *method_fn;
    void *method_handle;
} t_sl_exec_file_method;

/*t t_sl_exec_file_fn
 */
typedef struct t_sl_exec_file_fn
{
     int fn_value;
     const char *fn;

     char result;
     const char *args;
     const char *syntax;
     t_sl_exec_file_eval_fn *eval_fn;
} t_sl_exec_file_fn;

/*t t_sl_exec_file_arg_eval
 */
typedef enum
{
     sl_exec_file_arg_eval_no,
     sl_exec_file_arg_eval_string_variable,
     sl_exec_file_arg_eval_integer_variable,
     sl_exec_file_arg_eval_double_variable,
     sl_exec_file_arg_eval_function
} t_sl_exec_file_arg_eval;

/*t t_sl_exec_file_value_type
 */
typedef enum
{
     sl_exec_file_value_type_integer,
     sl_exec_file_value_type_double,
     sl_exec_file_value_type_string,
     sl_exec_file_value_type_variable,
     sl_exec_file_value_type_label,
     sl_exec_file_value_type_object,
     sl_exec_file_value_type_any=100,
     sl_exec_file_value_type_integer_or_double,
     sl_exec_file_value_type_none
} t_sl_exec_file_value_type;

/*t t_sl_exec_file_value
 */
typedef struct t_sl_exec_file_value
{
    t_sl_exec_file_value_type type;
    union {
        void *ptr; // used if a label or variable reference - must be mallocked
        char *string; // an alias for the pointer to avoid too many casts
    } p;
    char *variable_name;
    double real; // used if a double - filled in with variable value if an expression and variable value
    t_sl_uint64 integer; // used if an integer - filled in with variable value if an expression and variable value
    struct t_sl_exec_file_fn_instance *fn;
    t_sl_exec_file_arg_eval eval;
} t_sl_exec_file_value;

/*t t_sl_exec_file_lib_desc
 */
typedef struct
{
    t_sl_ef_lib_version version; // Indicates which fields are present
    const char *library_name;
    void *handle; // Handle for fn or command callbacks
    t_sl_exec_file_cmd_handler_fn *cmd_handler; // NULL if the commands have to be handed back
    t_sl_exec_file_cmd *file_cmds; // NULL if no commands supplied - cmds do not include a callback fn (yet)
    t_sl_exec_file_fn *file_fns;   // NULL if no fns supplied - fns include a callback fn
} t_sl_exec_file_lib_desc ;

/*t t_sl_exec_file_object_desc
 */
typedef struct t_sl_exec_file_object_desc
{
    t_sl_ef_object_version version; // Indicates which fields are present
    const char *name;   // Will not be freed on object destruction
    void *handle; // Handle for method handler callbacks
    t_sl_exec_file_method *methods;  // NULL if no methods supplied - each has a callback fn
    t_sl_exec_file_object_callback_fn *message_handler;
    t_sl_exec_file_object_callback_fn *free_fn;  // Called on exec_file_free, before freeing the object
    t_sl_exec_file_object_callback_fn *reset_fn; // Called on exec_file_reset
    t_sl_exec_file_object_callback_fn *checkpoint_fn;
    t_sl_exec_file_object_callback_fn *restore_fn;
} t_sl_exec_file_object_desc ;

/*t t_sl_exec_file_cmd_cb
 */
typedef struct t_sl_exec_file_cmd_cb
{
    struct t_sl_exec_file_data *file_data;
    c_sl_error *error;
    const char *filename; // For errors
    int line_number; // For errors
    int cmd;
    t_sl_exec_file_lib_desc *lib_desc;
    struct t_sl_exec_file_thread_execution_data *execution; // thread etc the cmd is called from
    int num_args;
    t_sl_exec_file_value *args;
} t_sl_exec_file_cmd_cb;

/*a External functions
 */
/*b Argument/result functions
 */
extern t_sl_uint64 sl_exec_file_eval_fn_get_argument_integer( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int number );
extern double sl_exec_file_eval_fn_get_argument_double( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int number );
extern const char *sl_exec_file_eval_fn_get_argument_string( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int number );
extern int sl_exec_file_eval_fn_set_result( struct t_sl_exec_file_data *file_data, t_sl_uint64 result );
extern int sl_exec_file_eval_fn_set_result( struct t_sl_exec_file_data *file_data, double result );
extern int sl_exec_file_eval_fn_set_result( struct t_sl_exec_file_data *file_data, const char *string, int copy_string );
extern int sl_exec_file_get_number_of_arguments( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args );

/*b Variable functions
 */
extern int sl_exec_file_set_integer_variable( struct t_sl_exec_file_data *file_data, const char *name, t_sl_uint64 *value );
extern int sl_exec_file_set_double_variable( struct t_sl_exec_file_data *file_data, const char *name, double *value );
extern t_sl_uint64 *sl_exec_file_get_integer_variable( struct t_sl_exec_file_data *file_data, const char *name );
extern double *sl_exec_file_get_double_variable( struct t_sl_exec_file_data *file_data, const char *name );

/*b Expansion registration functions
 */
extern int sl_exec_file_set_state_registration( struct t_sl_exec_file_data *file_data, t_sl_exec_file_register_state_fn register_state_fn, t_sl_exec_file_poststate_callback_fn register_state_complete_fn, void *register_state_handle );
extern int sl_exec_file_add_poststate_callback( struct t_sl_exec_file_data *file_data, t_sl_exec_file_poststate_callback_fn callback_fn, void *handle );
extern int sl_exec_file_set_environment_interrogation( struct t_sl_exec_file_data *file_data, t_sl_get_environment_fn env_fn, void *env_handle );
extern int sl_exec_file_add_library( struct t_sl_exec_file_data *file_data, t_sl_exec_file_lib_desc *lib_desc );
extern void *sl_exec_file_add_object_instance( struct t_sl_exec_file_data *file_data, t_sl_exec_file_object_desc *object_desc );
extern void sl_exec_file_object_instance_register_state( struct t_sl_exec_file_cmd_cb *cmd_cb, const char *object_name, const char *state_name, void *data, int width, int size );
extern void sl_exec_file_object_instance_register_state_desc( struct t_sl_exec_file_cmd_cb *cmd_cb, const char *object_name, t_sl_exec_file_state_desc_entry *state_desc_entry_array, void *base_data );

//extern int sl_exec_file_add_file_commands( struct t_sl_exec_file_data *file_data, t_sl_exec_file_cmd *file_cmds ); Deprecated - use libraries instead
//extern int sl_exec_file_add_file_functions( struct t_sl_exec_file_data *file_data, t_sl_exec_file_fn *file_fns, void *handle ); Deprecated - use libraries instead
extern int sl_exec_file_send_message_to_object( struct t_sl_exec_file_data *file_data, const char *object_name, const char *message, void *handle );
extern void sl_exec_file_send_message_to_all_objects( struct t_sl_exec_file_data *file_data, const char *message, void *handle );

/*b Construct/free functions
 */
extern t_sl_error_level sl_exec_file_allocate_and_read_exec_file( c_sl_error *error,
                                                                  c_sl_error *message,
                                                                  t_sl_exec_file_callback_fn callback_fn,
                                                                  void *callback_handle,
                                                                  const char *id,
                                                                  const char *filename,
                                                                  struct t_sl_exec_file_data **file_data_ptr,
                                                                  const char *user,
                                                                  t_sl_exec_file_cmd *file_cmds,
                                                                  t_sl_exec_file_fn *file_fns );
extern t_sl_error_level sl_exec_file_allocate_from_python_object( c_sl_error *error,
                                                                  c_sl_error *message,
                                                                  t_sl_exec_file_callback_fn callback_fn,
                                                                  void *callback_handle,
                                                                  const char *id,
                                                                  PyObject *py_object,
                                                                  struct t_sl_exec_file_data **file_data_ptr,
                                                                  const char *user,
                                                                  int clocked );
extern void sl_exec_file_free( t_sl_exec_file_data *file_data );

/*b Execution functions
 */
extern void sl_exec_file_reset( struct t_sl_exec_file_data *file_data );
extern int sl_exec_file_evaluate_arguments( struct t_sl_exec_file_data *file_data, const char *thread_id, int line_number, t_sl_exec_file_value *args, int max_args );
extern int sl_exec_file_despatch_next_cmd( struct t_sl_exec_file_data *file_data );
extern int sl_exec_file_thread_wait_on_callback( t_sl_exec_file_cmd_cb *cmd_cb, t_sl_exec_file_wait_callback_fn callback, t_sl_exec_file_wait_cb *wait_cb );

/*b Support functions
 */
extern char *sl_exec_file_vprintf( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int max_args );

/*b Interrogation functions
 */
extern int sl_exec_file_line_number( struct t_sl_exec_file_data *file_data );
extern const char *sl_exec_file_filename( struct t_sl_exec_file_data *file_data );
extern const char *sl_exec_file_current_threadname( struct t_sl_exec_file_data *file_data );
extern const char *sl_exec_file_command_threadname( struct t_sl_exec_file_data *file_data );
extern c_sl_error *sl_exec_file_error( struct t_sl_exec_file_data *file_data );
extern c_sl_error *sl_exec_file_message( struct t_sl_exec_file_data *file_data );
extern c_sl_error *sl_exec_file_message( struct t_sl_exec_file_data *file_data );
extern t_sl_exec_file_completion *sl_exec_file_completion( struct t_sl_exec_file_data *file_data );

/*b Python-specific functions
 */
extern int sl_exec_file_python_add_class_object( PyObject *module );

/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


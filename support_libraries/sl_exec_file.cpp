/*a Copyright
  
  This file 'sl_exec_file.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a To do
  Objects need a free fn
  Objects need a way to report an error (thread etc)
  Objects need a way to register their state through the exec_file (which has a state registration)
  Objects need a way to have a message sent to them, by other exec_file thingies
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "c_sl_error.h"
#include "sl_debug.h"
#include "sl_general.h" 
#include "sl_token.h" 
#include "sl_mif.h"
#include "sl_pthread_barrier.h"
#include "sl_ef_lib_random.h"
#include "sl_ef_lib_memory.h"
#include "sl_ef_lib_fifo.h"
#include "sl_ef_lib_event.h"
#include "sl_exec_file.h"

/*a Defines
 */
#if 1
#include <sys/time.h>
#include <pthread.h>
#define WHERE_I_AM {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%s:%d\n",tp.tv_sec,tp.tv_usec,pthread_self(),__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#endif
#ifndef SL_EXEC_FILE_PYTHON
#define PyObject void
#define Py_DECREF(x) {}
#define PyObject_CallMethod(a,...) (NULL)
#endif

/*a Types
 */
/*t t_sl_exec_file_fn_instance *fn
 */
typedef struct t_sl_exec_file_fn_instance
{
    int is_fn;
    int num_args;
    union
    {
        struct
        {
            t_sl_exec_file_fn *fn;
            struct t_sl_exec_file_lib_chain *lib;
        } fn;
        struct
        {
            t_sl_exec_file_method *method;
            struct t_sl_exec_file_object_chain *object;
        } object;
    } call;
    t_sl_exec_file_value args[SL_EXEC_FILE_MAX_FN_ARGS];
} t_sl_exec_file_fn_instance;

/*t t_sl_exec_file_stack_frame_type
 */
typedef enum
{
     sl_exec_file_stack_frame_type_gosub,
     sl_exec_file_stack_frame_type_for,
     sl_exec_file_stack_frame_type_while
} t_sl_exec_file_stack_frame_type;

/*t t_sl_exec_file_stack_frame
 */
typedef struct t_sl_exec_file_stack_frame
{
     t_sl_exec_file_stack_frame_type type;
     char *variable; // for 'FOR' statements only - not mallocked here
     int line_number; // for while loops, the line of the 'while' of the loop; for FOR statements, the line after the 'FOR'; for gosub the line after the 'gosub'
     int increment; // for 'FOR' statements only
     int last_value; // for 'FOR' statements only
} t_sl_exec_file_stack_frame;

/*t t_sl_exec_file_variable_type
 */
typedef enum
{
     sl_exec_file_variable_type_integer,
     sl_exec_file_variable_type_double,
     sl_exec_file_variable_type_string
} t_sl_exec_file_variable_type;

/*t t_sl_exec_file_variable
 */
typedef struct t_sl_exec_file_variable
{
    struct t_sl_exec_file_variable *next_in_list;
    t_sl_exec_file_variable_type type;
    t_sl_uint64 integer[8];
    double real[2];
    char *string; // always mallocked
    char name[1];
} t_sl_exec_file_variable;

/*t t_sl_exec_file_event_type
 */
typedef enum
{
     sl_exec_file_event_type_boolean,
} t_sl_exec_file_event_type;

/*t t_sl_exec_file_event
 */
typedef struct t_sl_exec_file_event
{
     struct t_sl_exec_file_event *next_in_list;
     struct t_sl_exec_file_event *prev_in_list;
     char *name;
     int fired;
} t_sl_exec_file_event;

/*t t_sl_exec_file_line_type
 */
typedef enum
{
     sl_exec_file_line_type_none,
     sl_exec_file_line_type_label,
     sl_exec_file_line_type_cmd, // Command from a library
     sl_exec_file_line_type_method, // Object method
} t_sl_exec_file_line_type;

/*t
 */
typedef enum
{
    cmd_type_declarative, // For object declarations
    cmd_type_executive
} t_sl_exec_file_line_cmd_type;

/*t t_sl_exec_file_line
 */
typedef struct t_sl_exec_file_line
{
    t_sl_exec_file_line_type type;
    int num_args;
    int cmd;
    t_sl_exec_file_line_cmd_type cmd_type;
    union
    {
        struct
        {
            t_sl_exec_file_cmd *cmd;
            struct t_sl_exec_file_lib_chain *lib;
        } cmd;
        struct
        {
            t_sl_exec_file_method *method;
            struct t_sl_exec_file_object_chain *object;
        } object;
    } call;
    int file_pos;
    t_sl_exec_file_value args[SL_EXEC_FILE_MAX_CMD_ARGS]; // for a label, args[0] is char *label (needs to be freed)
} t_sl_exec_file_line;

/*t t_sl_exec_file_thread_execution_type
 */
typedef enum
{
     sl_exec_file_thread_execution_type_running,
     sl_exec_file_thread_execution_type_waiting,
} t_sl_exec_file_thread_execution_type;

/*t t_sl_exec_file_thread_execution_data
 */
typedef struct
{
    t_sl_exec_file_thread_execution_type type;
    t_sl_exec_file_wait_callback_fn wait_callback;
    t_sl_exec_file_wait_cb wait_cb;
} t_sl_exec_file_thread_execution_data;

/*t t_sl_exec_file_thread_data
 */
typedef struct t_sl_exec_file_thread_data
{
    struct t_sl_exec_file_thread_data *next_in_list;
    struct t_sl_exec_file_thread_data *prev_in_list;
    struct t_sl_exec_file_data *file_data;
    char *id; // NULL for main thread
    int trace_level; // 0 for no trace
    int line_number;
    int stack_ptr; // incrementing stack pointer, empty at the stack pointer
    int stack_size;
    int halted;
    int pending_command;
    int cmd;
    int num_args;
    t_sl_exec_file_value *args;
    t_sl_exec_file_thread_execution_data execution;
    t_sl_exec_file_stack_frame stack[1];
} t_sl_exec_file_thread_data;

/*t t_sl_exec_file_lib_chain
 */
typedef struct t_sl_exec_file_lib_chain
{
    struct t_sl_exec_file_lib_chain *next_in_list;
    int base; // Base number of commands or fns; used if registration specifies a 'choose next' base address
    int max; // Maximum number of command or fn
    t_sl_exec_file_lib_desc lib_desc; // Copied from the library given, or created for individual cmds/fns
} t_sl_exec_file_lib_chain;

/*t t_sl_exec_file_object_chain
 */
typedef struct t_sl_exec_file_object_chain
{
    struct t_sl_exec_file_object_chain *next_in_list;
    int base; // Base number of commands or fns; used if registration specifies a 'choose next' base address
    int max; // Maximum number of command or fn
    int name_length; // strlen of name
    t_sl_exec_file_object_desc object_desc; // Copied from the object given, or created for individual cmds/fns
} t_sl_exec_file_object_chain;

/*t t_sl_exec_file_callback
 */
typedef struct t_sl_exec_file_callback
{
    struct t_sl_exec_file_callback *next_in_list;
    t_sl_exec_file_poststate_callback_fn callback_fn;
    void *handle;
} t_sl_exec_file_callback;

/*t t_sl_exec_file_data
 */
typedef struct t_sl_exec_file_data
{
    int number_lines;
    c_sl_error *error;
    c_sl_error *message;
    char *id;
    char *filename;
    PyObject *py_object;
    char *user;
    t_sl_exec_file_completion completion;

    t_sl_exec_file_register_state_fn register_state_fn;
    void *register_state_handle;
    t_sl_exec_file_callback *poststate_callbacks;
    t_sl_get_environment_fn env_fn;
    void *env_handle;
    int next_base; // Guaranteed to be greater than the largest cmd or fn base currently registered
    t_sl_exec_file_lib_chain *lib_chain;

    t_sl_exec_file_variable *variables;
    t_sl_exec_file_object_chain *object_chain; // Randoms, memories, events etc
    t_sl_exec_file_thread_data *threads; // Not python

    t_sl_exec_file_thread_data *command_from_thread;
    t_sl_exec_file_thread_data *current_thread;
    t_sl_exec_file_value *eval_fn_result;
    t_sl_exec_file_value cmd_result;
    int eval_fn_result_ok;

    int during_init; // Asserted during init (python only)
    int number_threads; // 0 if not multithreading; 1 if multithreading and no subthreads; Only for python at present

    t_sl_exec_file_line lines[1]; // must be last item in the structure
} t_sl_exec_file_data;

/*a Static function declarations
 */
static t_sl_exec_file_eval_fn ef_fn_eval_env_int;
static t_sl_exec_file_eval_fn ef_fn_eval_env_double;
static t_sl_exec_file_eval_fn ef_fn_eval_env_string;
static t_sl_exec_file_eval_fn ef_fn_eval_line;
static t_sl_exec_file_eval_fn ef_fn_eval_filename;
static t_sl_exec_file_eval_fn ef_fn_eval_threadname;
static int parse_argument( t_sl_exec_file_data *file_data, int line_number, char *token, int token_length, t_sl_exec_file_value *arg, char type, const char *syntax );

/*a Static variables
 */
/*v cmd_*
 */
enum
{
    cmd_int = sl_exec_file_cmd_first_internal,
    cmd_double,
    cmd_string,
    cmd_end,
    cmd_spawn,
    cmd_die,
    cmd_fail,
    cmd_pass,
    cmd_debug,
    cmd_printf,
    cmd_warning,

    cmd_trace,
    cmd_list,

    cmd_for,
    cmd_next,
    cmd_while,
    cmd_endwhile,

    cmd_goto,
    cmd_beq,
    cmd_bne,
    cmd_bge,
    cmd_bgt,
    cmd_ble,
    cmd_blt,

    cmd_gosub,
    cmd_return,

    cmd_set_string,
    cmd_sprintf,

    cmd_set,
    cmd_add,
    cmd_sub,
    cmd_mult,
    cmd_div,
    cmd_shl,
    cmd_ashr,
    cmd_lshr,
    cmd_rol32,
    cmd_ror32,
    cmd_rol64,
    cmd_ror64,
    cmd_and,
    cmd_or,
    cmd_xor,
    cmd_bic,

};

/*v fn_*
 */
enum
{
     fn_env_int = sl_exec_file_fn_first_internal,
     fn_env_double,
     fn_env_string,
     fn_line,
     fn_filename,
     fn_threadname,
};

/*v internal_cmds
 */
static t_sl_exec_file_cmd internal_cmds[] = 
{
     {cmd_int,           1, "!int", "v",         "int <variable name>" },
     {cmd_string,        1, "!string", "v",      "string <variable name>" },
     {cmd_double,        1, "!double", "v"       "double <variable name>" },
     {cmd_end,           0, "end", "",          "end" },
     {cmd_spawn,         3, "spawn", "sli",     "spawn <thread name> <label> <max stack depth>" },
     {cmd_die,           0, "die", "",          "die" },
     {cmd_fail,          2, "fail", "is",       "fail <integer> <message>" },
     {cmd_pass,          2, "pass", "is",       "pass <integer> <message>" },
     {cmd_trace,         1, "trace", "i",       "trace <level>" },
     {cmd_list,          1, "list", "s",        "list <string what - libs, fns, objects, cmds, lib, object>" },
     {cmd_goto,          1, "goto", "l",        "goto <label>" },
     {cmd_gosub,         1, "gosub", "l",       "gosub <label>" },
     {cmd_return,        0, "return", "",       "return" },
     {cmd_for,           4, "for", "viii",      "for <variable name> <start> <last> <step>" },
     {cmd_next,          1, "next", "v",        "next <variable name>" },
     {cmd_while,         1, "while", "i",       "while <expression>" },
     {cmd_endwhile,      0, "endwhile", "",     "endwhile" },
     {cmd_debug,         2, "debug", "is",    "debug <level> <message>" },
     {cmd_printf,        1, "printf", "sxxxxxxxxxxxxxx",    "printf <format> <args>" },
     {cmd_warning,       2, "warning", "is",    "warning <level> <warning>" },
     {cmd_beq,           3, "beq", "IIl",       "beq <expression 1> <expresssion 2> <label>" },
     {cmd_bne,           3, "bne", "IIl",       "bne <expression 1> <expresssion 2> <label>" },
     {cmd_bge,           3, "bge", "IIl",       "bge <expression 1> <expresssion 2> <label>" },
     {cmd_bgt,           3, "bgt", "IIl",       "bgt <expression 1> <expresssion 2> <label>" },
     {cmd_ble,           3, "ble", "IIl",       "ble <expression 1> <expresssion 2> <label>" },
     {cmd_blt,           3, "blt", "IIl",       "blt <expression 1> <expresssion 2> <label>" },
     {cmd_set_string,    2, "set_string", "vs",    "set_string <variable name> <expression>" },
     {cmd_sprintf,       2, "sprintf", "vsxxxxxxxxxxxx", "sprintf <variable name> <format> <args>" },
     {cmd_set,           2, "set", "vI",        "set <variable name> <expression>" },
     {cmd_add,           3, "add", "vII",       "add <variable name> <expression 1> <expression 2>" },
     {cmd_sub,           3, "sub", "vII",       "sub <variable name> <expression 1> <expression 2>" },
     {cmd_mult,          3, "mult", "vII",      "mult <variable name> <expression 1> <expression 2>" },
     {cmd_div,           3, "div", "vII",       "div <variable name> <expression 1> <expression 2>" },
     {cmd_lshr,          3, "lshr", "vII",      "lshr <variable name> <expression 1> <expression 2>" },
     {cmd_ashr,          3, "ashr", "vII",      "ashr <variable name> <expression 1> <expression 2>" },
     {cmd_shl,           3, "shl", "vII",       "shl <variable name> <expression 1> <expression 2>" },
     {cmd_rol32,         3, "rol", "vII",       "rol <variable name> <expression 1> <expression 2>" },
     {cmd_ror32,         3, "ror", "vII",       "ror <variable name> <expression 1> <expression 2>" },
     {cmd_rol64,         3, "rol64", "vII",     "rol64 <variable name> <expression 1> <expression 2>" },
     {cmd_ror64,         3, "ror64", "vII",     "ror64 <variable name> <expression 1> <expression 2>" },
     {cmd_and,           3, "and", "vii",       "and <variable name> <expression 1> <expression 2>" },
     {cmd_or,            3, "or", "vii",        "or  <variable name> <expression 1> <expression 2>" },
     {cmd_xor,           3, "xor", "vii",       "xor <variable name> <expression 1> <expression 2>" },
     {cmd_bic,           3, "bic", "vii",       "bic <variable name> <expression 1> <expression 2>" },
     {sl_exec_file_cmd_none, 0, NULL, NULL, NULL},
};

/*v internal_fns
 */
static t_sl_exec_file_fn internal_fns[] = 
{
     {fn_env_int,           "env_int",    'i', "s", "env_int(<string>)", ef_fn_eval_env_int },
     {fn_env_double,        "env_double", 'd', "s", "env_double(<string>)", ef_fn_eval_env_double },
     {fn_env_string,        "env_string", 's', "s", "env_string(<string>)", ef_fn_eval_env_string },
     {fn_line,              "line",       'i', "", "line()",         ef_fn_eval_line },
     {fn_filename,          "filename",   's', "", "filename()", ef_fn_eval_filename },
     {fn_threadname,        "threadname", 's', "", "threadname()", ef_fn_eval_threadname },
     {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },
};

/*a Thread handling
 */
/*f sl_exec_file_create_thread
 */
static int sl_exec_file_create_thread( t_sl_exec_file_data *file_data, const char *name, int stack_size, int line_number )
{
     t_sl_exec_file_thread_data *thread;

     thread = (t_sl_exec_file_thread_data *)malloc(sizeof(t_sl_exec_file_thread_data)+stack_size*sizeof(t_sl_exec_file_stack_frame));
     if (!thread)
          return 0;

     if (file_data->threads)
     {
          file_data->threads->prev_in_list = thread;
     }
     thread->next_in_list = file_data->threads;
     thread->prev_in_list = NULL;
     file_data->threads = thread;

     thread->file_data = file_data;
     thread->id = sl_str_alloc_copy( name );
     thread->trace_level =0;
     thread->line_number = line_number;
     thread->stack_ptr = 0;
     thread->stack_size = stack_size;
     thread->halted = 0;
     thread->pending_command = 0;
     thread->cmd = 0;
     thread->args = NULL;
     thread->execution.type = sl_exec_file_thread_execution_type_running;
     return 1;
}

/*f sl_exec_file_free_thread
 */
static void sl_exec_file_free_thread( t_sl_exec_file_data *file_data, t_sl_exec_file_thread_data *thread )
{
     if (thread->prev_in_list)
     {
          thread->prev_in_list->next_in_list = thread->next_in_list;
     }
     else
     {
          file_data->threads = thread->next_in_list;
     }
     if (thread->next_in_list)
     {
          thread->next_in_list->prev_in_list = thread->prev_in_list;
     }
     if (thread->id)
          free(thread->id);
     free(thread);

     if (!file_data->threads)
     {
         file_data->completion.finished = 1;
     }
}

/*f sl_exec_file_free_threads
 */
static void sl_exec_file_free_threads( t_sl_exec_file_data *file_data )
{
     while (file_data->threads)
     {
          sl_exec_file_free_thread( file_data, file_data->threads );
     }
}

/*a Variable handling
 */
/*f sl_exec_file_create_variable
 */
static t_sl_exec_file_variable *sl_exec_file_create_variable( t_sl_exec_file_data *file_data, char *name, t_sl_exec_file_variable_type type )
{
     t_sl_exec_file_variable *var;

     var = (t_sl_exec_file_variable *)malloc(sizeof(t_sl_exec_file_variable)+strlen(name));
     if (!var)
          return NULL;

     strcpy( var->name, name );
     var->next_in_list = file_data->variables;
     file_data->variables = var;
     var->type = type;
     var->integer[0] = 0;
     var->real[0] = 0;
     var->string = NULL;
     return var;
}

/*f sl_exec_file_find_variable
 */
static t_sl_exec_file_variable *sl_exec_file_find_variable( t_sl_exec_file_data *file_data, const char *name, int length )
{
     t_sl_exec_file_variable *var;

     for (var=file_data->variables; var; var=var->next_in_list)
     {
          if (!strncmp(name, var->name, length) && (var->name[length]==0))
          {
               return var;
          }
     }
     return NULL;
}

/*f sl_exec_file_find_variable
 */
static t_sl_exec_file_variable *sl_exec_file_find_variable( t_sl_exec_file_data *file_data, const char *name )
{
     t_sl_exec_file_variable *var;

     for (var=file_data->variables; var; var=var->next_in_list)
     {
          if (!strcmp(name, var->name))
          {
               return var;
          }
     }
     return NULL;
}

/*f sl_exec_file_set_variable
 */
extern int sl_exec_file_set_variable( t_sl_exec_file_data *file_data, char *name, t_sl_uint64 *value, double *value_d )
{
     t_sl_exec_file_variable *var;

     var = sl_exec_file_find_variable( file_data, name );
     if (!var)
     {
          return 0;
     }

     if (var->type==sl_exec_file_variable_type_integer)
     {
          var->integer[0] = value[0];
          var->real[0] = (double)value[0];
     }
     if (var->type==sl_exec_file_variable_type_double)
     {
          var->integer[0] = (t_sl_uint64)value_d[0];
          var->real[0] = value_d[0];
     }
     SL_DEBUG( sl_debug_level_info,
               "exec file %s variable %s value %08x",
               file_data->id,
               var->name,
               value[0] );
     return 1;
}

/*f sl_exec_file_set_integer_variable
 */
extern int sl_exec_file_set_integer_variable( t_sl_exec_file_data *file_data, char *name, t_sl_uint64 *value )
{
     t_sl_exec_file_variable *var;

     var = sl_exec_file_find_variable( file_data, name );
     if (!var)
     {
          var = sl_exec_file_create_variable( file_data, name, sl_exec_file_variable_type_integer );
     }
     if (!var)
     {
          return 0;
     }

     var->integer[0] = value[0];
     SL_DEBUG( sl_debug_level_info,
              "exec file %s variable %s value %08x",
              file_data->id,
              var->name,
              value[0] );
     return 1;
}

/*f sl_exec_file_set_double_variable
 */
extern int sl_exec_file_set_double_variable( t_sl_exec_file_data *file_data, char *name, double *value )
{
     t_sl_exec_file_variable *var;

     var = sl_exec_file_find_variable( file_data, name );
     if (!var)
     {
          var = sl_exec_file_create_variable( file_data, name, sl_exec_file_variable_type_double );
     }
     if (!var)
     {
          return 0;
     }

     var->real[0] = value[0];
     SL_DEBUG( sl_debug_level_info,
              "exec file %s variable %s value %f",
              file_data->id,
              var->name,
              value[0] );
     return 1;
}

/*f sl_exec_file_set_string_variable
 */
extern int sl_exec_file_set_string_variable( t_sl_exec_file_data *file_data, char *name, char *value )
{
     t_sl_exec_file_variable *var;

     var = sl_exec_file_find_variable( file_data, name );
     if (!var)
     {
          var = sl_exec_file_create_variable( file_data, name, sl_exec_file_variable_type_integer );
     }
     if (!var)
     {
          return 0;
     }

     if (var->string)
          free(var->string);
     var->string = sl_str_alloc_copy( value );
     SL_DEBUG( sl_debug_level_info,
              "exec file %s variable %s value '%s'",
              file_data->id,
              var->name,
              value );
     return 1;
}

/*f sl_exec_file_get_integer_variable
 */
extern t_sl_uint64 *sl_exec_file_get_integer_variable( t_sl_exec_file_data *file_data, const char *name )
{
     t_sl_exec_file_variable *var;

     var = sl_exec_file_find_variable( file_data, name );
     if (!var)
     {
          return NULL;
     }
     return var->integer;
}

/*f sl_exec_file_get_double_variable
 */
extern double *sl_exec_file_get_double_variable( t_sl_exec_file_data *file_data, char *name )
{
     t_sl_exec_file_variable *var;

     var = sl_exec_file_find_variable( file_data, name );
     if (!var)
     {
          return NULL;
     }
     return var->real;
}

/*f sl_exec_file_get_string_variable
 */
extern char *sl_exec_file_get_string_variable( t_sl_exec_file_data *file_data, char *name )
{
     t_sl_exec_file_variable *var;

     var = sl_exec_file_find_variable( file_data, name );
     if (!var)
     {
          return NULL;
     }
     return var->string;
}

/*a Exec file evaluation functions
 */
/*f ef_fn_eval_env_string
 */
static int ef_fn_eval_env_string( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    const char *result;
    result = (*(file_data->env_fn))( file_data->env_handle, sl_exec_file_eval_fn_get_argument_string( file_data, args, 0 ) );
    return sl_exec_file_eval_fn_set_result( file_data, result, 1 );
}

/*f ef_fn_eval_env_int
 */
static int ef_fn_eval_env_int( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    const char *text;
    t_sl_uint64 int_result;

    text = (*(file_data->env_fn))( file_data->env_handle, sl_exec_file_eval_fn_get_argument_string( file_data, args, 0 ) );
    if (text)
    {
        sscanf( text, "%lld", &int_result );
        return sl_exec_file_eval_fn_set_result( file_data, int_result );
    }
    return sl_exec_file_eval_fn_set_result( file_data, (t_sl_uint64)0 );
}

/*f ef_fn_eval_env_double
 */
static int ef_fn_eval_env_double( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    const char *text;
    double real_result;

    text = (*(file_data->env_fn))( file_data->env_handle, sl_exec_file_eval_fn_get_argument_string( file_data, args, 0 ) );
    sscanf( text, "%lf", &real_result );
    return sl_exec_file_eval_fn_set_result( file_data, real_result );
}

/*f ef_fn_eval_line
 */
static int ef_fn_eval_line( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    return sl_exec_file_eval_fn_set_result( file_data, (t_sl_uint64)(file_data->current_thread->line_number) );
}

/*f ef_fn_eval_filename
 */
static int ef_fn_eval_filename( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    return sl_exec_file_eval_fn_set_result( file_data, sl_exec_file_filename( file_data ), 1 );
}

/*f ef_fn_eval_threadname
 */
static int ef_fn_eval_threadname( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    return sl_exec_file_eval_fn_set_result( file_data, sl_exec_file_current_threadname( file_data ), 1 );
}

/*a Exec file registration functions
 */
/*f sl_exec_file_set_state_registration
 */
extern int sl_exec_file_set_state_registration( struct t_sl_exec_file_data *file_data, t_sl_exec_file_register_state_fn register_state_fn, t_sl_exec_file_poststate_callback_fn register_state_complete_fn, void *register_state_handle )
{
    if (!sl_exec_file_add_poststate_callback( file_data, register_state_complete_fn, register_state_handle ) ) return 0;
    file_data->register_state_fn = register_state_fn;
    file_data->register_state_handle = register_state_handle;
    return 1;
}

/*f sl_exec_file_add_poststate_callback
 */
extern int sl_exec_file_add_poststate_callback( struct t_sl_exec_file_data *file_data, t_sl_exec_file_poststate_callback_fn callback_fn, void *handle )
{
    t_sl_exec_file_callback *efc;
    if (!file_data)
        return 0;
    efc = (t_sl_exec_file_callback *)malloc(sizeof(t_sl_exec_file_callback));
    if (!efc) return 0;
    efc->callback_fn = callback_fn;
    efc->handle = handle;
    efc->next_in_list = file_data->poststate_callbacks;
    file_data->poststate_callbacks = efc;
    return 1;
}

/*f sl_exec_file_set_environment_interrogation
 */
extern int sl_exec_file_set_environment_interrogation( struct t_sl_exec_file_data *file_data, t_sl_get_environment_fn env_fn, void *env_handle )
{
     if (!file_data)
          return 0;
     file_data->env_fn = env_fn;
     file_data->env_handle = env_handle;
     return 1;
}

/*f sl_exec_file_add_file_commands
 */
extern int sl_exec_file_add_file_commands( struct t_sl_exec_file_data *file_data, t_sl_exec_file_cmd *file_cmds )
{
    t_sl_exec_file_lib_desc lib_desc;

    fprintf(stderr,"Unexpected call to sl_exec_file_add_file_commands - deprecated fn call - first command syntax '%s'\n",
            file_cmds[0].syntax );

    lib_desc.version = sl_ef_lib_version_initial;
    lib_desc.library_name = "DeprecatedAddFileCmds";
    lib_desc.handle = NULL;
    lib_desc.cmd_handler = NULL;
    lib_desc.file_cmds = file_cmds;
    lib_desc.file_fns = NULL;

    return (sl_exec_file_add_library( file_data, &lib_desc )!=0);
}

/*f sl_exec_file_add_file_functions
 */
extern int sl_exec_file_add_file_functions( struct t_sl_exec_file_data *file_data, t_sl_exec_file_fn *file_fns, void *handle )
{
    t_sl_exec_file_lib_desc lib_desc;

    fprintf(stderr,"Unexpected call to sl_exec_file_add_file_functions - deprecated fn call - first command syntax '%s'\n",
            file_fns[0].syntax );

    lib_desc.version = sl_ef_lib_version_initial;
    lib_desc.library_name = "DeprecatedAddFileCmds";
    lib_desc.handle = handle;
    lib_desc.cmd_handler = NULL;
    lib_desc.file_cmds = NULL;
    lib_desc.file_fns = file_fns;

    return (sl_exec_file_add_library( file_data, &lib_desc )!=0);
}

/*f sl_exec_file_add_library
 */
extern int sl_exec_file_add_library( struct t_sl_exec_file_data *file_data, t_sl_exec_file_lib_desc *lib_desc )
{
    int j, max;
    t_sl_exec_file_lib_chain *lib;

    if (!file_data || !lib_desc)
        return 0;

    lib = (t_sl_exec_file_lib_chain *)malloc(sizeof(t_sl_exec_file_lib_chain));
    if (!lib)
        return 0;

    lib->next_in_list = file_data->lib_chain;
    file_data->lib_chain = lib;

    memcpy( &(lib->lib_desc), lib_desc, sizeof(*lib_desc) );

    max = 0;
    if (lib_desc->file_cmds)
    {
        for (j=0; lib_desc->file_cmds[j].cmd_value!=sl_exec_file_cmd_none; j++ )
        {
            if (lib_desc->file_cmds[j].cmd_value >= max)
            {
                max = lib_desc->file_cmds[j].cmd_value;
            }
        }
    }
    if (lib_desc->file_fns)
    {
        for (j=0; lib_desc->file_fns[j].fn_value!=sl_exec_file_fn_none; j++ )
        {
            if (lib_desc->file_fns[j].fn_value >= max)
            {
                max = lib_desc->file_fns[j].fn_value;
            }
        }
    }

    lib->base = file_data->next_base;
    lib->max = max;

    file_data->next_base = lib->base+lib->max+1;

    return lib->base;
}

/*f sl_exec_file_add_object_instance
  Called in response to a callback to a declarative command
  Create an instances of an object, which has methods, which can be used as fns or commands
 */
extern void *sl_exec_file_add_object_instance( struct t_sl_exec_file_data *file_data, t_sl_exec_file_object_desc *object_desc )
{
    int j, max;
    t_sl_exec_file_object_chain *object;

    if (!file_data || !object_desc)
        return NULL;

    object = (t_sl_exec_file_object_chain *)malloc(sizeof(t_sl_exec_file_object_chain));
    if (!object)
        return NULL;

    object->next_in_list = file_data->object_chain;
    file_data->object_chain = object;

    memcpy( &(object->object_desc), object_desc, sizeof(*object_desc) );

    max = 0;
    if (object_desc->methods)
    {
        for (j=0; object_desc->methods[j].method; j++ );
        max = j;
    }

    object->base = file_data->next_base;
    object->max = max;
    object->name_length = strlen(object_desc->name);

    file_data->next_base = object->base+object->max+1;

    return (void *)object;
}

/*f sl_exec_file_object_instance_register_state
 */
extern void sl_exec_file_object_instance_register_state( struct t_sl_exec_file_cmd_cb *cmd_cb, const char *object_name, const char *state_name, void *data, int width, int size )
{
    char buffer[512];
    char *names;
    if (cmd_cb->file_data->register_state_fn)
    {
        snprintf( buffer, 512, "%s.%s", object_name, state_name );
        names = sl_str_alloc_copy(buffer);
        if (size<=0)
        {
            (*(cmd_cb->file_data->register_state_fn))( cmd_cb->file_data->register_state_handle, cmd_cb->file_data->id, names, 0, (int *)(data), width );
        }
        else
        {
            (*(cmd_cb->file_data->register_state_fn))( cmd_cb->file_data->register_state_handle, cmd_cb->file_data->id, names, 1, (int *)(data), width, size );
        }
    }
}

/*f sl_exec_file_object_instance_register_state_desc
 */
extern void sl_exec_file_object_instance_register_state_desc( struct t_sl_exec_file_cmd_cb *cmd_cb, const char *object_name, t_sl_exec_file_state_desc_entry *state_desc_entry_array, void *base_data )
{
    while (state_desc_entry_array->name)
    {
        sl_exec_file_object_instance_register_state( cmd_cb, object_name, state_desc_entry_array->name, (void *)(((char *)base_data)+state_desc_entry_array->offset), state_desc_entry_array->width, state_desc_entry_array->size );
        state_desc_entry_array++;
    }
}

/*a Exec file data structure creation/manipulation/freeing
 */
/*f find_command_in_library
 */
static t_sl_exec_file_lib_chain *find_command_in_library( t_sl_exec_file_data *file_data, int cmd )
{
    t_sl_exec_file_lib_chain *lib;
    for (lib=file_data->lib_chain; lib; lib=lib->next_in_list)
    {
        if (lib->lib_desc.file_cmds)
        {
            if ((cmd>=lib->base) && (cmd<=lib->base+lib->max))
            {
                return lib;
            }
        }
    }
    return NULL;
}

/*f find_command_in_libraries
 */
static t_sl_exec_file_cmd *find_command_in_libraries( t_sl_exec_file_data *file_data, const char *token, t_sl_exec_file_lib_chain **fnd_lib )
{
    t_sl_exec_file_lib_chain *chain;
    int j;

    for (chain=file_data->lib_chain; chain; chain=chain->next_in_list)
    {
        if (chain->lib_desc.file_cmds)
        {
            for (j=0; chain->lib_desc.file_cmds[j].cmd_value!=sl_exec_file_cmd_none; j++ )
            {
                int ofs=0;
                if (chain->lib_desc.file_cmds[j].cmd[0]=='!')
                    ofs=1;
                if (!strcmp(chain->lib_desc.file_cmds[j].cmd+ofs, token))
                {
                    *fnd_lib = chain;
                    return chain->lib_desc.file_cmds+j;
                }
            }
        }
    }

    return NULL;
}

/*f sl_exec_file_send_message_to_object
 */
extern int sl_exec_file_send_message_to_object( struct t_sl_exec_file_data *file_data, const char *object_name, const char *message, void *handle )
{
    t_sl_exec_file_object_chain *chain;
    t_sl_exec_file_object_cb obj_cb;

    obj_cb.data.message.message = message;
    obj_cb.data.message.client_handle = handle;
    for (chain=file_data->object_chain; chain; chain=chain->next_in_list)
    {
        if ( !strcmp( chain->object_desc.name, object_name ) )
        {
            if (chain->object_desc.message_handler)
            {
                obj_cb.object_handle = (void *)chain;
                obj_cb.object_desc = &(chain->object_desc);
                return chain->object_desc.message_handler( &obj_cb );
            }
            return 0;
        }
    }
    return 0;
}

/*f sl_exec_file_send_message_to_all_objects
 */
extern void sl_exec_file_send_message_to_all_objects( struct t_sl_exec_file_data *file_data, const char *message, void *handle )
{
    t_sl_exec_file_object_chain *chain;
    t_sl_exec_file_object_cb obj_cb;

    obj_cb.data.message.message = message;
    obj_cb.data.message.client_handle = handle;
    for (chain=file_data->object_chain; chain; chain=chain->next_in_list)
    {
        if (chain->object_desc.message_handler)
        {
            obj_cb.object_handle = (void *)chain;
            obj_cb.object_desc = &(chain->object_desc);
            chain->object_desc.message_handler( &obj_cb );
        }
    }
}

/*f find_method_in_objects
 */
static t_sl_exec_file_method *find_method_in_objects( t_sl_exec_file_data *file_data, const char *token, int token_length, t_sl_exec_file_object_chain **fnd_chain )
{
    t_sl_exec_file_object_chain *chain;
    int j;

    for (chain=file_data->object_chain; chain; chain=chain->next_in_list)
    {
        if ( (token_length>chain->name_length) &&
             !strncmp( chain->object_desc.name, token, chain->name_length ) &&
             (token[chain->name_length]=='.')  &&
             chain->object_desc.methods )
        {
            const char *token_tail = token+chain->name_length+1;
            int token_tail_length = token_length-(chain->name_length+1);
            for (j=0; chain->object_desc.methods[j].method; j++ )
            {
                if ( !strncmp(chain->object_desc.methods[j].method, token_tail, token_tail_length) &&
                     (chain->object_desc.methods[j].method[token_tail_length]==0) )
                {
                    *fnd_chain = chain;
                    return chain->object_desc.methods+j;
                }
            }
        }
    }
    return NULL;
}

/*f find_function_in_libraries
 */
static t_sl_exec_file_fn *find_function_in_libraries( t_sl_exec_file_data *file_data, const char *token, int token_length, t_sl_exec_file_lib_chain **fnd_chain )
{
    t_sl_exec_file_lib_chain *chain;
    int j;

    for (chain=file_data->lib_chain; chain; chain=chain->next_in_list)
    {
        if (chain->lib_desc.file_fns)
        {
            for (j=0; chain->lib_desc.file_fns[j].fn_value!=sl_exec_file_fn_none; j++ )
            {
                if ( !strncmp(chain->lib_desc.file_fns[j].fn, token, token_length) &&
                     (chain->lib_desc.file_fns[j].fn[token_length]==0) )
                {
                    *fnd_chain = chain;
                    return chain->lib_desc.file_fns+j;
                }
            }
        }
    }
    return NULL;
}

/*f parse_comma_separated_arguments
  Parse a set of comman separated arguments
  Return 1 if no args are given and the required type indicates no more arguments can be supplied
  Return 0 if the required type of a command is not given, but an argument exists
  Return 0 if the required type of a command is given but no argument is given
  Parse an argument otherwise, up to the next comma if given, and then recurse
 */
static int parse_comma_separated_arguments( t_sl_exec_file_data *file_data, int line_number, char *token, int token_length, t_sl_exec_file_value *args, const char *types, const char *syntax )
{
     int i;
     int first_token_length;
     int next_token_start;

     if (types[0]==0)
     {
          if (token_length!=0)
          {
               fprintf(stderr,"Unexpected characters when all expected arguments complete, rest is  '%s', syntax :%s\n", token, syntax );
               return 0;
          }
          return 1;
     }
     if ( (token_length==0) || (token[0]==',') )
     {
          fprintf(stderr,"Expected more arguments (or null argument when one expected - syntax %s\n", syntax );
          return 0;
     }
     for (i=0; i<token_length; i++)
     {
          if (token[i]==',')
          {
               break;
          }
     }
     if (i==token_length)
     {
          first_token_length = token_length;
          next_token_start = token_length;
     }
     else
     {
          first_token_length = i;
          next_token_start = i+1;
     }
     //printf("Parse argument %s %d %d type %c\n", token, first_token_length, next_token_start,types[0] );
     if (!parse_argument( file_data, line_number, token, first_token_length, args+0, types[0], syntax ))
          return 0;
     return parse_comma_separated_arguments( file_data, line_number, token+next_token_start, token_length-next_token_start, args+1, types+1, syntax );
}

/*f parse_variable_or_function
 */
static int parse_variable_or_function( t_sl_exec_file_data *file_data, int line_number, char *token, int token_length, t_sl_exec_file_value *arg, t_sl_exec_file_value_type type )
{
    int i, bra, ket;
    t_sl_exec_file_fn *fn;
    t_sl_exec_file_lib_chain *lib;
    t_sl_exec_file_method *method;
    t_sl_exec_file_object_chain *object;
    t_sl_exec_file_variable *var;
    const char *reqd_args;
    const char *syntax;

    bra=-1; ket=-1;
    for (i=0; i<token_length; i++)
    {
        if ((token[i]=='(') && (bra==-1))
        {
            bra = i;
        }
        if (token[i]==')')
        {
            ket = i;
        }
    }
    if (bra>=0)
    {
        char result;
        if (ket!=token_length-1)
        {
            fprintf(stderr, "NNE:Mismatched brackets stuff at line %d\n",line_number+1);
            //mismatched or other bad bracket stuff;
            return 0;
        }
        method = find_method_in_objects( file_data, token, bra, &object );
        fn = NULL;
        if (!method)
            fn = find_function_in_libraries( file_data, token, bra, &lib );
        if (!fn && !method)
        {
            fprintf(stderr, "NNE:Bad function '%s' at line %d\n", token, line_number+1);
            //bad function
            return 0;
        }
        result = method ?method->result:fn->result;
        if ( ((result=='s') && (type!=sl_exec_file_value_type_string) && (type!=sl_exec_file_value_type_any)) ||
             ((result=='d') && (type!=sl_exec_file_value_type_double) && (type!=sl_exec_file_value_type_integer_or_double) && (type!=sl_exec_file_value_type_any)) ||
             ((result=='i') && (type!=sl_exec_file_value_type_integer) && (type!=sl_exec_file_value_type_integer_or_double) && (type!=sl_exec_file_value_type_any)) )
        {
            fprintf(stderr, "NNE:Bad return value at line %d\n", line_number+1);
            //bad function
            return 0;
        }

        arg->fn = (t_sl_exec_file_fn_instance *)malloc(sizeof(t_sl_exec_file_fn_instance));
        if (!arg->fn)
        {
            fprintf(stderr, "NNE:Malloc failed\n");
            return 0;
        }
        if (method)
        {
            arg->fn->is_fn = 0;
            arg->fn->call.object.method = method;
            arg->fn->call.object.object = object;
            reqd_args = method->args;
            syntax = method->syntax;
        }
        else
        {
            arg->fn->is_fn = 1;
            arg->fn->call.fn.fn = fn;
            arg->fn->call.fn.lib = lib;
            reqd_args = fn->args;
            syntax = fn->syntax;
        }
        arg->fn->num_args = strlen(reqd_args);
        if (result=='s')
            arg->type = sl_exec_file_value_type_string;
        else if (result=='d')
            arg->type = sl_exec_file_value_type_double;
        else
            arg->type = sl_exec_file_value_type_integer;

        for (i=0; i<SL_EXEC_FILE_MAX_FN_ARGS; i++)
        {
            arg->fn->args[i].p.ptr = NULL;
            arg->fn->args[i].eval = sl_exec_file_arg_eval_no;
        }
        arg->eval = sl_exec_file_arg_eval_function;
        return parse_comma_separated_arguments( file_data, line_number, token+bra+1, token_length-bra-2, arg->fn->args, reqd_args, syntax );
    }
    if (ket>=0)
    {
        fprintf(stderr, "NNE:Mismatched brackets stuff\n");
        //mismatched;
        return 0;
    }

    arg->p.string = NULL;
    arg->variable_name = NULL;
    var=sl_exec_file_find_variable( file_data, token, token_length );
    if ((type==sl_exec_file_value_type_any) && (var))
    {
        arg->variable_name = sl_str_alloc_copy( token, token_length );
        if (var->type==sl_exec_file_variable_type_integer)
        {
            arg->eval = sl_exec_file_arg_eval_integer_variable;
            arg->type = sl_exec_file_value_type_integer;
            return 1;
        }
        else if (var->type==sl_exec_file_variable_type_double)
        {
            arg->eval = sl_exec_file_arg_eval_double_variable;
            arg->type = sl_exec_file_value_type_double;
            return 1;
        }
        else
        {
            arg->eval = sl_exec_file_arg_eval_string_variable;
            arg->type = sl_exec_file_value_type_string;
        }
        return 1;
    }

    if ((type==sl_exec_file_value_type_integer_or_double) && (var))
    {
        arg->variable_name = sl_str_alloc_copy( token, token_length );
        if (var->type==sl_exec_file_variable_type_integer)
        {
            arg->eval = sl_exec_file_arg_eval_integer_variable;
            arg->type = sl_exec_file_value_type_integer;
            return 1;
        }
        else if (var->type==sl_exec_file_variable_type_double)
        {
            arg->eval = sl_exec_file_arg_eval_double_variable;
            arg->type = sl_exec_file_value_type_double;
            return 1;
        }
        else
        {
            arg->eval = sl_exec_file_arg_eval_string_variable;
            arg->type = sl_exec_file_value_type_string;
        }
        return 1;
    }

    if (type==sl_exec_file_value_type_string)
    {
        if (var)
        {
            arg->eval = sl_exec_file_arg_eval_string_variable;
            arg->variable_name = sl_str_alloc_copy( token, token_length );
            arg->p.string = NULL;
        }
        else
        {
             arg->p.string = sl_str_alloc_copy( token, token_length );
             arg->eval = sl_exec_file_arg_eval_no;
        }
        arg->type = sl_exec_file_value_type_string;
        return 1;
    }

    if (type==sl_exec_file_value_type_double)
    {
        arg->variable_name = sl_str_alloc_copy( token, token_length );
        arg->eval = sl_exec_file_arg_eval_double_variable;
        arg->type = sl_exec_file_value_type_double;
        return 1;
    }

    arg->variable_name = sl_str_alloc_copy( token, token_length );
    arg->eval = sl_exec_file_arg_eval_integer_variable;
    arg->type = sl_exec_file_value_type_integer;
    return 1;
}

/*f parse_argument
 */
static int parse_argument( t_sl_exec_file_data *file_data, int line_number, char *token, int token_length, t_sl_exec_file_value *arg, char type, const char *syntax )
{
     t_sl_exec_file_value *args;
     int k;

     arg->type = sl_exec_file_value_type_none;
     switch (type)
     {
     case 'v': // variable for assignment
          arg->p.string = sl_str_alloc_copy( token, token_length );
          arg->type = sl_exec_file_value_type_variable;
          break;
     case 'l': // label for branching, gosub, spawn, etc
          for (k=0; k<file_data->number_lines; k++)
          {
               args = file_data->lines[k].args;
               if ( (file_data->lines[k].type == sl_exec_file_line_type_label) &&
                    (!strncmp(token,args[0].p.string, token_length)) &&
                    ((args[0].p.string[token_length]==0)) )
               {
                    break;
               }
          }
          if (k>=file_data->number_lines)
          {
               file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_unknown_label, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                   error_arg_type_malloc_string, token,
                                                   error_arg_type_malloc_filename, file_data->filename,
                                                   error_arg_type_line_number, line_number+1,
                                                   error_arg_type_none );
               return 0;
          }
          arg->integer = k;
          arg->type = sl_exec_file_value_type_label;
          break;
     case 'i': // integer, variable use, or function - no spaces allowed, though
          if (isalpha(token[0]))
          {
               if (!parse_variable_or_function( file_data, line_number, token, token_length, arg, sl_exec_file_value_type_integer ))
                    return 0;
          }
          else if (sl_integer_from_token( token, &(arg->integer) ))
          {
               arg->p.ptr = NULL;
               arg->eval = sl_exec_file_arg_eval_no;
               arg->type = sl_exec_file_value_type_integer;
          }
          else 
          {
               file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_syntax, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                   error_arg_type_const_string, syntax,
                                                   error_arg_type_malloc_filename, file_data->filename,
                                                   error_arg_type_line_number, line_number+1,
                                                   error_arg_type_none );
               arg->integer = 0;
               return 0;
          }
          break;
     case 'd': // double, variable use, or function - no spaces allowed, though
          if (isalpha(token[0]))
          {
               if (!parse_variable_or_function( file_data, line_number, token, token_length, arg, sl_exec_file_value_type_double ))
                    return 0;
          }
          else if (sl_double_from_token( token, &(arg->real) ))
          {
               arg->p.ptr = NULL;
               arg->eval = sl_exec_file_arg_eval_no;
               arg->type = sl_exec_file_value_type_double;
          }
          else 
          {
               file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_syntax, error_id_sl_exec_file_allocate_and_read_exec_file,
                                            error_arg_type_const_string, syntax,
                                            error_arg_type_malloc_filename, file_data->filename,
                                            error_arg_type_line_number, line_number+1,
                                            error_arg_type_none );
               arg->real = 0;
               arg->integer = 0;
               return 0;
          }
          break;
     case 'I': // integer or double, variable use, or function - no spaces allowed, though
          if (isalpha(token[0]))
          {
               if (!parse_variable_or_function( file_data, line_number, token, token_length, arg, sl_exec_file_value_type_integer_or_double ))
                    return 0;
          }
          else if (strchr(token,'.') && sl_double_from_token( token, &(arg->real) ))
          {
               arg->p.ptr = NULL;
               arg->eval = sl_exec_file_arg_eval_no;
               arg->type = sl_exec_file_value_type_double;
               arg->integer = (t_sl_uint64)arg->real;
          }
          else if (sl_integer_from_token( token, &(arg->integer) ))
          {
               arg->p.ptr = NULL;
               arg->eval = sl_exec_file_arg_eval_no;
               arg->type = sl_exec_file_value_type_integer;
               arg->real = (double)arg->integer;
          }
          else 
          {
               file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_syntax, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                   error_arg_type_const_string, syntax,
                                                   error_arg_type_malloc_filename, file_data->filename,
                                                   error_arg_type_line_number, line_number+1,
                                                   error_arg_type_none );
               arg->integer = 0;
               arg->real = 0;
               return 0;
          }
          break;
     case 's': // string - quoted string, function, or unquoted string (no plain variables)
          arg->type = sl_exec_file_value_type_string;
          if ( (token_length>=2) &&
                    (token[0]=='"') &&
                    (token[token_length-1]=='"') )
          {
               arg->p.string = sl_str_alloc_copy( token+1, token_length-2 );
          }
          else
          {
               if (!parse_variable_or_function( file_data, line_number, token, token_length, arg, sl_exec_file_value_type_string ))
                    return 0;
          }
          break;
     case 'x': // integer or string - quoted string, integer, double, function, or variable use
          if ( (token_length>=2) &&
                    (token[0]=='"') &&
                    (token[token_length-1]=='"') )
          {
               arg->type = sl_exec_file_value_type_string;
               arg->p.string = sl_str_alloc_copy( token+1, token_length-2 );
          }
          else if (isalpha(token[0]))
          {
               if (!parse_variable_or_function( file_data, line_number, token, token_length, arg, sl_exec_file_value_type_any ))
                    return 0;
          }
          else if (strchr(token,'.') && sl_double_from_token( token, &(arg->real) ))
          {
               arg->p.ptr = NULL;
               arg->eval = sl_exec_file_arg_eval_no;
               arg->type = sl_exec_file_value_type_double;
          }
          else if (sl_integer_from_token( token, &(arg->integer) ))
          {
               arg->p.ptr = NULL;
               arg->eval = sl_exec_file_arg_eval_no;
               arg->type = sl_exec_file_value_type_integer;
          }
          else
          {
               fprintf(stderr, "NNE:Well that went wrong as an argument didn't it\n");
               return 0;
          }
          break;
     }
     return 1;
}

/*f parse_line
  return 0 if the line is complete (error, or comment line)
  return 1 if the line could be a declarative command (int, event, etc)
 */
static int parse_line( struct t_sl_exec_file_data *file_data, char *text, int line_number )
{
     char *line, *token, *line_end;
     t_sl_exec_file_line *file_line;
     int j;
     const char *syntax;
     t_sl_exec_file_cmd *line_cmd;
     t_sl_exec_file_lib_chain *fnd_lib;
     t_sl_exec_file_method *line_method;
     t_sl_exec_file_object_chain *fnd_object;

     WHERE_I_AM;

     /*b Get pointer to the line - improves readablity
      */
     file_line = &(file_data->lines[line_number]);

     /*b Skip lines with labels
      */
     if (file_line->type == sl_exec_file_line_type_label)
          return 0;

     /*b Get first token on the line - skip blank or comment lines
      */
     line = text+file_line->file_pos;
     line_end = line+strlen(line);
     token = sl_token_next( 0, line, line_end ); // changes the 'line' - but that's okay - inserts a null at end of string
     if ( (!token) ||
          (token[0]==';') ||
          (token[0]=='#') ||
          ( (token[0]=='/') && (token[1]=='/') ) )
     {
          return 0;
     }

     /*b Determine the command specified on this line
      */
     fnd_lib = NULL;
     fnd_object = NULL;
     line_method = NULL;
     line_cmd = NULL;
     line_method = find_method_in_objects( file_data, token, strlen(token), &fnd_object );
     if (!line_method)
         line_cmd = find_command_in_libraries( file_data, token, &fnd_lib );
     if (!line_method && !line_cmd)
     {
          file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_unknown_command, error_id_sl_exec_file_allocate_and_read_exec_file,
                                              error_arg_type_malloc_string, token,
                                              error_arg_type_malloc_filename, file_data->filename,
                                              error_arg_type_line_number, line_number+1,
                                              error_arg_type_none );
          return 0;
     }

     /*b Parse arguments for the command on the line
      */
     syntax = line_method ? (line_method->syntax):(line_cmd->syntax);
     {
         const char *reqd_args;
         reqd_args = line_method ? (line_method->args):(line_cmd->args);
         for (j=0;j<SL_EXEC_FILE_MAX_CMD_ARGS;j++)
         {
             file_line->args[j].type = sl_exec_file_value_type_none;
         }
         j=0;
         token=sl_token_next( 1, token, line_end);
         while ( (j<SL_EXEC_FILE_MAX_CMD_ARGS) && (token) && (reqd_args[j]) )
         {
             if (!parse_argument( file_data, line_number, token, strlen(token), &(file_line->args[j]), reqd_args[j], syntax ))
             {
                 return 0;
             }
             j++;
             token=sl_token_next( 1, token, line_end);
         }
     }

     /*b Check correct number of arguments given
      */
     if (token) // left over tokens means too many args given
     {
          file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_syntax, error_id_sl_exec_file_allocate_and_read_exec_file,
                                              error_arg_type_const_string, syntax,
                                              error_arg_type_malloc_filename, file_data->filename,
                                              error_arg_type_line_number, line_number+1,
                                              error_arg_type_none );
          return 0;
     }
     if ( (line_cmd && (j<line_cmd->min_args)) || // Too few means too few...
          (line_method && (j<line_method->min_args)) )
     {
          file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_syntax, error_id_sl_exec_file_allocate_and_read_exec_file,
                                              error_arg_type_const_string, syntax,
                                              error_arg_type_malloc_filename, file_data->filename,
                                              error_arg_type_line_number, line_number+1,
                                              error_arg_type_none );
          return 0;
     }

     /*b Fill line command correctly now everything is set
      */
     if (line_cmd)
     {
         file_line->type = sl_exec_file_line_type_cmd;
         file_line->cmd_type = (line_cmd->cmd[0]=='!')?cmd_type_declarative:cmd_type_executive;
         file_line->cmd = line_cmd->cmd_value + fnd_lib->base;
         file_line->call.cmd.cmd = line_cmd;
         file_line->call.cmd.lib = fnd_lib;
     }
     else
     {
         file_line->type = sl_exec_file_line_type_method;
         file_line->cmd_type = (line_method->method[0]=='!')?cmd_type_declarative:cmd_type_executive;
         file_line->call.object.method = line_method;
         file_line->call.object.object = fnd_object;
     }
     file_line->num_args = j;
     return 1;
}

/*f handle_declarative_command
 */
static void handle_declarative_command( struct t_sl_exec_file_data *file_data, int line_number )
{
     t_sl_exec_file_variable *var;
     t_sl_exec_file_line *file_line;

     WHERE_I_AM;

     /*b Get pointer to the line - improves readablity
      */
     file_line = &(file_data->lines[line_number]);

     /*b Interpret a command, if declarative
      */
     if (file_line->cmd_type==cmd_type_declarative)
     {
         switch (file_line->cmd)
         {
         case cmd_string:
             var = sl_exec_file_create_variable( file_data, file_line->args[0].p.string, sl_exec_file_variable_type_string );
             SL_DEBUG( sl_debug_level_info,
                       "file %s string variable %s", file_data->filename, var->name ) ;
             break;
         case cmd_int:
             var = sl_exec_file_create_variable( file_data, file_line->args[0].p.string, sl_exec_file_variable_type_integer );
             if (file_data->register_state_fn)
             {
                 (*(file_data->register_state_fn))( file_data->register_state_handle, file_data->id, var->name, 0, (int *)(var->integer), 32 );
             }
             SL_DEBUG( sl_debug_level_info,
                       "file %s integer variable %s", file_data->filename, var->name ) ;
             break;
         case cmd_double:
             var = sl_exec_file_create_variable( file_data, file_line->args[0].p.string, sl_exec_file_variable_type_double );
             SL_DEBUG( sl_debug_level_info,
                       "file %s double variable %s", file_data->filename, var->name ) ;
             break;
         default:
         {
             t_sl_exec_file_lib_chain *lib;
             t_sl_exec_file_cmd_cb cmd_cb;
             cmd_cb.cmd = file_line->cmd;
             lib = find_command_in_library( file_data, cmd_cb.cmd );
             if (lib && lib->lib_desc.cmd_handler)
             {
                 t_sl_error_level err;
                 cmd_cb.file_data = file_data;
                 cmd_cb.error = file_data->error;
                 cmd_cb.filename = file_data->filename;
                 cmd_cb.line_number = line_number;
                 cmd_cb.cmd -= lib->base;
                 cmd_cb.lib_desc = &(lib->lib_desc);
                 cmd_cb.num_args = file_line->num_args;
                 cmd_cb.args = file_line->args;
                 err = lib->lib_desc.cmd_handler( &cmd_cb, lib->lib_desc.handle );
             }
             break;
         }
         }
     }
}

/*f sl_exec_file_data_init
 */
static void sl_exec_file_data_init( t_sl_exec_file_data *file_data,
                                    c_sl_error *error,
                                    c_sl_error *message,
                                    const char *id,
                                    const char *user )
{
     file_data->during_init = 1;
     file_data->number_threads = 0;

     file_data->error = error;
     file_data->message = message;
     file_data->id = sl_str_alloc_copy( id );
     file_data->filename = NULL;
     file_data->py_object = NULL;
     file_data->user = sl_str_alloc_copy( user );

     file_data->completion.finished = 0;
     file_data->completion.passed = 0;
     file_data->completion.failed = 0;
     file_data->completion.return_code = 0;

     file_data->register_state_fn = NULL;
     file_data->poststate_callbacks = NULL;

     file_data->env_fn = NULL;
     file_data->env_handle = NULL;
     file_data->lib_chain = NULL;

     file_data->variables = NULL;
     file_data->object_chain = NULL;

     file_data->threads = NULL;

     file_data->number_lines = 0;
     file_data->next_base = 0;

}

/*f sl_exec_file_data_add_default_cmds
 */
static void sl_exec_file_data_add_default_cmds( t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_lib_desc lib_desc;

    lib_desc.version = sl_ef_lib_version_cmdcb;
    lib_desc.library_name = "sys";
    lib_desc.handle = 0;
    lib_desc.cmd_handler = NULL;
    lib_desc.file_cmds = internal_cmds;
    lib_desc.file_fns = internal_fns;
    sl_exec_file_add_library( file_data, &lib_desc );

    sl_ef_lib_random_add_exec_file_enhancements( file_data );
    sl_ef_lib_memory_add_exec_file_enhancements( file_data );
    sl_ef_lib_fifo_add_exec_file_enhancements( file_data );
    sl_ef_lib_event_add_exec_file_enhancements( file_data );
}

/*f sl_exec_file_allocate_and_read_exec_file
  opens the file
  reads it in to a temporary buffer
  counts the lines
  allocates an execution structure given the number of lines
  parses each line - could be a label, a goto, or a given command style, or a comment, or an error
  stores parse data for each line
  resets execution pointer
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
                                                                  t_sl_exec_file_fn *file_fns )
{
     char *text, *line;
     t_sl_error_level result;
     int i, j;
     int nlines;
     t_sl_exec_file_line *file_line;

     WHERE_I_AM;

     /*b Read in the file
      */
     text=NULL;
     result = sl_allocate_and_read_file( error, filename, &text, user );
     if (result!=error_level_okay)
          return result;

     /*b Count the lines in the file
      */
     nlines=1;
     for (i=0; text[i]!=0; i++)
     {
          if (text[i]=='\n')
          {
               nlines++;
          }
     }

     /*b Allocate structure for the file data
      */
     *file_data_ptr = (t_sl_exec_file_data *)malloc(sizeof(t_sl_exec_file_data) + nlines*sizeof(t_sl_exec_file_line) );
     if (!*file_data_ptr)
     {
          return error->add_error( (void *)user, error_level_fatal, error_number_general_malloc_failed, error_id_sl_exec_file_allocate_and_read_exec_file, error_arg_type_malloc_string, filename, error_arg_type_none );
     }
     sl_exec_file_data_init( *file_data_ptr, error, message, id, user );
     (*file_data_ptr)->filename = sl_str_alloc_copy(filename);
     (*file_data_ptr)->number_lines = nlines;

     /*b Add default commands and functions
      */
     sl_exec_file_data_add_default_cmds( *file_data_ptr );

     /*b Add User commands and functions - deprecated (should use a library)
      */
     //if (file_cmds) sl_exec_file_add_file_commands( *file_data_ptr, file_cmds );
     //if (file_fns) sl_exec_file_add_file_functions( *file_data_ptr, file_fns, callback_handle ); // default to callback_handle

     /*b Invoke callback to fill in extra functions and add other libraries; may also declare objects
      */
     if (callback_fn)
     {
          callback_fn( callback_handle, *file_data_ptr );
     }

     /*b Fill structure for each line in the file, and convert whitespace to ' '
      */
     nlines=0;
     (*file_data_ptr)->lines[nlines].type = sl_exec_file_line_type_none;
     (*file_data_ptr)->lines[nlines].file_pos = 0;
     for (i=0; text[i]!=0; i++)
     {
          if (text[i]=='\n')
          {
               nlines++;
               (*file_data_ptr)->lines[nlines].type = sl_exec_file_line_type_none;
               (*file_data_ptr)->lines[nlines].file_pos = i+1;
               text[i] = 0;
          }
          if ( (text[i]=='\t') ||
               (text[i]=='\r') )
          {
               text[i] = ' ';
          }
     }
     nlines++;

     /*b Fill outline of structure for each line - in particular, mark labels here as well
      */
     for (i=0; i<nlines; i++)
     {
          file_line = &((*file_data_ptr)->lines[i]);
          for (j=0; j<SL_EXEC_FILE_MAX_CMD_ARGS; j++)
          {
               file_line->args[j].p.ptr = NULL;
               file_line->args[j].eval = sl_exec_file_arg_eval_no;
          }
          line = text+file_line->file_pos;
          for (j=0; isalnum(line[j]) || (line[j]=='_'); j++);
          if ((j>0) && (line[j]==':'))
          {
               file_line->type = sl_exec_file_line_type_label;
               line[j]=0;
               file_line->args[0].p.string = sl_str_alloc_copy( line );
               SL_DEBUG( sl_debug_level_info,
                        "file %s label %s:%d", filename, line, j ) ;
               continue;
          }
     }

     /*b Parse every line in the file
      */
     for (i=0; i<nlines; i++)
     {
          file_line = &((*file_data_ptr)->lines[i]);

          /*b Parse the line and build command instance
           */
          if (!parse_line( *file_data_ptr, text, i ))
               continue;

          /*b Handle declarative command - object instances
           */
          handle_declarative_command( *file_data_ptr, i );
     }

     /*b Finish registering state
      */
     {
         t_sl_exec_file_callback *efc;
         for (efc=(*file_data_ptr)->poststate_callbacks; efc; efc=efc->next_in_list)
         {
             (efc->callback_fn)( efc->handle );
         }
     }

     /*b Free text buffer, reset the threads,  and return
      */
     free(text);
     sl_exec_file_reset( *file_data_ptr );
     SL_DEBUG( sl_debug_level_info,
              "file %s complete",filename ) ;

     return error_level_okay;
}

/*f sl_exec_file_free
  frees the data structure if not NULL
 */
extern void sl_exec_file_free( t_sl_exec_file_data *file_data )
{
     t_sl_exec_file_variable *var, *next_var;
     int i, j;

     WHERE_I_AM;

     if (file_data)
     {
          free(file_data->id);
          if (file_data->filename)
              free(file_data->filename);
          if (file_data->py_object)
          {
              WHERE_I_AM;
              Py_DECREF(file_data->py_object);
          }
          free(file_data->user);
          for (i=0; i<file_data->number_lines; i++) // number_lines==0 for Python
          {
               for (j=0; j<SL_EXEC_FILE_MAX_CMD_ARGS; j++)
               {
                    if (file_data->lines[i].args[j].p.ptr)
                    {
                         free(file_data->lines[i].args[j].p.ptr);
                    }
               }
          }
          for (var = file_data->variables; var; var=next_var)
          {
               next_var = var->next_in_list;
               free(var);
          }
          {
              t_sl_exec_file_object_chain *obj, *next_obj;
              for (obj=file_data->object_chain; obj; obj=next_obj)
              {
                  if (obj->object_desc.free_fn)
                  {
                      t_sl_exec_file_object_cb obj_cb;
                      obj_cb.object_handle = (void *)obj;
                      obj_cb.object_desc = &(obj->object_desc);
                      obj->object_desc.free_fn(&obj_cb);
                  }
                  next_obj=obj->next_in_list;
                  free(obj);
              }
          }
          free(file_data);
     }
     WHERE_I_AM;
}

/*f sl_exec_file_reset
 */
static void sl_exec_file_py_reset( t_sl_exec_file_data *file_data );
static int sl_exec_file_py_despatch( t_sl_exec_file_data *file_data );
extern void sl_exec_file_reset( struct t_sl_exec_file_data *file_data )
{
    WHERE_I_AM;
    if (file_data->filename)
    {
        sl_exec_file_free_threads( file_data );
        sl_exec_file_create_thread( file_data, "main", SL_EXEC_FILE_MAX_STACK_DEPTH, 0 );
    }
    file_data->completion.finished = 0;
    file_data->completion.passed = 0;
    file_data->completion.failed = 0;
    file_data->completion.return_code = 0;
    {
        t_sl_exec_file_object_chain *obj;
        for (obj=file_data->object_chain; obj; obj=obj->next_in_list)
        {
            if (obj->object_desc.reset_fn)
            {
                t_sl_exec_file_object_cb obj_cb;
                obj_cb.object_handle = (void *)obj;
                obj_cb.object_desc = &(obj->object_desc);
                obj->object_desc.reset_fn(&obj_cb);
            }
        }
    }

    if (file_data->py_object)
    {
        sl_exec_file_py_reset( file_data );
    }
}

/*a Stack frame handling
 */
/*f sl_exec_file_stack_entry_read
 */
static int sl_exec_file_stack_entry_read( t_sl_exec_file_data *file_data, t_sl_exec_file_thread_data *thread, t_sl_exec_file_stack_frame_type type, char *variable, int *line_number, t_sl_uint64 *increment, t_sl_uint64 *last_value, const char *reason )
{
     if (thread->stack_ptr<=0)
     {
          file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_exec_file_stack_underflow, error_id_sl_exec_file_stack_entry_read,
                                       error_arg_type_malloc_string, reason,
                                       error_arg_type_malloc_string, thread->id,
                                       error_arg_type_malloc_filename, file_data->filename,
                                       error_arg_type_line_number, thread->line_number,
                                       error_arg_type_none );
          return 0;
     }

     if ( (thread->stack[ thread->stack_ptr-1 ].type != type) ||
          (thread->stack[ thread->stack_ptr-1 ].variable && !variable) ||
          (!thread->stack[ thread->stack_ptr-1 ].variable && variable) ||
          (variable && strcmp(thread->stack[ thread->stack_ptr-1 ].variable,variable)) )
     {
          file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_exec_file_stack_item_mismatch, error_id_sl_exec_file_stack_entry_read,
                                       error_arg_type_malloc_string, reason,
                                       error_arg_type_malloc_string, thread->id,
                                       error_arg_type_malloc_filename, file_data->filename,
                                       error_arg_type_line_number, thread->line_number,
                                       error_arg_type_none );
          return 0;
     }
     if (line_number)
          *line_number = thread->stack[ thread->stack_ptr-1 ].line_number;
     if (increment)
          *increment = thread->stack[ thread->stack_ptr-1 ].increment;
     if (last_value)
          *last_value = thread->stack[ thread->stack_ptr-1 ].last_value;

     return 1;
}

/*f sl_exec_file_stack_entry_push
 */
static int sl_exec_file_stack_entry_push( t_sl_exec_file_data *file_data, t_sl_exec_file_thread_data *thread, t_sl_exec_file_stack_frame_type type, char *variable, int line_number, int increment, int last_value, const char *reason )
{
     if (thread->stack_ptr>=thread->stack_size)
     {
          file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_exec_file_stack_overflow, error_id_sl_exec_file_stack_entry_push,
                                       error_arg_type_integer, thread->stack_size,
                                       error_arg_type_malloc_string, reason,
                                       error_arg_type_malloc_string, thread->id,
                                       error_arg_type_malloc_filename, file_data->filename,
                                       error_arg_type_line_number, thread->line_number,
                                       error_arg_type_none );
          return 0;
     }
     thread->stack[ thread->stack_ptr ].type = type;
     thread->stack[ thread->stack_ptr ].variable = variable;
     thread->stack[ thread->stack_ptr ].line_number = line_number;
     thread->stack[ thread->stack_ptr ].increment = increment;
     thread->stack[ thread->stack_ptr ].last_value = last_value;
     thread->stack_ptr++;
     return 1;
}

/*f sl_exec_file_stack_entry_pop
 */
static int sl_exec_file_stack_entry_pop( t_sl_exec_file_data *file_data, t_sl_exec_file_thread_data *thread, t_sl_exec_file_stack_frame_type type, char *variable, int *line_number, t_sl_uint64 *increment, t_sl_uint64 *last_value, const char *reason )
{
     if (!sl_exec_file_stack_entry_read( file_data, thread, type, variable, line_number, increment, last_value, reason ))
          return 0;
     thread->stack_ptr--;
     return 1;
}

/*a Execution handling
 */
/*f sl_exec_file_evaluate_arguments
 */
extern int sl_exec_file_evaluate_arguments( t_sl_exec_file_data *file_data, const char *thread_id, int line_number, t_sl_exec_file_value *args, int max_args )
{
     int j;
     t_sl_uint64 *var;
     double *var_d;
     char *text;

     WHERE_I_AM;

     for (j=0; j<max_args; j++)
     {
          switch (args[j].eval)
          {
          case sl_exec_file_arg_eval_no:
               break;
          case sl_exec_file_arg_eval_integer_variable:
               var = sl_exec_file_get_integer_variable( file_data, args[j].variable_name );
               if (!var)
               {
                    file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_unknown_variable, error_id_sl_exec_file_get_next_cmd,
                                                 error_arg_type_const_string, args[j].variable_name,
                                                 error_arg_type_malloc_string, thread_id,
                                                 error_arg_type_malloc_filename, file_data->filename,
                                                 error_arg_type_line_number, line_number+1,
                                                 error_arg_type_none );
                    return 0;
               }
               args[j].integer = var[0];
               args[j].real = (double)var[0];
               break;
          case sl_exec_file_arg_eval_double_variable:
               var_d = sl_exec_file_get_double_variable( file_data, args[j].variable_name );
               if (!var_d)
               {
                    file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_unknown_variable, error_id_sl_exec_file_get_next_cmd,
                                                 error_arg_type_const_string, args[j].variable_name,
                                                 error_arg_type_malloc_string, thread_id,
                                                 error_arg_type_malloc_filename, file_data->filename,
                                                 error_arg_type_line_number, line_number+1,
                                                 error_arg_type_none );
                    return 0;
               }
               args[j].real = var_d[0];
               args[j].integer = (int)var_d[0];
               break;
          case sl_exec_file_arg_eval_string_variable:
               text = sl_exec_file_get_string_variable( file_data, args[j].variable_name );
               if (!text)
               {
                    file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_unknown_variable, error_id_sl_exec_file_get_next_cmd,
                                                 error_arg_type_const_string, args[j].variable_name,
                                                 error_arg_type_malloc_string, thread_id,
                                                 error_arg_type_malloc_filename, file_data->filename,
                                                 error_arg_type_line_number, line_number+1,
                                                 error_arg_type_none );
                    return 0;
               }
               args[j].p.string = sl_str_alloc_copy( text ); // because we always free, we need to always malloc
               break;
          case sl_exec_file_arg_eval_function:
               if (!sl_exec_file_evaluate_arguments( file_data, thread_id, line_number, args[j].fn->args, SL_EXEC_FILE_MAX_FN_ARGS ))
               {
                    return 0;
               }
               if (args[j].p.ptr)
               {
                    free(args[j].p.ptr);
               }
               args[j].p.ptr = NULL;
               file_data->eval_fn_result = &args[j];
               file_data->eval_fn_result_ok = 0;
               if (args[j].fn->is_fn)
               {
                   if ( !(args[j].fn->call.fn.fn->eval_fn)( args[j].fn->call.fn.lib->lib_desc.handle, file_data, args[j].fn->args ) )
                   {
                       file_data->eval_fn_result_ok = 0;
                   }
               }
               else
               {
                   t_sl_exec_file_cmd_cb cmd_cb;
                   cmd_cb.file_data = file_data;
                   cmd_cb.error = file_data->error;
                   cmd_cb.filename = file_data->filename;
                   cmd_cb.line_number = line_number;
                   cmd_cb.num_args = args[j].fn->num_args;
                   cmd_cb.cmd = 0;
                   cmd_cb.lib_desc = NULL;
                   cmd_cb.args = args[j].fn->args;
                   if ( (args[j].fn->call.object.method->method_fn)( &cmd_cb,
                                                                     (void *)(args[j].fn->call.object.object),
                                                                     &(args[j].fn->call.object.object->object_desc),
                                                                     args[j].fn->call.object.method)!=error_level_okay )
                   {
                       file_data->eval_fn_result_ok = 0;
                   }
               }
               if (!file_data->eval_fn_result_ok)
               {
                   file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_exec_file_bad_function_evaluation, error_id_sl_exec_file_get_next_cmd,
                                                error_arg_type_malloc_string, args[j].fn->is_fn? args[j].fn->call.fn.fn->fn:args[j].fn->call.object.method->method,
                                                error_arg_type_malloc_string, thread_id,
                                                error_arg_type_malloc_filename, file_data->filename,
                                                error_arg_type_line_number, line_number+1,
                                                error_arg_type_none );
                    return 0;
               }
               break;
          }
     }
     return 1;
}

/*f sl_exec_file_eval_fn_set_result( string )
 */
extern int sl_exec_file_eval_fn_set_result( t_sl_exec_file_data *file_data, const char *string, int copy_string )
{
    WHERE_I_AM;
    if (!file_data->eval_fn_result)
        return 0;
    if (!string)
        return 0;

    if (copy_string)
    {
        file_data->eval_fn_result->p.string = sl_str_alloc_copy( string );
        file_data->eval_fn_result_ok = 1;
        return 1;
    }
    file_data->eval_fn_result->p.string = (char *)string;	/* Only if copy_string is false */
    file_data->eval_fn_result_ok = 1;
    return 1;
}

/*f sl_exec_file_eval_fn_set_result( integer )
 */
extern int sl_exec_file_eval_fn_set_result( t_sl_exec_file_data *file_data, t_sl_uint64 result )
{
     WHERE_I_AM;
    if (!file_data->eval_fn_result)
        return 0;

    file_data->eval_fn_result->integer = result;
    file_data->eval_fn_result->real = (double) result;
    file_data->eval_fn_result_ok = 1;
    return 1;
}

/*f sl_exec_file_eval_fn_set_result( double )
 */ 
extern int sl_exec_file_eval_fn_set_result( t_sl_exec_file_data *file_data, double result )
{
     WHERE_I_AM;
    if (!file_data->eval_fn_result)
        return 0;

    file_data->eval_fn_result->integer = (t_sl_uint64) result;
    file_data->eval_fn_result->real = result;
    file_data->eval_fn_result_ok = 1;
    return 1;
}

/*f sl_exec_file_eval_fn_get_argument_integer
 */
extern t_sl_uint64 sl_exec_file_eval_fn_get_argument_integer( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int number )
{
     WHERE_I_AM;
    return args[number].integer;
}

/*f sl_exec_file_eval_fn_get_argument_double
 */
extern double sl_exec_file_eval_fn_get_argument_double( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int number )
{
     WHERE_I_AM;
    return args[number].real;
}

/*f sl_exec_file_eval_fn_get_argument_string
 */
extern const char *sl_exec_file_eval_fn_get_argument_string( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int number )
{
     WHERE_I_AM;
    return args[number].p.string;
}

/*f sl_exec_file_get_number_of_arguments
 */
extern int sl_exec_file_get_number_of_arguments( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
     WHERE_I_AM;
    if ( (file_data) &&
         (file_data->command_from_thread) )
    {
        return file_data->command_from_thread->num_args;
    }

    return -1;
}

/*f sl_exec_file_thread_wait_on_callback
 */
extern int sl_exec_file_thread_wait_on_callback( t_sl_exec_file_cmd_cb *cmd_cb, t_sl_exec_file_wait_callback_fn callback, t_sl_exec_file_wait_cb *wait_cb )
{
     WHERE_I_AM;
     if (!cmd_cb)
          return 0;

     if (!cmd_cb->file_data->command_from_thread)
         return 0;

     cmd_cb->file_data->command_from_thread->execution.type = sl_exec_file_thread_execution_type_waiting;
     cmd_cb->file_data->command_from_thread->execution.wait_callback = callback;
     cmd_cb->file_data->command_from_thread->execution.wait_cb = *wait_cb;
     return 1;
}

/*f sl_exec_file_thread_execute
  Return 0 for no execution occurred, 1 if something did
 */
static int sl_exec_file_thread_execute( t_sl_exec_file_data *file_data, t_sl_exec_file_thread_data *thread )
{
     int i, loops;
     t_sl_uint64 res;
     double res_d;
     t_sl_uint64 increment, last_value;
     t_sl_uint64 *var;
     char *text;

     WHERE_I_AM;

     if ( (!file_data) || (!thread) || (thread->halted) || (thread->pending_command))
          return 0;

     switch (thread->execution.type)
     {
     case sl_exec_file_thread_execution_type_waiting:
         if (!thread->execution.wait_callback(&(thread->execution.wait_cb)))
             return 0;
         thread->execution.type = sl_exec_file_thread_execution_type_running;
         break;
     case sl_exec_file_thread_execution_type_running:
          break;
     }

     file_data->current_thread = thread;
     loops = 0;
     while ( (thread->execution.type == sl_exec_file_thread_execution_type_running) &&
             (loops<SL_EXEC_FILE_MAX_OPS_PER_CMD) )
     {
          if (thread->trace_level)
          {
               fprintf( stderr, "%s:%d:%s:trace executing\n", file_data->filename, thread->line_number+1, thread->id );
          }

          i=thread->line_number;

          if ((i<0) || (i>=file_data->number_lines))
          {
               file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_exec_file_end_of_file_reached, error_id_sl_exec_file_get_next_cmd,
                                            error_arg_type_malloc_string, thread->id,
                                            error_arg_type_malloc_filename, file_data->filename,
                                            error_arg_type_none );
               thread->halted = 1;
               return 1;
          }

          if (!sl_exec_file_evaluate_arguments( file_data, thread->id, i, file_data->lines[i].args, SL_EXEC_FILE_MAX_CMD_ARGS ))
          {
               thread->line_number++;
               continue;
          }

          switch (file_data->lines[ i ].type)
          {
          case sl_exec_file_line_type_none:
          case sl_exec_file_line_type_label:
               thread->line_number++;
               break;
          case sl_exec_file_line_type_method:
              if (file_data->lines[i].cmd_type==cmd_type_declarative)
              {
                  thread->line_number++;
              }
              else
              {
                  t_sl_exec_file_cmd_cb cmd_cb;
                  t_sl_error_level err;

                  cmd_cb.file_data = file_data;
                  cmd_cb.error = file_data->error;
                  cmd_cb.filename = file_data->filename;
                  cmd_cb.line_number = thread->line_number;
                  cmd_cb.cmd = 0;
                  cmd_cb.lib_desc = NULL;
                  cmd_cb.args = file_data->lines[i].args;
                  cmd_cb.num_args = file_data->lines[i].num_args;
                  file_data->command_from_thread = thread;
                  file_data->eval_fn_result = &file_data->cmd_result;
                  file_data->eval_fn_result_ok = 0;
                  err = (file_data->lines[i].call.object.method->method_fn)( &cmd_cb,
                                                                            (void *)(file_data->lines[i].call.object.object),
                                                                             &(file_data->lines[i].call.object.object->object_desc),

                                                                             file_data->lines[i].call.object.method);
                  if (err!=error_level_okay)
                  {
                      fprintf(stderr,"NNE: method handler '%s' returned an error at line %d\n", file_data->lines[i].call.object.method->method, thread->line_number+1);
                  }
                  thread->line_number++;
              }
              break;
          case sl_exec_file_line_type_cmd:
              if (file_data->lines[i].cmd_type==cmd_type_declarative)
              {
                  thread->line_number++;
              }
              else
              {
                  switch (file_data->lines[i].cmd)
                  {
                  case cmd_list:
                      if (!strcmp(file_data->lines[i].args[0].p.string, "libs"))
                      {
                          t_sl_exec_file_lib_chain *lib;
                          printf("sl_exec_file libraries for %s:\n", file_data->user );
                          for (lib=file_data->lib_chain; lib; lib=lib->next_in_list)
                          {
                              printf("\t%s\n", lib->lib_desc.library_name);
                          }
                      }
                      else if (!strcmp(file_data->lines[i].args[0].p.string, "fns"))
                      {
                          t_sl_exec_file_lib_chain *lib;
                          printf("sl_exec_file functions registered for %s:\n", file_data->user );
                          for (lib=file_data->lib_chain; lib; lib=lib->next_in_list)
                          {
                              int j;
                              for (j=0; lib->lib_desc.file_fns && lib->lib_desc.file_fns[j].fn_value!=sl_exec_file_fn_none; j++)
                              {
                                  printf("\t%s\n", lib->lib_desc.file_fns[j].fn);
                              }
                          }
                      }
                      else if (!strcmp(file_data->lines[i].args[0].p.string, "cmds"))
                      {
                          t_sl_exec_file_lib_chain *lib;
                          printf("sl_exec_file commands registered for %s:\n", file_data->user );
                          for (lib=file_data->lib_chain; lib; lib=lib->next_in_list)
                          {
                              int j;
                              for (j=0; lib->lib_desc.file_cmds && lib->lib_desc.file_cmds[j].cmd_value!=sl_exec_file_cmd_none; j++)
                              {
                                  printf("\t%s\n", lib->lib_desc.file_cmds[j].cmd);
                              }
                          }
                      }
                      else if (!strcmp(file_data->lines[i].args[0].p.string, "objs"))
                      {
                          t_sl_exec_file_object_chain *obj;
                          printf("sl_exec_file objects registered in %s:\n", file_data->user );
                          for (obj=file_data->object_chain; obj; obj=obj->next_in_list)
                          {
                              printf("\t%s\n", obj->object_desc.name);
                          }
                      }
                      else
                      {
                          t_sl_exec_file_lib_chain *lib;
                          for (lib=file_data->lib_chain; lib; lib=lib->next_in_list)
                          {
                              if (!strcmp(file_data->lines[i].args[0].p.string, lib->lib_desc.library_name))
                              {
                                  int j;
                                  printf("sl_exec_file library '%s' registered in %s:\n", lib->lib_desc.library_name, file_data->user );
                                  printf("\tCommands:\n");
                                  for (j=0; lib->lib_desc.file_cmds && lib->lib_desc.file_cmds[j].cmd_value!=sl_exec_file_cmd_none; j++)
                                  {
                                      printf("\t\t%s\n", lib->lib_desc.file_cmds[j].cmd);
                                  }
                                  printf("\tFunctions:\n");
                                  for (j=0; lib->lib_desc.file_fns && lib->lib_desc.file_fns[j].fn_value!=sl_exec_file_fn_none; j++)
                                  {
                                      printf("\t\t%s\n", lib->lib_desc.file_fns[j].fn);
                                  }
                              }
                          }
                          t_sl_exec_file_object_chain *obj;
                          for (obj=file_data->object_chain; obj; obj=obj->next_in_list)
                          {
                              if (!strcmp(file_data->lines[i].args[0].p.string, obj->object_desc.name))
                              {
                                  int j;
                                  printf("sl_exec_file object '%s' registered in %s:\n", obj->object_desc.name, file_data->user );
                                  printf("\tMethods:\n");
                                  for (j=0; obj->object_desc.methods[j].method; j++ )
                                  {
                                      printf("\t\t%s\n", obj->object_desc.methods[j].method);
                                  }
                              }
                          }
                      }
                      fflush(stdout);
                      thread->line_number++;
                      break;
                  case cmd_trace:
                      thread->trace_level = file_data->lines[i].args[0].integer;
                      thread->line_number++;
                      break;
                  case cmd_goto:
                      thread->line_number = file_data->lines[i].args[0].integer;
                      break;
                  case cmd_gosub:
                      if (!sl_exec_file_stack_entry_push( file_data, thread, sl_exec_file_stack_frame_type_gosub, NULL, i+1, 0, 0, "gosub" ))
                      {
                          thread->halted = 1;
                          return 1;
                      }
                      thread->line_number = file_data->lines[i].args[0].integer;
                      break;
                  case cmd_return:
                      if (!sl_exec_file_stack_entry_pop( file_data, thread, sl_exec_file_stack_frame_type_gosub, NULL, &thread->line_number, NULL, NULL, "gosub" ))
                      {
                          thread->halted = 1;
                          return 1;
                      }
                      break;
                  case cmd_while:
                      if (file_data->lines[i].args[0].integer)
                      {
                          if (!sl_exec_file_stack_entry_push( file_data, thread, sl_exec_file_stack_frame_type_while, NULL, i, 0, 0, "while" ))
                          {
                              thread->halted = 1;
                              return 1;
                          }
                          thread->line_number++;
                      }
                      else
                      {
                          for (i=thread->line_number+1; i<file_data->number_lines; i++)
                          {
                              if ( (file_data->lines[i].type == sl_exec_file_line_type_cmd) &&
                                   (file_data->lines[i].cmd == cmd_endwhile) )
                              {
                                  break;
                              }
                          }
                          thread->line_number = i+1;
                      }
                      break;
                  case cmd_endwhile:
                      if (!sl_exec_file_stack_entry_pop( file_data, thread, sl_exec_file_stack_frame_type_while, NULL, &thread->line_number, NULL, NULL, "while" ))
                      {
                          thread->halted = 1;
                          return 1;
                      }
                      break;
                  case cmd_for:
                      if (!sl_exec_file_set_integer_variable( file_data, file_data->lines[i].args[0].p.string, &(file_data->lines[i].args[1].integer) ))
                      {
                          thread->halted = 1;
                          return 1;
                      }
                      if (!sl_exec_file_stack_entry_push( file_data, thread, sl_exec_file_stack_frame_type_for, file_data->lines[i].args[0].p.string, i+1, file_data->lines[i].args[3].integer, file_data->lines[i].args[2].integer, "for" ))
                      {
                          thread->halted = 1;
                          return 1;
                      }
                      thread->line_number++;
                      break;
                  case cmd_next:
                      if (!sl_exec_file_stack_entry_read( file_data, thread, sl_exec_file_stack_frame_type_for, file_data->lines[i].args[0].p.string, &thread->line_number, &increment, &last_value, "for" ))
                      {
                          thread->halted = 1;
                          return 1;
                      }
                      var = sl_exec_file_get_integer_variable( file_data, file_data->lines[i].args[0].p.string );
                      var[0] += increment;
                      sl_exec_file_set_integer_variable( file_data, file_data->lines[i].args[0].p.string, var );
                      if (var[0]>last_value)
                      {
                          (void)sl_exec_file_stack_entry_pop( file_data, thread, sl_exec_file_stack_frame_type_for, file_data->lines[i].args[0].p.string, &thread->line_number, &increment, &last_value, "for" );
                          thread->line_number = i+1;
                      }
                      break;
                  case cmd_debug:
                      file_data->message->add_error( (void *)file_data->user,
                                                     error_level_info,
                                                     error_number_sl_debug, error_id_sl_exec_file_get_next_cmd,
                                                     error_arg_type_uint64, file_data->lines[i].args[0].integer,
                                                     error_arg_type_const_string, file_data->lines[i].args[1].p.string,
                                                     error_arg_type_malloc_string, thread->id,
                                                     error_arg_type_malloc_filename, file_data->filename,
                                                     error_arg_type_line_number, i+1,
                                                     error_arg_type_none );
                      thread->line_number++;
                      break;
                  case cmd_printf:
                      text = sl_exec_file_vprintf( file_data, file_data->lines[i].args, SL_EXEC_FILE_MAX_CMD_ARGS );
                      if (text)
                      {
                          file_data->message->add_error( (void *)file_data->user,
                                                         error_level_info,
                                                         error_number_sl_message, error_id_sl_exec_file_get_next_cmd,
                                                         error_arg_type_malloc_string, text,
                                                         error_arg_type_malloc_filename, file_data->filename,
                                                         error_arg_type_line_number, i+1,
                                                         error_arg_type_none );
                          free(text);
                      }
                      thread->line_number++;
                      break;
                  case cmd_warning: // Use malloc for the string as the exec_file may be freed before the error comes out
                      file_data->message->add_error( (void *)file_data->user,
                                                     error_level_warning,
                                                     error_number_sl_message, error_id_sl_exec_file_get_next_cmd,
                                                     error_arg_type_uint64, file_data->lines[i].args[0].integer,
                                                     error_arg_type_malloc_string, file_data->lines[i].args[1].p.string,
                                                     error_arg_type_malloc_string, thread->id,
                                                     error_arg_type_malloc_filename, file_data->filename,
                                                     error_arg_type_line_number, i+1,
                                                     error_arg_type_none );
                      thread->line_number++;
                      break;
                  case cmd_pass: // Use malloc for the string as the exec_file may be freed before the error comes out
                      file_data->message->add_error( (void *)file_data->user,
                                                     error_level_serious,
                                                     error_number_sl_pass, error_id_sl_exec_file_get_next_cmd,
                                                     error_arg_type_uint64, file_data->lines[i].args[0].integer,
                                                     error_arg_type_malloc_string, file_data->lines[i].args[1].p.string,
                                                     error_arg_type_malloc_string, thread->id,
                                                     error_arg_type_malloc_filename, file_data->filename,
                                                     error_arg_type_line_number, i+1,
                                                     error_arg_type_none );
                      thread->line_number++;
                      file_data->completion.passed = 1;
                      file_data->completion.return_code = file_data->lines[i].args[0].integer;
                      break;
                  case cmd_fail: // Use malloc for the string as the exec_file may be freed before the error comes out
                      file_data->message->add_error( (void *)file_data->user,
                                                     error_level_info,
                                                     error_number_sl_fail, error_id_sl_exec_file_get_next_cmd,
                                                     error_arg_type_uint64, file_data->lines[i].args[0].integer,
                                                     error_arg_type_malloc_string, file_data->lines[i].args[1].p.string,
                                                     error_arg_type_malloc_string, thread->id,
                                                     error_arg_type_malloc_filename, file_data->filename,
                                                     error_arg_type_line_number, i+1,
                                                     error_arg_type_none );
                      thread->line_number++;
                      file_data->completion.failed = 1;
                      file_data->completion.return_code = file_data->lines[i].args[0].integer;
                      break;
                  case cmd_spawn:
                      sl_exec_file_create_thread( file_data, file_data->lines[i].args[0].p.string, file_data->lines[i].args[2].integer, file_data->lines[i].args[1].integer );
                      file_data->message->add_error( (void *)file_data->user,
                                                     error_level_info,
                                                     error_number_sl_spawn, error_id_sl_exec_file_get_next_cmd,
                                                     error_arg_type_malloc_string, file_data->lines[i].args[0].p.string,
                                                     error_arg_type_malloc_string, thread->id,
                                                     error_arg_type_malloc_filename, file_data->filename,
                                                     error_arg_type_line_number, i+1,
                                                     error_arg_type_none );
                      thread->line_number++;
                      break;
                  case cmd_die:
                      file_data->message->add_error( (void *)file_data->user,
                                                     error_level_info,
                                                     error_number_sl_die, error_id_sl_exec_file_get_next_cmd,
                                                     error_arg_type_malloc_string, thread->id,
                                                     error_arg_type_malloc_filename, file_data->filename,
                                                     error_arg_type_line_number, i+1,
                                                     error_arg_type_none );
                      sl_exec_file_free_thread( file_data, thread );
                      return 1;
                  case cmd_end:
                      file_data->message->add_error( (void *)file_data->user,
                                                     error_level_info,
                                                     error_number_sl_end, error_id_sl_exec_file_get_next_cmd,
                                                     error_arg_type_malloc_string, thread->id,
                                                     error_arg_type_malloc_filename, file_data->filename,
                                                     error_arg_type_line_number, i+1,
                                                     error_arg_type_none );
                      sl_exec_file_free_thread( file_data, thread );
                      file_data->completion.finished = 1;
                      return 1;
                  case cmd_set_string:
                      sl_exec_file_set_string_variable( file_data, file_data->lines[i].args[0].p.string, file_data->lines[i].args[1].p.string );
                      thread->line_number++;
                      break;
                  case cmd_sprintf:
                      text = sl_exec_file_vprintf( file_data, file_data->lines[i].args+1, SL_EXEC_FILE_MAX_CMD_ARGS );
                      sl_exec_file_set_string_variable( file_data, file_data->lines[i].args[0].p.string, text );
                      thread->line_number++;
                      break;
                  case cmd_set:
                  case cmd_and:
                  case cmd_or:
                  case cmd_xor:
                  case cmd_bic:
                  case cmd_lshr:
                  case cmd_ashr:
                  case cmd_shl:
                  case cmd_ror32:
                  case cmd_rol32:
                  case cmd_ror64:
                  case cmd_rol64:
                  case cmd_add:
                  case cmd_sub:
                  case cmd_mult:
                  case cmd_div:
                      res = 0;
                      res_d = 0;
                      switch (file_data->lines[i].cmd)
                      {
                      case cmd_set:
                          res = file_data->lines[i].args[1].integer;
                          res_d = file_data->lines[i].args[1].real;
                          break;
                      case cmd_and:
                          res = file_data->lines[i].args[1].integer & file_data->lines[i].args[2].integer;
                          break;
                      case cmd_or:
                          res = file_data->lines[i].args[1].integer | file_data->lines[i].args[2].integer;
                          break;
                      case cmd_xor:
                          res = file_data->lines[i].args[1].integer ^ file_data->lines[i].args[2].integer;
                          break;
                      case cmd_bic:
                          res = file_data->lines[i].args[1].integer &~ file_data->lines[i].args[2].integer;
                          break;
                      case cmd_lshr:
                          res = ((t_sl_uint64) file_data->lines[i].args[1].integer) >> file_data->lines[i].args[2].integer;
                          break;
                      case cmd_ashr:
                          res = ((t_sl_sint64) file_data->lines[i].args[1].integer) >> file_data->lines[i].args[2].integer;
                          break;
                      case cmd_shl:
                          res = file_data->lines[i].args[1].integer << file_data->lines[i].args[2].integer;
                          break;
                      case cmd_rol32:
                          res = ((file_data->lines[i].args[1].integer << file_data->lines[i].args[2].integer) | ((file_data->lines[i].args[1].integer)>>(32-file_data->lines[i].args[2].integer))) & 0xffffffffLL;
                          break;
                      case cmd_rol64:
                          res = (file_data->lines[i].args[1].integer << file_data->lines[i].args[2].integer) | ((file_data->lines[i].args[1].integer)>>(64-file_data->lines[i].args[2].integer));
                          break;
                      case cmd_ror32:
                          res = ((file_data->lines[i].args[1].integer >> file_data->lines[i].args[2].integer) | ((file_data->lines[i].args[1].integer)<<(32-file_data->lines[i].args[2].integer))) & 0xffffffffLL;
                          break;
                      case cmd_ror64:
                          res = (file_data->lines[i].args[1].integer >> file_data->lines[i].args[2].integer) | ((file_data->lines[i].args[1].integer)<<(64-file_data->lines[i].args[2].integer));
                          break;
                      case cmd_add:
                          res = file_data->lines[i].args[1].integer + file_data->lines[i].args[2].integer;
                          res_d = file_data->lines[i].args[1].real + file_data->lines[i].args[2].real;
                          break;
                      case cmd_sub:
                          res = file_data->lines[i].args[1].integer - file_data->lines[i].args[2].integer;
                          res_d = file_data->lines[i].args[1].real - file_data->lines[i].args[2].real;
                          break;
                      case cmd_mult:
                          res = file_data->lines[i].args[1].integer * file_data->lines[i].args[2].integer;
                          res_d = file_data->lines[i].args[1].real * file_data->lines[i].args[2].real;
                          break;
                      case cmd_div:
                          res = file_data->lines[i].args[1].integer / file_data->lines[i].args[2].integer;
                          res_d = file_data->lines[i].args[1].real / file_data->lines[i].args[2].real;
                          break;
                      default:
                          break;
                      }
                      sl_exec_file_set_variable( file_data, file_data->lines[i].args[0].p.string, &res, &res_d );
                      thread->line_number++;
                      break;
                  case cmd_beq:
                  case cmd_bne:
                  case cmd_ble:
                  case cmd_blt:
                  case cmd_bge:
                  case cmd_bgt:
                      switch (file_data->lines[i].cmd)
                      {
                      case cmd_beq:
                          res = (file_data->lines[i].args[0].integer == file_data->lines[i].args[1].integer);
                          break;
                      case cmd_bne:
                          res = (file_data->lines[i].args[0].integer != file_data->lines[i].args[1].integer);
                          break;
                      case cmd_ble:
                          res = ((t_sl_sint64)file_data->lines[i].args[0].integer <= (t_sl_sint64)file_data->lines[i].args[1].integer);
                          break;
                      case cmd_blt:
                          res = ((t_sl_sint64)file_data->lines[i].args[0].integer < (t_sl_sint64)file_data->lines[i].args[1].integer);
                          break;
                      case cmd_bge:
                          res = ((t_sl_sint64)file_data->lines[i].args[0].integer >= (t_sl_sint64)file_data->lines[i].args[1].integer);
                          break;
                      case cmd_bgt:
                          res = ((t_sl_sint64)file_data->lines[i].args[0].integer > (t_sl_sint64)file_data->lines[i].args[1].integer);
                          break;
                      default:
                          break;
                      }
                      if (res)
                      {
                          thread->line_number = file_data->lines[i].args[2].integer;
                      }
                      else
                      {
                          thread->line_number++;
                      }
                      break;
                  default:
                      thread->cmd = file_data->lines[i].cmd;
                      thread->num_args = file_data->lines[i].num_args;
                      thread->args = file_data->lines[i].args;
                      thread->pending_command = 1;
                      thread->line_number++;
                      return 1;
                  }
                  break;
              }
          }
     }
     if (thread->execution.type == sl_exec_file_thread_execution_type_running)
     {
         file_data->error->add_error( (void *)file_data->user, error_level_serious, error_number_sl_exec_file_too_much_looping, error_id_sl_exec_file_get_next_cmd,
                                      error_arg_type_malloc_filename, file_data->filename,
                                      error_arg_type_line_number, i+1,
                                      error_arg_type_none );
         thread->halted = 1;
     }
     return 1;
}

/*f sl_exec_file_execute
  For each thread...
    If thread has pending command, then return
    Else execute thread
 */
extern void sl_exec_file_execute( t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_thread_data *thread, *next_thread;
    int done, i;

    WHERE_I_AM;
    if (!file_data || !file_data->threads)
    {
        return;
    }

    done = 0;
    for (i=0; (!done) && (i<SL_EXEC_FILE_MAX_THREAD_LOOPS); i++)
    {
        done = 1;
        for (thread=file_data->threads; thread; thread=next_thread )
        {
            WHERE_I_AM;
            next_thread = thread->next_in_list; // Capture it now in case the thread completes and gets freed
            if ((thread->pending_command) || (thread->halted))
            {
                continue;
            }
            // Run the thread through to stall or having a command pending or a halting error - if anyh operations did occur, it might change the scheduling and we'll reexecute every thread
            if (sl_exec_file_thread_execute( file_data, thread )) // after this function call thread can have been freed!
            {
                done=0;
            }
        }
    }
}

/*f internal_get_next_cmd
  Execute all threads until they have a pending command, they stall, or they halt
  Return 0 if no command, 1 if there is a command
 */
static int internal_get_next_cmd( t_sl_exec_file_data *file_data, int *cmd, t_sl_exec_file_value **args )
{
     t_sl_exec_file_thread_data *thread;

     WHERE_I_AM;

     if (!file_data)
          return 0;

     sl_exec_file_execute( file_data );
     file_data->command_from_thread = NULL;

     for (thread=file_data->threads; thread; thread=thread->next_in_list)
     {
          if (thread->pending_command)
          {
               thread->pending_command = 0;
               *cmd = thread->cmd;
               *args = thread->args;
               file_data->command_from_thread = thread;
               return 1;
          }
     }

     return 0;
}

/*f sl_exec_file_despatch_next_cmd - main entry point for execution
  Execute all threads until they have a pending command, they stall, or they halt
    Return 0 if no commands ready to be executed, 1 if a command may be ready to be executed
 */
extern int sl_exec_file_despatch_next_cmd( t_sl_exec_file_data *file_data )
{
    t_sl_exec_file_cmd_cb cmd_cb;
    t_sl_exec_file_lib_chain *lib;

     WHERE_I_AM;

    if (!file_data)
        return 0;

    if (file_data->py_object)
    {
        return sl_exec_file_py_despatch( file_data );
    }

    if (!internal_get_next_cmd( file_data, &cmd_cb.cmd, &cmd_cb.args ))
        return 0;

    /*b Match command to a library
     */
    lib = find_command_in_library( file_data, cmd_cb.cmd );

    /*b If found, despatch to the command handler
     */
    //printf("Found cmd %d in library %p\n", cmd_cb.cmd, lib );
    if (lib && lib->lib_desc.cmd_handler)
    {
        t_sl_error_level err;
        cmd_cb.file_data = file_data;
        cmd_cb.error = file_data->error;
        cmd_cb.filename = file_data->filename;
        cmd_cb.line_number = file_data->command_from_thread->line_number;
        cmd_cb.cmd -= lib->base;
        cmd_cb.lib_desc = &(lib->lib_desc);
        cmd_cb.args = cmd_cb.args;
        cmd_cb.num_args = sl_exec_file_get_number_of_arguments( file_data, cmd_cb.args );
        file_data->eval_fn_result = &file_data->cmd_result;
        file_data->eval_fn_result_ok = 0;

        err = lib->lib_desc.cmd_handler( &cmd_cb, lib->lib_desc.handle );
        if (err!=error_level_okay)
        {
            fprintf(stderr,"NNE: Cmd handler callback returned an error\n");
        }
    }
    else
    {
        // If there is no match, then print an error
        file_data->error->add_error( (void *)file_data->user, error_level_fatal, error_number_sl_unknown_command, error_id_sl_exec_file_get_next_cmd,
                                     error_arg_type_malloc_string, lib->lib_desc.file_cmds[cmd_cb.cmd-lib->base].cmd,
                                     error_arg_type_malloc_filename, file_data->filename,
                                     error_arg_type_line_number, file_data->command_from_thread->line_number,
                                     error_arg_type_none );
    }

    /*b Return indicating there may be more commands which could be done
     */
    return 1;
}

/*a Exec file support functions
 */
/*f arg_from_vprintf_string
 */
static int arg_from_vprintf_string( char *string, int *used, t_sl_exec_file_value_type type,  t_sl_exec_file_value *args, int max_args )
{
     int j, k;

     j=string[0]-'0';
     if ((j>=0) && (j<=9))
     {
          for( k=1; isdigit(string[k]); k++ )
          {
               j = (j*10)+(string[k]-'0');
          }
          if ((string[k]=='%') && (j>=0) && (j<max_args-1) && (args[j+1].type==type))
          {
               *used = k+1;
               return j;
          }
     }
     return -1;
}

/*f sl_exec_file_vprintf -- FIX FOR PYTHON
 */
extern char *sl_exec_file_vprintf( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int max_args )
{
     char *text, buffer[32];
     int i, j, k;
     int ch;
     int pos, text_size;
     int done;

     text_size=1024;
     text = (char *)malloc(1024);
     if (!text)
          return NULL;

     for (i=0, pos=0; args[0].p.string[i]; i++)
     {
          if (pos>=text_size)
          {
               pos = text_size-1;
               break;
          }
          switch (args[0].p.string[i])
          {
          case '\\':
               i++;
               switch (args[0].p.string[i])
               {
               case 'n':
                    text[pos++] = '\n';
                    break;
               case 0:
                    i--;
                    break;
               default:
                    text[pos++] = args[0].p.string[i];
                    break;
               }
               break;
          case '%':
               i++;
               done = 0;
               ch = args[0].p.string[i];
               switch (ch)                   
               {
               case 'F':
                    if (args[0].p.string[i+1]=='%')
                    {
                         sl_str_copy_max( text+pos, file_data->filename, text_size-pos );
                         i++;
                         done=1;
                    }
                    break;
               case 'T':
                    if (args[0].p.string[i+1]=='%')
                    {
                         sl_str_copy_max( text+pos, file_data->current_thread->id, text_size-pos );
                         i++;
                         done = 1;
                    }
                    break;
               case 'L':
                    if (args[0].p.string[i+1]=='%')
                    {
                         sprintf( buffer, "%d", file_data->current_thread->line_number );
                         sl_str_copy_max( text+pos, buffer, text_size-pos );
                         i++;
                         done = 1;
                    }
                    break;
               case 'd':
                    j = arg_from_vprintf_string( args[0].p.string+i+1, &k, sl_exec_file_value_type_integer, args, max_args );
                    if (j>=0)
                    {
                        sprintf( buffer, "%lld", (t_sl_sint64)args[j+1].integer );
                         sl_str_copy_max( text+pos, buffer, text_size-pos );
                         done = 1;
                         i+=k;
                    }
                    break;
               case 'x':
               case 'X':
                    j = arg_from_vprintf_string( args[0].p.string+i+1, &k, sl_exec_file_value_type_integer, args, max_args );
                    if (j>=0)
                    {
                        sprintf( buffer, (ch=='x')?"%08llx":"%016llx", args[j+1].integer );
                        sl_str_copy_max( text+pos, buffer, text_size-pos );
                        done = 1;
                        i+=k;
                    }
                    break;
               case 'f':
                    j = arg_from_vprintf_string( args[0].p.string+i+1, &k, sl_exec_file_value_type_double, args, max_args );
                    if (j>=0)
                    {
                         sprintf( buffer, "%f", args[j+1].real );
                         sl_str_copy_max( text+pos, buffer, text_size-pos );
                         done = 1;
                         i+=k;
                    }
                    break;
               case 's':
                    j = arg_from_vprintf_string( args[0].p.string+i+1, &k, sl_exec_file_value_type_string, args, max_args );
                    if (j>=0)
                    {
                        if (args[j+1].p.string)
                        {
                            sl_str_copy_max( text+pos, args[j+1].p.string, text_size-pos );
                        }
                        else
                        {
                            sl_str_copy_max( text+pos, "<NULL>", text_size-pos );
                        }
                        done = 1;
                        i+=k;
                    }
                    break;
               default:
                    break;
               }
               if (done)
               {
                    pos = strlen(text);
               }
               else
               {
                    i--;
                    text[pos++] = args[0].p.string[i];
               }
               break;
          default:
               text[pos++] = args[0].p.string[i];
               break;
          }
     }
     if (pos>=text_size)
          pos = text_size-1;
     text[pos]=0;
     return text;
}

/*a Simple interrogation functions
 */
/*f sl_exec_file_line_number
 */
extern int sl_exec_file_line_number( t_sl_exec_file_data *file_data )
{
     if ( (file_data) &&
          (file_data->command_from_thread) )
     {
          return file_data->command_from_thread->line_number;
     }
     return -1;
}

/*f sl_exec_file_filename
 */
extern const char *sl_exec_file_filename( t_sl_exec_file_data *file_data )
{
     return file_data->filename;
}

/*f sl_exec_file_current_threadname
 */
extern const char *sl_exec_file_current_threadname( t_sl_exec_file_data *file_data )
{
     if (file_data->current_thread)
          return file_data->current_thread->id;
     return "<no thread>";
}

/*f sl_exec_file_command_threadname
 */
extern const char *sl_exec_file_command_threadname( t_sl_exec_file_data *file_data )
{
     if (file_data->command_from_thread)
          return file_data->command_from_thread->id;
     return "<no thread>";
}

/*f sl_exec_file_error
 */
extern c_sl_error *sl_exec_file_error( t_sl_exec_file_data *file_data )
{
     return file_data->error;
}

/*f sl_exec_file_message
 */
extern c_sl_error *sl_exec_file_message( t_sl_exec_file_data *file_data )
{
     return file_data->message;
}

/*f sl_exec_file_completion
 */
extern t_sl_exec_file_completion *sl_exec_file_completion( t_sl_exec_file_data *file_data )
{
    return &(file_data->completion);
}

/*a Python extensions - only if SL_EXEC_FILE_PYTHON is defined
 */
/*f Wrapper
 */
#ifdef SL_EXEC_FILE_PYTHON

/*t Local defines
 */
#define PyObj(file_data) ((t_py_object *)(file_data->py_object))

/*t t_py_object
 */
typedef struct t_py_object
{
    PyObject_HEAD
    PyInterpreterState *py_interp;
    t_sl_exec_file_data *file_data;
    int clocked;
    t_sl_pthread_barrier barrier;
    t_sl_pthread_barrier_thread_ptr barrier_thread; // Main thread descriptor equivalent to the mainline invoking thread
    pthread_mutex_t thread_sync_mutex;  // These three are used to spawn and sync a subthread
    pthread_cond_t  thread_sync_cond;   // These three are used to spawn and sync a subthread
    int             thread_spawn_state; // These three are used to spawn and sync a subthread
} t_py_object;

/*t t_py_object_exec_file_library
 */
typedef struct
{
    PyObject_HEAD
    t_sl_exec_file_data *file_data;
    t_sl_exec_file_lib_chain *lib_chain;
} t_py_object_exec_file_library;

/*t t_py_object_exec_file_object
 */
typedef struct
{
    PyObject_HEAD
    t_sl_exec_file_data *file_data;
    t_sl_exec_file_object_chain *object_chain;
} t_py_object_exec_file_object;

/*t t_py_barrier_thread_user_state
 */
typedef enum
{
    py_barrier_thread_user_state_init,
    py_barrier_thread_user_state_running,
    py_barrier_thread_user_state_die
} t_py_barrier_thread_user_state;

/*f start_new_py_thread_started
 */
static void start_new_py_thread_started( t_py_object *py_object )
{
    WHERE_I_AM;

    pthread_mutex_lock( &py_object->thread_sync_mutex );
    WHERE_I_AM;
    if (py_object->thread_spawn_state==1) // Waiting on signal
    {
    WHERE_I_AM;
        py_object->thread_spawn_state = 4;
        pthread_cond_signal( &py_object->thread_sync_cond );
    WHERE_I_AM;
    }
    else
    {
    WHERE_I_AM;
        py_object->thread_spawn_state = 3;
    WHERE_I_AM;
    }
    WHERE_I_AM;
    pthread_mutex_unlock( &py_object->thread_sync_mutex );
    WHERE_I_AM;
}

/*f start_new_py_thread
 */
static int start_new_py_thread( void (*thread_fn)(), t_py_object *py_object )
{
    pthread_t th;
    int status;
    int result = 1;
    WHERE_I_AM;
    int        rc=pthread_mutex_init( &py_object->thread_sync_mutex, NULL );
    if (rc==0) rc=pthread_cond_init(  &py_object->thread_sync_cond, NULL );
    if (rc!=0)
        return 0;

    WHERE_I_AM;
    py_object->thread_spawn_state = 0;

    pthread_mutex_lock( &py_object->thread_sync_mutex );
    WHERE_I_AM;
    status = pthread_create(&th, (pthread_attr_t*)NULL, (void* (*)(void *))thread_fn, (void *)py_object );
    WHERE_I_AM;
    if (status==0)
    {
    WHERE_I_AM;
        pthread_detach(th);
        if (py_object->thread_spawn_state==0)
        {
    WHERE_I_AM;
            py_object->thread_spawn_state=1;
            Py_BEGIN_ALLOW_THREADS;
    WHERE_I_AM;
            pthread_cond_wait( &py_object->thread_sync_cond, &py_object->thread_sync_mutex );
    WHERE_I_AM;
            py_object->thread_spawn_state=5;
    WHERE_I_AM;
            Py_END_ALLOW_THREADS;
    WHERE_I_AM;
        }
        else
        {
    WHERE_I_AM;
            py_object->thread_spawn_state=2;
    WHERE_I_AM;
        }
    }
    WHERE_I_AM;
    pthread_mutex_unlock(  &py_object->thread_sync_mutex );
    WHERE_I_AM;
    pthread_mutex_destroy( &py_object->thread_sync_mutex );
    WHERE_I_AM;
    pthread_cond_destroy(  &py_object->thread_sync_cond );
    WHERE_I_AM;
    return result;
}

/*f py_object_init
 */
static int py_object_init( t_py_object *self, PyObject *args, PyObject *kwds )
{
    WHERE_I_AM;
    fprintf(stderr,"Object init %p\n",self);
    self->file_data = NULL;
    self->py_interp = NULL;
    self->clocked = 0;
	return 0;
}

/*f py_object_new - unused as we override this for some reason
 */
static PyObject *py_object_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
    WHERE_I_AM;
    fprintf(stderr,"Noddy_new\n");
    return subtype->tp_alloc(subtype,1);
}

/*f py_object_getattro
 */
static PyObject *py_object_getattro( PyObject *o, PyObject *attr_name )
{
    char *name = PyString_AsString(attr_name);
    WHERE_I_AM;
    fprintf(stderr,"Noddy_getattro %s\n", name);
    return PyObject_GenericGetAttr(o, attr_name);
}

/*f py_engine_cb_args
 */
static int py_engine_cb_args( PyObject* args, const char*arg_string, t_sl_exec_file_cmd_cb *cmd_cb )
{
    int arg_len;
    int j;
    arg_len = (int)PyTuple_Size(args);
    for (j=0;j<SL_EXEC_FILE_MAX_CMD_ARGS;j++)
    {
        cmd_cb->args[j].type = sl_exec_file_value_type_none;
    }
    for (j=0; (j<SL_EXEC_FILE_MAX_CMD_ARGS) && (j<arg_len);j++)
    {
        PyObject *obj=PyTuple_GetItem( args, j );
        if (arg_string[j]=='s')
        {
            const char *value;
            value = PyString_AsString(obj);
            cmd_cb->args[j].type = sl_exec_file_value_type_string;
            cmd_cb->args[j].p.string = sl_str_alloc_copy( value );
        }
        else if (arg_string[j]=='i')
        {
            PyErr_Clear();
            cmd_cb->args[j].integer = PyInt_AsLong(obj);
            if (PyErr_Occurred())
            {
                const char *value;
                value = PyString_AsString(obj);
                fprintf(stderr,"Error occurred parsing int\n");
                return -1;
            }
            cmd_cb->args[j].type = sl_exec_file_value_type_integer;
        }
        else if (arg_string[j]=='o')
        {
            cmd_cb->args[j].p.ptr = obj;
            cmd_cb->args[j].type = sl_exec_file_value_type_object;
        }
        else
        {
            fprintf(stderr,"Arg not comprehended by python %c",arg_string[j]);
            return -1;
        }
    }
    cmd_cb->num_args = j;
    return j;
}

/*f py_engine_cb
 */
static PyObject *py_engine_cb( PyObject* self, PyObject* args )
{
    WHERE_I_AM;
    PyObject *o;
    long type, offset;
    t_sl_exec_file_lib_desc *lib_desc;
    t_py_object_exec_file_library *py_ef_lib;
    t_sl_exec_file_cmd_cb cmd_cb;
    t_sl_exec_file_value cmd_cb_args[SL_EXEC_FILE_MAX_CMD_ARGS];

    o = PyTuple_GetItem(self, 0);
    type = PyInt_AsLong(PyTuple_GetItem(self, 1));
    offset = PyInt_AsLong(PyTuple_GetItem(self, 2));

    py_ef_lib = (t_py_object_exec_file_library *)o;
    
    lib_desc = &(py_ef_lib->lib_chain->lib_desc);
    fprintf(stderr,"self %p\n",self);
    cmd_cb.file_data = py_ef_lib->file_data;
    cmd_cb.error = py_ef_lib->file_data->error;
    cmd_cb.filename = py_ef_lib->file_data->filename;
    cmd_cb.line_number = 0;
    cmd_cb.cmd = 0;
    cmd_cb.args = cmd_cb_args;
    cmd_cb.lib_desc = lib_desc;
    py_ef_lib->file_data->command_from_thread = NULL;
    py_ef_lib->file_data->eval_fn_result = &py_ef_lib->file_data->cmd_result;
    py_ef_lib->file_data->eval_fn_result_ok = 0;
    if (type==0)
    {
        WHERE_I_AM;
        fprintf(stderr,"call of function %d/%s\n", offset, lib_desc->file_fns[offset].fn );
        if (py_engine_cb_args( args, lib_desc->file_fns[offset].args, &cmd_cb)<0)
            return Py_None; // Should be NULL
    }
    else if (type==1)
    {
        // Must ensure that declaratives are only called during init; nondeclaratives can be called at any point
        // Declaratives should probably force adding of stuff to the dict...
        t_sl_exec_file_lib_desc *lib_desc;
        WHERE_I_AM;
        lib_desc = &(py_ef_lib->lib_chain->lib_desc);
        fprintf(stderr,"invocation of command %d/%s\n", offset, lib_desc->file_cmds[offset].cmd );
        cmd_cb.cmd = lib_desc->file_cmds[offset].cmd_value;
        WHERE_I_AM;
        if (py_engine_cb_args( args, lib_desc->file_cmds[offset].args, &cmd_cb)<0)
        {
            fprintf(stderr,"Misparsed args\n");
            return Py_None; // Should be NULL
        }
        WHERE_I_AM;
        if (cmd_cb.num_args<lib_desc->file_cmds[offset].min_args)
        {
            fprintf(stderr,"Short of args %d/%d\n", cmd_cb.num_args,lib_desc->file_cmds[offset].min_args);
            return Py_None; // Should be NULL
        }
        WHERE_I_AM;
        lib_desc->cmd_handler( &cmd_cb, lib_desc->handle );
        WHERE_I_AM;
        return Py_None;
    }
    return Py_None;
}

/*f py_exec_file_library_getattro
 */
static PyObject *py_exec_file_library_getattro( PyObject *o, PyObject *attr_name )
{
    char *name = PyString_AsString(attr_name);
    t_py_object_exec_file_library *py_ef_lib = (t_py_object_exec_file_library *)o;
    t_sl_exec_file_lib_desc *lib_desc;
    int j;

    WHERE_I_AM;

    lib_desc = &(py_ef_lib->lib_chain->lib_desc);
    if (lib_desc->file_fns)
    {
        for (j=0; lib_desc->file_fns[j].fn_value!=sl_exec_file_fn_none; j++ )
        {
            if ( !strcmp(lib_desc->file_fns[j].fn, name) )
            {
                static PyMethodDef method_def = { "exec_file_callback", py_engine_cb, METH_VARARGS, "sl_exec_file callback"};
                PyObject *tuple;
                tuple = PyTuple_New(3);
                PyTuple_SetItem(tuple, 0, o);
                PyTuple_SetItem(tuple, 1, PyInt_FromLong(0));
                PyTuple_SetItem(tuple, 2, PyInt_FromLong(j));
                return PyCFunction_New( &method_def, tuple );
            }
        }
    }
    if (lib_desc->file_cmds)
    {
        for (j=0; lib_desc->file_cmds[j].cmd_value!=sl_exec_file_cmd_none; j++ )
        {
            int ofs=0;
            if (lib_desc->file_cmds[j].cmd[0]=='!')
                ofs=1;
            if ( !strcmp(lib_desc->file_cmds[j].cmd+ofs, name) )
            {
                static PyMethodDef method_def = { "exec_file_callback", py_engine_cb, METH_VARARGS, "sl_exec_file callback"};
                PyObject *tuple;
                tuple = PyTuple_New(3);
                PyTuple_SetItem(tuple, 0, o);
                PyTuple_SetItem(tuple, 1, PyInt_FromLong(1));
                PyTuple_SetItem(tuple, 2, PyInt_FromLong(j));
                return PyCFunction_New( &method_def, tuple );
            }
        }
    }
    fprintf(stderr,"object_getattro %s\n", name);
    return PyObject_GenericGetAttr(o, attr_name);
}

/*f py_exec_file_object_getattro
 */
static PyObject *py_exec_file_object_getattro( PyObject *o, PyObject *attr_name )
{
    char *name = PyString_AsString(attr_name);
    t_py_object_exec_file_object *py_ef_obj = (t_py_object_exec_file_object *)o;
    t_sl_exec_file_object_chain *chain;
    WHERE_I_AM;
    int j;
    fprintf(stderr,"object_getattro %s\n", name);
    return PyObject_GenericGetAttr(o, attr_name);
}

/*v py_object_methods
 */
static PyMethodDef py_object_methods[] = { {NULL} };

/*v py_class_object__exec_file
 */
static PyTypeObject py_class_object__exec_file = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "py_engine.exec_file",             /*tp_name*/
    sizeof(t_py_object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    py_object_getattro,            /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT |
    Py_TPFLAGS_BASETYPE | 
    Py_TPFLAGS_HAVE_CLASS,        /*tp_flags*/
    "Noddy objects",           /* tp_doc */
    0, /* tp_traverse*/
    0, /*tp_clear */

    0, /*tp_richcompare */

    0, /*tp_weaklistoffset */

    0, /*tp_iter */
    0, /*tp_iternext */

    py_object_methods, /**tp_methods */
    0, /**tp_members */
    0, /**tp_getset */
    0, /**tp_base */
    0, /**tp_dict */
    0, /*tp_descr_get */
    0, /*tp_descr_set */
    0, /*tp_dictoffset */
    (initproc)py_object_init, /*tp_init */
    PyType_GenericAlloc, /*tp_alloc */
    (newfunc)py_object_new, /*tp_new - does not seem to be called*/
    0, /*tp_free; Low-level free-memory routine  */
    0, /*tp_is_gc; For PyObject_IS_GC  */
    0, /**tp_bases */
    0, /**tp_mro; method resolution order  */
    0, /**tp_cache */
    0, /**tp_subclasses */
    0, /**tp_weaklist */

};

/*v py_object_exec_file_library
 */
static PyTypeObject py_object_exec_file_library =
{
    PyObject_HEAD_INIT(NULL)
    0,
    "exec_file_library", // for printing
    sizeof( t_py_object_exec_file_library ), // basic size
    0, // item size
    0, /*tp_dealloc*/
    0,   /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */
	0,			/* tp_call - called if the object itself is invoked as a method */
	0, /*tp_str */
    py_exec_file_library_getattro,            /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT        /*tp_flags*/
};

/*v py_object_exec_file_object
 */
static PyTypeObject py_object_exec_file_object =
{
    PyObject_HEAD_INIT(NULL)
    0,
    "exec_file_object", // for printing
    sizeof( t_py_object_exec_file_object ), // basic size
    0, // item size
    0, /*tp_dealloc*/
    0,   /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */
	0,			/* tp_call - called if the object itself is invoked as a method */
    0,                         /*tp_str*/
    py_exec_file_object_getattro,            /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
};

/*f sl_exec_file_py_thread
 */
static void sl_exec_file_py_thread( t_py_object *py_object )
{
    PyInterpreterState* py_interp;
    PyThreadState* py_thread;
    PyObject *py_callable = NULL;
    t_sl_pthread_barrier_thread_ptr barrier_thread;
    int running;
    int loop_count;

    WHERE_I_AM;
    fprintf(stderr,"sl_exec_file_py_thread:%p:%p\n",py_object, py_callable);
    barrier_thread = sl_pthread_barrier_thread_add( &py_object->barrier );
    WHERE_I_AM;

    sl_pthread_barrier_thread_set_user_state( barrier_thread, py_barrier_thread_user_state_init, NULL );

    PyEval_AcquireLock();
    py_thread = PyThreadState_New(py_object->py_interp);
    PyThreadState_Clear(py_thread);
    PyEval_ReleaseLock();

    start_new_py_thread_started( py_object );

    // Invoke callable (or 'run' if none given)
    PyObject *py_obj;
    PyEval_AcquireThread(py_thread);
    if (py_callable)
    {
        WHERE_I_AM;
        py_obj = PyObject_CallObject( py_callable, (PyObject *)py_object );
        Py_DECREF(py_callable);
    }
    else
    {
        py_obj = PyObject_CallMethod( (PyObject *)py_object, "exec_run", NULL );
    }
    if (py_obj)
    {
        WHERE_I_AM;
        Py_DECREF(py_obj);
    }
    WHERE_I_AM;
    PyEval_ReleaseThread(py_thread);

    WHERE_I_AM;
    sl_pthread_barrier_thread_delete( &py_object->barrier, barrier_thread );
    WHERE_I_AM;

    WHERE_I_AM;
    PyEval_AcquireLock();
    PyThreadState_Clear(py_thread);
    PyThreadState_Delete(py_thread);
    PyEval_ReleaseLock();
    WHERE_I_AM;

    WHERE_I_AM;
    pthread_exit((void *)py_object);
    WHERE_I_AM;
}

/*f sl_exec_file_python_add_class_object
 */
extern int sl_exec_file_python_add_class_object( PyObject *module )
{
    py_class_object__exec_file.tp_new = PyType_GenericNew; // Why is this an override here?
    if (PyType_Ready(&py_class_object__exec_file) < 0)
        return error_level_fatal;

    Py_INCREF(&py_class_object__exec_file);
    PyModule_AddObject(module, "exec_file", (PyObject *)&py_class_object__exec_file);
    return error_level_okay;
}

/*f sl_exec_file_allocate_from_python_object
  opens the file
  reads it in to a temporary buffer
  counts the lines
  allocates an execution structure given the number of lines
  parses each line - could be a label, a goto, or a given command style, or a comment, or an error
  stores parse data for each line
  resets execution pointer
 */
extern t_sl_error_level sl_exec_file_allocate_from_python_object( c_sl_error *error,
                                                                  c_sl_error *message,
                                                                  t_sl_exec_file_callback_fn callback_fn,
                                                                  void *callback_handle,
                                                                  const char *id,
                                                                  PyObject *py_object,
                                                                  struct t_sl_exec_file_data **file_data_ptr,
                                                                  const char *user,
                                                                  int clocked )
{
     WHERE_I_AM;

     /*b Check the object is derived from the sl_exec_file class
      */
     if (!PyObject_IsInstance(py_object, (PyObject *)&py_class_object__exec_file))
     {
          return error->add_error( (void *)user, error_level_fatal, error_number_general_error_s, error_id_sl_exec_file_allocate_and_read_exec_file, error_arg_type_malloc_string, "Object provided is not a subclass of the sl_exec_file class", error_arg_type_none );
     }

     /*b Allocate structure for the file data
      */
     WHERE_I_AM;

     *file_data_ptr = (t_sl_exec_file_data *)malloc(sizeof(t_sl_exec_file_data));
     if (!*file_data_ptr)
     {
          return error->add_error( (void *)user, error_level_fatal, error_number_general_malloc_failed, error_id_sl_exec_file_allocate_and_read_exec_file, error_arg_type_malloc_string, user, error_arg_type_none );
     }
     sl_exec_file_data_init( *file_data_ptr, error, message, id, user );
     (*file_data_ptr)->py_object = py_object;

     /*b Add default commands and functions
      */
     WHERE_I_AM;

     sl_exec_file_data_add_default_cmds( *file_data_ptr );

     /*b Invoke callback to fill in extra functions and add other libraries; old exec file declared objects, not sure what to do about that here
      */
     WHERE_I_AM;

     if (callback_fn)
     {
         WHERE_I_AM;
         callback_fn( callback_handle, *file_data_ptr );
     }

     /*b Finish registering state
      */
     WHERE_I_AM;
     {
         t_sl_exec_file_callback *efc;
         for (efc=(*file_data_ptr)->poststate_callbacks; efc; efc=efc->next_in_list)
         {
             (efc->callback_fn)( efc->handle );
         }
     }

     /*b Add in objects for each sl_exec_file library
      */
     {
         t_sl_exec_file_lib_chain *chain;
         int j;

         WHERE_I_AM;
         for (chain=(*file_data_ptr)->lib_chain; chain; chain=chain->next_in_list)
         {
         WHERE_I_AM;
             fprintf(stderr,"Adding library %s to object (%p)\n",chain->lib_desc.library_name,chain);
             t_py_object_exec_file_library *py_ef_lib;
             py_ef_lib = PyObject_New( t_py_object_exec_file_library, &py_object_exec_file_library );
             py_ef_lib->file_data = *file_data_ptr;
             py_ef_lib->lib_chain = chain;
         WHERE_I_AM;
             PyObject_SetAttrString( py_object, chain->lib_desc.library_name, (PyObject *)py_ef_lib );
         WHERE_I_AM;
         }
     }

     /*b Find the py_interp we are in, and create a barrier thread, if clocked
      */
     WHERE_I_AM;
     if (clocked)
     {
         PyThreadState* py_thread;
         
         WHERE_I_AM;
         ((t_py_object *)py_object)->clocked = 1;
         PyEval_InitThreads();
         py_thread = PyThreadState_Get();
         ((t_py_object *)py_object)->py_interp = py_thread->interp;
         sl_pthread_barrier_init( &((t_py_object *)py_object)->barrier );
         ((t_py_object *)py_object)->barrier_thread = sl_pthread_barrier_thread_add( &((t_py_object *)py_object)->barrier );
     }

     /*b Invoke 'exec_init' method
      */
     WHERE_I_AM;
     PyObject *result;
     result = PyObject_CallMethod( (*file_data_ptr)->py_object, (char *)"exec_init", NULL );
     (*file_data_ptr)->during_init = 0;
     if (result) Py_DECREF(result);

     /*b Reset the file
      */
     WHERE_I_AM;
     sl_exec_file_reset( *file_data_ptr );
     SL_DEBUG( sl_debug_level_info,
              "pyobject %s complete for user",user ) ;

     WHERE_I_AM;
     return error_level_okay;
}

/*f sl_exec_file_py_kill_subthreads_cb
 */
void sl_exec_file_py_kill_subthreads_cb( void *handle, t_sl_pthread_barrier_thread_ptr barrier_thread )
{
    t_py_object *py_object = (t_py_object *)handle;
    py_object = py_object;
    WHERE_I_AM;

    sl_pthread_barrier_thread_set_user_state( barrier_thread, py_barrier_thread_user_state_die, NULL );
    WHERE_I_AM;
}

/*f sl_exec_file_py_kill_subthreads
 */
static void sl_exec_file_py_kill_subthreads( t_sl_exec_file_data *file_data )
{
    t_py_object *py_object = PyObj(file_data);
    WHERE_I_AM;

    fprintf(stderr, "obj %p clocked %x\n", py_object, py_object->clocked);
    if (!py_object->clocked) return;

    // We want to kill all subthreads - we don't really care about the order
    // So we broadcast to each subthread a user state of 'die'
    // Then we hit the barrier; some may die before they hit the barrier (no harm done), others will die after the barrier
    // So we hit the barrier again; we should come through cleanly when the subthreads die
    WHERE_I_AM;
    sl_pthread_barrier_thread_iter( &py_object->barrier, sl_exec_file_py_kill_subthreads_cb, py_object );
    WHERE_I_AM;
    sl_pthread_barrier_wait( &py_object->barrier, py_object->barrier_thread, NULL, NULL );
    WHERE_I_AM;
    sl_pthread_barrier_wait( &py_object->barrier, py_object->barrier_thread, NULL, NULL );
    WHERE_I_AM;
}

/*f sl_exec_file_py_despatch
 */
static int sl_exec_file_py_despatch( t_sl_exec_file_data *file_data )
{
    t_py_object *py_object = PyObj(file_data);
    if (py_object->clocked)
    {
        // Here we should 'tick' the barrier, in case threads are waiting on it; increment cycle also
        return 0;
    }
    else
    {
        PyObject *result;
        result = PyObject_CallMethod( (PyObject *)py_object, (char *)"exec_run", NULL );
        if (result)
        {
            WHERE_I_AM;
            Py_DECREF(result);
        }
        return 0;
    }
}

/*f sl_exec_file_py_reset - Reset a Python exec file (kill threads, start new one, if clocked - else just do 'exec_reset' method)
 */
static void sl_exec_file_py_reset( t_sl_exec_file_data *file_data )
{
    t_py_object *py_object = PyObj(file_data);
    WHERE_I_AM;

    if (!py_object->clocked)
    {
        PyObject *result;
        result = PyObject_CallMethod( (PyObject *)py_object, (char *)"exec_reset", NULL );
        if (result) Py_DECREF(result);
        return;
    }

    // Kill subthreads
    WHERE_I_AM;
    sl_exec_file_py_kill_subthreads( file_data );
    WHERE_I_AM;

    // Create new subthread and execute 'exec_run' method in that
    fprintf(stderr,"sl_exec_file_py_reset:%p:%p\n",py_object,file_data);
    if (!start_new_py_thread( (void (*)())sl_exec_file_py_thread, py_object ))
    {
    WHERE_I_AM;
        fprintf(stderr,"Failed to start new subthread in sl_exec_file.cpp - argh\n");
        exit(4);
    }
    WHERE_I_AM;
}

/*f Wrapper ELSE
 */
#else
static void sl_exec_file_py_reset( t_sl_exec_file_data *file_data ){}
static int sl_exec_file_py_despatch( t_sl_exec_file_data *file_data ){}

/*f Wrapper End
 */
#endif // ifdef SL_EXEC_FILE_PYTHON

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


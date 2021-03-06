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
#include <signal.h>
#include <unistd.h>
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
#if 0
#include <sys/time.h>
#include <pthread.h>
#define WHERE_I_AM {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%8ld.%06d:%p:%s:%d\n",tp.tv_sec,(int)tp.tv_usec,(void*)pthread_self(),__func__,__LINE__ );}
#else
#define WHERE_I_AM {}
#endif

#if 0
#include <sys/time.h>
#include <pthread.h>
#define WHERE_I_AM_TH_STR(s, this_thread, other, barrier_sync_callback_handle) {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%4.4d:%3ld.%06d:%6.6lx:%6.6lx:%s:%.*s:%s\n",__LINE__,tp.tv_sec % 1000,(int)tp.tv_usec,(((long)pthread_self())>>12)&0xFFFFFF,((long)other)&0xFFFFFF,(char*)barrier_sync_callback_handle,(((int)((unsigned long)pthread_self())>>12)+((int)((unsigned long)pthread_self())>>17)) & 31,"                                ",s );}
#define WHERE_I_AM_TH_STR2(s, this_thread, other, barrier_sync_callback_handle) {struct timeval tp; gettimeofday(&tp,NULL);fprintf(stderr,"%4.4d:%3ld.%06d:%6.6lx:%6.6lx:%s:%.*s:%s\n",__LINE__,tp.tv_sec % 1000,(int)tp.tv_usec,(((long)pthread_self())>>12)&0xFFFFFF,((long)other)&0xFFFFFF,(char*)barrier_sync_callback_handle,(((int)((unsigned long)pthread_self())>>12)+((int)((unsigned long)pthread_self())>>17)) & 31,"                                ",s );}
#else
#define WHERE_I_AM_TH_STR(s, this_thread, other, barrier_sync_callback_handle) {}
#define WHERE_I_AM_TH_STR2(s, this_thread, other, barrier_sync_callback_handle) {}
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
typedef struct t_sl_exec_file_thread_execution_data
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
static void sl_exec_file_py_reset( t_sl_exec_file_data *file_data );
static int sl_exec_file_py_despatch( t_sl_exec_file_data *file_data );
static void sl_exec_file_py_add_object_instance( t_sl_exec_file_data *file_data, t_sl_exec_file_object_chain *object_chain );
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
static t_sl_exec_file_variable *sl_exec_file_create_variable( t_sl_exec_file_data *file_data, const char *name, t_sl_exec_file_variable_type type )
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
extern int sl_exec_file_set_variable( t_sl_exec_file_data *file_data, const char *name, t_sl_uint64 *value, double *value_d )
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
extern int sl_exec_file_set_integer_variable( t_sl_exec_file_data *file_data, const char *name, t_sl_uint64 *value )
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

    sl_exec_file_py_add_object_instance( file_data, object );

    return (void *)object;
}

/*f sl_exec_file_object_instance_register_state
 */
extern void sl_exec_file_object_instance_register_state( struct t_sl_exec_file_cmd_cb *cmd_cb, const char *object_name, const char *state_name, void *data, int width, int size )
{
    char buffer[512];
    char *names;
    WHERE_I_AM;
    if (cmd_cb->file_data->register_state_fn)
    {
        WHERE_I_AM;
        snprintf( buffer, 512, "%s.%s", object_name, state_name );
        WHERE_I_AM;
        names = sl_str_alloc_copy(buffer);
        if (size<=0)
        {
            WHERE_I_AM;
            (*(cmd_cb->file_data->register_state_fn))( cmd_cb->file_data->register_state_handle, cmd_cb->file_data->id, names, 0, (int *)(data), width );
        }
        else
        {
            WHERE_I_AM;
            (*(cmd_cb->file_data->register_state_fn))( cmd_cb->file_data->register_state_handle, cmd_cb->file_data->id, names, 1, (int *)(data), width, size );
        }
    }
    WHERE_I_AM;
}

/*f sl_exec_file_object_instance_register_state_desc
 */
extern void sl_exec_file_object_instance_register_state_desc( struct t_sl_exec_file_cmd_cb *cmd_cb, const char *object_name, t_sl_exec_file_state_desc_entry *state_desc_entry_array, void *base_data )
{
    WHERE_I_AM;
    while (state_desc_entry_array->name)
    {
    WHERE_I_AM;
        sl_exec_file_object_instance_register_state( cmd_cb, object_name, state_desc_entry_array->name, (void *)(((char *)base_data)+state_desc_entry_array->offset), state_desc_entry_array->width, state_desc_entry_array->size );
        state_desc_entry_array++;
    WHERE_I_AM;
    }
    WHERE_I_AM;
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
    t_sl_exec_file_lib_chain *lib = NULL;
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
                 cmd_cb.file_data = file_data;
                 cmd_cb.error = file_data->error;
                 cmd_cb.filename = file_data->filename;
                 cmd_cb.line_number = line_number;
                 cmd_cb.cmd -= lib->base;
                 cmd_cb.lib_desc = &(lib->lib_desc);
                 cmd_cb.execution = NULL;
                 cmd_cb.num_args = file_line->num_args;
                 cmd_cb.args = file_line->args;
                 (void) lib->lib_desc.cmd_handler( &cmd_cb, lib->lib_desc.handle ); // ditch the error
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
         (*file_data_ptr)->register_state_fn = NULL;
         (*file_data_ptr)->poststate_callbacks = NULL; // SHOULD FREE THE LIST
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
                   cmd_cb.execution = NULL;
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

/*f sl_exec_file_eval_fn_set_result( file_data*, string )
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

/*f sl_exec_file_eval_fn_set_result( cmd_cb*, string )
 */
extern int sl_exec_file_eval_fn_set_result( t_sl_exec_file_cmd_cb *cmd_cb, const char *string, int copy_string )
{
    return sl_exec_file_eval_fn_set_result( cmd_cb->file_data, string, copy_string );
}

/*f sl_exec_file_eval_fn_set_result( file_data *,integer )
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

/*f sl_exec_file_eval_fn_set_result( cmd_cb *, integer )
 */
extern int sl_exec_file_eval_fn_set_result( t_sl_exec_file_cmd_cb *cmd_cb, t_sl_uint64 result )
{
    return sl_exec_file_eval_fn_set_result( cmd_cb->file_data, result );
}

/*f sl_exec_file_eval_fn_set_result( file_data *, double )
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

/*f sl_exec_file_eval_fn_set_result( cmd_cb *, double )
 */ 
extern int sl_exec_file_eval_fn_set_result( t_sl_exec_file_cmd_cb *cmd_cb, double result )
{
    return sl_exec_file_eval_fn_set_result( cmd_cb->file_data, result );
}

/*f sl_exec_file_eval_fn_get_argument_type( cmd_cb *)
 */
extern t_sl_exec_file_value_type sl_exec_file_eval_fn_get_argument_type( struct t_sl_exec_file_cmd_cb *cmd_cb, int number )
{
    return cmd_cb->args[number].type;
}

/*f sl_exec_file_eval_fn_get_argument_integer( file_data *)
 */
extern t_sl_uint64 sl_exec_file_eval_fn_get_argument_integer( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int number )
{
     WHERE_I_AM;
    return args[number].integer;
}

/*f sl_exec_file_eval_fn_get_argument_integer( cmd_cb * )
 */
extern t_sl_uint64 sl_exec_file_eval_fn_get_argument_integer( struct t_sl_exec_file_cmd_cb *cmd_cb, int number )
{
    WHERE_I_AM;
    return cmd_cb->args[number].integer;
}

/*f sl_exec_file_eval_fn_get_argument_double( file_data *)
 */
extern double sl_exec_file_eval_fn_get_argument_double( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int number )
{
     WHERE_I_AM;
    return args[number].real;
}

/*f sl_exec_file_eval_fn_get_argument_double( cmd_cb *)
 */
extern double sl_exec_file_eval_fn_get_argument_double( struct t_sl_exec_file_cmd_cb *cmd_cb, int number )
{
    WHERE_I_AM;
    return cmd_cb->args[number].real;
}

/*f sl_exec_file_eval_fn_get_argument_string( file_data *)
 */
extern const char *sl_exec_file_eval_fn_get_argument_string( struct t_sl_exec_file_data *file_data, t_sl_exec_file_value *args, int number )
{
    WHERE_I_AM;
    return args[number].p.string;
}

/*f sl_exec_file_eval_fn_get_argument_string( cmd_cb * )
 */
extern const char *sl_exec_file_eval_fn_get_argument_string( struct t_sl_exec_file_cmd_cb *cmd_cb, int number )
{
    WHERE_I_AM;
    return cmd_cb->args[number].p.string;
}

/*f sl_exec_file_eval_fn_get_argument_pointer( cmd_cb * )
 */
extern void *sl_exec_file_eval_fn_get_argument_pointer( struct t_sl_exec_file_cmd_cb *cmd_cb, int number )
{
    WHERE_I_AM;
    return cmd_cb->args[number].p.ptr;
}

/*f sl_exec_file_get_number_of_arguments( file_data *)
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

/*f sl_exec_file_get_number_of_arguments( cmd_cb *)
 */
extern int sl_exec_file_get_number_of_arguments( struct t_sl_exec_file_cmd_cb *cmd_cb )
{
    WHERE_I_AM;
    return cmd_cb->num_args;
}

/*f sl_exec_file_thread_wait_on_callback
 */
extern int sl_exec_file_thread_wait_on_callback( t_sl_exec_file_cmd_cb *cmd_cb, t_sl_exec_file_wait_callback_fn callback, t_sl_exec_file_wait_cb *wait_cb )
{
     WHERE_I_AM;
     if (!cmd_cb)
          return 0;

     WHERE_I_AM;
     if (!cmd_cb->execution)
         return 0;

     WHERE_I_AM;
     cmd_cb->execution->type = sl_exec_file_thread_execution_type_waiting;
     cmd_cb->execution->wait_callback = callback;
     cmd_cb->execution->wait_cb = *wait_cb;
     WHERE_I_AM;
     return 1;
}

/*f
 */
static void display_list_for_cmd( t_sl_exec_file_data *file_data, FILE *f, const char *arg )
{
    if (!strcmp(arg, "libs"))
    {
        t_sl_exec_file_lib_chain *lib;
        fprintf(f,"sl_exec_file libraries for %s:\n", file_data->user );
        for (lib=file_data->lib_chain; lib; lib=lib->next_in_list)
        {
            fprintf(f,"\t%s\n", lib->lib_desc.library_name);
        }
        return;
    }
    if (!strcmp(arg, "fns"))
    {
        t_sl_exec_file_lib_chain *lib;
        fprintf(f,"sl_exec_file functions registered for %s:\n", file_data->user );
        for (lib=file_data->lib_chain; lib; lib=lib->next_in_list)
        {
            int j;
            for (j=0; lib->lib_desc.file_fns && lib->lib_desc.file_fns[j].fn_value!=sl_exec_file_fn_none; j++)
            {
                fprintf(f,"\t%s\n", lib->lib_desc.file_fns[j].fn);
            }
        }
        return;
    }
    if (!strcmp(arg, "cmds"))
    {
        t_sl_exec_file_lib_chain *lib;
        fprintf(f,"sl_exec_file commands registered for %s:\n", file_data->user );
        for (lib=file_data->lib_chain; lib; lib=lib->next_in_list)
        {
            int j;
            for (j=0; lib->lib_desc.file_cmds && lib->lib_desc.file_cmds[j].cmd_value!=sl_exec_file_cmd_none; j++)
            {
                fprintf(f,"\t%s\n", lib->lib_desc.file_cmds[j].cmd);
            }
        }
        return;
    }
    if (!strcmp(arg, "objs"))
    {
        t_sl_exec_file_object_chain *obj;
        fprintf(f,"sl_exec_file objects registered in %s:\n", file_data->user );
        for (obj=file_data->object_chain; obj; obj=obj->next_in_list)
        {
            fprintf(f,"\t%s\n", obj->object_desc.name);
        }
        return;
    }

    t_sl_exec_file_lib_chain *lib;
    for (lib=file_data->lib_chain; lib; lib=lib->next_in_list)
    {
        if (!strcmp(arg, lib->lib_desc.library_name))
        {
            int j;
            fprintf(f,"sl_exec_file library '%s' registered in %s:\n", lib->lib_desc.library_name, file_data->user );
            fprintf(f,"\tCommands:\n");
            for (j=0; lib->lib_desc.file_cmds && lib->lib_desc.file_cmds[j].cmd_value!=sl_exec_file_cmd_none; j++)
            {
                fprintf(f,"\t\t%s\n", lib->lib_desc.file_cmds[j].cmd);
            }
            fprintf(f,"\tFunctions:\n");
            for (j=0; lib->lib_desc.file_fns && lib->lib_desc.file_fns[j].fn_value!=sl_exec_file_fn_none; j++)
            {
                fprintf(f,"\t\t%s\n", lib->lib_desc.file_fns[j].fn);
            }
        }
    }

    t_sl_exec_file_object_chain *obj;
    for (obj=file_data->object_chain; obj; obj=obj->next_in_list)
    {
        if (!strcmp(arg, obj->object_desc.name))
        {
            int j;
            fprintf(f,"sl_exec_file object '%s' registered in %s:\n", obj->object_desc.name, file_data->user );
            fprintf(f,"\tMethods:\n");
            for (j=0; obj->object_desc.methods[j].method; j++ )
            {
                fprintf(f,"\t\t%s\n", obj->object_desc.methods[j].method);
            }
        }
    }
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
     i = 0;
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
                  cmd_cb.execution = &(thread->execution);
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
                      display_list_for_cmd( file_data, stdout, file_data->lines[i].args[0].p.string);
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
        cmd_cb.execution = &(file_data->command_from_thread->execution);
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

/*a Python extensions - only if SL_PYTHON is defined
 */
/*f Wrapper
 */
#ifdef SL_PYTHON

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
} t_py_object;

/*t t_py_object_exec_file_library
 */
typedef struct
{
    PyObject_HEAD
    t_sl_exec_file_data *file_data;
    t_py_object *py_object; // this is a copy of file_data->py_object
    t_sl_exec_file_lib_chain *lib_chain;
} t_py_object_exec_file_library;

/*t t_py_object_exec_file_object
 */
typedef struct
{
    PyObject_HEAD
    t_sl_exec_file_data *file_data;
    t_py_object *py_object; // this is a copy of file_data->py_object
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

/*t t_py_thread_data
 */
typedef struct t_py_thread_data
{
    // This is inside the barrier thread data
    PyThreadState *py_thread;
    PyObject *barrier_thread_object; // A CObject that contains a pointer to this barrier thread, so this can be used as an element of a tuple arg to a PyMethod bound to this thread
    t_sl_exec_file_thread_execution_data execution;
} t_py_thread_data;

/*t t_py_thread_start_data
 */
typedef struct t_py_thread_start_data
{
    // This data exists to get Python threads started.
    t_py_object *py_object;
    PyObject *callable;
    PyObject *args;
    pthread_mutex_t thread_sync_mutex;  // These three are used to spawn and sync a subthread
    pthread_cond_t  thread_sync_cond;   // These three are used to spawn and sync a subthread
    int             thread_spawn_state; // These three are used to spawn and sync a subthread
} t_py_thread_start_data;

/*f sl_exec_file_py_thread
 * Forward declaration
 */
static void sl_exec_file_py_thread( t_py_thread_start_data *py_start_data );
static int sl_exec_file_create_python_thread( t_sl_exec_file_data *file_data, PyObject *callable, PyObject *args );

/*v cmd_*
 */
enum
{
    cmd_pyspawn,
    cmd_pypass,
    cmd_pyfail,
    cmd_pylist,
    cmd_pypassed,
};

/*v py_internal_cmds
 */
static t_sl_exec_file_cmd py_internal_cmds[] = 
{
     {cmd_pyspawn,       2, "pyspawn", "oo",    "spawn <function> <args>" },
     {cmd_pypass,        2, "pypass",  "is",    "pass <integer> <message>" },
     {cmd_pyfail,        2, "pyfail",  "is",    "fail <integer> <message>" },
     {cmd_pylist,        1, "list",    "s",     "list <what>" },
     {sl_exec_file_cmd_none, 0, NULL, NULL, NULL},
};

static t_sl_exec_file_eval_fn py_fn_handler_pypassed;

/*v py_internal_fns
 */
static t_sl_exec_file_fn py_internal_fns[] =
{
     {cmd_pypassed,                   "pypassed",            'i', "", "pypassed()", py_fn_handler_pypassed },
     {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },
};

/*f static py_cmd_handler_cb
 */
static t_sl_error_level py_cmd_handler_cb( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    switch (cmd_cb->cmd)
    {
    case cmd_pylist:
        display_list_for_cmd( cmd_cb->file_data, stdout, cmd_cb->args[0].p.string);
        fflush(stdout);
        break;
    case cmd_pyspawn:
        sl_exec_file_create_python_thread( cmd_cb->file_data, (PyObject*)cmd_cb->args[0].p.ptr, (PyObject*)cmd_cb->args[1].p.ptr );
        break;
    case cmd_pypass: // Use malloc for the string as the exec_file may be freed before the error comes out
        cmd_cb->file_data->message->add_error( (void *)cmd_cb->file_data->user,
                                       error_level_serious,
                                       error_number_sl_pass, error_id_sl_exec_file_get_next_cmd,
                                       error_arg_type_uint64, cmd_cb->args[0].integer,
                                       error_arg_type_malloc_string, cmd_cb->args[1].p.string,
                                       error_arg_type_malloc_string, "PyThread",
                                       error_arg_type_malloc_filename, cmd_cb->file_data->filename,
                                       error_arg_type_line_number, 0,
                                       error_arg_type_none );
        cmd_cb->file_data->completion.passed = 1;
        cmd_cb->file_data->completion.return_code = cmd_cb->args[0].integer;
        break;
    case cmd_pyfail: // Use malloc for the string as the exec_file may be freed before the error comes out
        cmd_cb->file_data->message->add_error( (void *)cmd_cb->file_data->user,
                                       error_level_info,
                                       error_number_sl_fail, error_id_sl_exec_file_get_next_cmd,
                                       error_arg_type_uint64, cmd_cb->args[0].integer,
                                       error_arg_type_malloc_string, cmd_cb->args[1].p.string,
                                       error_arg_type_malloc_string, "PyThread",
                                       error_arg_type_malloc_filename, cmd_cb->file_data->filename,
                                       error_arg_type_line_number, 0,
                                       error_arg_type_none );
        cmd_cb->file_data->completion.failed = 1;
        cmd_cb->file_data->completion.return_code = cmd_cb->args[0].integer;
        break;
    default:
        return error_level_serious;
    }
    return error_level_okay;
}

/*f py_fn_handler_pypassed
 */
static int py_fn_handler_pypassed( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    WHERE_I_AM;

    sl_exec_file_eval_fn_set_result( file_data, (t_sl_uint64) (file_data->completion.passed && !file_data->completion.failed) );
    return 1;
}

/*f start_new_py_thread_started
 * Important: must be called with GIL _not_ held (otherwise deadlock)
 */
static void start_new_py_thread_started( t_py_object *py_object, t_py_thread_start_data *py_start_data )
{
    WHERE_I_AM;

    WHERE_I_AM_TH_STR("threadsync+", pthread_self(), py_start_data, "SNPyTS");
    pthread_mutex_lock( &py_start_data->thread_sync_mutex );
    WHERE_I_AM_TH_STR("threadsync=", pthread_self(), py_start_data, "SNPyTS");
    WHERE_I_AM;
    if (py_start_data->thread_spawn_state==1) // Waiting on signal
    {
    WHERE_I_AM;
        py_start_data->thread_spawn_state = 4;
	WHERE_I_AM_TH_STR("signalsynccond+", pthread_self(), py_start_data, "SNPyTS");
        pthread_cond_signal( &py_start_data->thread_sync_cond );
	WHERE_I_AM_TH_STR("signalsynccond-", pthread_self(), py_start_data, "SNPyTS");
    WHERE_I_AM;
    }
    else
    {
    WHERE_I_AM;
        py_start_data->thread_spawn_state = 3;
    WHERE_I_AM;
    }
    WHERE_I_AM;
    WHERE_I_AM_TH_STR("threadsync-", pthread_self(), py_start_data, "SNPyTS");
    pthread_mutex_unlock( &py_start_data->thread_sync_mutex );
    WHERE_I_AM;
}

/*f start_new_py_thread
 * incoming references are _borrowed_
 */
static int start_new_py_thread( t_py_object *py_object, PyObject *callable, PyObject *args )
{
    pthread_t th;
    int status;
    int result = 1;
    t_py_thread_start_data *py_start_data;

    WHERE_I_AM;
    py_start_data = (t_py_thread_start_data *)malloc(sizeof(t_py_thread_start_data));
    if (py_start_data == NULL)
        return 0;
    py_start_data->py_object = py_object;
    py_start_data->callable = callable;
    py_start_data->args = args;
    WHERE_I_AM_TH_STR("obj+", pthread_self(), py_start_data, "PyStDI");
    int        rc=pthread_mutex_init( &py_start_data->thread_sync_mutex, NULL );
    if (rc==0) rc=pthread_cond_init(  &py_start_data->thread_sync_cond, NULL );
    if (rc!=0)
        return 0;
    Py_INCREF(py_object);
    Py_INCREF(callable);
    Py_XINCREF(args);
    // (corresponding DECREFs for these are at the end of the spawned thread)
    WHERE_I_AM_TH_STR("gil-", pthread_self(), py_start_data, "SNPyTh");
    Py_BEGIN_ALLOW_THREADS;
    WHERE_I_AM;
    WHERE_I_AM_TH_STR("threadsync+", pthread_self(), py_start_data, "SNPyTh");
    pthread_mutex_lock( &py_start_data->thread_sync_mutex );
    WHERE_I_AM_TH_STR("threadsync=", pthread_self(), py_start_data, "SNPyTh");
    WHERE_I_AM;
    py_start_data->thread_spawn_state = 0;
    WHERE_I_AM_TH_STR("gil+", pthread_self(), py_start_data, "SNPyTh");
    Py_END_ALLOW_THREADS;
    WHERE_I_AM_TH_STR("gil=", pthread_self(), py_start_data, "SNPyTh");
    WHERE_I_AM;
    status = pthread_create(&th, (pthread_attr_t*)NULL, (void* (*)(void *))sl_exec_file_py_thread, (void *)py_start_data );
    WHERE_I_AM;
    if (status==0)
    {
        WHERE_I_AM;
        pthread_detach(th);
        if (py_start_data->thread_spawn_state==0)
        {
            WHERE_I_AM;
            py_start_data->thread_spawn_state=1;
	    WHERE_I_AM_TH_STR("gil-", pthread_self(), py_start_data, "SNPyTh");
            Py_BEGIN_ALLOW_THREADS;
            WHERE_I_AM;
	    WHERE_I_AM_TH_STR("threadsync-W", pthread_self(), py_start_data, "SNPyTh");
            pthread_cond_wait( &py_start_data->thread_sync_cond, &py_start_data->thread_sync_mutex );
	    WHERE_I_AM_TH_STR("threadsync=+W", pthread_self(), py_start_data, "SNPyTh");
            WHERE_I_AM;
            py_start_data->thread_spawn_state=5;
            WHERE_I_AM;
	    WHERE_I_AM_TH_STR("gil+", pthread_self(), py_start_data, "SNPyTh");
            Py_END_ALLOW_THREADS;
	    WHERE_I_AM_TH_STR("gil=", pthread_self(), py_start_data, "SNPyTh");
            WHERE_I_AM;
        }
        else
        {
            WHERE_I_AM;
            py_start_data->thread_spawn_state=2;
            WHERE_I_AM;
        }
    }
    WHERE_I_AM;
    WHERE_I_AM_TH_STR("threadsync-", pthread_self(), py_start_data, "SNPyTh");
    pthread_mutex_unlock(  &py_start_data->thread_sync_mutex );
    WHERE_I_AM;
    return result;
}

/*f py_object_init
 */
static int py_object_init( t_py_object *self, PyObject *args, PyObject *kwds )
{
    WHERE_I_AM;
    self->file_data = NULL;
    self->py_interp = NULL;
    self->clocked = 0;
    WHERE_I_AM;
    return 0;
}

/*f py_object_dealloc
 */
static void py_object_dealloc( t_py_object *self )
{
    WHERE_I_AM;
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
        cmd_cb->args[j].p.ptr = NULL;
    }
    for (j=0; (j<SL_EXEC_FILE_MAX_CMD_ARGS) && (j<arg_len);j++)
    {
        PyObject *obj=PyTuple_GetItem( args, j );
        if (arg_string[j]=='s')
        {
            const char *value;
            value = PyString_AsString(obj);
            if (!value)
            {
                fprintf(stderr,"Error occurred parsing string\n");
                if (PyErr_Occurred()) PyErr_PrintEx(1);
                return -1;
            }
            cmd_cb->args[j].type = sl_exec_file_value_type_string;
            cmd_cb->args[j].p.string = sl_str_alloc_copy( value );
        }
        else if (arg_string[j]=='i')
        {
            PyErr_Clear();
            if (PyLong_Check(obj))
            {
                cmd_cb->args[j].integer = PyLong_AsUnsignedLongLongMask(obj);
            }
            else
            {
                cmd_cb->args[j].integer = PyInt_AsSsize_t(obj);
            }
            if (PyErr_Occurred())
            {
                fprintf(stderr,"Error occurred parsing int\n");
                PyErr_PrintEx(1);
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

/*f py_engine_cb_block_on_wait
  If clocked...
  We can either be in init here (in which case sl_exec_file_py_reset is waiting at the barrier for us if we block) OR we have just been running, so we are between tick-start and tick-done barriers.
  If not clocked, barrier_thread and barrier_thread_data will be NULL - just return
 */
static void py_engine_cb_block_on_wait( t_py_object *py_object, t_sl_pthread_barrier_thread_ptr barrier_thread, t_py_thread_data *barrier_thread_data )
{
    WHERE_I_AM;
    if (!barrier_thread_data) return;
    if (!barrier_thread_data->py_thread) return; // If init thread

    // First, if we are not waiting, then just keep going - our clock tick work or reset has not completed yet
    WHERE_I_AM;
    WHERE_I_AM_TH_STR2("entry", pthread_self(), barrier_thread, "PyBoWt");
    if (barrier_thread_data->execution.type == sl_exec_file_thread_execution_type_waiting)
    {
         if (barrier_thread_data->execution.wait_callback(&(barrier_thread_data->execution.wait_cb)))
         {
             barrier_thread_data->execution.type = sl_exec_file_thread_execution_type_running;
         }
    }
    WHERE_I_AM;
    if (barrier_thread_data->execution.type == sl_exec_file_thread_execution_type_running)
        return;

    WHERE_I_AM;
    // First check to see if we are at the init barrier - in which case we move to init_done
    // Note that we hold the Python GIL on entry...
    if (sl_pthread_barrier_thread_get_user_state(barrier_thread)==py_barrier_thread_user_state_init)
    {
        WHERE_I_AM_TH_STR2("init", pthread_self(), barrier_thread, "PyBoWt");
        sl_pthread_barrier_thread_set_user_state(barrier_thread,py_barrier_thread_user_state_running);
        WHERE_I_AM_TH_STR2("running", pthread_self(), barrier_thread, "PyBoWt");
    }
    // Here we are either just after reset, in which case sl_exec_file_py_reset is waiting on post-reset barrier, or after doing a clock tick of work, so clock function is waiting on the tick-done barrier
    WHERE_I_AM;
    while (1)
    {
        WHERE_I_AM_TH_STR2("cb_block_cycle", pthread_self(), barrier_thread, "PyBoWt");
        WHERE_I_AM;
        // Wait for post-reset barrier or tick-done barrier; then wait for tick-start barrier (or pre-finalize barrier if we are being killed off)
        Py_BEGIN_ALLOW_THREADS;
        WHERE_I_AM;
        sl_pthread_barrier_wait( &py_object->barrier, barrier_thread, NULL, (void*)"PyBoW1" );
        WHERE_I_AM;
        sl_pthread_barrier_wait( &py_object->barrier, barrier_thread, NULL, (void*)"PyBoW2" );
        WHERE_I_AM;
        Py_END_ALLOW_THREADS;
        WHERE_I_AM;
        // Now we are after pre-finalize or tick-start; check for pre-finalize
        if (sl_pthread_barrier_thread_get_user_state(barrier_thread)==py_barrier_thread_user_state_die)
        {
            WHERE_I_AM_TH_STR2("exit", pthread_self(), barrier_thread, "PyBoWt");
            PyEval_ReleaseThread(barrier_thread_data->py_thread);
            PyEval_AcquireLock();
            Py_DECREF(barrier_thread_data->barrier_thread_object);
            PyThreadState_Clear(barrier_thread_data->py_thread);
            PyThreadState_Delete(barrier_thread_data->py_thread);
            // sl_pthread_barrier_thread_delete can cause barrier_thread to be freed; barrier_thread_data is potentially part of barrier_thread
            // It seems best to finish using barrier_thread_data before deleting the pthread_barrier_thread, therefore :-)
            sl_pthread_barrier_thread_delete( &py_object->barrier, barrier_thread );
            PyEval_ReleaseLock();
            pthread_exit(NULL);
        }
        // Not told to die - just after tick-start, so check if we should return and do stuff
    WHERE_I_AM;
         if (barrier_thread_data->execution.wait_callback(&(barrier_thread_data->execution.wait_cb)))
         {
             barrier_thread_data->execution.type = sl_exec_file_thread_execution_type_running;
            WHERE_I_AM_TH_STR2("run", pthread_self(), barrier_thread, "PyBoWt");
    WHERE_I_AM;
             return;
         }
         // Okay, tum-te-tum, waiting on the event still...
    }
}

/*f py_engine_cb
  Called with a tuple of (py_thread, py_ef_lib/py_ef_obj, int type (0=>function, 1=>command), int offset (which cmd/function in the library))
 */
static PyObject *py_engine_cb( PyObject* self, PyObject* args )
{
    WHERE_I_AM;
    PyObject *barrier_thread_obj, *py_ef_lib_or_obj_obj;
    t_py_object *py_object;
    t_sl_exec_file_data *file_data;
    long type, offset;
    t_sl_pthread_barrier_thread_ptr barrier_thread;
    t_py_thread_data *barrier_thread_data;
    t_sl_exec_file_cmd_cb cmd_cb;
    t_sl_exec_file_value cmd_cb_args[SL_EXEC_FILE_MAX_CMD_ARGS];

    WHERE_I_AM_TH_STR2("entry", pthread_self(), self, "PyEnCb");

    barrier_thread_obj = PyTuple_GetItem(self, 0);
    py_ef_lib_or_obj_obj = PyTuple_GetItem(self, 1);
    type = PyInt_AsLong(PyTuple_GetItem(self, 2));
    offset = PyInt_AsLong(PyTuple_GetItem(self, 3));

    if ((type>=0) && (type<=1))
    {
        t_py_object_exec_file_library *py_ef_lib;
        py_ef_lib = (t_py_object_exec_file_library *)py_ef_lib_or_obj_obj;
        file_data = py_ef_lib->file_data;
        py_object = py_ef_lib->py_object;
    }
    else
    {
        t_py_object_exec_file_object *py_ef_obj;
        py_ef_obj = (t_py_object_exec_file_object *)py_ef_lib_or_obj_obj;
        file_data = py_ef_obj->file_data;
        py_object = py_ef_obj->py_object;
    }
    Py_XINCREF(py_object);
    barrier_thread = NULL;
    barrier_thread_data = NULL;
    if (py_object->clocked && (barrier_thread_obj!=Py_None))
    {
        // If called from the main thread during init then barrier_thread_obj is Py_None
        WHERE_I_AM_TH_STR2("thread_obj", pthread_self(), barrier_thread_obj, "PyEnCb");
        barrier_thread = (t_sl_pthread_barrier_thread_ptr) PyCObject_AsVoidPtr(barrier_thread_obj); // Can replace with PyCapsule_GetPointer(barrier_thread_data->py_thread_object, NULL);
        if (barrier_thread)
            barrier_thread_data = (t_py_thread_data *)sl_pthread_barrier_thread_get_user_ptr(barrier_thread);
        WHERE_I_AM_TH_STR2("thread?", pthread_self(), barrier_thread, "PyEnCb");
        WHERE_I_AM;
    }
    
    //fprintf(stderr,"self %p\n",self);
    cmd_cb.file_data  = file_data;
    cmd_cb.error      = file_data->error;
    cmd_cb.filename   = file_data->filename;
    cmd_cb.line_number = 0;
    cmd_cb.cmd = 0;
    cmd_cb.args = cmd_cb_args;
    cmd_cb.lib_desc = NULL;
    cmd_cb.execution = barrier_thread_data ? &(barrier_thread_data->execution) : NULL;

    int errored = 0;
    file_data->command_from_thread = NULL;
    if (type==0)
    {
        t_py_object_exec_file_library *py_ef_lib;
        t_sl_exec_file_lib_desc *lib_desc;
        t_sl_exec_file_fn *fn;
        WHERE_I_AM;
        py_ef_lib = (t_py_object_exec_file_library *)py_ef_lib_or_obj_obj;
        lib_desc = &(py_ef_lib->lib_chain->lib_desc);
        cmd_cb.lib_desc = lib_desc;
        fn = &(lib_desc->file_fns[offset]);
        //fprintf(stderr,"call of function %d/%s\n", offset, fn->fn  );
        if (py_engine_cb_args( args, fn->args, &cmd_cb)>=0)
        {
            file_data->eval_fn_result = &py_ef_lib->file_data->cmd_result;
            file_data->eval_fn_result_ok = 0;
            if ( !(fn->eval_fn)( lib_desc->handle, file_data, cmd_cb_args) )
            {
                file_data->eval_fn_result_ok = 0;
            }
            if (fn->result=='s') return PyString_FromString(file_data->cmd_result.p.string);
            if (fn->result=='d') return PyFloat_FromDouble(file_data->cmd_result.real);
            if (fn->result=='i') return PyInt_FromLong(file_data->cmd_result.integer);
        }
        else
        {
            errored = 1;
        }
    }
    else if (type==1)
    {
        // Must ensure that declaratives are only called during init; nondeclaratives can be called at any point
        // Declaratives should probably force adding of stuff to the dict...
        // Do not call them for now please
        t_py_object_exec_file_library *py_ef_lib;
        t_sl_exec_file_lib_desc *lib_desc;
        py_ef_lib = (t_py_object_exec_file_library *)py_ef_lib_or_obj_obj;
        WHERE_I_AM;
        lib_desc = &(py_ef_lib->lib_chain->lib_desc);
        cmd_cb.lib_desc = lib_desc;
        //fprintf(stderr,"invocation of command %p/%d/%s\n", lib_desc, offset, lib_desc->file_cmds[offset].cmd );
        cmd_cb.cmd = lib_desc->file_cmds[offset].cmd_value;
        WHERE_I_AM;
        if (py_engine_cb_args( args, lib_desc->file_cmds[offset].args, &cmd_cb)<0)
        {
            fprintf(stderr,"Misparsed args\n");
            errored = 1;
        }
        else
        {
            WHERE_I_AM;
            if (cmd_cb.num_args<lib_desc->file_cmds[offset].min_args)
            {
                fprintf(stderr,"Short of args %d/%d\n", cmd_cb.num_args,lib_desc->file_cmds[offset].min_args);
            }
            else
            {
                WHERE_I_AM;
                lib_desc->cmd_handler( &cmd_cb, lib_desc->handle );
                WHERE_I_AM;
                // If not clocked, then barrier_thread and barrier_thread_data will be NULL
                py_engine_cb_block_on_wait( py_object, barrier_thread, barrier_thread_data );
                WHERE_I_AM;
            }
        }
    }
    else if (type==2)
    {
        t_py_object_exec_file_object *py_ef_obj;
        t_sl_exec_file_object_desc *object_desc;
        t_sl_exec_file_method *method;
        WHERE_I_AM;
        py_ef_obj = (t_py_object_exec_file_object *)py_ef_lib_or_obj_obj;
        object_desc = &(py_ef_obj->object_chain->object_desc);
        method = &(object_desc->methods[offset]);
        //fprintf(stderr,"call of function %d/%s\n", offset, fn->fn  );
        if (py_engine_cb_args( args, method->args, &cmd_cb)>=0)
        {
            file_data->eval_fn_result = &file_data->cmd_result;
            file_data->eval_fn_result_ok = 0;
            WHERE_I_AM;
            //fprintf(stderr,"%p,%p,%d\n",&cmd_cb,&cmd_cb_args,cmd_cb.num_args);
            if ( !(method->method_fn)( &cmd_cb, method->method_handle, object_desc, method) )
            {
                file_data->eval_fn_result_ok = 0;
            }
            if (method->result=='s') return PyString_FromString(file_data->cmd_result.p.string);
            if (method->result=='d') return PyFloat_FromDouble(file_data->cmd_result.real);
            if (method->result=='i') return PyInt_FromLong(file_data->cmd_result.integer);
            py_engine_cb_block_on_wait( py_object, barrier_thread, barrier_thread_data );
        }
        else
        {
            errored = 1;
        }
    }
    Py_XDECREF(py_object);
    if (errored)
    {
        PyErr_SetString( PyExc_TypeError, "Errored parsing arguments somehow" );
        return NULL;
    }
    Py_INCREF( Py_None );
    return Py_None;
}

/*f py_exec_file_find_thread_cb
 */
typedef struct t_py_exec_file_find_thread
{
    PyThreadState *py_thread;
    t_py_thread_data *barrier_thread_data;
} t_py_exec_file_find_thread;
void py_exec_file_find_thread_cb( void *handle, t_sl_pthread_barrier_thread_ptr barrier_thread )
{
    t_py_exec_file_find_thread *find_thread = (t_py_exec_file_find_thread *)handle;
    t_py_thread_data *barrier_thread_data;
    barrier_thread_data = (t_py_thread_data *)sl_pthread_barrier_thread_get_user_ptr(barrier_thread);
    if (barrier_thread_data->py_thread == find_thread->py_thread)
        find_thread->barrier_thread_data = barrier_thread_data;
}

/*f py_exec_file_library_dealloc
  The library is getting freed, get rid of extraneous references.
 */
static void py_exec_file_library_dealloc( PyObject* py_ef_lib_obj )
{
    t_py_object_exec_file_library *py_ef_lib = (t_py_object_exec_file_library *)py_ef_lib_obj;
    
    Py_XDECREF(py_ef_lib->py_object);
    py_ef_lib_obj->ob_type->tp_free(py_ef_lib_obj);
}


/*f py_exec_file_library_getattro
  If getting a function or command, build a bound method (bound to thread, library, cmd/function call, cmd/function number) and return that
 */
static PyObject *py_exec_file_library_getattro( PyObject *py_ef_lib_obj, PyObject *attr_name )
{
    char *name = PyString_AsString(attr_name);
    t_py_object_exec_file_library *py_ef_lib = (t_py_object_exec_file_library *)py_ef_lib_obj;
    t_sl_exec_file_lib_desc *lib_desc;
    int call_type; // 0 for function
    int fn_or_cmd=0;

    WHERE_I_AM;
    WHERE_I_AM_TH_STR2(name, pthread_self(), NULL, "PyLbGa");

    lib_desc = &(py_ef_lib->lib_chain->lib_desc);
    call_type = -1;
    if (lib_desc->file_fns)
    {
        for (int j=0; lib_desc->file_fns[j].fn_value!=sl_exec_file_fn_none; j++ )
        {
            if ( !strcmp(lib_desc->file_fns[j].fn, name) )
            {
                call_type = 0;
                fn_or_cmd = j;
            }
        }
    }
    if ((call_type<0) && (lib_desc->file_cmds))
    {
        for (int j=0; lib_desc->file_cmds[j].cmd_value!=sl_exec_file_cmd_none; j++ )
        {
            int ofs=0;
            if (lib_desc->file_cmds[j].cmd[0]=='!')
                ofs=1;
            if ( !strcmp(lib_desc->file_cmds[j].cmd+ofs, name) )
            {
                call_type = 1;
                fn_or_cmd = j;
            }
        }
    }
    if (call_type<0)
    {
        return PyObject_GenericGetAttr(py_ef_lib_obj, attr_name);
    }
    
    static PyMethodDef method_def = { "exec_file_callback", py_engine_cb, METH_VARARGS, "sl_exec_file callback"};
    PyObject *tuple;
    PyObject *barrier_thread_object;
    PyThreadState* py_thread;
    t_py_exec_file_find_thread find_thread;

    barrier_thread_object = Py_None;
    if (py_ef_lib->py_object->clocked)
    {
        py_thread = PyThreadState_Get();
        find_thread.py_thread = py_thread;
        find_thread.barrier_thread_data = NULL;
        sl_pthread_barrier_thread_iter( &py_ef_lib->py_object->barrier, py_exec_file_find_thread_cb, (void *)&find_thread );
        if (find_thread.barrier_thread_data)
        {
            barrier_thread_object = find_thread.barrier_thread_data->barrier_thread_object;
        }
    }

    Py_INCREF( barrier_thread_object );
    Py_INCREF( py_ef_lib_obj );
    tuple = PyTuple_New(4);
    PyTuple_SetItem(tuple, 0, barrier_thread_object );
    PyTuple_SetItem(tuple, 1, py_ef_lib_obj);
    PyTuple_SetItem(tuple, 2, PyInt_FromLong(call_type));
    PyTuple_SetItem(tuple, 3, PyInt_FromLong(fn_or_cmd));
    WHERE_I_AM_TH_STR2("tuple", pthread_self(), tuple, "PyLbGa");
    WHERE_I_AM_TH_STR2("thread_obj", pthread_self(), barrier_thread_object, "PyLbGa");
    return PyCFunction_New( &method_def, tuple );
}

/*f py_exec_file_object_getattro
 */
static PyObject *py_exec_file_object_getattro( PyObject *py_ef_obj_obj, PyObject *attr_name )
{
    char *name = PyString_AsString(attr_name);
    t_py_object_exec_file_object *py_ef_obj = (t_py_object_exec_file_object *)py_ef_obj_obj;
    t_sl_exec_file_object_desc *obj_desc;
    int call_type; // 0 for function
    int fn_or_cmd;

    WHERE_I_AM;
    WHERE_I_AM_TH_STR2(name, pthread_self(), NULL, "PyObGa");

    obj_desc = &(py_ef_obj->object_chain->object_desc);
    call_type = -1;
    if (obj_desc->methods)
    {
        for (int j=0; obj_desc->methods[j].method; j++ )
        {
            if ( !strcmp(obj_desc->methods[j].method, name) )
            {
                call_type = 2;
                fn_or_cmd = j;
            }
        }
    }
    if (call_type<0)
    {
        return PyObject_GenericGetAttr(py_ef_obj_obj, attr_name);
    }
    
    static PyMethodDef method_def = { "exec_file_callback", py_engine_cb, METH_VARARGS, "sl_exec_file callback"};
    PyObject *tuple;
    PyObject *barrier_thread_object;
    PyThreadState* py_thread;
    t_py_exec_file_find_thread find_thread;

    barrier_thread_object = Py_None;
    if (py_ef_obj->py_object->clocked)
    {
        py_thread = PyThreadState_Get();
        find_thread.py_thread = py_thread;
        find_thread.barrier_thread_data = NULL;
        sl_pthread_barrier_thread_iter( &py_ef_obj->py_object->barrier, py_exec_file_find_thread_cb, (void *)&find_thread );
        if (find_thread.barrier_thread_data)
        {
            barrier_thread_object = find_thread.barrier_thread_data->barrier_thread_object;
        }
    }

    Py_INCREF( barrier_thread_object );
    Py_INCREF( py_ef_obj_obj );
    tuple = PyTuple_New(4);
    PyTuple_SetItem(tuple, 0, barrier_thread_object );
    PyTuple_SetItem(tuple, 1, py_ef_obj_obj);
    PyTuple_SetItem(tuple, 2, PyInt_FromLong(call_type));
    PyTuple_SetItem(tuple, 3, PyInt_FromLong(fn_or_cmd));
    WHERE_I_AM_TH_STR2("tuple", pthread_self(), tuple, "PyObGa");
    WHERE_I_AM_TH_STR2("thread_obj", pthread_self(), barrier_thread_object, "PyObGa");
    return PyCFunction_New( &method_def, tuple );
}

/*v py_object_methods
 */
static PyMethodDef py_object_methods[] = { {NULL} };

/*v py_class_object__exec_file
 */
static PyTypeObject py_class_object__exec_file = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                         /*ob_size*/
    "py_engine.exec_file",             /*tp_name*/
    sizeof(t_py_object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)py_object_dealloc, /*tp_dealloc*/
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
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT |
    Py_TPFLAGS_BASETYPE | 
    Py_TPFLAGS_HAVE_CLASS,        /*tp_flags*/
    "exec_file objects",           /* tp_doc */
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
    0, /*tp_new - does not seem to be called*/
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
    py_exec_file_library_dealloc, /*tp_dealloc*/
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
  This is the function invoked for every new pthread.
  The py_start_data contains the mutex and condition for the thread start synchronization
  and the py_callable that should be invoked for the thread
 */
static void barrier_thread_object_destroy_callback( void *p )
{
    WHERE_I_AM_TH_STR2("thread#", pthread_self(), p, "PyThDst");
}
int pending_signals = 0;
static void py_thread_signal( int sig )
{
    //fprintf(stderr,"Thread %p received signal %d pending %x\n", pthread_self(), sig, pending_signals );
    pending_signals |= (1<<sig);
}
static void sl_exec_file_py_thread( t_py_thread_start_data *py_start_data )
{
    PyThreadState* py_thread;
    PyObject *py_callable = py_start_data->callable;
    PyObject *py_args = py_start_data->args;
    t_sl_pthread_barrier_thread_ptr barrier_thread;
    t_py_thread_data *barrier_thread_data;
    t_py_object *py_object = py_start_data->py_object;
    PyObject *barrier_thread_object;

    signal( 2, py_thread_signal ); // Catch ctrl-C in this thread; it will cause the signal to go to the parent 

    WHERE_I_AM;
    //fprintf(stderr,"sl_exec_file_py_thread:%p:%p\n",py_object, py_callable);
    barrier_thread = sl_pthread_barrier_thread_add( &py_object->barrier, sizeof(t_py_thread_data) );
    barrier_thread_data = (t_py_thread_data *)sl_pthread_barrier_thread_get_user_ptr(barrier_thread);
    WHERE_I_AM_TH_STR2("thread+", pthread_self(), barrier_thread, "PyThrd");
    WHERE_I_AM;

    sl_pthread_barrier_thread_set_user_state( barrier_thread, py_barrier_thread_user_state_init );

    WHERE_I_AM_TH_STR2("initgil+", pthread_self(), &py_object->barrier, "PyThrd");
    PyEval_AcquireLock();
    WHERE_I_AM_TH_STR2("initgil=", pthread_self(), &py_object->barrier, "PyThrd");
    py_thread = PyThreadState_New(py_object->py_interp);
    PyThreadState_Clear(py_thread);
    barrier_thread_data->py_thread = py_thread;
    barrier_thread_data->execution.type = sl_exec_file_thread_execution_type_running;
    barrier_thread_data->barrier_thread_object = PyCObject_FromVoidPtr( (void *)barrier_thread, barrier_thread_object_destroy_callback ); // Can replace with PyCapsule_New( (void *)py_thread, NULL, NULL );
    WHERE_I_AM_TH_STR2("ref+", pthread_self(), barrier_thread_data->barrier_thread_object, "PyThrd");
    WHERE_I_AM_TH_STR2("initgil-", pthread_self(), &py_object->barrier, "PyThrd");
    PyEval_ReleaseLock();

    // Important: must be called with GIL _not_ held (otherwise deadlock)
    start_new_py_thread_started( py_object, py_start_data );

    // Invoke callable 
    // This is expected to call numerous waits
    // It might also spawn more subthreads
    // If it does so, those subthreads will be added to the barrier, before this one hits the barrier
    // This breaks if this thread spawns subthreads and then finishes - as this thread will not hit a barrier at all but die gracefully
    // However, before this one can die the subthreads will have added themselves to the barrier also - so that barrier will still work cleanly.
    // Basically, this means that sl_exec_file_py_reset should launch threads and then barrier wait - all subthreads should either have died or hit the barrier in state 'init_done'.
    // Then the world is synchronized
    PyObject *py_obj;
    WHERE_I_AM_TH_STR("gilthrd+", pthread_self(), &py_object->barrier, "PyThrd");
    PyEval_AcquireThread(py_thread);
    WHERE_I_AM_TH_STR("gilthrd=", pthread_self(), &py_object->barrier, "PyThrd");
    WHERE_I_AM;
    WHERE_I_AM_TH_STR2("pycall+", pthread_self(), py_callable, "PyThrd");
    py_obj = PyObject_CallObject( py_callable, py_args );
    WHERE_I_AM_TH_STR2("pycall-", pthread_self(), py_callable, "PyThrd");
    Py_DECREF(py_callable);
    Py_XDECREF(py_args);
    WHERE_I_AM;
    if (PyErr_Occurred()) {PyErr_Print();PyErr_Clear();}
    if (py_obj)
    {
        WHERE_I_AM;
        Py_DECREF(py_obj);
    }
    WHERE_I_AM;
    WHERE_I_AM_TH_STR2("gilthrd-", pthread_self(), &py_object->barrier, "PyThrd");
    PyEval_ReleaseThread(py_thread);

    // Save the thread object because we're about to deallocate where it lives.
    barrier_thread_object = barrier_thread_data->barrier_thread_object;
    WHERE_I_AM;
    WHERE_I_AM_TH_STR2("thread-", pthread_self(), barrier_thread, "PyThrd");
    sl_pthread_barrier_thread_delete( &py_object->barrier, barrier_thread );
    WHERE_I_AM;

    WHERE_I_AM;
    PyEval_AcquireLock();
    WHERE_I_AM_TH_STR2("ref-", pthread_self(), barrier_thread_object, "PyThrd");
    Py_DECREF(barrier_thread_object);
    Py_DECREF(py_object);
    PyThreadState_Clear(py_thread);
    PyThreadState_Delete(py_thread);
    PyEval_ReleaseLock();
    WHERE_I_AM;

    // Free the start data
    WHERE_I_AM_TH_STR2("obj-", pthread_self(), py_start_data, "PyStDD");
    pthread_mutex_destroy( &py_start_data->thread_sync_mutex );
    pthread_cond_destroy(  &py_start_data->thread_sync_cond  );
    free(py_start_data);

    WHERE_I_AM;
    pthread_exit(NULL);
    WHERE_I_AM;
}

/*f sl_exec_file_python_add_class_object
 */
extern int sl_exec_file_python_add_class_object( PyObject *module )
{
    // Apparently some platforms/compilers complain about static initialization
    // of a structure member with an external function. So we initialize here.
    // Sounds like FUD to me but that's what the Python manual states.
    py_class_object__exec_file.tp_new = PyType_GenericNew; 
    if (PyType_Ready(&py_class_object__exec_file) < 0)
        return error_level_fatal;

    // Python's object model breaks aliasing; here we have to incref a type object which is
    // not a python object and gcc warns (as we do not want to compile with -fno-strict-aliasing)
    // So we hack around it as we 'know what we are doing'.
    PyObject *pyobj_ef_alias = (PyObject *)((void *)&py_class_object__exec_file);
    Py_INCREF(pyobj_ef_alias);
    PyModule_AddObject(module, "exec_file", (PyObject *)&py_class_object__exec_file);
    return error_level_okay;
}

/*f sl_exec_file_py_add_object_instance
 */
static void sl_exec_file_py_add_object_instance( t_sl_exec_file_data *file_data, t_sl_exec_file_object_chain *object_chain )
{
    t_py_object_exec_file_object *py_ef_obj;

    WHERE_I_AM;
    if (!file_data->py_object) return;

    WHERE_I_AM;
    //fprintf(stderr,"Adding object %s to object (%p)\n",object_chain->object_desc.name,object_chain);
    py_ef_obj = PyObject_New( t_py_object_exec_file_object, &py_object_exec_file_object );
    py_ef_obj->py_object = PyObj(file_data);
    py_ef_obj->file_data = file_data;
    py_ef_obj->object_chain = object_chain;
    WHERE_I_AM;
    PyObject_SetAttrString( file_data->py_object, (char *)(object_chain->object_desc.name), (PyObject *)py_ef_obj );
    WHERE_I_AM;
}

/*f sl_exec_file_create_python_thread
  takes file data, a callable and args
  spawns a new Python thread
  kicks it into life
  returns 1 if OK, 0 if something bad happened
  incoming references are borrowed, must INCREF them to keep them
 */
static int sl_exec_file_create_python_thread( t_sl_exec_file_data *file_data, PyObject *callable, PyObject *args )
{
    if (!file_data->py_object) return 0;
    return start_new_py_thread( PyObj(file_data), callable, args );
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
                                                                  PyObject *py_object_obj,
                                                                  struct t_sl_exec_file_data **file_data_ptr,
                                                                  const char *user,
                                                                  int clocked )
{
    WHERE_I_AM;
    t_py_object *py_object = (t_py_object *)py_object_obj;

    /*b Check the object is derived from the sl_exec_file class
     */
    if (!PyObject_IsInstance(py_object_obj, (PyObject *)&py_class_object__exec_file))
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
    Py_INCREF(py_object_obj);
    (*file_data_ptr)->py_object = py_object_obj;

    /*b Add default commands and functions
     */
    WHERE_I_AM;

    sl_exec_file_data_add_default_cmds( *file_data_ptr );

    /*b Add Python-specific commands and functions
     */
    t_sl_exec_file_lib_desc lib_desc;
    lib_desc.version = sl_ef_lib_version_cmdcb;
    lib_desc.library_name = "py";
    lib_desc.handle = NULL;
    lib_desc.cmd_handler = py_cmd_handler_cb;
    lib_desc.file_cmds = py_internal_cmds;
    lib_desc.file_fns = py_internal_fns;
    sl_exec_file_add_library( *file_data_ptr, &lib_desc );


    /*b Invoke callback to fill in extra functions and add other libraries; old exec file declared objects, not sure what to do about that here
     */
    WHERE_I_AM;

    if (callback_fn)
    {
        WHERE_I_AM;
        callback_fn( callback_handle, *file_data_ptr );
    }

    /*b Add in objects for each sl_exec_file library
     */
    {
        t_sl_exec_file_lib_chain *chain;

        WHERE_I_AM;
        for (chain=(*file_data_ptr)->lib_chain; chain; chain=chain->next_in_list)
        {
            WHERE_I_AM;
            //fprintf(stderr,"Adding library %s to object (%p)\n",chain->lib_desc.library_name,chain);
            t_py_object_exec_file_library *py_ef_lib;
            py_ef_lib = PyObject_New( t_py_object_exec_file_library, &py_object_exec_file_library );
            Py_INCREF(py_object);
            py_ef_lib->py_object = py_object;
            py_ef_lib->file_data = *file_data_ptr;
            py_ef_lib->lib_chain = chain;
            WHERE_I_AM;
            PyObject_SetAttrString( py_object_obj, (char *)(chain->lib_desc.library_name), (PyObject *)py_ef_lib );
            WHERE_I_AM;
        }
    }

    /*b Find the py_interp we are in, and create a barrier thread, if clocked
     */
    WHERE_I_AM;
    if (clocked)
    {
        PyThreadState* py_thread;
        t_sl_pthread_barrier_thread_ptr barrier_thread;
        t_py_thread_data *barrier_thread_data;
         
        WHERE_I_AM;
        py_object->clocked = 1;
        PyEval_InitThreads();
        py_thread = PyThreadState_Get();
        py_object->py_interp = py_thread->interp;
        sl_pthread_barrier_init( &py_object->barrier );
        barrier_thread = sl_pthread_barrier_thread_add( &py_object->barrier, sizeof(t_py_thread_data) );
        barrier_thread_data = (t_py_thread_data *)sl_pthread_barrier_thread_get_user_ptr(barrier_thread);
        barrier_thread_data->py_thread = NULL;
        py_object->barrier_thread = barrier_thread;
    }

    /*b Invoke 'exec_init' method
     */
    WHERE_I_AM;
    PyObject *result;
    result = PyObject_CallMethod( (*file_data_ptr)->py_object, (char *)"exec_init", NULL );
    (*file_data_ptr)->during_init = 0;
    if (result) {Py_DECREF(result);}
    if (PyErr_Occurred()) {PyErr_Print();PyErr_Clear();}

    /*b Finish registering state
     */
    WHERE_I_AM;
    {
        t_sl_exec_file_callback *efc;
        for (efc=(*file_data_ptr)->poststate_callbacks; efc; efc=efc->next_in_list)
        {
            (efc->callback_fn)( efc->handle );
        }
        (*file_data_ptr)->register_state_fn = NULL;
        (*file_data_ptr)->poststate_callbacks = NULL; // SHOULD FREE THE LIST
    }

    /*b Add in objects for each sl_exec_file object
     */
    {
        t_sl_exec_file_object_chain *chain;
        for (chain=(*file_data_ptr)->object_chain; chain; chain=chain->next_in_list)
        {
            sl_exec_file_py_add_object_instance( *file_data_ptr, chain );
        }
    }

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
    WHERE_I_AM;

    sl_pthread_barrier_thread_set_user_state( barrier_thread, py_barrier_thread_user_state_die );
    WHERE_I_AM;
}

/*f sl_exec_file_py_kill_subthreads
 */
static void sl_exec_file_py_kill_subthreads( t_sl_exec_file_data *file_data )
{
    t_py_object *py_object = PyObj(file_data);
    WHERE_I_AM;

    if (!py_object->clocked) return;

    // We want to kill all subthreads - we don't really care about the order
    // So we broadcast to each subthread a user state of 'die'
    // Then we hit the barrier; some may die before they hit the barrier (no harm done), others will die after the barrier
    // So we hit the barrier again; we should come through cleanly when the subthreads die
    WHERE_I_AM;
    sl_pthread_barrier_thread_iter( &py_object->barrier, sl_exec_file_py_kill_subthreads_cb, py_object );
    WHERE_I_AM;
    Py_BEGIN_ALLOW_THREADS;
    sl_pthread_barrier_wait( &py_object->barrier, py_object->barrier_thread, NULL, (void*)"PyKST1" );
    WHERE_I_AM;
    sl_pthread_barrier_wait( &py_object->barrier, py_object->barrier_thread, NULL, (void*)"PyKST2" );
    Py_END_ALLOW_THREADS;
    WHERE_I_AM;
}

/*f sl_exec_file_py_despatch
 */
static int sl_exec_file_py_despatch( t_sl_exec_file_data *file_data )
{
    t_py_object *py_object = PyObj(file_data);
    if (py_object->clocked)
    {
        WHERE_I_AM;
        Py_BEGIN_ALLOW_THREADS;
        sl_pthread_barrier_wait( &py_object->barrier, py_object->barrier_thread, NULL, (void*)"PyDes1" );
        WHERE_I_AM;
        sl_pthread_barrier_wait( &py_object->barrier, py_object->barrier_thread, NULL, (void*)"PyDes2" );
        WHERE_I_AM;
        Py_END_ALLOW_THREADS;
        if (pending_signals)
        {
            sl_exec_file_py_kill_subthreads( file_data );
            //fprintf(stderr,"Python thread has pending signals %x\n", pending_signals );
            signal( 2, NULL );
            int i;
            for (i=0; i<10; i++)
            {
                if (pending_signals&(1<<i))
                {
                    pending_signals = pending_signals &~ (1<<i);
                    kill(getpid(),i);
                }
            }
        }
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
        if (PyErr_Occurred()) {PyErr_Print();PyErr_Clear();}
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
        if (PyErr_Occurred()) {PyErr_Print();PyErr_Clear();}
        if (result) {Py_DECREF(result);}
        return;
    }

    // Kill subthreads
    WHERE_I_AM;
    sl_exec_file_py_kill_subthreads( file_data );
    WHERE_I_AM;

    // Create new subthread and execute 'exec_run' method in that - the subthread hits reset complete barrier after the exec_run completes
    PyObject *exec_run_obj = PyObject_GetAttrString((PyObject*)py_object, "exec_run");
    if (PyErr_Occurred()) 
    {
        PyErr_Print();
        PyErr_Clear();
        return;
    }
    if (!start_new_py_thread( py_object, exec_run_obj, NULL ))
    {
    WHERE_I_AM;
        fprintf(stderr,"Failed to start new subthread in sl_exec_file.cpp - argh\n");
        exit(4);
    }
    WHERE_I_AM;
    Py_DECREF(exec_run_obj);

    // Reset complete barrier - all subthreads will have started and changed state to 'init_done'
    Py_BEGIN_ALLOW_THREADS;
    sl_pthread_barrier_wait( &py_object->barrier, py_object->barrier_thread, NULL, (void*)"PyPRST" );
    Py_END_ALLOW_THREADS;
}

/*f Wrapper ELSE
 */
#else
static void sl_exec_file_py_reset( t_sl_exec_file_data *file_data ){}
static int sl_exec_file_py_despatch( t_sl_exec_file_data *file_data ){return 0;}
static void sl_exec_file_py_add_object_instance( t_sl_exec_file_data *file_data, t_sl_exec_file_object_chain *object_chain ) {}
extern t_sl_error_level sl_exec_file_allocate_from_python_object( c_sl_error *error,
                                                                  c_sl_error *message,
                                                                  t_sl_exec_file_callback_fn callback_fn,
                                                                  void *callback_handle,
                                                                  const char *id,
                                                                  PyObject *py_object_obj,
                                                                  struct t_sl_exec_file_data **file_data_ptr,
                                                                  const char *user,
                                                                  int clocked )
{
    *file_data_ptr = NULL;
    return error->add_error( (void *)user, error_level_fatal, error_number_general_error_s, error_id_sl_exec_file_allocate_and_read_exec_file, error_arg_type_malloc_string, "Cannot allocate exec file from Python object in non-Python build", error_arg_type_none );
}

/*f Wrapper End
 */
#endif // ifdef SL_PYTHON

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


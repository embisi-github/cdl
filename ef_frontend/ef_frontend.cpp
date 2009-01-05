/*a Copyright
  
  This file 'ef_frontend.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include <stdarg.h>
#include <getopt.h>
#include "sl_debug.h"
#include "c_sl_error.h"
#include "sl_option.h"
#include "sl_exec_file.h"
#include "c_model_descriptor.h"
#include "ef_errors.h"

/*a Defines
 */
#define MAX_MD_TEMPS (8)

/*a Types
 */
/*t options
 */
enum 
{
     option_help,
     option_ef_debug,
};

/*t cmd_*
 */
enum
{
    cmd_struct,
     cmd_struct_elt,
     cmd_struct_elt_type,
     cmd_struct_elt_type_array,
     cmd_struct_elt_bv_array,
     cmd_module,
     cmd_prototype,
     cmd_clock,
     cmd_input,
     cmd_output,
     cmd_comb_input,
     cmd_clocked_input,
     cmd_comb_output,
     cmd_clocked_output,
     cmd_comb,
     cmd_net,
     cmd_state,
     cmd_state_type,
     cmd_state_type_array,
     cmd_state_bv_array,
     cmd_state_reset,
     cmd_code,
     cmd_push_int,
     cmd_push_var,
     cmd_push_lvar,
     cmd_subscript,
     cmd_assign_comb,
     cmd_assign_state,
     cmd_if,
     cmd_ifelse,
     cmd_expr,
     cmd_lvar,
     cmd_lvar_index,
     cmd_lvar_range,
     cmd_lvar_bit,
     cmd_port_lvar,
     cmd_group,
     cmd_case,
     cmd_casex,
     cmd_case_exp,
     cmd_default,
     cmd_switch,
     cmd_priority,
     cmd_instantiate,
     cmd_inst_clock,
     cmd_inst_input,
     cmd_inst_output,
     cmd_addcode
};

/*t t_expr_op
 */
typedef struct t_expr_op
{
     const char *text;
     t_md_expr_fn fn;
     int num_args;
} t_expr_op;

/*t t_stack_entry
 */
typedef struct t_stack_entry
{
     struct t_stack_entry *next_in_stack;
     struct t_stack_entry *prev_in_stack;
     union 
     {
          t_md_expression *expression;
          t_md_switch_item *switch_item;
          t_md_statement *statement;
     } data;
} t_stack_entry;

/*t t_stack
 */
typedef struct t_stack
{
     t_stack_entry *stack_top;
     t_stack_entry *stack_bottom;
     int stack_depth;
} t_stack;

/*t t_data
 */
typedef struct
{
    c_model_descriptor *model;
    c_sl_error *error;
    t_sl_option_list env_options;
    t_md_module *module;
    t_stack expr_stack;
    t_stack stmt_stack;
    t_stack switem_stack;
    t_md_type_definition_handle types[4];
    t_md_lvar *lvar;
    t_md_port_lvar *port_lvar;
} t_data;

/*a Statics
 */
/*v ef_debug
 */
static int ef_debug;

/*v file_cmds
 */
//    option
//    force
static t_sl_exec_file_cmd file_cmds[] =
{
     {cmd_struct,          2, "struct", "si", "struct <name> <num_elements>"},
     {cmd_struct_elt,      3, "struct_elt", "sisi", "strelt <name> <el_number> <el_name> <width>"},
     {cmd_struct_elt_type, 4, "struct_elt_type", "siss", "strelt <name> <el_number> <el_name> <el_type>"},
     {cmd_struct_elt_type_array, 5, "struct_elt_type_array", "sisis", "strelttypearray <name> <el_number> <el_name> <array size> <el_type>"},
     {cmd_struct_elt_bv_array, 5, "struct_elt_bv_array", "sisii", "streltbvarray <name> <el_number> <el_name> <array size> <vector size>"},
     {cmd_module,        1, "module", "s", "module <module>"},
     {cmd_prototype,     1, "prototype", "s", "prototype <module name>"},
     {cmd_clock,         1, "clock", "s", "clock <name>"},
     {cmd_input,         1, "input", "si", "input <name> [<width>]"},
     {cmd_output,        1, "output", "si", "output <name> [<width>]"},
     {cmd_clocked_input, 3, "input_clocked", "sis", "input_clocked <clock name> <edge> <name>"},
     {cmd_comb_input,    1, "input_comb", "s", "input_comb <name>"},
     {cmd_clocked_output,3, "output_clocked", "sis", "output <clock name> <edge> <name>"},
     {cmd_comb_output,   1, "output_comb", "s", "output_comb <name>"},
     {cmd_comb,          1, "comb", "si", "comb <name> [<width>]"},
     {cmd_net,           1, "net", "si", "net <name> [<width>]"},
     {cmd_state,         7, "state", "sisisii", "state <name> <width> <clock> <edge> <reset> <level> <reset_value>"},
     {cmd_state_type,    6, "state_type", "sssisi", "statetype <name> <type> <clock> <edge> <reset> <level>"},
     {cmd_state_type_array,  7, "state_type_array", "ssisisi", "statetype <name> <type> <array size> <clock> <edge> <reset> <level>"},
     {cmd_state_bv_array,    7, "state_bv_array", "siisisi", "statetype <name> <array size> <vector size> <clock> <edge> <reset> <level>"},
     {cmd_state_reset,   3, "state_reset", "sii", "statereset <name> <width> <reset_value>"},
     {cmd_code,          1, "code", "s", "code <name>"},
     {cmd_push_int,      1, "push_int", "ii", "pushint <width> <integer> - pushes expression"},
     {cmd_push_var,      1, "push_var", "s", "pushvar <input/comb/state> - pushes expression"},
     {cmd_push_lvar,     0, "push_lvar", "", "pushlvar - pushes top lvar as expression"},
     {cmd_expr,          1, "expr", "s", "expr <op> - uses top (1/2/3) expressions, pushes expression"},
     {cmd_lvar,          1, "lvar", "s", "lvar <name> - picks state, input, output, comb, or element of last lvar"},
     {cmd_lvar_index,    1, "lvar_index", "i", "lvarindex <integer> - sets integer index for last lvar"},
     {cmd_lvar_range,    1, "lvar_range", "ii", "lvarrange <length> [<lowest integer>] - sets bit range for last lvar; if no lowest value given, uses expression on stack"},
     {cmd_lvar_bit,      0, "lvar_bit", "i", "lvarbit <integer> - selects bit from last lvar; if no integer give, uses expression on stack"},
     {cmd_port_lvar,     1, "port_lvar", "s", "port_lvar <name> - picks input or output port or element of last port_lvar"},
     {cmd_assign_comb,   0, "assign_comb", "s", "assign_comb - uses top expression and lvar, pushes statement"},
     {cmd_assign_state,  0, "assign_state", "s", "assign_state  - uses top expression and lvar, pushes statement"},
     {cmd_if,            0, "if", "", "if"},
     {cmd_ifelse,        0, "ifelse", "", "ifelse"},
     {cmd_group,         1, "group", "i", "group <number statements> - uses top 'n' statements, pushes statement"},
     {cmd_case,          1, "case", "ii", "case <width> <integer> - uses top statement, pushes case item"},
     {cmd_casex,         2, "casex", "iii", "case <width> <integer> <mask - 0's for x's> - uses top statement, pushes case item"},
     {cmd_case_exp,      0, "case_exp", "", "case_exp - uses top expression and statement, pushes case item"},
     {cmd_switch,        1, "switch", "i", "switch <num items> - guaranteed parallel switch - uses top 'n' case items, pushes statement"},
     {cmd_priority,      1, "priority", "i", "priority <num items> - prioritized (non-parallel) switch - uses top 'n' case items, pushes statement"},
     {cmd_instantiate,   2, "instantiate", "ss", "instantiate <type> <name> - instantiate a module of type 'type' with name 'name'"},
     {cmd_inst_clock,    3, "inst_clock", "sss", "inst_clock <name> <port> <clock signal> - assign a clock to a named instance's port"},
     {cmd_inst_input,    1, "inst_input", "s", "inst_input <instance> - assign top expression as input to last port_lvar"},
     {cmd_inst_output,   1, "inst_output", "s", "inst_output <instance> - assign lvar as driven by the port_lvar"},
     {cmd_addcode,       1, "add_code", "is", "addcode <number statements> <code block> - uses top 'n' statements"},
     {sl_exec_file_cmd_none, 0, NULL, NULL, NULL }
};

/*v options
 */
static option options[] = {
     BE_OPTIONS
     SL_OPTIONS
     { "help", no_argument, NULL, option_help },
     { "ef_debug", no_argument, NULL, option_ef_debug },
     { NULL, no_argument, NULL, 0 }
};

#if 0
/*v md_temps
 */
static t_md_signal_value md_temps[MAX_MD_TEMPS];

/*v expr_ops
 */
static t_expr_op expr_ops[] = 
{
     { "neg", md_expr_fn_neg, 1 },
     { "add", md_expr_fn_add, 2 },
     { "sub", md_expr_fn_sub, 2 },
     { "not", md_expr_fn_not, 1 },
     { "and", md_expr_fn_and, 2 },
     { "or", md_expr_fn_or, 2 },
     { "bic", md_expr_fn_bic, 2 },
     { "xor", md_expr_fn_xor, 2 },
     { "eq", md_expr_fn_eq, 2 },
     { "neq", md_expr_fn_neq, 2 },
     { NULL, md_expr_fn_none, -1 }
};
#endif

/*v default_error_messages
 */
static t_sl_error_text default_error_messages[] = {
     C_EF_ERROR_DEFAULT_MESSAGES
};

/*a Usage
 */
/*f usage
 */
static void usage( void )
{
     printf( "Usage: [options] file\n");
     sl_option_getopt_usage();
     printf( "\t--model <name> \t\tSpecify the name of the model\n");
     printf( "\t--cpp <file> \t\tOutput CPP model file\n");
     printf( "\t--vhdl <file> \t\tOutput VHDL file\n");
     printf( "\t--help \t\tDisplay this help message\n");
}

/*a Output functions for debug
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
               fprintf( f, indent_string );
     }
     vfprintf( f, format, ap );
     fflush(f);
     va_end( ap );
}

#if 0
/*a Build functions
 */
/*f md_temp_value_from_integer
 */
static t_md_signal_value *md_temp_value_from_integer( int slot, int width, int value )
{
     md_temps[slot].width = width;
     md_temps[slot].value[0] = value;
     return &(md_temps[slot]);
}

/*f switch_item_push
 */
static void switch_item_push( t_stack *expr_stack, t_md_switch_item *switch_item )
{
     t_stack_entry *entry;

     entry = (t_stack_entry *)malloc(sizeof(t_stack_entry));
     entry->next_in_stack = NULL;
     entry->prev_in_stack = expr_stack->stack_top;
     entry->data.switch_item = switch_item;
     if (expr_stack->stack_top)
     {
          expr_stack->stack_top->next_in_stack = entry;
          expr_stack->stack_depth++;
     }
     else
     {
          expr_stack->stack_bottom = entry;
          expr_stack->stack_depth = 1;
     }
     expr_stack->stack_top = entry;
}

/*f switch_item_pop
 */
static t_md_switch_item *switch_item_pop( t_sl_exec_file_data *exec_file_data, t_stack *expr_stack )
{
     t_stack_entry *last;
     t_md_switch_item *result;

     result = NULL;
     if (expr_stack->stack_top)
     {
          last = expr_stack->stack_top;
          expr_stack->stack_top = last->prev_in_stack;
          expr_stack->stack_depth--;
          result = last->data.switch_item;
          free(last);
     }
     else
     {
          sl_exec_file_error( exec_file_data)->add_error( NULL, error_level_fatal, error_number_ef_stack_underflow, error_id_ef_frontend_main,
                                                          error_arg_type_const_string, "case item",
                                                          error_arg_type_malloc_filename, sl_exec_file_filename( exec_file_data ),
                                                          error_arg_type_line_number, sl_exec_file_line_number( exec_file_data ),
                                                          error_arg_type_none );
     }
     return result;
}

/*f switch_item_at_depth
 */
static t_md_switch_item *switch_item_at_depth( t_stack *expr_stack, int depth )
{
     t_stack_entry *ptr;

     ptr = expr_stack->stack_top;
     for (; depth>0 && (ptr); ptr=ptr->prev_in_stack, depth--);
     if (!ptr)
          return NULL;
     return ptr->data.switch_item;
}

/*f statement_push
 */
static int statement_push( t_stack *expr_stack, t_md_statement *statement )
{
     t_stack_entry *entry;

     if (!statement)
          return 0;

     entry = (t_stack_entry *)malloc(sizeof(t_stack_entry));
     entry->next_in_stack = NULL;
     entry->prev_in_stack = expr_stack->stack_top;
     entry->data.statement = statement;
     if (expr_stack->stack_top)
     {
          expr_stack->stack_top->next_in_stack = entry;
          expr_stack->stack_depth++;
     }
     else
     {
          expr_stack->stack_bottom = entry;
          expr_stack->stack_depth = 1;
     }
     expr_stack->stack_top = entry;
     return 1;
}

/*f statement_pop
 */
static t_md_statement *statement_pop( t_sl_exec_file_data *exec_file_data, t_stack *expr_stack )
{
     t_stack_entry *last;
     t_md_statement *result;

     result = NULL;
     if (expr_stack->stack_top)
     {
          last = expr_stack->stack_top;
          expr_stack->stack_top = last->prev_in_stack;
          expr_stack->stack_depth--;
          result = last->data.statement;
          free(last);
     }
     else
     { 
          sl_exec_file_error( exec_file_data)->add_error( NULL, error_level_fatal, error_number_ef_stack_underflow, error_id_ef_frontend_main,
                                                          error_arg_type_const_string, "statement",
                                                          error_arg_type_malloc_filename, sl_exec_file_filename( exec_file_data ),
                                                          error_arg_type_line_number, sl_exec_file_line_number( exec_file_data ),
                                                          error_arg_type_none );
     }
     return result;
}

/*f statement_at_depth
 */
static t_md_statement *statement_at_depth( t_stack *expr_stack, int depth )
{
     t_stack_entry *ptr;

     ptr = expr_stack->stack_top;
     for (; depth>0 && (ptr); ptr=ptr->prev_in_stack, depth--);
     if (!ptr)
          return NULL;
     return ptr->data.statement;
}

/*f expression_push
 */
static void expression_push( t_stack *expr_stack, t_md_expression *expression )
{
     t_stack_entry *entry;

     entry = (t_stack_entry *)malloc(sizeof(t_stack_entry));
     entry->next_in_stack = NULL;
     entry->prev_in_stack = expr_stack->stack_top;
     entry->data.expression = expression;
     if (expr_stack->stack_top)
     {
          expr_stack->stack_top->next_in_stack = entry;
          expr_stack->stack_depth++;
     }
     else
     {
          expr_stack->stack_bottom = entry;
          expr_stack->stack_depth = 1;
     }
     expr_stack->stack_top = entry;
}

/*f expression_pop
 */
static t_md_expression *expression_pop( t_sl_exec_file_data *exec_file_data, t_stack *expr_stack )
{
     t_stack_entry *last;
     t_md_expression *result;

     result = NULL;
     if (expr_stack->stack_top)
     {
          last = expr_stack->stack_top;
          expr_stack->stack_top = last->prev_in_stack;
          expr_stack->stack_depth--;
          result = last->data.expression;
          free(last);
     }
     else
     {
          sl_exec_file_error( exec_file_data)->add_error( NULL, error_level_fatal, error_number_ef_stack_underflow, error_id_ef_frontend_main,
                                                          error_arg_type_const_string, "expresion",
                                                          error_arg_type_malloc_filename, sl_exec_file_filename( exec_file_data ),
                                                          error_arg_type_line_number, sl_exec_file_filename( exec_file_data ),
                                                          error_arg_type_none );
     }
     return result;
}

/*f expression_op
 */
static void expression_op( t_sl_exec_file_data *exec_file_data, c_sl_error *error, c_model_descriptor *model, t_stack *expr_stack, t_md_module *module, t_md_expr_fn fn, int num_args )
{
     t_md_expression *expr;
     int i;

     expr = NULL;
     if (expr_stack->stack_depth<num_args)
     {
          error->add_error( NULL, error_level_fatal, error_number_ef_stack_underflow, error_id_ef_frontend_main,
                            error_arg_type_const_string, "expression",
                            error_arg_type_malloc_filename, sl_exec_file_filename( exec_file_data ),
                            error_arg_type_line_number, sl_exec_file_line_number( exec_file_data ),
                            error_arg_type_none );
     }
     switch (num_args)
     {
     case 1:
          expr = model->expression( module, fn, expr_stack->stack_top->data.expression, NULL, NULL );
          break;
     case 2:
          expr = model->expression( module, fn, expr_stack->stack_top->prev_in_stack->data.expression, expr_stack->stack_top->data.expression, NULL );
          break;
     case 3:
          expr = model->expression( module, fn, expr_stack->stack_top->prev_in_stack->prev_in_stack->data.expression, expr_stack->stack_top->prev_in_stack->data.expression, expr_stack->stack_top->data.expression );
          break;
     }
     for (i=0; i<num_args; i++)
          expression_pop( exec_file_data,  expr_stack );
     if (expr)
          expression_push( expr_stack, expr );
}

/*f exec_file_cmd_handler
 */
static t_sl_error_level exec_file_cmd_handler( struct t_sl_exec_file_cmd_cb *cmd_cb, void *handle )
{
    t_data *data;
    data = (t_data *)handle;
    int exec_error;
    int i;

    exec_error = 0;
    switch (cmd_cb->cmd)
    {
    case cmd_struct:
        data->types[0] = data->model->type_definition_structure_create( cmd_cb->args[0].p.string, 1, cmd_cb->args[1].integer );
        exec_error = !MD_TYPE_DEFINITION_HANDLE_VALID(data->types[0]);
        break;
    case cmd_struct_elt:
        data->types[0] = data->model->type_definition_find( cmd_cb->args[0].p.string );
        data->types[1] = data->model->type_definition_bit_vector_create( cmd_cb->args[3].integer );
        if ( MD_TYPE_DEFINITION_HANDLE_VALID(data->types[0]) &&
             MD_TYPE_DEFINITION_HANDLE_VALID(data->types[1]) )
        {
            exec_error = !data->model->type_definition_structure_set_element( data->types[0], cmd_cb->args[2].p.string, 1, cmd_cb->args[1].integer, data->types[1] );
        }
        else
        {
            exec_error = 1;
        }
        break;
    case cmd_struct_elt_type:
        data->types[0] = data->model->type_definition_find( cmd_cb->args[0].p.string );
        data->types[1] = data->model->type_definition_find( cmd_cb->args[3].p.string );
        if ( MD_TYPE_DEFINITION_HANDLE_VALID(data->types[0]) &&
             MD_TYPE_DEFINITION_HANDLE_VALID(data->types[1]) )
        {
            exec_error = !data->model->type_definition_structure_set_element( data->types[0], cmd_cb->args[2].p.string, 1, cmd_cb->args[1].integer, data->types[1] );
        }
        else
        {
            exec_error = 1;
        }
        break;
    case cmd_struct_elt_type_array:
        data->types[0] = data->model->type_definition_find( cmd_cb->args[0].p.string );
        data->types[1] = data->model->type_definition_find( cmd_cb->args[4].p.string );
        if ( MD_TYPE_DEFINITION_HANDLE_VALID(data->types[0]) &&
             MD_TYPE_DEFINITION_HANDLE_VALID(data->types[1]) )
        {
            data->types[2] = data->model->type_definition_array_create( NULL, 0, cmd_cb->args[3].integer, data->types[1] );
            if ( MD_TYPE_DEFINITION_HANDLE_VALID(data->types[2]) )
            {
                exec_error = !data->model->type_definition_structure_set_element( data->types[0], cmd_cb->args[2].p.string, 1, cmd_cb->args[1].integer, data->types[2] );
            }
            else
            {
                exec_error = 1;
            }
        }
        else
        {
            exec_error = 1;
        }
        break;
    case cmd_struct_elt_bv_array:
        data->types[0] = data->model->type_definition_find( cmd_cb->args[0].p.string );
        data->types[1] = data->model->type_definition_bit_vector_create( cmd_cb->args[3].integer );
        if ( MD_TYPE_DEFINITION_HANDLE_VALID(data->types[0]) &&
             MD_TYPE_DEFINITION_HANDLE_VALID(data->types[1]) )
        {
            data->types[2] = data->model->type_definition_array_create( NULL, 0, cmd_cb->args[3].integer, data->types[1] );
            if ( MD_TYPE_DEFINITION_HANDLE_VALID(data->types[2]) )
            {
                exec_error = !data->model->type_definition_structure_set_element( data->types[0], cmd_cb->args[2].p.string, 1, cmd_cb->args[1].integer, data->types[2] );
            }
            else
            {
                exec_error = 1;
            }
        }
        else
        {
            exec_error = 1;
        }
        break;
    case cmd_module:
        data->module = data->model->module_create( cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), NULL );
        break;
    case cmd_prototype:
        data->module = data->model->module_create_prototype( cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ) );
        break;
    case cmd_clock:
        data->model->signal_add_clock( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ) );
        break;
    case cmd_input:
        if (sl_exec_file_get_number_of_arguments( cmd_cb->file_data, cmd_cb->args )==1)
        {
            data->model->signal_add_input( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), 1 );
        }
        else
        {
            data->model->signal_add_input( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[1].integer );
        }
        break;
    case cmd_output:
        if (sl_exec_file_get_number_of_arguments( cmd_cb->file_data, cmd_cb->args )==1)
        {
            data->model->signal_add_output( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), 1 );
        }
        else
        {
            data->model->signal_add_output( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[1].integer );
        }
        break;
    case cmd_comb_input:
        data->model->signal_input_used_combinatorially( data->module, cmd_cb->args[0].p.string );
        break;
    case cmd_clocked_input:
        data->model->signal_input_used_on_clock( data->module, cmd_cb->args[2].p.string, cmd_cb->args[0].p.string, cmd_cb->args[1].integer>0?md_edge_pos:md_edge_neg );
        break;
    case cmd_comb_output:
        data->model->signal_output_derived_combinatorially( data->module, cmd_cb->args[0].p.string );
        break;
    case cmd_clocked_output:
        data->model->signal_output_generated_from_clock( data->module, cmd_cb->args[2].p.string, cmd_cb->args[0].p.string, cmd_cb->args[1].integer>0?md_edge_pos:md_edge_neg );
        break;
    case cmd_comb:
        if (sl_exec_file_get_number_of_arguments( cmd_cb->file_data, cmd_cb->args )==1)
        {
            data->model->signal_add_combinatorial( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), 1,  md_usage_type_rtl );
        }
        else
        {
            data->model->signal_add_combinatorial( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[1].integer,  md_usage_type_rtl );
        }
        break;
    case cmd_net:
        if (sl_exec_file_get_number_of_arguments( cmd_cb->file_data, cmd_cb->args )==1)
        {
            data->model->signal_add_net( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), 1 );
        }
        else
        {
            data->model->signal_add_net( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[1].integer );
        }
        break;
    case cmd_state:
        data->model->state_add( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), cmd_cb->args[1].integer, md_usage_type_rtl,
                          cmd_cb->args[2].p.string, cmd_cb->args[3].integer>0?md_edge_pos:md_edge_neg,
                          cmd_cb->args[4].p.string, cmd_cb->args[5].integer>0?1:0,
                          md_temp_value_from_integer( 0, cmd_cb->args[1].integer, cmd_cb->args[6].integer ) );
        break;
    case cmd_state_type:
        data->types[0] = data->model->type_definition_find( cmd_cb->args[1].p.string );
        if ( MD_TYPE_DEFINITION_HANDLE_VALID(data->types[0]) )
        {
            exec_error = !data->model->state_add( data->module,
                                                  cmd_cb->args[0].p.string, 1, // name
                                                  NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), // client ref
                                                  0, data->types[0], // size, type
                                                  md_usage_type_rtl, 
                                                  cmd_cb->args[2].p.string, cmd_cb->args[3].integer>0?md_edge_pos:md_edge_neg,
                                                  cmd_cb->args[4].p.string, cmd_cb->args[5].integer>0?1:0 );
        }
        else
        {
            exec_error = 1;
        }
        break;
    case cmd_state_type_array:
        data->types[0] = data->model->type_definition_find( cmd_cb->args[1].p.string );
        if ( MD_TYPE_DEFINITION_HANDLE_VALID(data->types[0]) )
        {
            exec_error = !data->model->state_add( data->module,
                                                  cmd_cb->args[0].p.string, 1, // name
                                                  NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), // client ref
                                                  cmd_cb->args[2].integer, data->types[0], // size, type
                                                  md_usage_type_rtl, 
                                                  cmd_cb->args[3].p.string, cmd_cb->args[4].integer>0?md_edge_pos:md_edge_neg,
                                                  cmd_cb->args[5].p.string, cmd_cb->args[6].integer>0?1:0 );
        }
        else
        {
            exec_error = 1;
        }
        break;
    case cmd_state_bv_array:
        data->types[0] = data->model->type_definition_bit_vector_create( cmd_cb->args[2].integer );
        if ( MD_TYPE_DEFINITION_HANDLE_VALID(data->types[0]) )
        {
            exec_error = !data->model->state_add( data->module,
                                                  cmd_cb->args[0].p.string, 1, // name
                                                  NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), // client ref
                                                  cmd_cb->args[1].integer, data->types[0], // size, type
                                                  md_usage_type_rtl, 
                                                  cmd_cb->args[3].p.string, cmd_cb->args[4].integer>0?md_edge_pos:md_edge_neg,
                                                  cmd_cb->args[5].p.string, cmd_cb->args[6].integer>0?1:0 );
        }
        else
        {
            exec_error = 1;
        }
        break;
    case cmd_state_reset:
        data->model->state_reset( data->module, cmd_cb->args[0].p.string, md_temp_value_from_integer( 0, cmd_cb->args[1].integer, cmd_cb->args[2].integer ) );
        break;
    case cmd_code:
        data->model->code_block( data->module, cmd_cb->args[0].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), NULL );
        break;
    case cmd_push_var:
        expression_push( &data->expr_stack, data->model->expression( data->module, cmd_cb->args[0].p.string ));
        break;
    case cmd_push_lvar:
        if (data->lvar)
        {
            expression_push( &data->expr_stack, data->model->expression( data->module, data->lvar ));
            data->lvar = NULL;
        }
        else
        {
            exec_error = 1;
        }
        break;
    case cmd_push_int:
        expression_push( &data->expr_stack, data->model->expression( data->module, cmd_cb->args[0].integer, md_temp_value_from_integer( 0, cmd_cb->args[0].integer, cmd_cb->args[1].integer ) ) );
        break;
    case cmd_expr:
        for (i=0; expr_ops[i].text; i++)
        {
            if (!strcmp( cmd_cb->args[0].p.string, expr_ops[i].text) )
            {
                expression_op( cmd_cb->file_data, data->error, data->model, &data->expr_stack, data->module, expr_ops[i].fn, expr_ops[i].num_args );
                break;
            }
        }
        if (!expr_ops[i].text)
        {
            exec_error = 1;
        }
        break;
    case cmd_lvar: // Either a sublvar of an lvar, or an lvar itself
        data->lvar = data->model->lvar_reference( data->module, data->lvar, cmd_cb->args[0].p.string );
        break;
    case cmd_lvar_index: // An index in to a current lvar
        if (!data->lvar)
        {
            exec_error=1;
        }
        else
        {
            data->lvar = data->model->lvar_index( data->module, data->lvar, cmd_cb->args[0].integer );
        }
        break;
    case cmd_lvar_bit:
        if (!data->lvar)
        {
            exec_error=1;
        }
        else
        {
            if (sl_exec_file_get_number_of_arguments( cmd_cb->file_data, cmd_cb->args )==1)
            {
                data->lvar = data->model->lvar_bit_select( data->module, data->lvar, cmd_cb->args[0].integer );
            }
            else
            {
                data->lvar = data->model->lvar_bit_select( data->module, data->lvar, expression_pop( cmd_cb->file_data, &data->expr_stack ) );
            }
        }
        break;
    case cmd_lvar_range:
        if (!data->lvar)
        {
            exec_error=1;
        }
        else
        {
            if (sl_exec_file_get_number_of_arguments( cmd_cb->file_data, cmd_cb->args )==2)
            {
                data->lvar = data->model->lvar_bit_range_select( data->module, data->lvar, data->model->expression( data->module, 32, md_temp_value_from_integer( 0, 32, cmd_cb->args[1].integer ) ), cmd_cb->args[0].integer );
            }
            else
            {
                data->lvar = data->model->lvar_bit_range_select( data->module, data->lvar, expression_pop( cmd_cb->file_data, &data->expr_stack ), cmd_cb->args[0].integer );
            }
        }
        break;
    case cmd_port_lvar: // Either a sublvar of an lvar, or an lvar itself
        data->port_lvar = data->model->port_lvar_reference( data->module, data->port_lvar, cmd_cb->args[0].p.string );
        break;
    case cmd_assign_comb:
        exec_error = !statement_push( &data->stmt_stack, data->model->statement_create_combinatorial_assignment( data->module, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), data->lvar, expression_pop( cmd_cb->file_data,  &data->expr_stack )) );
        data->lvar = NULL;
        break;
    case cmd_assign_state:
        exec_error = !statement_push( &data->stmt_stack, data->model->statement_create_state_assignment( data->module, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), data->lvar, expression_pop( cmd_cb->file_data,  &data->expr_stack )) );
        data->lvar = NULL;
        break;
    case cmd_if:
        statement_push( &data->stmt_stack, data->model->statement_create_if_else( data->module, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), expression_pop( cmd_cb->file_data,  &data->expr_stack), statement_pop( cmd_cb->file_data,  &data->stmt_stack), NULL ));
        break;
    case cmd_ifelse:
        t_md_statement *iftrue, *iffalse;
        iffalse = statement_pop( cmd_cb->file_data,  &data->stmt_stack);
        iftrue = statement_pop( cmd_cb->file_data,  &data->stmt_stack);
        statement_push( &data->stmt_stack, data->model->statement_create_if_else( data->module, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), expression_pop( cmd_cb->file_data,  &data->expr_stack), iftrue, iffalse ));
        break;
    case cmd_group:
        t_md_statement *statement;
        statement = statement_at_depth( &data->stmt_stack, cmd_cb->args[0].integer-1 );
        for (i=cmd_cb->args[0].integer-2; i>=0; i--)
        {
            data->model->statement_add_to_chain( statement, statement_at_depth( &data->stmt_stack, i ));
        }
        for (i=cmd_cb->args[0].integer-2; i>=0; i--)
        {
            statement_pop( cmd_cb->file_data,  &data->stmt_stack );
        }
        break;
    case cmd_case:
        switch_item_push( &data->switem_stack, data->model->switch_item_create( data->module, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), md_temp_value_from_integer( 0, cmd_cb->args[0].integer, cmd_cb->args[1].integer ), statement_pop( cmd_cb->file_data,  &data->stmt_stack) ) );
        break;
    case cmd_casex:
        switch_item_push( &data->switem_stack, data->model->switch_item_create( data->module, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), md_temp_value_from_integer( 0, cmd_cb->args[0].integer, cmd_cb->args[1].integer ), md_temp_value_from_integer( 1, cmd_cb->args[0].integer, cmd_cb->args[2].integer ), statement_pop( cmd_cb->file_data,  &data->stmt_stack) ) );
        break;
    case cmd_default:
        switch_item_push( &data->switem_stack, data->model->switch_item_create( data->module, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), statement_pop( cmd_cb->file_data,  &data->stmt_stack) ) );
        break;
    case cmd_case_exp:
        switch_item_push( &data->switem_stack, data->model->switch_item_create( data->module, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), expression_pop( cmd_cb->file_data,  &data->expr_stack), statement_pop( cmd_cb->file_data,  &data->stmt_stack) ) );
        break;
    case cmd_switch: // assume full static for now
        t_md_switch_item *switem;
        switem = NULL;
        for (i=cmd_cb->args[0].integer-1; i>=0; i--)
        {
            switem = data->model->switch_item_chain( switem, switch_item_at_depth( &data->switem_stack, i ) );
        }
        statement_push( &data->stmt_stack, data->model->statement_create_static_switch( data->module, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ), 1, 1, expression_pop( cmd_cb->file_data,  &data->expr_stack ), switem) );
        for (i=cmd_cb->args[0].integer-1; i>=0; i--)
        {
            switch_item_pop( cmd_cb->file_data,  &data->switem_stack );
        }
        break;
    case cmd_priority:
        break;
    case cmd_instantiate:
        exec_error = !data->model->module_instance( data->module, cmd_cb->args[0].p.string, 1, cmd_cb->args[1].p.string, 1, NULL, sl_exec_file_filename(cmd_cb->file_data), sl_exec_file_line_number( cmd_cb->file_data ) );
        break;
    case cmd_inst_clock:
        exec_error = !data->model->module_instance_add_clock( data->module, cmd_cb->args[0].p.string, cmd_cb->args[1].p.string, cmd_cb->args[2].p.string );
        break;
    case cmd_inst_input:
        if (!data->port_lvar)
        {
            exec_error=1;
        }
        else
        {
            exec_error = !data->model->module_instance_add_input( data->module, cmd_cb->args[0].p.string, data->port_lvar, expression_pop( cmd_cb->file_data, &data->expr_stack ) );
        }
        data->port_lvar = NULL;
        break;
    case cmd_inst_output:
        if ((!data->lvar) || (!data->port_lvar))
        {
            exec_error=1;
        }
        else
        {
            exec_error = !data->model->module_instance_add_output( data->module, cmd_cb->args[0].p.string, data->port_lvar, data->lvar );
        }
        data->lvar = NULL;
        data->port_lvar = NULL;
        break;
    case cmd_addcode:
        for (i=cmd_cb->args[0].integer-1; i>=0; i--)
        {
            data->model->code_block_add_statement( data->module, cmd_cb->args[1].p.string, statement_at_depth( &data->stmt_stack, i ));
        }
        for (i=cmd_cb->args[0].integer-1; i>=0; i--)
        {
            statement_pop( cmd_cb->file_data,  &data->stmt_stack );
        }
        break;
    default:
        return error_level_serious;
    }
    if (exec_error)
    {
        data->error->add_error( NULL, error_level_fatal, error_number_ef_uncaught_error, error_id_ef_frontend_main,
                          error_arg_type_malloc_filename, sl_exec_file_filename( cmd_cb->file_data ),
                          error_arg_type_line_number, sl_exec_file_line_number( cmd_cb->file_data ),
                          error_arg_type_none );
        return error_level_fatal;
    }
    return error_level_okay;
}
#endif

/*f exec_file_instantiate_callback
 */
static void exec_file_instantiate_callback( void *handle, struct t_sl_exec_file_data *file_data )
{
     sl_exec_file_set_environment_interrogation( file_data, (t_sl_get_environment_fn)sl_option_get_string, handle );
}

/*f build_model
    returns !=0 if error
 */
static int build_model( c_model_descriptor *model, c_sl_error *error, const char *filename, t_sl_option_list env_options )
{
    t_sl_exec_file_data *exec_file_data;
    t_data data;

    data.model = model;
    data.error = error;
    data.env_options = env_options;
    data.expr_stack.stack_top = NULL;
    data.expr_stack.stack_bottom = NULL;
    data.expr_stack.stack_depth = 0;
    data.stmt_stack.stack_top = NULL;
    data.stmt_stack.stack_bottom = NULL;
    data.stmt_stack.stack_depth = 0;
    data.switem_stack.stack_top = NULL;
    data.switem_stack.stack_bottom = NULL;
    data.switem_stack.stack_depth = 0;
    data.lvar = NULL;
    data.port_lvar = NULL;

    error->add_text_list( 1, default_error_messages );

    sl_exec_file_allocate_and_read_exec_file( error, error, exec_file_instantiate_callback, (void *)(&data), "build_exec_file", filename, &exec_file_data, "model builder", file_cmds, NULL );

    if (error->check_errors_and_reset( stderr, error_level_info, error_level_serious ))
        return 1;

    if (!exec_file_data)
        return 1;

    sl_exec_file_reset( exec_file_data );

    data.module = NULL;
    while (sl_exec_file_despatch_next_cmd( exec_file_data ));
    if (data.stmt_stack.stack_depth!=0)
    {
        error->add_error( NULL, error_level_fatal, error_number_ef_stack_not_empty, error_id_ef_frontend_main,
                          error_arg_type_const_string, "statement",
                          error_arg_type_malloc_filename, filename,
                          error_arg_type_none );
    }
    if (data.expr_stack.stack_depth!=0)
    {
        error->add_error( NULL, error_level_fatal, error_number_ef_stack_not_empty, error_id_ef_frontend_main,
                          error_arg_type_const_string, "expression",
                          error_arg_type_malloc_filename, filename,
                          error_arg_type_none );
    }
    if (data.switem_stack.stack_depth!=0)
    {
        error->add_error( NULL, error_level_fatal, error_number_ef_stack_not_empty, error_id_ef_frontend_main,
                          error_arg_type_const_string, "case items",
                          error_arg_type_malloc_filename, filename,
                          error_arg_type_none );
    }
    if (error->check_errors_and_reset( stderr, error_level_info, error_level_serious ))
    {
        return 1;
    }

    if (ef_debug)
    {
        model->type_definition_display( debug_output, (void *)stdout );
    }
    return 0;
}

/*a Main routine
 */
/*f string_from_reference
  Our references are module, object, subitem (probably 0)
 */
static int string_from_reference( void *handle, void *base_handle, const void *item_handle, int item_reference, char *buffer, int buffer_size, t_md_client_string_type type )
{
    switch (type)
    {
    case md_client_string_type_coverage:
        sprintf(buffer,"%s:%d:char", (const char *)item_handle, item_reference);
        return 1;
    default:
    	break;
    }
    return 0;
}

/*f main
 */
extern int main( int argc, char **argv )
{
    int c, so_far, i;
    c_model_descriptor *model;
    c_sl_error *error;
    t_sl_option_list env_options;

    sl_debug_set_level( sl_debug_level_verbose_info );
    sl_debug_enable( 0 );
    env_options = NULL;

    so_far = 0;
    c = 0;
    optopt=1;
    while(c>=0)
    {
        c = getopt_long( argc, argv, "hv", options, &so_far );
        if ( !be_handle_getopt( &env_options, c, optarg) &&
             !sl_option_handle_getopt( &env_options, c, optarg) )
        {
            switch (c)
            {
            case 'h':
            case option_help:
                usage();
                //be_usage();
                //sl_usage();
                exit(0);
                break;
            case option_ef_debug:
                ef_debug = 1;
                break;
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

    error = new c_sl_error();

    model = new c_model_descriptor( env_options, error, string_from_reference, NULL );
    for (i=optind; argv[i]; i++)
    {
        if (build_model( model, error, argv[i], env_options ))
        {
            exit(4);
        }
    }

    model->analyze();
    if (error->check_errors_and_reset( stderr, error_level_info, error_level_fatal ))
        exit(4);
    if (ef_debug)
    {
        model->display_instances( debug_output, (void *)stdout );
        model->display_references( debug_output, (void *)stdout );
    }

    model->generate_output( env_options );

    delete( model );
    return 0;
}

/*a To do
  add error code
  check module exists before doing all that building
  analyze module if a new module is declared
  add subscript to C++ and vhdl output
 */

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


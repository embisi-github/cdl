/*a Copyright
  
  This file 'c_co_statement.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_STATEMENT
#else
#define __INC_C_CO_STATEMENT

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Classes
 */
/*t t_statement_type
 */
typedef enum
{
    statement_type_assignment,
    statement_type_if_statement,
    statement_type_for_statement,
    statement_type_full_switch_statement,
    statement_type_partial_switch_statement,
    statement_type_priority_statement,
    statement_type_block,
    statement_type_print,
    statement_type_assert,
    statement_type_cover,
    statement_type_log,
    statement_type_instantiation,
} t_statement_type;

/*t c_co_statement
 */
class c_co_statement: public c_cyc_object
{
public:
    c_co_statement( class c_co_lvar *lvar, class c_co_nested_assignment *nested_assignment, int clocked_or_wired_or, t_string *documentation ); // assignment statment
    c_co_statement( class c_co_clock_reset_defn *clock_spec, class c_co_clock_reset_defn *reset_spec, t_statement_type type, c_co_expression *assertion, c_co_expression *value_list, t_string *text, c_co_expression *text_args, c_co_statement *code ); // print, assertion or cover statement
    c_co_statement( class c_co_clock_reset_defn *clock_spec, class c_co_clock_reset_defn *reset_spec, t_string *log_message, class c_co_named_expression *log_values); // log message and arguments
    c_co_statement( c_co_statement *stmt_list ); // statement list
    c_co_statement( t_statement_type statement_type, class c_co_expression *expression, class c_co_case_entry *cases, t_string *expr_documentation ); // switch statement
    c_co_statement( class c_co_expression *expression, c_co_statement *if_true, c_co_statement *if_false, c_co_statement *elsif, t_string *expr_documentation, t_string *else_documentation );
    c_co_statement( c_co_instantiation *instantiation );
    c_co_statement( t_symbol *symbol, class c_co_expression *iterations, class c_co_statement *statement, class c_co_expression *first_value, class c_co_expression *last_value, class c_co_expression *next_value );
    ~c_co_statement();
    c_co_statement *chain_tail( c_co_statement *entry );

    void cross_reference_pre_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, class c_co_clock_reset_defn *default_clock, class c_co_clock_reset_defn *default_reset );
    void cross_reference_post_type_resolution( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void check_types( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    void evaluate_constant_expressions( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );
    void high_level_checks( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables );
    struct t_md_statement *build_model( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module );


    t_statement_type statement_type;               // Type of statement: assign, if, switch, etc
    union
    {
        struct
        {
            int clocked;                 // 1 if it is a '<=' stmt, 0 for '='
            int wired_or;                // 1 if it is a '|=' stmt, 0 for '='
            class c_co_lvar *lvar;            // What is being assigned, ownership link
            class c_co_nested_assignment *nested_assignment;            // Value to assign, ownership link
            t_string *documentation;
        } assign_stmt;
        struct
        {
            class c_co_clock_reset_defn *clock_spec; // Clock edge to print on - this is like a register
            class c_co_clock_reset_defn *reset_spec; // Reset not to print on
            class c_co_expression *assertion; // Expression that (if false) causes printing OR if it doesn't match an expression in value_list
            class c_co_expression *value_list; // Expressions to match against
            t_string *text; // Text to print
            class c_co_expression *text_args; // Expressions to use with the text in printing
            class c_co_statement *statement; // Code to run if the assertion fails or cover statement succeeds (once only for cover)
        } print_assert_stmt;
        struct
        {
            t_symbol *symbol; // Symbol
            class c_co_expression *iterations;          // Maximum number of cycles - good upper limit
            class c_co_statement *statement;            // Statement to iterate
            class c_co_expression *first_value;         // Expression for first value - if NULL, go from 0 to number of cycles-1 - must be compile time constant (can use globals or iterator values)
            class c_co_expression *last_value;          // Expression for last value - only if first_value is not NULL, and must be compile time constant (can use globals or iterator values)
            class c_co_expression *next_value;          // Expression for next value - only if first_value is not NULL, and must be compile time constant (can use globals or iterator values)
            t_co_scope *scope;                          // Scope of variables the statement may use - includes this iterator
            t_type_value type_value;                    // Iterator value as it goes through the loop
            t_string *documentation;
        } for_stmt;
        struct
        {
            class c_co_expression *expression;         // Expression of the if statement
            t_string *expr_documentation;
            t_string *else_documentation;
            class c_co_statement *if_true;     // Statements to execute if the expression is true
            class c_co_statement *if_false;        // Statements to execute if the expression is false - NULL if there is an elsif
            class c_co_statement *elsif;       // If statements to follow if the expression is false - NULL if there is an 'else' or 'if_false'
        } if_stmt;
        class c_co_statement *block;                // If a block, then its just a list of statements, ownership link
        struct
        {
            class c_co_expression *expression;             
            t_string *expr_documentation;
            class c_co_case_entry *cases;
        } switch_stmt;
        class c_co_instantiation *instantiation; // Instantiation
        struct
        {
            class c_co_clock_reset_defn *clock_spec; // Clock edge to print on - this is like a register
            class c_co_clock_reset_defn *reset_spec; // Reset not to print on
            t_string *message;
            class c_co_named_expression *values;
        } log_stmt;
    } type_data;                   // Data needed for this type of statement

    c_co_statement *parent;            // Parent statement of this parse level; NULL if this is toplevel
    c_co_statement *toplevel;      // Toplevel statement of this statement (up the chain of parents). Self reference if this is toplevel.
    class c_co_code_label *code_label;         // Last label at top level prior to this statement - i.e. the label this statement 'belongs to'
};

/*a Wrapper
 */
#endif



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/




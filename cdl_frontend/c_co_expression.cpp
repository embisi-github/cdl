/*a Copyright
  
  This file 'c_co_expression.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "c_type_value_pool.h"
#include "c_lexical_analyzer.h"
#include "c_cyclicity.h"
#include "cyclicity_grammar.h"
#include "sl_debug.h"

#include "c_co_constant_declaration.h"
#include "c_co_enum_definition.h"
#include "c_co_enum_identifier.h"
#include "c_co_expression.h"
#include "c_co_fsm_state.h"
#include "c_co_lvar.h"
#include "c_co_nested_assignment.h"
#include "c_co_signal_declaration.h"
#include "c_co_statement.h"
#include "c_co_token_union.h"
#include "c_co_toplevel.h"
#include "c_co_type_definition.h"
#include "c_co_type_specifier.h"
#include "c_co_type_struct.h"

/*a Types
 */
/*t coerce_permitted_to_*
 */
typedef enum
{
    coerce_permitted_to_none,
    coerce_permitted_to_bit_vector,
    coerce_permitted_to_integer,
};

/*t t_expression_arg_type
 */
typedef enum
{
    expression_arg_type_none,
    expression_arg_type_bit_vector,
    expression_arg_type_integer,
    expression_arg_type_bool,
    expression_arg_type_match0,
    expression_arg_type_match1,
    expression_arg_type_string
} t_expression_arg_type;

/*t t_expression_fn
 */
typedef int (*t_expression_fn)( c_cyclicity *cyclicity, c_co_expression *expression, t_expr_subtype expr_subtype, t_type_value first, t_type_value second, t_type_value third );

/*t t_expression_fn_list_entry
 */
typedef struct t_expression_fn_list_subentry
{
    t_expression_arg_type args[3];
    t_expression_fn fn;
    t_expression_arg_type result_type;
} t_expression_fn_list_subentry;

/*t t_expression_fn_list_entry
 */
typedef struct t_expression_fn_list_entry
{
    t_expr_subtype op;
    int coercions; // 0 -> any permitted conversions (bool->int, int->bv, bool->bv); -1 -> no conversions; 1-7 bitmask of args that have fixed type and require conversion to specified in subentry[0]
    t_expression_fn_list_subentry subentry[4];
} t_expression_fn_list_entry;

/*a Declarations
 */
static int logical_op( c_cyclicity *cyclicity, c_co_expression *expression, t_expr_subtype expr_subtype, t_type_value first, t_type_value second, t_type_value third );
static int integer_op( c_cyclicity *cyclicity, c_co_expression *expression, t_expr_subtype expr_subtype, t_type_value first, t_type_value second, t_type_value third );
static int vector_op( c_cyclicity *cyclicity, c_co_expression *expression, t_expr_subtype expr_subtype, t_type_value first, t_type_value second, t_type_value third );
static int special_op( c_cyclicity *cyclicity, c_co_expression *expression, t_expr_subtype expr_subtype, t_type_value first, t_type_value second, t_type_value third );

/*a Statics
 */
/*v bit_mask
 */
static t_sl_uint64 bit_mask[] = {
     0LL, 1LL, 3LL, 7LL,
     0xfLL, 0x1fLL, 0x3fLL, 0x7fLL,
     0xffLL, 0x1ffLL, 0x3ffLL, 0x7ffLL,
     0xfffLL, 0x1fffLL, 0x3fffLL, 0x7fffLL,
     ((1LL<<16)-1), ((1LL<<17)-1), ((1LL<<18)-1), ((1LL<<19)-1), 
     ((1LL<<20)-1), ((1LL<<21)-1), ((1LL<<22)-1), ((1LL<<23)-1), 
     ((1LL<<24)-1), ((1LL<<25)-1), ((1LL<<26)-1), ((1LL<<27)-1), 
     ((1LL<<28)-1), ((1LL<<29)-1), ((1LL<<30)-1), ((1LL<<31)-1), 
     ((1LL<<32)-1), ((1LL<<33)-1), ((1LL<<34)-1), ((1LL<<35)-1), 
     ((1LL<<36)-1), ((1LL<<37)-1), ((1LL<<38)-1), ((1LL<<39)-1), 
     ((1LL<<40)-1), ((1LL<<41)-1), ((1LL<<42)-1), ((1LL<<43)-1), 
     ((1LL<<44)-1), ((1LL<<45)-1), ((1LL<<46)-1), ((1LL<<47)-1), 
     ((1LL<<48)-1), ((1LL<<49)-1), ((1LL<<50)-1), ((1LL<<51)-1), 
     ((1LL<<52)-1), ((1LL<<53)-1), ((1LL<<54)-1), ((1LL<<55)-1), 
     ((1LL<<56)-1), ((1LL<<57)-1), ((1LL<<58)-1), ((1LL<<59)-1), 
     ((1LL<<60)-1), ((1LL<<61)-1), ((1LL<<62)-1), ((1ULL<<63)-1), 
     ((0LL)-1)
};

/*v expr_subtype_strings - must match enum in c_co_expression.h
 */
static const char *expr_subtype_strings[] = 
{
    "<none>",
    "<expr>",
    "?",
    "+",
    "-",
    "*",
    "/",
    "%",
    "!",
    "&&",
    "||",
    "<<",
    ">>",
    "~",
    "&",
    "|",
    "^",
    "<",
    "<=",
    ">",
    ">=",
    "==",
    "!=",
    "-",
    "sizeof()"
};

/*v expression_fn_list
  Each entry is expr_subtype, polymorphic, {args}, evaluation_fn, result type, arg of match (if type is matching)
  polymorphic means that the args can be of any type
  not polymorphic means that the args MUST be of the type
  There should only be one entry per expr_subtype if not polymorphic

  This is used in:
    type_value_from_expression_fn - from an entry in the table and arg (-1 for result instead), return undefined/int/bool or the matching type if that is required
    cross_reference_within_type_context - given a preferred type; find first match in the table, and if polymorphic cross reference args with preferred of undefined, but if not polymorphic (args have fixed type) cross reference args within the specified types from the args
    type_check_within_type_context - given a required type, find first match in the table; if not polymorphic, check result type and type check args; if polymorphic, find matching entry in table for args, and type check that entry

    Implicit casts supported:
    bool to bit
    bool to integer
    bit vector to bool (implicit !=0)
    bit vector to integer (for indexes)
    integer to bool (implicit !=0)
    integer to bit vector (sign-extend)

    Operations supported:
    || && ! are logical, on bools only
    == != <= >= < > on integers
    == != <= >= < > on bit vectors
    int << int, int >> int
    + - * on bit vectors (inc unary -)
    + - * / % on integers (inc unary -)
    & | ^ ~ on bit vectors
    & | ^ ~ on integers

    integer == integer yields result bool
    bool == bool       yields result bool
    bool == integer    yields cast1->same2, result bool
    integer == bool    yields cast1->same2, result bool
    bv == same1        yields result bool
    bv == integer      yields cast2->same1, result bool
    integer == bv      yields cast1->same2, result bool
    bv[1] == bool      yields cast2->same1, result bool
    bool == bv[1]      yields cast2->same1, result bool
    First find precise match
    If no precise match, try casting bools to integers and then find the match
    If still no match, try casting integers to matching bit vectors if possible
    If still not match, there is no match
    Each expression subtype appears once
    It has a set of functions that do stuff (for valid types; e.g. vector_op for bit vectors, integer_op for integers, logical_op for bools)
    It has a list of coercions in order (bool->integer, integer->bit vector)
    So for example, '==' would have:
        bool,bool,<> : logical_op
        int,int,<>   : integer_op
        bv,match0,<> : vector_op
        coerce 0:bool->int (this one can only effect bool,int or int,bool)
        coerce 1:bool->int (this one can only effect bool,int or int,bool)
        coerce 0:int->bvmatch1 (in conjunction with first coerce this allows for bool or int to be == to bit vector)
        coerce 1:int->bvmatch0 (in conjunction with first coerce this allows for bool or int to be == to bit vector)

   So the structure is now:
        { expr_subtype, <coercions required/allowed>, {<type>,<type>,<type>,<op_fn>,<result type>}*4 } }
   Types can be bool, int, bit vector any size, match0, match1
   Coercions allowed can be 0 for any, -1 for none, or a bit mask (1-7) for required coercions (1 for arg0, etc) - they must be to the relevant arg of the first function given in the list
   The casts are implicit; the only ones permitted are bool->int, bool->bv (sign extend), int->bv unless they are required
        { subtype, {{bool,bool,none,logical_op},
                    {int,int,none,integer_op},
                    {bv,match0,none,vector_op}} }
   The functions are called at internal_evaluate time
   The result can be cast to something else - for example, if requires a bool, so the result is cast to a bool then; indices cast to ints;
   Note that the argument coercions go first. Coercions are only attempted if they will help.
 */
static t_expression_fn_list_entry expression_fn_list[] = 
{
    /*b Binary operators - binary not, and, or, xor
     */
    { expr_subtype_binary_not, 0, { { {expression_arg_type_bit_vector, expression_arg_type_none,   expression_arg_type_none}, vector_op,  expression_arg_type_match0 },
                                    { {expression_arg_type_integer,    expression_arg_type_none,   expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                    { {expression_arg_type_bool,       expression_arg_type_none,   expression_arg_type_none}, logical_op, expression_arg_type_match0 },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    { expr_subtype_binary_and, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_match0 },
                                    { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                    { {expression_arg_type_bool,       expression_arg_type_match0, expression_arg_type_none}, logical_op, expression_arg_type_match0 },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    { expr_subtype_binary_or,  0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_match0 },
                                    { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                    { {expression_arg_type_bool,       expression_arg_type_match0, expression_arg_type_none}, logical_op, expression_arg_type_match0 },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    { expr_subtype_binary_xor, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_match0 },
                                    { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                    { {expression_arg_type_bool,       expression_arg_type_match0, expression_arg_type_none}, logical_op, expression_arg_type_match0 },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },

    /*b Logical operators - logical not, and, or, xor
     */
    { expr_subtype_logical_not, 3, { { {expression_arg_type_bool,       expression_arg_type_none,   expression_arg_type_none}, logical_op,  expression_arg_type_bool },
                                     { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,        expression_arg_type_match0 },
                                     { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,        expression_arg_type_match0 },
                                     { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,        expression_arg_type_match0 } } },
    { expr_subtype_logical_and, 3, { { {expression_arg_type_bool,       expression_arg_type_bool,   expression_arg_type_none}, logical_op,  expression_arg_type_bool },
                                     { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,        expression_arg_type_match0 },
                                     { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,        expression_arg_type_match0 },
                                     { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,        expression_arg_type_match0 } } },
    { expr_subtype_logical_or,  3, { { {expression_arg_type_bool,       expression_arg_type_bool,   expression_arg_type_none}, logical_op,  expression_arg_type_match0 },
                                     { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,        expression_arg_type_match0 },
                                     { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,        expression_arg_type_match0 },
                                     { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,        expression_arg_type_match0 } } },

    /*b Comparison operators: ==, !=, <, <=, >, >=
     */
    { expr_subtype_compare_eq, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_bool },
                                    { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_bool },
                                    { {expression_arg_type_bool,       expression_arg_type_match0, expression_arg_type_none}, logical_op, expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool } } },
    { expr_subtype_compare_ne, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_bool },
                                    { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_bool },
                                    { {expression_arg_type_bool,       expression_arg_type_match0, expression_arg_type_none}, logical_op, expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool } } },
    { expr_subtype_compare_lt, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_bool },
                                    { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool } } },
    { expr_subtype_compare_le, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_bool },
                                    { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool } } },
    { expr_subtype_compare_gt, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_bool },
                                    { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool } } },
    { expr_subtype_compare_ge, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_bool },
                                    { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool },
                                    { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_bool } } },

    /*b Arithmetic operators: +, -, *, /, % and unary -
     */
    { expr_subtype_negate,          0, { { {expression_arg_type_bit_vector, expression_arg_type_none,   expression_arg_type_none}, vector_op,  expression_arg_type_match0 },
                                         { {expression_arg_type_integer,    expression_arg_type_none,   expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                         { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                         { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    { expr_subtype_arithmetic_plus, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_match0 },
                                         { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                         { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                         { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    { expr_subtype_arithmetic_minus, 0, { { {expression_arg_type_bit_vector, expression_arg_type_match0, expression_arg_type_none}, vector_op,  expression_arg_type_match0 },
                                          { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                          { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                          { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    { expr_subtype_arithmetic_multiply, 0, { { {expression_arg_type_bit_vector, expression_arg_type_bit_vector, expression_arg_type_none}, vector_op,  expression_arg_type_match0 },
                                             { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                             { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                             { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    { expr_subtype_arithmetic_divide, -1, { { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                            { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                            { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                            { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    { expr_subtype_arithmetic_remainder, -1, { { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                               { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                               { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                               { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    /*b Shift operations - integers only
     */
    { expr_subtype_logical_shift_left,  -1, { { {expression_arg_type_integer,    expression_arg_type_match0,   expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                              { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                              { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                              { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },
    { expr_subtype_logical_shift_right, -1, { { {expression_arg_type_integer,    expression_arg_type_match0, expression_arg_type_none}, integer_op, expression_arg_type_match0 },
                                              { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                              { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 },
                                              { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },

    /*b special operations on stuff
     */
    { expr_subtype_query,               1, { { {expression_arg_type_bool,       expression_arg_type_bit_vector, expression_arg_type_match1}, vector_op, expression_arg_type_match1 },
                                             { {expression_arg_type_bool,       expression_arg_type_integer,    expression_arg_type_match1}, integer_op, expression_arg_type_match1 },
                                             { {expression_arg_type_bool,       expression_arg_type_bool,       expression_arg_type_match1}, logical_op, expression_arg_type_match1 },
                                             { {expression_arg_type_none,       expression_arg_type_none,   expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },

    { expr_subtype_sizeof,              0, { { {expression_arg_type_bit_vector, expression_arg_type_none, expression_arg_type_none}, special_op, expression_arg_type_integer },
                                             { {expression_arg_type_integer,    expression_arg_type_none, expression_arg_type_none}, special_op, expression_arg_type_integer },
                                             { {expression_arg_type_bool,       expression_arg_type_none, expression_arg_type_none}, special_op, expression_arg_type_integer },
                                             { {expression_arg_type_none,       expression_arg_type_none, expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },

    { expr_subtype_expression,          0, { { {expression_arg_type_bit_vector, expression_arg_type_none, expression_arg_type_none}, special_op, expression_arg_type_match0 },
                                             { {expression_arg_type_integer,    expression_arg_type_none, expression_arg_type_none}, special_op, expression_arg_type_match0 },
                                             { {expression_arg_type_bool,       expression_arg_type_none, expression_arg_type_none}, special_op, expression_arg_type_match0 },
                                             { {expression_arg_type_none,       expression_arg_type_none, expression_arg_type_none}, NULL,      expression_arg_type_match0 } } },

    /*b End marker
     */
    { expr_subtype_none, 0, { { {expression_arg_type_none, expression_arg_type_none, expression_arg_type_none}, NULL, expression_arg_type_none},
                              { {expression_arg_type_none, expression_arg_type_none, expression_arg_type_none}, NULL, expression_arg_type_none},
                              { {expression_arg_type_none, expression_arg_type_none, expression_arg_type_none}, NULL, expression_arg_type_none},
                              { {expression_arg_type_none, expression_arg_type_none, expression_arg_type_none}, NULL, expression_arg_type_none} } }

};

/*a Functions
 */
/*f vector_op
 */
static int vector_op( c_cyclicity *cyclicity, c_co_expression *expression, t_expr_subtype expr_subtype, t_type_value first, t_type_value second, t_type_value third )
{
    int a_size, b_size, c_size;
    t_sl_uint64 *a_data, *a_mask;
    t_sl_uint64 *b_data, *b_mask;
    t_sl_uint64 *c_data, *c_mask;
    int size_mismatch=0;
    int okay=1;
    if (expr_subtype==expr_subtype_query)
    {
        if ((first!=type_value_error) && (second!=type_value_error) && (third!=type_value_error))
        {
            if (cyclicity->type_value_pool->logical_value( first ))
            {
                expression->type_value = second;
            }
            else
            {
                expression->type_value = third;
            }
            return 1;
        }
        return 0;
    }
    a_size = b_size = c_size = -1;
    a_data = a_mask = b_data = b_mask = c_data = c_mask = NULL;
    if (first!=type_value_error)  okay&=cyclicity->type_value_pool->bit_vector_values( first, &a_size, &a_data, &a_mask );
    if (second!=type_value_error) okay&=cyclicity->type_value_pool->bit_vector_values( second, &b_size, &b_data, &b_mask );
    if (third!=type_value_error)  okay&=cyclicity->type_value_pool->bit_vector_values( third, &c_size, &c_data, &c_mask );
    if (!okay)
        return 0;
    if ((a_size>64) || (b_size>64) || (c_size>64))
    {
        fprintf(stderr, "c_co_expression vector_op:Size out of range %d %d %d\n", a_size,b_size,c_size);
        return 0;
    }
    if ( (a_mask&&a_mask[0]) ||
         (b_mask&&b_mask[0]) ||
         (c_mask&&c_mask[0]) )
    {
        fprintf(stderr, "c_co_expression vector_op:Masks not yet supported\n" );
        return 0;
    }
    SL_DEBUG( sl_debug_level_info, "Vector op %s:%d,%d,%d", expr_subtype_strings[expr_subtype], a_size,b_size,c_size);
    switch (expr_subtype)
    {
    case expr_subtype_binary_not:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bit_array( a_size, (~a_data[0])&bit_mask[a_size+1], 0ULL );
        break;
    case expr_subtype_binary_and:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bit_array( a_size, (a_data[0]&b_data[0])&bit_mask[a_size+1], 0ULL );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_binary_or:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bit_array( a_size, (a_data[0]|b_data[0])&bit_mask[a_size+1], 0ULL );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_binary_xor:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bit_array( a_size, (a_data[0]^b_data[0])&bit_mask[a_size+1], 0ULL );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_arithmetic_plus:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bit_array( a_size, (a_data[0]+b_data[0])&bit_mask[a_size+1], 0ULL );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_arithmetic_minus:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bit_array( a_size, (a_data[0]-b_data[0])&bit_mask[a_size+1], 0ULL );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_arithmetic_multiply:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bit_array( a_size, (a_data[0]*b_data[0])&bit_mask[a_size+1], 0ULL );
        // NOTE that the multiply returns the size of the first, independent of the size of the second - hence not size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_negate:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bit_array( a_size, (-a_data[0])&bit_mask[a_size+1], 0ULL );
        break;
    case expr_subtype_compare_lt:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a_data[0]<b_data[0] );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_compare_le:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a_data[0]<=b_data[0] );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_compare_gt:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a_data[0]>b_data[0] );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_compare_ge:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a_data[0]>=b_data[0] );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_compare_eq:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a_data[0]==b_data[0] );
        size_mismatch = (a_size!=b_size);
        break;
    case expr_subtype_compare_ne:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a_data[0]!=b_data[0] );
        size_mismatch = (a_size!=b_size);
        break;
    default:
        fprintf(stderr,"Unexpectedly here in c_co_expression vector_op (%s)\n",expr_subtype_strings[expr_subtype]);
        return 0;
    }
    return 1;
}

/*f logical_op
 */
static int logical_op( c_cyclicity *cyclicity, c_co_expression *expression, t_expr_subtype expr_subtype, t_type_value first, t_type_value second, t_type_value third )
{
    int a, b, c;

    a = cyclicity->type_value_pool->logical_value( first );
    b = cyclicity->type_value_pool->logical_value( second );
    c = cyclicity->type_value_pool->logical_value( third );
    SL_DEBUG( sl_debug_level_info, "Logical op %d:%d,%d,%d", expr_subtype,a,b,c);
    switch (expr_subtype)
    {
    case expr_subtype_binary_not:
    case expr_subtype_logical_not:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( !a );
        break;
    case expr_subtype_binary_and:
    case expr_subtype_logical_and:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a&&b );
        break;
    case expr_subtype_binary_or:
    case expr_subtype_logical_or:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a||b );
        break;
    case expr_subtype_binary_xor:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a^b );
        break;
    case expr_subtype_compare_eq:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a==b );
        break;
    case expr_subtype_compare_ne:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a!=b );
        break;
    case expr_subtype_query:
        if (a)
            expression->type_value =  second;
        else
            expression->type_value =  third;
        break;
    default:
        fprintf(stderr,"Unexpectedly here in c_co_expression logical_op (%s)\n",expr_subtype_strings[expr_subtype]);
        return 0;
    }
    return 1;
}

/*f integer_op
 */
static int integer_op( c_cyclicity *cyclicity, c_co_expression *expression, t_expr_subtype expr_subtype, t_type_value first, t_type_value second, t_type_value third )
{
    t_sl_sint64 a, b, c;
    if (expr_subtype==expr_subtype_query)
    {
        if ((first!=type_value_error) && (second!=type_value_error) && (third!=type_value_error))
        {
            if (cyclicity->type_value_pool->logical_value( first ))
            {
                expression->type_value = second;
            }
            else
            {
                expression->type_value = third;
            }
            return 1;
        }
        return 0;
    }
    a = (first!=type_value_error)?cyclicity->type_value_pool->integer_value( first ):0;
    b = (second!=type_value_error)?cyclicity->type_value_pool->integer_value( second ):0;
    c = (third!=type_value_error)?cyclicity->type_value_pool->integer_value( third ):0;
    SL_DEBUG( sl_debug_level_info, "Integer op %s:%lld,%lld,%lld", expr_subtype_strings[expr_subtype], a,b,c);
    switch (expr_subtype)
    {
    case expr_subtype_binary_not:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( ~a );
        break;
    case expr_subtype_binary_and:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( a&b );
        break;
    case expr_subtype_binary_or:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( a|b );
        break;
    case expr_subtype_binary_xor:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( a^b );
        break;
    case expr_subtype_arithmetic_plus:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( a+b );
        break;
    case expr_subtype_arithmetic_minus:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( a-b );
        break;
    case expr_subtype_arithmetic_multiply:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( a*b );
        break;
    case expr_subtype_arithmetic_divide:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( a/b );
        break;
    case expr_subtype_arithmetic_remainder:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( a%b );
        break;
    case expr_subtype_negate:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( -a );
        break;
    case expr_subtype_logical_shift_left:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( a<<b );
        break;
    case expr_subtype_logical_shift_right:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( (t_sl_sint64)(((t_sl_uint64) a)>>b) );
        break;
    case expr_subtype_compare_lt:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a<b );
        break;
    case expr_subtype_compare_le:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a<=b );
        break;
    case expr_subtype_compare_gt:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a>b );
        break;
    case expr_subtype_compare_ge:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a>=b );
        break;
    case expr_subtype_compare_eq:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a==b );
        break;
    case expr_subtype_compare_ne:
        expression->type_value = cyclicity->type_value_pool->new_type_value_bool( a!=b );
        break;
    default:
        fprintf(stderr,"Unexpectedly here in c_co_expression integer_op (%s)\n",expr_subtype_strings[expr_subtype]);
        return 0;
    }
    return 1;
}

/*f special_op
 */
static int special_op( c_cyclicity *cyclicity, c_co_expression *expression, t_expr_subtype expr_subtype, t_type_value first, t_type_value second, t_type_value third )
{
    switch (expr_subtype)
    {
    case expr_subtype_expression:
        expression->type_value = first;
        break;
    case expr_subtype_sizeof:
        expression->type_value = cyclicity->type_value_pool->new_type_value_integer( cyclicity->type_value_pool->get_size_of( first ) );
        break;
    default:
        return 0;
    }
    return 1;
}

/*f c_co_expression::check_type_match_from_expression_fn - Check expression can be coerced to the argument required
 */
int c_co_expression::check_type_match_from_expression_fn( class c_cyclicity *cyclicity, int table_index, int subentry, int arg, c_co_expression *expression, int coercions_permitted )
{
    t_expression_arg_type type_to_match;

    type_to_match = expression_fn_list[table_index].subentry[subentry].args[arg];
    if (type_to_match==expression_arg_type_match0)
    {
        type_to_match = expression_fn_list[table_index].subentry[subentry].args[0];
    }
    if (type_to_match==expression_arg_type_match1)
    {
        type_to_match = expression_fn_list[table_index].subentry[subentry].args[1];
    }
    switch (type_to_match)
    {
    case expression_arg_type_bit_vector:
        if (!expression) return 0;
        if (cyclicity->type_value_pool->derefs_to_bit_vector(expression->type))
            return 1;
        if (cyclicity->type_value_pool->derefs_to_bit(expression->type))
            return 1;
        if ( (coercions_permitted==coerce_permitted_to_bit_vector) &&
             (cyclicity->type_value_pool->derefs_to_bool(expression->type) || cyclicity->type_value_pool->derefs_to_integer(expression->type)) )
            return 1;
        return 0;
    case expression_arg_type_integer:
        if (!expression) return 0;
        if (cyclicity->type_value_pool->derefs_to_integer(expression->type))
            return 1;
        if ( (coercions_permitted==coerce_permitted_to_integer) &&
             (cyclicity->type_value_pool->derefs_to_bool(expression->type)) )
            return 1;
        return 0;
    case expression_arg_type_bool:
        if (!expression) return 0;
        if (cyclicity->type_value_pool->derefs_to_bool(expression->type))
            return 1;
        return 0;
    case expression_arg_type_none:
        if (!expression)
            return 1;
        return 0;
    default:
        return 0;
    }
    return 0;
}

/*f c_co_expression::coerce_if_necessary - 
 */
int c_co_expression::coerce_if_necessary( c_cyclicity *cyclicity, int table_index, int subentry, int arg, c_co_expression *subexpr )
{
    t_expression_arg_type type_to_match;
    c_co_expression *expr_to_match;
    if (!subexpr)
        return 1;

    /*b If this arg is supposed to match something else, find that - note that that arg may be being coerced, so the matching expression's type may not be correct
      However, if this arg is supposed to end up as a bit vector and it is a bit vector, then it is not subject to coercion
      If the arg to match is a bit vector then the sizes of the two must match
      If the other arg is not a bit vector then it will be coerced to match this bit vector
      If this arg is not a bit vector then it must be coerced to match the other arg
     */
    expr_to_match = NULL;
    type_to_match = expression_fn_list[table_index].subentry[subentry].args[arg];
    if (type_to_match==expression_arg_type_match0)
    {
        type_to_match = expression_fn_list[table_index].subentry[subentry].args[0];
        expr_to_match = first;
    }
    if (type_to_match==expression_arg_type_match1)
    {
        type_to_match = expression_fn_list[table_index].subentry[subentry].args[1];
        expr_to_match = second;
    }

    /*b Match types
     */
    switch (type_to_match)
    {
    case expression_arg_type_integer: // It must be an int or coerce to an int
        if (cyclicity->type_value_pool->derefs_to_integer(subexpr->type))
            return 1;
        return subexpr->coerce(cyclicity, type_value_integer);
    case expression_arg_type_bool: // It must be a bool or coerce to a bool
        if (cyclicity->type_value_pool->derefs_to_bool(subexpr->type))
            return 1;
        return subexpr->coerce(cyclicity, type_value_bool);
    case expression_arg_type_bit_vector: // This must be a bit/bit vector (in which case, if the expr_to_match is a vector, check the size matches) or coerce it to one of the correct size
        if ( cyclicity->type_value_pool->derefs_to_bit_vector(subexpr->type) ||
             cyclicity->type_value_pool->derefs_to_bit(subexpr->type) )
        {
            t_type_value match_type_derefed, type_derefed;
            if (expr_to_match && ( cyclicity->type_value_pool->derefs_to_bit_vector(expr_to_match->type) || cyclicity->type_value_pool->derefs_to_bit(expr_to_match->type)) )
            {
                match_type_derefed = cyclicity->type_value_pool->derefs_to( expr_to_match->type );
                type_derefed = cyclicity->type_value_pool->derefs_to( type );
                if (match_type_derefed != type_derefed)
                {
                    return subexpr->coerce(cyclicity, expr_to_match->type ); // This is basically to force a type mismatch error in the same way as other errors occur
                }
            }
            return 1;
        }
        // Aarrgh - need to coerce to a bit vector, but what size? Well, there must be a bit vector that this must match, find that
        if ((arg==0) && (expression_fn_list[table_index].subentry[subentry].args[1]==expression_arg_type_match0)) expr_to_match=second;
        else if ((arg==0) && (expression_fn_list[table_index].subentry[subentry].args[2]==expression_arg_type_match0)) expr_to_match=third;
        else if ((arg==1) && (expression_fn_list[table_index].subentry[subentry].args[2]==expression_arg_type_match1)) expr_to_match=third;
        break;
    default:
        break;
    }
    if (expr_to_match)
        return subexpr->coerce(cyclicity, expr_to_match->type);
    return 0;
}

/*f c_co_expression::cross_reference_within_type_context
  This method cross references an expression within the scopes given, with an expected target type (if that helps resolve the references)
 */
void c_co_expression::cross_reference_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type_context, t_type_value expected_type )
{
    int i;
    int table_index;

    type = type_value_undefined;
    switch (expr_type)
    {
    case expr_type_sized_int:
        break;
    case expr_type_lvar:
        if (lvar)
        {
            lvar->cross_reference_post_type_resolution_within_type_context( cyclicity, types, variables, type_context );
        }
        break;
    case expr_type_unary_function:
    case expr_type_binary_function:
    case expr_type_ternary_function:
        table_index = -1;
        for (i=0; expression_fn_list[i].op!=expr_subtype_none; i++)
        {
            if (expression_fn_list[i].op == expr_subtype)
            {
                table_index = i;
                break;
            }
        }
        if (table_index>=0)
        {
            if (first)
            {
                first->cross_reference_within_type_context( cyclicity, types, variables, type_context, type_value_undefined );
            }
            if (second)
            {
                second->cross_reference_within_type_context( cyclicity, types, variables, type_context, type_value_undefined );
            }
            if (third)
            {
                third->cross_reference_within_type_context( cyclicity, types, variables, type_context, type_value_undefined );
            }
        }
        else
        {
            fprintf(stderr,"c_co_expression::cross_reference_within_type_context:NYI expression subtype %d:****************************************\n", expr_subtype );
        }
        break;
    case expr_type_bundle:
        c_co_expression *subexpr;
        for (subexpr=first; subexpr; subexpr=(c_co_expression *)(subexpr->next_in_list))
        {
            subexpr->cross_reference_within_type_context( cyclicity, types, variables, type_context, expression_arg_type_bit_vector );
        }
        break;
    default:
        fprintf(stderr,"c_co_expression::cross_reference_within_type_context:NYI expression type %d:****************************************\n", expr_type );
    }
    // Shouldn't we do the next in the list? - no, that is up to the caller
}

/*f c_co_expression::type_check_within_type_context
  This method checks the type of an expression within the scopes given, with a required target type
  If the target type is undefined, then the type of the expression should be set to the type that comes from the expression without any casting
  If the target type is defined then an error should be reported if an implicit cast cannot be performed from the expression value to that required type
  This also fills in the internal_table_index which indicates which function of a polymorphic set the expression refers to
 */
int c_co_expression::type_check_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value required_type )
{
    int i;
    int table_index;

    /*b Determine type of expression
     */
    { char buffer[256];
        CO_DEBUG( sl_debug_level_verbose_info, "This %p: Types %p variables %p required_type %s", this, types, variables, cyclicity->type_value_pool->display(buffer,sizeof(buffer),required_type) );
    }
    type = type_value_error;
    internal_table_index = -1;
    switch (expr_type)
    {
    case expr_type_sized_int:
        type = cyclicity->type_value_pool->get_type(sized_int->type_value);
        break;
    case expr_type_lvar:
        if (lvar)
        {
            lvar->check_types( cyclicity, types, variables );
            type = lvar->type;
            // A clock referenced as an lvar will have a type of -1
        }
        break;
    case expr_type_unary_function:
    case expr_type_binary_function:
    case expr_type_ternary_function:
        CO_DEBUG( sl_debug_level_verbose_info, "Expression %p of type %s (%p, %p, %p)", this, expr_subtype_strings[expr_subtype], first, second, third );
        table_index = -1;
        for (i=0; expression_fn_list[i].op!=expr_subtype_none; i++)
        {
            if (expression_fn_list[i].op == expr_subtype)
            {
                table_index = i;
                break;
            }
        }
        if ((table_index>=0) && first)
        {
            int subentry;
            int coercion_required;

            /*b Check the types first - no coercions yet
             */
            if ((first) && !first->type_check_within_type_context( cyclicity, types, variables, type_value_undefined )) return 0;
            if ((second) && !second->type_check_within_type_context( cyclicity, types, variables, type_value_undefined )) return 0;
            if ((third) && !third->type_check_within_type_context( cyclicity, types, variables, type_value_undefined )) return 0;
            if (first) { char buffer[256]; CO_DEBUG( sl_debug_level_verbose_info, "First %p has type %s", first, cyclicity->type_value_pool->display(buffer,sizeof(buffer),first->type) ); }
            if (second) { char buffer[256]; CO_DEBUG( sl_debug_level_verbose_info, "Second %p has type %s", second, cyclicity->type_value_pool->display(buffer,sizeof(buffer),second->type) ); }
            if (third) { char buffer[256]; CO_DEBUG( sl_debug_level_verbose_info, "Third %p has type %s", third, cyclicity->type_value_pool->display(buffer,sizeof(buffer),third->type) ); }

            /*b Find all required coercions - warn on int to bool if int is non-zero? Do that at compile time. Don't think its necessary actually.
             */
            if (expression_fn_list[table_index].coercions>0)
            {
                if (first && (expression_fn_list[table_index].coercions&1))  first->coerce(cyclicity, expression_fn_list[table_index].subentry[0].args[0]==expression_arg_type_bool?type_value_bool:type_value_integer);
                if (second && (expression_fn_list[table_index].coercions&2)) second->coerce(cyclicity, expression_fn_list[table_index].subentry[0].args[1]==expression_arg_type_bool?type_value_bool:type_value_integer);
                if (third && (expression_fn_list[table_index].coercions&4))  third->coerce(cyclicity, expression_fn_list[table_index].subentry[0].args[2]==expression_arg_type_bool?type_value_bool:type_value_integer);
            }

            /*b Now try a direct match
             */
            subentry = -1;
            coercion_required = coerce_permitted_to_none;
            for (i=0; (i<4) && (subentry<0); i++)
            {
                if (expression_fn_list[table_index].subentry[i].fn)
                {
                    if ( check_type_match_from_expression_fn( cyclicity, table_index, i, 0, first, coerce_permitted_to_none ) &&
                         check_type_match_from_expression_fn( cyclicity, table_index, i, 1, second, coerce_permitted_to_none ) &&
                         check_type_match_from_expression_fn( cyclicity, table_index, i, 2, third, coerce_permitted_to_none ) )
                    {
                        subentry=i;
                    }
                }
            }
            { CO_DEBUG( sl_debug_level_verbose_info, "Direct match check - found %d", subentry); }

            /*b If not found, try to see which coercions are required for each argument: if none are permitted, skip this.
              look at degrading the arguments; bools first to ints, to see if they match; then bools to bv and int to bv. These casts need no warnings
              First see what would happen if a function match can be found by ignoring bools mismatching ints. If it then matches, coerce as required for the match
              If that fails, see what would happen if a function match can be found by ignoring any non-bit vectors. If it then matches, coerce as required for the match
             */
            for (i=0; (i<4) && (subentry<0); i++)
            {
                if (expression_fn_list[table_index].subentry[i].fn)
                {
                    if ( check_type_match_from_expression_fn( cyclicity, table_index, i, 0, first, coerce_permitted_to_integer ) &&
                         check_type_match_from_expression_fn( cyclicity, table_index, i, 1, second, coerce_permitted_to_integer ) &&
                         check_type_match_from_expression_fn( cyclicity, table_index, i, 2, third, coerce_permitted_to_integer ) )
                    {
                        subentry = i;
                        coercion_required = coerce_permitted_to_integer;
                    }
                }
            }
            { CO_DEBUG( sl_debug_level_verbose_info, "Coercions to integer checked - found %d", subentry); }
            for (i=0; (i<4) && (subentry<0); i++)
            {
                if (expression_fn_list[table_index].subentry[i].fn)
                {
                    if ( check_type_match_from_expression_fn( cyclicity, table_index, i, 0, first, coerce_permitted_to_bit_vector ) &&
                         check_type_match_from_expression_fn( cyclicity, table_index, i, 1, second, coerce_permitted_to_bit_vector ) &&
                         check_type_match_from_expression_fn( cyclicity, table_index, i, 2, third, coerce_permitted_to_bit_vector ) )
                    {
                        subentry = i;
                        coercion_required = coerce_permitted_to_bit_vector;
                    }
                }
            }
            { CO_DEBUG( sl_debug_level_verbose_info, "Coercions to bit vector checked - found %d", subentry); }

            /*b Okay, if found do coercions, and mark the subentry found too
             */
            if (subentry>=0)
            {
                int okay=1;
                okay &= coerce_if_necessary( cyclicity, table_index, subentry, 0, first );
                okay &= coerce_if_necessary( cyclicity, table_index, subentry, 1, second );
                okay &= coerce_if_necessary( cyclicity, table_index, subentry, 2, third );
                { CO_DEBUG( sl_debug_level_verbose_info, "Coercions required done, got result %d", okay); }
                if (!okay)
                {
                    subentry = -1;
                }
                else
                {
                    internal_table_index = subentry;
                    switch (expression_fn_list[table_index].subentry[subentry].result_type)
                    {
                    case expression_arg_type_integer: type=type_value_integer; break;
                    case expression_arg_type_bool:    type=type_value_bool; break;
                    case expression_arg_type_match0:  type=first->type; break;
                    case expression_arg_type_match1:  type=second->type; break;
                    default: fprintf(stderr,"Unexpectedly reached default in result of type_check_within_type_context\n"); break;
                    }
                    { char buffer[256]; CO_DEBUG( sl_debug_level_verbose_info, "Result %p has type %s", this, cyclicity->type_value_pool->display(buffer,sizeof(buffer),type) ); }
                }
            }
            if (subentry<0)
            {
                char buffer[256];
                char buffer2[256];
                char buffer3[256];
                if (!second)
                {
                    cyclicity->set_parse_error( this, co_compile_stage_check_types, "No matching operator for '%s' on type %s",
                                                expr_subtype_strings[expr_subtype],
                                                cyclicity->type_value_pool->display(buffer,sizeof(buffer),first->type) );
                }
                else if (!third)
                {
                    cyclicity->set_parse_error( this, co_compile_stage_check_types, "No matching operator for '%s' on types %s, %s",
                                                expr_subtype_strings[expr_subtype],
                                                cyclicity->type_value_pool->display(buffer,sizeof(buffer),first->type),
                                                cyclicity->type_value_pool->display(buffer2,sizeof(buffer2),second->type) );
                }
                else
                {
                    cyclicity->set_parse_error( this, co_compile_stage_check_types, "No matching operator for '%s' on types %s, %s, %s",
                                                expr_subtype_strings[expr_subtype],
                                                cyclicity->type_value_pool->display(buffer,sizeof(buffer),first->type),
                                                cyclicity->type_value_pool->display(buffer2,sizeof(buffer2),second->type),
                                                cyclicity->type_value_pool->display(buffer3,sizeof(buffer3),third->type) );
                }
                type = type_value_error;
                return 0;
            }
        }
        else
        {
            fprintf(stderr,"c_co_expression::type_check_within_type_context:NYI expression subtype %d or bad first:****************************************\n", expr_subtype );
        }
        break;
    case expr_type_bundle: // For a bundle, each subexpression in the expression list should deref to a bit vector of any size
        c_co_expression *subexpr;
        int width;
        width=0;
        for (subexpr=first; subexpr; subexpr=(c_co_expression *)(subexpr->next_in_list))
        {
            t_type_value subexpr_type;
            subexpr->type_check_within_type_context( cyclicity, types, variables, type_value_bit_array_any_size );
            subexpr_type = cyclicity->type_value_pool->derefs_to( subexpr->type );
            if (!cyclicity->type_value_pool->is_bit_vector(subexpr_type))
            {
                fprintf(stderr,"c_co_expression::type_check_within_type_context:NYI possibly: implicit cast inside a bundle:****************************************\n" ); // Although possibly there is a cast in the subexpression for this? In which case its a vector size 1?
            }
            width += cyclicity->type_value_pool->get_size_of( subexpr_type );
        }
        type = cyclicity->type_value_pool->add_bit_array_type( width );
        break;
    default:
        fprintf(stderr,"c_co_expression::type_check_within_type_context:NYI expression type %d:****************************************\n", expr_type );
    }

    /*b Try to cast
     */
    return coerce( cyclicity, required_type );
}

/*f c_co_expression::coerce() - add an implicit cast to the expression to get it to the correct type
 */
int c_co_expression::coerce( class c_cyclicity *cyclicity, t_type_value required_type )
{
    t_type_value required_type_derefed, type_derefed;

    /*b Check types match - maybe a cast is not required
     */
    required_type_derefed = cyclicity->type_value_pool->derefs_to( required_type );
    type_derefed = cyclicity->type_value_pool->derefs_to( type );
    if ( (required_type==type_value_undefined) || (required_type_derefed==type_derefed) )
    {
        return 1;
    }
    // If we required any size of vector, then any size of vector is okay without a cast
    if (required_type==type_value_bit_array_any_size && (cyclicity->type_value_pool->is_bit_vector(type_derefed)))
    {
        type = type_derefed;
        return 1;
    }

    /*b Check to see if we can do an implicit cast
     */
    {
        char buffer[256];
        char buffer2[256];
        CO_DEBUG( sl_debug_level_verbose_info, "Coercion required in expression %p (required %s got %s)", this,
                  cyclicity->type_value_pool->display(buffer,sizeof(buffer),required_type),
                  cyclicity->type_value_pool->display(buffer2,sizeof(buffer2),type) );
    }
    // Can convert a bool to a bit vector of size 1 - its a copy
    if (cyclicity->type_value_pool->is_bit(required_type_derefed) && (cyclicity->type_value_pool->is_bool(type_derefed)))
    {
        implicit_cast = expr_cast_bool_to_bit_vector;
        precast_type = type;
        type = required_type;
        return 1;
    }
    // Can convert a bit vector to a bool - its an implicit (x != 0)
    if (cyclicity->type_value_pool->is_bool(required_type_derefed) && (cyclicity->type_value_pool->is_bit_vector(type_derefed)))
    {
        implicit_cast = expr_cast_bit_vector_to_bool;
        precast_type = type;
        type = required_type;
        return 1;
    }
    // Can convert an integer to a bool - its an implicit (x != 0)
    if (cyclicity->type_value_pool->is_bool(required_type_derefed) && (cyclicity->type_value_pool->is_integer(type_derefed)))
    {
        implicit_cast = expr_cast_integer_to_bool;
        precast_type = type;
        type = required_type;
        return 1;
    }
    // Can convert an integer to a bit vector - its an implicit sign-extended copy
    if (cyclicity->type_value_pool->is_bit_vector(required_type_derefed) && (cyclicity->type_value_pool->is_integer(type_derefed)))
    {
        implicit_cast = expr_cast_integer_to_bit_vector;
        precast_type = type;
        type = required_type;
        CO_DEBUG( sl_debug_level_verbose_info, "Coerced %p to type %d", this, this->type );
        return 1;
    }
    // Can convert a bit vector to an integer
    if (cyclicity->type_value_pool->is_integer(required_type_derefed) && (cyclicity->type_value_pool->is_bit_vector(type_derefed)))
    {
        implicit_cast = expr_cast_bit_vector_to_integer;
        precast_type = type;
        type = required_type;
        return 1;
    }
    // Can convert a bool to an integer
    if (cyclicity->type_value_pool->is_integer(required_type_derefed) && (cyclicity->type_value_pool->is_bool(type_derefed)))
    {
        implicit_cast = expr_cast_bool_to_integer;
        precast_type = type;
        type = required_type;
        return 1;
    }

    /*b Type mismatch, no implicit cast possible
     */
    {
        char buffer[256];
        char buffer2[256];
        cyclicity->set_parse_error( this, co_compile_stage_check_types, "Type mismatch in expression (required %s got %s)",
                                    cyclicity->type_value_pool->display(buffer,sizeof(buffer),required_type),
                                    cyclicity->type_value_pool->display(buffer2,sizeof(buffer2),type) );
    }
    type = type_value_error;
    return 0;
}

/*f check_operand  - check operand
  return 0 for failure
  return 1 for okay
  return -1 for try a different match
*/
static int check_operand( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int table_index, int subentry, int arg, c_co_expression *expression, int reevaluate )
{
    t_expression_arg_type type_to_match;

    if (expression_fn_list[table_index].subentry[subentry].args[arg] == expression_arg_type_none)
        return 1;

    if (!expression)
        return 0;

    if ((expression->type_value == type_value_undefined) || reevaluate)
    {
        if (!expression->internal_evaluate( cyclicity, types, variables, reevaluate ))
        {
            return 0;
        }
    }

    if (!cyclicity->type_value_pool->check_resolved( expression->type_value ))
    {
        cyclicity->set_parse_error( expression, co_compile_stage_evaluate_constants, "Use of unresolved value (%d) in evaluation\n", expression->type_value  );
        return 0;
    }

    type_to_match = expression_fn_list[table_index].subentry[subentry].args[arg];
    if (type_to_match==expression_arg_type_match0)
    {
        type_to_match = expression_fn_list[table_index].subentry[subentry].args[0];
    }
    if (type_to_match==expression_arg_type_match1)
    {
        type_to_match = expression_fn_list[table_index].subentry[subentry].args[1];
    }

    switch (type_to_match)
    {
    case expression_arg_type_bool:
        if (!cyclicity->type_value_pool->derefs_to_bool( expression->type_value ) )
            return -1;
        break;
    case expression_arg_type_integer:
        if (!cyclicity->type_value_pool->derefs_to_integer( expression->type_value ) )
            return -1;
        break;
    case expression_arg_type_string:
        //if (type!=type_value_string)
            return -1;
        break;
    case expression_arg_type_bit_vector:
        if ( !cyclicity->type_value_pool->derefs_to_bit_vector( expression->type_value ) &&
             !cyclicity->type_value_pool->derefs_to_bit( expression->type_value ) )
            return -1;
        break;
    default:
        fprintf(stderr, "c_co_expression::check_operand:NYI:DID NOT EXPECT TO GET HERE\n");
        break;
    }
    return 1;
}

/*f c_co_expression::internal_evaluate
  Return 1 if evaluated okay.
  We expect to leave a single item on the top of the stack.
*/
int c_co_expression::internal_evaluate( c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    int i;

    CO_DEBUG( sl_debug_level_info, "Evaluate %p reevaluate %d", this, reevaluate );

    switch (expr_type)
    {
    case expr_type_sized_int:
        type_value = sized_int->type_value;
        break;
    case expr_type_lvar:
        if (!lvar)
        {
            CO_DEBUG( sl_debug_level_high_warning, "Attempted to dereference a non-referenced lvar in expression evaluation %p", this );
            return 0;
        }
        if (!lvar->non_signal_declaration.cyc_object)
        {
            lvar->evaluate_constant_expressions( cyclicity, types, variables, reevaluate );
            return 0;
        }
        if (lvar->first)
            return 0;
        switch (lvar->non_signal_declaration.cyc_object->co_type)
        {
        case co_type_constant_declaration:
            this->type_value = lvar->non_signal_declaration.co_constant_declaration->type_value;
            break;
        case co_type_fsm_state:
            this->type_value = lvar->non_signal_declaration.co_fsm_state->type_value;
            break;
        case co_type_enum_identifier:
            this->type_value = lvar->non_signal_declaration.co_enum_identifier->type_value;
            break;
        case co_type_type_definition:
            this->type_value = lvar->non_signal_declaration.co_type_definition->type_value;
            break;
        case co_type_statement:
            if (lvar->non_signal_declaration.co_statement->statement_type==statement_type_for_statement)
            {
                this->type_value = lvar->non_signal_declaration.co_statement->type_data.for_stmt.type_value;
                if (this->type_value==type_value_undefined) // Iterator not defined yet; must be a potential evaluation, not a real call
                {
                    return 0;
                }
            }
            break;
        default:
            fprintf( stderr, "Not yet implemented symbol type %d for reference in evaluation\n", lvar->non_signal_declaration.cyc_object->co_type);
            return 0;
            break;
        }
        break;
    case expr_type_unary_function:
    case expr_type_binary_function:
    case expr_type_ternary_function:
        CO_DEBUG( sl_debug_level_verbose_info, "Evaluating expression %p of type %s (%p, %p, %p)", this, expr_subtype_strings[expr_subtype], first, second, third );
        for (i=0; expression_fn_list[i].op!=expr_subtype_none; i++)
        {
            if (expression_fn_list[i].op == expr_subtype)
            {
                int okay;
                okay = 1; // Do these as 3 separate statements so that they all get performed - they evaluate constants all the way through
                okay &= check_operand( cyclicity, types, variables, i, internal_table_index, 0, first, reevaluate );
                okay &= check_operand( cyclicity, types, variables, i, internal_table_index, 1, second, reevaluate );
                okay &= check_operand( cyclicity, types, variables, i, internal_table_index, 2, third, reevaluate );
                if (okay)
                {
                    if (!expression_fn_list[i].subentry[internal_table_index].fn( cyclicity,
                                                                                  this,
                                                                                  expr_subtype,
                                                                                  first?first->type_value:type_value_error,
                                                                                  second?second->type_value:type_value_error,
                                                                                  third?third->type_value:type_value_error ))
                    {
                        return 0;
                    }
                    break; // Fall through to the implicit cast stuff
                }
                else
                {
                    return 0;
                }
            }
        }
        if (expression_fn_list[i].op==expr_subtype_none)
        {
            fprintf(stderr,"Unimplemented op %d\n",expr_subtype);
            cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Unimplemented subtype %d of binary function", expr_subtype );
            return 0;
        }
        break;
    case expr_type_bundle:
        c_co_expression *subexpr;
        for (subexpr=first; subexpr; subexpr=(c_co_expression *)(subexpr->next_in_list))
        {
            subexpr->internal_evaluate( cyclicity, types, variables, reevaluate );
        }
        // We don't as yet do bundling of constant bits; this is not a high priority, as this is a rare requirement
        return 0;
    }
    switch (implicit_cast)
    {
    case expr_cast_none:
        break;
    case expr_cast_integer_to_bit_vector:
    {
        t_sl_uint64 integer;
        int size;
        integer = cyclicity->type_value_pool->integer_value(type_value);
        size = cyclicity->type_value_pool->array_size(type);
        CO_DEBUG( sl_debug_level_verbose_info, "casting integer %llx to bit vector of size %d", integer, size );
        type_value = cyclicity->type_value_pool->new_type_value_bit_array( size, integer, 0 );
        break;
    }
    case expr_cast_integer_to_bool:
    {
        int boolean;
        boolean = cyclicity->type_value_pool->logical_value(type_value);
        CO_DEBUG( sl_debug_level_verbose_info, "casting integer to bool %d", boolean );
        type_value = cyclicity->type_value_pool->new_type_value_bool( boolean );
        break;
    }
    case expr_cast_bit_vector_to_bool:
    {
        int boolean;
        boolean = cyclicity->type_value_pool->logical_value(type_value);
        CO_DEBUG( sl_debug_level_verbose_info, "casting bit vector to bool %d", boolean );
        type_value = cyclicity->type_value_pool->new_type_value_bool( boolean );
        break;
    }
    case expr_cast_bit_vector_to_integer:
    {
        t_sl_uint64 integer;
        integer = cyclicity->type_value_pool->integer_value(type_value);
        CO_DEBUG( sl_debug_level_verbose_info, "casting bit vector to integer %lld", integer );
        type_value = cyclicity->type_value_pool->new_type_value_integer( integer );
        break;
    }
    case expr_cast_bool_to_bit_vector:
    {
        int boolean;
        boolean = cyclicity->type_value_pool->logical_value(type_value);
        CO_DEBUG( sl_debug_level_verbose_info, "casting bool %d to bit vector", boolean );
        type_value = cyclicity->type_value_pool->new_type_value_bit_array( 1, boolean, 0 );
        break;
    }
    case expr_cast_bool_to_integer:
    {
        int boolean;
        boolean = cyclicity->type_value_pool->logical_value(type_value);
        CO_DEBUG( sl_debug_level_verbose_info, "casting bool %d to integer", boolean );
        type_value = cyclicity->type_value_pool->new_type_value_integer( boolean );
        break;
    }
    }
    CO_DEBUG( sl_debug_level_info, "Evaluate okay %p type %d value %d", this, this->type, this->type_value );
    return 1;
}

/*f c_co_expression::evaluate_constant
  Should only be called if the expression has been type checked okay
 */
int c_co_expression::evaluate_constant( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    CO_DEBUG( sl_debug_level_verbose_info, "evaluating this %p reevaluate %d expected type %d", this, reevaluate, type );

    if (cyclicity->type_value_pool->check_resolved( this->type_value ) && !reevaluate)
    {
        return 1;
    }

    // Now evaluate this (and any subsequent if a list)
    if (!this->internal_evaluate( cyclicity, types, variables, reevaluate ))
        return 0;

    // Now our item should be defined - if not, something screwy going on
    if (!cyclicity->type_value_pool->check_resolved( this->type_value ))
    {
        cyclicity->set_parse_error( (c_cyc_object *)this, co_compile_stage_evaluate_constants, "Probable bug (or not-yet implemented feature) - expected to have resolved expression, but had not" );
        return 0;
    }
    {
        char buffer[256];
        lex_string_from_terminal( cyclicity, this->type_value, buffer, 256 );
//          CO_DEBUG( sl_debug_level_verbose_info, "result %s", buffer );
        CO_DEBUG( sl_debug_level_info, "result %s (%d) (%d)", buffer, this->type_value, cyclicity->type_value_pool->integer_value(this->type_value) );
    }
    return 1;
}

/*f c_co_expression::evaluate_potential_constant
  Should only be called if the expression has been type checked okay
  Returns 1 if the expression is a constant
  Set reevaluate if the contents may currently be invalid (for example in a 'for' loop - it may have evaluated cleanly once, but it needs a complete reevaluation in a new scope)
 */
int c_co_expression::evaluate_potential_constant( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate )
{
    CO_DEBUG( sl_debug_level_verbose_info, "evaluating this %p (%d/%d) reevaluate %d", this, this->type, this->type_value, reevaluate );

    if ( reevaluate || 
         !cyclicity->type_value_pool->check_resolved( this->type_value ) )
    {
        internal_evaluate( cyclicity, types, variables, reevaluate );
    }
    return cyclicity->type_value_pool->check_resolved( this->type_value );
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



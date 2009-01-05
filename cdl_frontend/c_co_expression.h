/*a Copyright
  
  This file 'c_co_expression.h' copyright Gavin J Stark 2003, 2004
  
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
#ifdef __INC_C_CO_EXPRESSION
#else
#define __INC_C_CO_EXPRESSION

/*a Includes
 */
#include "lexical_types.h"
#include "c_cyc_object.h"

/*a Types
 */
/*t t_expr_type
 */
typedef enum
{
    expr_type_sized_int,
    expr_type_lvar,
    expr_type_ternary_function,
    expr_type_binary_function,
    expr_type_unary_function,
    expr_type_bundle
} t_expr_type;

/*t t_expr_subtype
 */
typedef enum
{
    expr_subtype_none,
    expr_subtype_expression,
    expr_subtype_query,
    expr_subtype_arithmetic_plus,
    expr_subtype_arithmetic_minus,
    expr_subtype_arithmetic_multiply,
    expr_subtype_arithmetic_divide,
    expr_subtype_arithmetic_remainder,
    expr_subtype_logical_not,
    expr_subtype_logical_and,
    expr_subtype_logical_or,
    expr_subtype_logical_shift_left,
    expr_subtype_logical_shift_right,
    expr_subtype_binary_not,
    expr_subtype_binary_and,
    expr_subtype_binary_or,
    expr_subtype_binary_xor,
    expr_subtype_compare_lt,
    expr_subtype_compare_le,
    expr_subtype_compare_gt,
    expr_subtype_compare_ge,
    expr_subtype_compare_eq,
    expr_subtype_compare_ne,
    expr_subtype_negate,
    expr_subtype_sizeof,
} t_expr_subtype;

/*t t_expr_cast
  Potential casts that should be performed after the expression element has been evaluated - i.e. this is ((cast)(expression))
  These will be generated implicitly where required
 */
typedef enum
{
    expr_cast_none,

    expr_cast_bool_to_bit_vector,
    expr_cast_integer_to_bit_vector,

    expr_cast_bit_vector_to_bool,
    expr_cast_integer_to_bool,

    expr_cast_bool_to_integer,
    expr_cast_bit_vector_to_integer,

} t_expr_cast;

/*t c_co_expression
 */
class c_co_expression: public c_cyc_object
{
public:
    c_co_expression( t_sized_int *sized_int );
    c_co_expression( class c_co_lvar *lvar );
    c_co_expression( c_co_expression *expression, t_symbol *element );
    c_co_expression( t_expr_subtype expr_subtype, c_co_expression *left, c_co_expression *mid, c_co_expression *right );
    c_co_expression( t_expr_subtype expr_subtype, c_co_expression *left, c_co_expression *right );
    c_co_expression( t_expr_subtype expr_subtype, c_co_expression *arg );
    c_co_expression( t_expr_type expr_type, t_expr_subtype expr_subtype, c_co_expression *arg );
    ~c_co_expression();
    c_co_expression *chain_tail( c_co_expression *entry );

    int check_type_match_from_expression_fn( class c_cyclicity *cyclicity, int table_index, int subentry, int arg, class c_co_expression *expression, int coercions_permitted );
    int coerce( class c_cyclicity *cyclicity, t_type_value required_type );
    int coerce_if_necessary( class c_cyclicity *cyclicity, int table_index, int subentry, int arg, c_co_expression *subexpr );
    void cross_reference_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value type_context, t_type_value expected_type );
    int type_check_within_type_context( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, t_type_value required_type );
    int internal_evaluate( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );
    int evaluate_constant( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );
    int evaluate_potential_constant( class c_cyclicity *cyclicity, t_co_scope *types, t_co_scope *variables, int reevaluate );
    struct t_md_expression *build_model( class c_cyclicity *cyclicity, class c_model_descriptor *model, struct t_md_module *module, struct t_md_lvar *model_lvar_context );
//    struct t_md_expression *build_model_given_element( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module, t_md_lvar *model_lvar_context, t_symbol *symbol );
    struct t_md_lvar *build_lvar( class c_cyclicity *cyclicity, c_model_descriptor *model, t_md_module *module, t_md_lvar *model_lvar_context );

    t_type_value type_value;           // compile-time type and value of the expression
    t_type_value type;                 // type of expression (after casting, once that is determined)
private:
    t_expr_type expr_type;             // function, sized int, subexpression, etc
    t_expr_subtype expr_subtype;       // type of function, etc
    t_sized_int *sized_int;            // only for constant values
    class c_co_lvar *lvar;             // for a signal reference
    c_co_expression *first;            // for indexed or functions, ownership link
    c_co_expression *second;           // for double indexed or binary functions, ownership link
    c_co_expression *third;            // only for ternary functions, ownership link

    t_expr_cast implicit_cast;         // implicit cast to be performed after the expression is evaluated
    t_type_value precast_type;         // type of expression before implicit casting - only if implicit casting is done
    int internal_table_index;          // internal value to indicate which expression entry this is for a polymorphic function
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




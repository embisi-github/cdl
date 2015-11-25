/*a Copyright
  
  This file 'cyclicity_grammar.y' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a Documentation
To turn on debugging, set yydebug=1 in yyparse()
Also define YYLOG() to be printf
*/

/*a C Headers
 */
%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "c_co_token_union.h"
#include "c_co_expression.h"
#include "c_co_sized_int_pair.h"
#include "c_co_instantiation.h"
#include "c_co_case_entry.h"
#include "c_co_clock_reset_defn.h"
#include "c_co_code_label.h"
#include "c_co_constant_declaration.h"
#include "c_co_declspec.h"
#include "c_co_enum_identifier.h"
#include "c_co_enum_definition.h"
#include "c_co_fsm_state.h"
#include "c_co_lvar.h"
#include "c_co_module_body_entry.h"
#include "c_co_module.h"
#include "c_co_module_prototype_body_entry.h"
#include "c_co_module_prototype.h"
#include "c_co_nested_assignment.h"
#include "c_co_named_expression.h"
#include "c_co_schematic_symbol.h"
#include "c_co_schematic_symbol_body_entry.h"
#include "c_co_signal_declaration.h"
#include "c_co_statement.h"
#include "c_co_timing_spec.h"
#include "c_co_type_specifier.h"
#include "c_co_type_struct.h"
#include "c_co_type_definition.h"
#include "c_co_toplevel.h"
#include "c_co_port_map.h"

#include "c_lexical_analyzer.h"
#include "c_cyclicity.h"
// The next line forces inclusion of yytoknum in FedoraCore3.0
#define YYPRINT(a,b,c)

#if 0
#define YYLOG(x) do {printf(x);} while (0)
#else
#define YYLOG(x) do {} while (0)
#endif

static int yylex( void );
static void yyerror(const char *s );
static void yyerror(t_lex_file_posn err_file_posn, const char *s );
static int yyparse( void );

static c_cyclicity *cyc;
static t_lex_file_posn *last_ok_file_posn;

%}

/*a Grammar declarations
 */
/*t type union
 */
%union {
    struct t_terminal_entry *terminal; // For single or two-character tokens
    struct t_string *string;			// For quoted strings
    struct t_symbol *symbol;			// For user-defined symbols
    struct t_sized_int *sized_int;		// For a sized or unsized integer
    class c_co_token_union *symbol_list;
    class c_co_token_union *token_union;
    class c_co_sized_int_pair *sized_int_pair;
    class c_co_expression *expr;
    class c_co_case_entry *case_entry;
    class c_co_declspec *declspec;
    class c_co_lvar *lvar;
    class c_co_code_label *code_label;
    class c_co_type_specifier *type_specifier;		// For a type specifier
    class c_co_type_definition *type_definition;
    class c_co_enum_definition *enum_definition;
    class c_co_statement *statement;
    class c_co_enum_identifier *enum_identifier;			// For an enumeration
    class c_co_clock_reset_defn *clock_spec, *reset_spec;
    class c_co_nested_assignment *nested_assignment;
    class c_co_named_expression *named_expression;
    class c_co_signal_declaration *signal_declaration;
    class c_co_module_body_entry *module_body_entry;
    class c_co_module_prototype_body_entry *module_prototype_body_entry;
    class c_co_timing_spec *timing_spec;
    class c_co_port_map *port_map;
    class c_co_instantiation *instantiation;
    class c_co_module *module;
    class c_co_module_prototype *module_prototype;
    class c_co_schematic_symbol *schematic_symbol;
    class c_co_schematic_symbol_body_entry *schematic_symbol_body_entry;
    class c_co_fsm_state *fsm_definition;
    class c_co_type_struct *struct_definition;
    class c_co_constant_declaration *constant_declaration;
    class c_co_toplevel *toplevel;
    int value;
}

/*t Types of symbols and grammar statements
 */  
%token <symbol> TOKEN_TEXT_PRECLOCK
%token <symbol> TOKEN_TEXT_INCLUDE
%token <symbol> TOKEN_TEXT_REGISTER
%token <symbol> TOKEN_TEXT_ASSERT
%token <symbol> TOKEN_TEXT_COVER
%token <symbol> TOKEN_TEXT_TYPEDEF
%token <symbol> TOKEN_TEXT_BIT
%token <symbol> TOKEN_TEXT_INTEGER
%token <symbol> TOKEN_TEXT_STRING
%token <symbol> TOKEN_TEXT_ENUM
%token <symbol> TOKEN_TEXT_EXTERN
%token <symbol> TOKEN_TEXT_TIMING
%token <symbol> TOKEN_TEXT_TO
%token <symbol> TOKEN_TEXT_FROM
%token <symbol> TOKEN_TEXT_MODULE
%token <symbol> TOKEN_TEXT_INPUT
%token <symbol> TOKEN_TEXT_OUTPUT
%token <symbol> TOKEN_TEXT_PARAMETER
%token <symbol> TOKEN_TEXT_BUNDLE
%token <symbol> TOKEN_TEXT_DEFAULT
%token <symbol> TOKEN_TEXT_CLOCK
%token <symbol> TOKEN_TEXT_RISING
%token <symbol> TOKEN_TEXT_FALLING
%token <symbol> TOKEN_TEXT_GATED_CLOCK
%token <symbol> TOKEN_TEXT_DECL_ASYNC_READ
%token <symbol> TOKEN_TEXT_DECL_APPROVED
%token <symbol> TOKEN_TEXT_RESET
%token <symbol> TOKEN_TEXT_ACTIVE_LOW
%token <symbol> TOKEN_TEXT_ACTIVE_HIGH
%token <symbol> TOKEN_TEXT_CLOCKED
%token <symbol> TOKEN_TEXT_COMB
%token <symbol> TOKEN_TEXT_NET
%token <symbol> TOKEN_TEXT_FOR
%token <symbol> TOKEN_TEXT_IF
%token <symbol> TOKEN_TEXT_ELSIF
%token <symbol> TOKEN_TEXT_ELSE
%token <symbol> TOKEN_TEXT_FULLSWITCH
%token <symbol> TOKEN_TEXT_PARTSWITCH
%token <symbol> TOKEN_TEXT_PRIORITY
%token <symbol> TOKEN_TEXT_CASE
%token <symbol> TOKEN_TEXT_BREAK
%token <symbol> TOKEN_TEXT_FSM
%token <symbol> TOKEN_TEXT_STRUCT
%token <symbol> TOKEN_TEXT_ONE_HOT
%token <symbol> TOKEN_TEXT_ONE_COLD
%token <symbol> TOKEN_TEXT_SIZEOF
%token <symbol> TOKEN_TEXT_LOG
%token <symbol> TOKEN_TEXT_PRINT
%token <symbol> TOKEN_TEXT_CONFIG
%token <symbol> TOKEN_TEXT_REPEAT
%token <symbol> TOKEN_TEXT_WHILE
%token <symbol> TOKEN_TEXT_GENERATE
%token <symbol> TOKEN_TEXT_MATCHES
%token <string> TOKEN_STRING
%token <string> TOKEN_TEXT_CONSTANT
%token TOKEN_EOF

%token <sized_int> TOKEN_SIZED_INT
%token <symbol> TOKEN_USER_ID

%type <string> optional_documentation
%type <symbol_list> userid_list userid_list_entry
%type <symbol> port_direction assert_or_cover
%type <type_specifier> type_specifier base_type
%type <type_definition> type_definition
%type <module> module
%type <module_prototype> module_prototype
%type <timing_spec> timing_spec
%type <enum_definition> enum_definition
%type <enum_identifier> enum_definition_entry enum_definition_list
%type <fsm_definition> fsm_state_list fsm_state_entry fsm_definition
%type <struct_definition> struct_definition struct_entry_list struct_entry
%type <expr> expression expression_list optional_expression_list
%type <lvar> lvar
%type <statement> assignment_statement assert_statement log_statement print_statement switch_statement if_statement elsif_statement for_statement
%type <statement> statement statement_list
%type <case_entry> case_entries case_entry
%type <signal_declaration> comb_variable clocked_variable net_variable gated_clock_defn
%type <named_expression> log_argument_list named_expression
%type <declspec> clocked_decl_spec
%type <reset_spec> var_reset_spec reset_spec reset_defn
%type <clock_spec> var_clock_spec clock_spec clock_defn
%type <nested_assignment> nested_assignment nested_assignment_bracketed_list nested_assignment_list nested_assignments
%type <code_label> code_label
%type <module_prototype_body_entry> module_prototype_body_entry module_prototype_body_list bracketed_module_prototype_body_list
%type <module_body_entry> module_body_entry module_body_list bracketed_module_body_list
%type <signal_declaration> port port_list bracketed_port_list
%type <port_map> port_map port_map_list bracketed_port_map_list
%type <instantiation> instantiation
//%type <schematic_symbol> schematic_symbol
//%type <schematic_symbol_body_entry> schematic_symbol_body_list schematic_symbol_body_entry
%type <constant_declaration> constant_declaration
%type <toplevel> toplevel toplevel_list

%type <terminal> ';' '{' '}' ']' '[' '(' ')' ':' '.' '!' '~' '-' '*'

%token_table

/*t Precedence specification - matches C as of Feb 08
 */
%left '?' ':'
%left "||"
%left "&&"
%left '|'
%left '^'
%left '&'
%left "==" "!="
%left "<=" ">=" '<' '>'
%left "<<" ">>"
%left '-' '+'
%left '*' '/' '%'
%right NEG     /* negation--unary minus */

/*a Grammar definition
 */
/*f Header
 */  
%%

/*g File and toplevel
 */
file:
     toplevel_list TOKEN_EOF
    {
        cyc->set_toplevel_list( $1 );
        YYACCEPT;
    }
| error TOKEN_EOF
    {
        if (last_ok_file_posn)
        {
            yyerror( *last_ok_file_posn, "Undiscovered toplevel error");
        }
        else
        {
            yyerror("Undiscovered toplevel error");
        };
        YYACCEPT;
    }
;
     
toplevel_list:
/* empty */
    {
        last_ok_file_posn = NULL;
        $$=NULL;
    }
|
toplevel_list toplevel
    {
         if ($1)
         {
              $$ = $1->chain_tail($2);
              if ($2)
                  last_ok_file_posn = &($2->end_posn);
         }
         else
         {
              $$=$2;
              if ($2)
                  last_ok_file_posn = &($2->end_posn);
         }
    }
;

toplevel:
type_definition
    {
        if ($1)
        {
            $$ = new c_co_toplevel($1); 
            $$->co_set_file_bound( $1, $1 );
        }
        else
        {
            $$=NULL;
        }
    }
|
module
    {
        if ($1)
        {
            $$ = new c_co_toplevel($1);
            $$->co_set_file_bound( $1, $1 );
        }
        else
        {
            $$=NULL;
        }
    }
|
module_prototype
    {
        if ($1)
        {
            $$ = new c_co_toplevel($1);
            $$->co_set_file_bound( $1, $1 );
        }
        else
        {
            $$=NULL;
        }
    }
|
constant_declaration
    {
        if ($1)
        {
            $$ = new c_co_toplevel($1);
            $$->co_set_file_bound( $1, $1 );
        }
        else
        {
            $$=NULL;
        }
    }
;

/*g Type definition - enums, types, fsms
 */
type_definition:
TOKEN_TEXT_TYPEDEF enum_definition TOKEN_USER_ID ';'
    {
        if ($2)
        {
            $$ = new c_co_type_definition( $3, $2 );
            $$->co_link_symbol_list( co_compile_stage_parse, $1, $3, NULL );
            $$->co_set_file_bound( $1->file_posn, $4 );
            YYLOG("Completed typedef enum\n");
        }
        else
        {
            $$=NULL;
        }
    }
|
TOKEN_TEXT_TYPEDEF fsm_definition TOKEN_USER_ID ';'
    {
        if ($2)
        {
            $$ = new c_co_type_definition( $3, $2 );
            $$->co_link_symbol_list( co_compile_stage_parse, $1, $3, NULL );
            $$->co_set_file_bound( $1->file_posn, $4 );
            YYLOG("Completed typedef fsm\n");
        }
        else
        {
            $$=NULL;
        }
    }
|
TOKEN_TEXT_TYPEDEF type_specifier TOKEN_USER_ID ';'
    {
        if ($2)
        {
            $$ = new c_co_type_definition( $3, $2 );
            $$->co_link_symbol_list( co_compile_stage_parse, $1, $3, NULL );
            $$->co_set_file_bound( $1->file_posn, $4 );
            YYLOG("Completed typedef type\n");
        }
        else
        {
            $$=NULL;
        }
    }
|
TOKEN_TEXT_TYPEDEF struct_definition TOKEN_USER_ID ';'
    {
        if ($2)
        {
            $$ = new c_co_type_definition( $3, $2 );
            $$->co_link_symbol_list( co_compile_stage_parse, $1, $3, NULL );
            $$->co_set_file_bound( $1->file_posn, $4 );
            YYLOG("Completed typedef struct\n");
        }
        else
        {
            $$=NULL;
        }
    }
|
TOKEN_TEXT_TYPEDEF error ';'
    {
         YYLOG("Error in typedef\n");
         yyerror($1->file_posn, "Syntax error in typedef definition: typedef struct/enum/fsm/type");
         yyerrok;
         $$=NULL;
    }
|
TOKEN_TEXT_TYPEDEF error TOKEN_EOF
    {
         YYLOG("Error in typedef EOF\n");
         yyerror($1->file_posn, "Syntax error in typedef definition - end of file discovered");
         yyerrok;
         $$=NULL;
    }
;

struct_definition:
TOKEN_TEXT_STRUCT '{' struct_entry_list '}'
    {
         $$ = $3;
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $4 );
         }
    }
;

struct_entry_list:
struct_entry 
    {
         $$=$1;
    }
|
struct_entry_list struct_entry
    {
        if ($1)
        {
            $$=$1->chain_tail($2);
        }
        else
        {
            $$=NULL;
        }
    }
;

struct_entry:
type_specifier TOKEN_USER_ID optional_documentation ';'
    {
        if ($1)
        {
            $$ = new c_co_type_struct( $2, $1, $3 );
            $$->co_link_symbol_list( co_compile_stage_parse, $2, NULL );
            $$->co_set_file_bound( $1, $4 );
        }
        else
        {
            $$=NULL;
        }
    }
|
error
{
    yyerror("Syntax error in structure element: <type> <name> [<documentation>] ;");
    $$=NULL;
}
;

fsm_definition:
TOKEN_TEXT_FSM '{' fsm_state_list '}'
    {
         $$ = $3;
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $4 );
         }
    }
| TOKEN_TEXT_ONE_HOT TOKEN_TEXT_FSM '{' fsm_state_list '}'
    {
         $$ = $4;
         if ($$)
         {
             $$->set_style( fsm_style_one_hot );
             $$->co_link_symbol_list( co_compile_stage_parse, $2, NULL );
             $$->co_set_file_bound( $1->file_posn, $5 );
         }
    }
| TOKEN_TEXT_ONE_COLD TOKEN_TEXT_FSM '{' fsm_state_list '}'
    {
         $$ = $4;
         if ($$)
         {
             $$->set_style( fsm_style_one_cold );
             $$->co_link_symbol_list( co_compile_stage_parse, $2, NULL );
             $$->co_set_file_bound( $1->file_posn, $5 );
         }
    }
;

fsm_state_list:
fsm_state_entry
    {
        $$=$1;
    }
|
fsm_state_list fsm_state_entry
    {
        if ($1)
        {
            $$=$1->chain_tail($2);
        }
        else
        {
            $$=NULL;
        }
    }
;

fsm_state_entry:
TOKEN_USER_ID optional_documentation ';'
    {
         $$ = new c_co_fsm_state( $1, NULL, NULL, NULL, $2 );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         if ($2)
              $$->co_set_file_bound( $1->file_posn, $2->file_posn );
         else
              $$->co_set_file_bound( $1->file_posn, $1->file_posn );
    }
|
TOKEN_USER_ID '=' expression optional_documentation ';'
    {
         $$ = new c_co_fsm_state( $1, $3, NULL, NULL, $4 );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         if ($4)
              $$->co_set_file_bound( $1->file_posn, $4->file_posn );
         else
              $$->co_set_file_bound( $1->file_posn, $3 );
    }
|
TOKEN_USER_ID '{' userid_list '}' optional_documentation ';'
    {
         $$ = new c_co_fsm_state( $1, NULL, $3, NULL, $5 );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         if ($5)
              $$->co_set_file_bound( $1->file_posn, $5->file_posn );
         else
              $$->co_set_file_bound( $1->file_posn, $3 );
    }
|
TOKEN_USER_ID '{' userid_list '}' '=' expression optional_documentation ';'
    {
         $$ = new c_co_fsm_state( $1, $6, $3, NULL, $7 );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         if ($7)
              $$->co_set_file_bound( $1->file_posn, $7->file_posn );
         else
              $$->co_set_file_bound( $1->file_posn, $6 );
    }
|
TOKEN_USER_ID '{' userid_list '}' '{' userid_list '}' optional_documentation ';'
    {
         $$ = new c_co_fsm_state( $1, NULL, $3, $6, $8 );
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             if ($8)
                 $$->co_set_file_bound( $1->file_posn, $8->file_posn );
             else
                 $$->co_set_file_bound( $1->file_posn, $6 );
         }
    }
|
TOKEN_USER_ID '{' userid_list '}' '{' userid_list '}' '=' expression optional_documentation ';'
    {
         $$ = new c_co_fsm_state( $1, $9, $3, $6, $10 );
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             if ($10)
                 $$->co_set_file_bound( $1->file_posn, $10->file_posn );
             else
                 $$->co_set_file_bound( $1->file_posn, $9 );
         }
    }
| error
{
    yyerror("Syntax error in FSM state: <name> [{ <state_names>,* }] [{ <state_names>,* }] [= <expression>] [<documentation>] ;");
    $$=NULL;
}
;

enum_definition:
TOKEN_TEXT_ENUM '[' expression ']' '{' enum_definition_list '}'
    {
         $$ = new c_co_enum_definition($6,$3,NULL);
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $7 );
         }
    }
|
TOKEN_TEXT_ENUM '[' expression ']' '{' enum_definition_list ',' '}'
    {
         $$ = new c_co_enum_definition($6,$3,NULL);
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $8 );
         }
    }
| error
{
    yyerror("Syntax error in typedef enumeration: enum [<n>] { <enums> }");
    $$=NULL;
}
;

enum_definition_list:
enum_definition_entry
    {
         $$=$1;
    }
|
enum_definition_list ',' enum_definition_entry
    {
        if ($1)
        {
            $$ = $1->chain_tail($3);
        }
        else
        {
            $$=NULL;
        }
    }
;

enum_definition_entry:
TOKEN_USER_ID optional_documentation
    {
         $$ = new c_co_enum_identifier( $1, NULL );
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $1->file_posn );
         }
    }
|
TOKEN_USER_ID '=' expression optional_documentation
    {
         $$ = new c_co_enum_identifier( $1, $3 );
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $3 );
         }
    }
| error
{
    yyerror("Syntax error in enumeration definition: <name> [ = <expression> ]");
    $$=NULL;
}
;

userid_list:
userid_list_entry
    {
         $$=$1;
    }
|
userid_list ',' userid_list_entry
    {
        if ($1)
        {
            $$ = $1->chain_tail( $3 );
        }
        else
        {
            $$=NULL;
        }
    }
;

userid_list_entry:
TOKEN_USER_ID
    {
         $$ = new c_co_token_union($1);
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $1->file_posn );
         }
    }
;

/*g Constant declaration
 */
constant_declaration:
TOKEN_TEXT_CONSTANT type_specifier TOKEN_USER_ID '=' expression optional_documentation ';'
    {
         $$ = new c_co_constant_declaration( $3, $2, $5, $6 );
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $7 );
             YYLOG("Completed constant_declaration\n");
         }
    }
|
TOKEN_TEXT_CONSTANT error ';'
    {
         yyerror($1->file_posn, "Syntax error in constant declaration: constant <type> <name> = <expression> [<documentation>] ;");
         yyerrok;
         $$=NULL;
    }
|
TOKEN_TEXT_CONSTANT error TOKEN_EOF
    {
         yyerror($1->file_posn, "Syntax error in constant declaration: constant <type> <name> = <expression> [<documentation>] ;");
         yyerrok;
         $$=NULL;
    }
;

/*g Type specifiers
 */
type_specifier:
TOKEN_USER_ID
    {
        $$ = new c_co_type_specifier( cyc, $1 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $1->file_posn );
    }
|
base_type
    {
         $$=$1;
    }
|
type_specifier '[' expression ']'
    {
         $$ = new c_co_type_specifier( $1, $3, NULL );
         $$->co_set_file_bound( $1, $3 );
    }
;

base_type:
TOKEN_TEXT_BIT
    {
        $$ = new c_co_type_specifier( cyc, $1 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $1->file_posn );
    }
|
TOKEN_TEXT_INTEGER
    {
        $$ = new c_co_type_specifier( cyc, $1 );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         $$->co_set_file_bound( $1->file_posn, $1->file_posn );
    }
|
TOKEN_TEXT_STRING
    {
        $$ = new c_co_type_specifier( cyc, $1 );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         $$->co_set_file_bound( $1->file_posn, $1->file_posn );
    }
;

/*g Module and prototype
 */
    /*g Module
     */
module:
TOKEN_TEXT_MODULE TOKEN_USER_ID bracketed_port_list optional_documentation bracketed_module_body_list
    {
        $$ = new c_co_module( $2, $3, $5, $4 );
        if ($$)
        {
            $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
            $$->co_set_file_bound( $1->file_posn, $5 );
        }
    }
|
TOKEN_TEXT_MODULE error '}'
    {
        yyerror($1->file_posn, "Syntax error in module definition: module <name> ( <ports>,* ) [<documentation>] { <module body entries> }");
        yyerrok;
        $$=NULL;
    }
|
TOKEN_TEXT_MODULE error TOKEN_EOF
    {
        yyerror($1->file_posn, "Syntax error in module definition: module <name> ( <ports>,* ) [<documentation>] { <module body entries> }");
        yyerrok;
        $$=NULL;
    }
;

    /*g Module prototype
     */
module_prototype:
TOKEN_TEXT_EXTERN TOKEN_TEXT_MODULE TOKEN_USER_ID bracketed_port_list optional_documentation bracketed_module_prototype_body_list
    {
        $$ = new c_co_module_prototype( $3, $4, $6, $5 );
        if ($$)
        {
            $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, NULL );
            $$->co_set_file_bound( $1->file_posn, $6 );
            YYLOG("Completed module_prototype\n");
        }
    }
|
TOKEN_TEXT_EXTERN TOKEN_TEXT_MODULE error '}'
    {
        yyerror($1->file_posn, "Syntax error in module prototype: extern module <name> ( <ports>,* ) [<documentation>] { <module prototype body entries> }");
        yyerrok;
        $$=NULL;
    }
|
TOKEN_TEXT_EXTERN TOKEN_TEXT_MODULE error TOKEN_EOF
    {
        yyerror($1->file_posn, "Syntax error in module prototype: extern module <name> ( <ports>,* ) [<documentation>] { <module prototype body entries> }");
        yyerrok;
        $$=NULL;
    }
;

    /*g bracketed port list
     */
bracketed_port_list:
'(' ')'
    {
        $$=NULL;
        YYLOG("Completed empty bracketed_port_list\n");
    }
|
'(' port_list ')'
    {
        $$=$2;
        YYLOG("Completed bracketed_port_list\n");
    }
;

    /*g port list
     */
port_list:
port
    {
        $$=$1;
        YYLOG("Tail of port list\n");
    }
|
port_list ',' port
    {
        if ($3)
        {
            $$ = $1?$1->chain_tail( $3 ):$1;
            YYLOG("Chained port list\n");
        }
        else
        {
            $$=$1;
        }
    }
|
error ','
    {
        yyerror("Syntax error in module definition port list");
        yyerrok;
        $$=NULL;
    }
|
error TOKEN_EOF
    {
        yyerror("Syntax error in module definition port list - end of file discovered");
        yyerrok;
        $$=NULL;
    }
;

    /*g port
     */
port:
TOKEN_TEXT_CLOCK TOKEN_USER_ID
    {
         $$ = new c_co_signal_declaration( $2, NULL );
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $2->file_posn );
             YYLOG("Clock port specified without documentation\n");
         }
    }
| 
TOKEN_TEXT_CLOCK TOKEN_USER_ID TOKEN_STRING
    {
         $$ = new c_co_signal_declaration( $2, $3 );
         if ($$)
         {
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
             $$->co_set_file_bound( $1->file_posn, $3->file_posn );
             YYLOG("Clock port specified with documentation\n");
         }
    }
|
port_direction type_specifier TOKEN_USER_ID
    {
         $$ = new c_co_signal_declaration( $3, $1, $2, NULL );
         if ($$)
         {
             $$->co_set_file_bound( $1->file_posn, $3->file_posn );
             YYLOG("Port specified without documentation\n");
         }
    }
|
port_direction type_specifier TOKEN_USER_ID TOKEN_STRING
    {
         $$ = new c_co_signal_declaration( $3, $1, $2, $4 );
         if ($$)
         {
             $$->co_set_file_bound( $1->file_posn, $4->file_posn );
             YYLOG("Port specified with documentation\n");
         }
    }
|
TOKEN_TEXT_PARAMETER type_specifier TOKEN_USER_ID optional_documentation
    {
         $$ = new c_co_signal_declaration( signal_declaration_type_parameter, $3, $2, $4 );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         if ($4)
         {
              $$->co_set_file_bound( $1->file_posn, $4->file_posn );
         }
         else
         {
              $$->co_set_file_bound( $1->file_posn, $3->file_posn );
         }
    }
|
TOKEN_TEXT_CLOCK error
    {
        yyerror("Syntax error in port: clock <name> [<documentation>]");
        yyerrok;
        $$=NULL;
    }
|
port_direction error
    {
        yyerror("Syntax error in port: input|output <type> <name> [<documentation>]");
        yyerrok;
        $$=NULL;
    }
|
error TOKEN_USER_ID
    {
        yyerror("Syntax error in port: ((clock <name>) | (input|output <type> <name>)) [<documentation>]");
        yyerrok; 
        $$=NULL;
    }
;

    /*g port_direction
     */
port_direction:
TOKEN_TEXT_INPUT
    {
        $$=$1;
    }
| TOKEN_TEXT_OUTPUT
    {
        $$=$1;
    }
;

    /*g bracketed module body list
      module_body_list is error free, as module_body is error free
     */
bracketed_module_body_list:
'{' module_body_list '}'
    {
        $$=$2;
        YYLOG("Completed bracketed module body list\n");
    }
| '{' module_body_list TOKEN_EOF
    {
        YYLOG("EOF in bracketed module body list\n");
        if ($2)
        {
            yyerror($2->start_posn, "Syntax error in module body: module <name> ( <ports> ) { <module body entry> ; * }");
            yyerrok;
        }
        $$=NULL;
    }
;

    /*g bracketed module prototype body list
     */
bracketed_module_prototype_body_list:
'{' module_prototype_body_list '}'
    {
        $$=$2;
        YYLOG("Completed module prototype body list\n");
    }
| error '}'
    {
        yyerror("Syntax error in module prototype body: extern module <name> ( <ports> ) { <module prototype body entry> ; * }");
        yyerrok;
        $$=NULL;
    }
;

    /*g Module prototype body
     */
module_prototype_body_list:
 /*Empty*/
    {
         $$ = NULL;
         YYLOG("Empty module prototype body entry\n");
    }
|
module_prototype_body_list module_prototype_body_entry
    {
         if ($1)
         { 
              if ($2)
              {
                   $$=$1->chain_tail( $2 );
                   YYLOG("Chained module prototype body entry\n");
              }
              else
              {
                   $$=$1;
                   YYLOG("Unexpected use of module prototype body entry\n");
              }
         }
         else
         {
              $$=$2;
              YYLOG("Chained last module prototype body entry\n");
         }
    }
;

module_prototype_body_entry:
timing_spec
    {
        $$ = new c_co_module_prototype_body_entry( $1 );
        $$->co_set_file_bound( $1, $1 );
    }
|
 error ';'
    {
        yyerror("Syntax error in module prototype body line - expected a timing specification - timing <from|to|comb> ...");
        yyerrok;
        $$=NULL;
    }
;

    /*g timing_sepc
     */

timing_spec:
TOKEN_TEXT_TIMING TOKEN_TEXT_TO TOKEN_TEXT_RISING TOKEN_TEXT_CLOCK TOKEN_USER_ID userid_list ';'
{
    $$ = new c_co_timing_spec( timing_spec_type_to_clock, timing_spec_clock_edge_rising, $5, $6 );
    $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, $3, $4, NULL );
    $$->co_set_file_bound( $1->file_posn, $7 );
    YYLOG("Added to rising clock input timing\n");
}
|
 TOKEN_TEXT_TIMING TOKEN_TEXT_FROM TOKEN_TEXT_RISING TOKEN_TEXT_CLOCK TOKEN_USER_ID userid_list ';'
{
    $$ = new c_co_timing_spec( timing_spec_type_from_clock, timing_spec_clock_edge_rising, $5, $6 );
    $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, $3, $4, NULL );
    $$->co_set_file_bound( $1->file_posn, $7 );
    YYLOG("Added from rising clock input timing\n");
}
|
TOKEN_TEXT_TIMING TOKEN_TEXT_TO TOKEN_TEXT_FALLING TOKEN_TEXT_CLOCK TOKEN_USER_ID userid_list ';'
{
    $$ = new c_co_timing_spec( timing_spec_type_to_clock, timing_spec_clock_edge_falling, $5, $6 );
    $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, $3, $4, NULL );
    $$->co_set_file_bound( $1->file_posn, $7 );
    YYLOG("Added to falling clock input timing\n");
}
|
 TOKEN_TEXT_TIMING TOKEN_TEXT_FROM TOKEN_TEXT_FALLING TOKEN_TEXT_CLOCK TOKEN_USER_ID userid_list ';'
{
    $$ = new c_co_timing_spec( timing_spec_type_from_clock, timing_spec_clock_edge_falling, $5, $6 );
    $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, $3, $4, NULL );
    $$->co_set_file_bound( $1->file_posn, $7 );
    YYLOG("Added from falling clock output timing\n");
}
|
 TOKEN_TEXT_TIMING TOKEN_TEXT_COMB TOKEN_TEXT_INPUT userid_list ';'
{
    $$ = new c_co_timing_spec( timing_spec_type_comb_inputs, $4 );
    $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, $3, NULL );
    $$->co_set_file_bound( $1->file_posn, $5 );
    YYLOG("Added comb input timing\n");
}
|
 TOKEN_TEXT_TIMING TOKEN_TEXT_COMB TOKEN_TEXT_OUTPUT userid_list ';'
{
    $$ = new c_co_timing_spec( timing_spec_type_comb_outputs, $4 );
    $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, $3, NULL );
    $$->co_set_file_bound( $1->file_posn, $5 );
    YYLOG("Added comb output timing\n");
}
| TOKEN_TEXT_TIMING error
{
    yyerror("Syntax error in module prototype body line - expected a timing specification - timing <from|to|comb> ...");
    $$=NULL;
}
 ;

    /*g Module body
     */
module_body_list:
 /*Empty*/
    {
        $$ = NULL;
        YYLOG("End of module body list\n");
    }
|
module_body_list module_body_entry
    {
         if ($1)
         { 
              if ($2)
              {
                   $$=$1->chain_tail( $2 );
                   YYLOG("Chaining to module body list\n");
              }
              else
              {
                  YYLOG("Unexpected chaining to end of module body list\n");
                   $$=$1;
              }
         }
         else
         {
             YYLOG("Completed module body list\n");
              $$=$2;
         }
    }
;

module_body_entry:
  clock_defn
    {
        if ($1)
        {
            $$ = new c_co_module_body_entry( 1, $1 );
            $$->co_set_file_bound( $1, $1 );
            YYLOG("Added clock defn\n");
        }
        else
        {
            $$=NULL;
        }
    }
|
  reset_defn
    {
        if ($1)
        {
            $$ = new c_co_module_body_entry( 0, $1 );
            $$->co_set_file_bound( $1, $1 );
            YYLOG("Added reset defn\n");
        }
        else
        {
            $$=NULL;
        }
    }
| 
  clocked_variable
    {
        if ($1)
        {
            $$ = new c_co_module_body_entry( $1 );
            $$->co_set_file_bound( $1, $1 );
            YYLOG("Added clocked var\n");
        }
        else
        {
            $$=NULL;
        }
    }
| 
  gated_clock_defn
    {
        if ($1)
        {
            $$ = new c_co_module_body_entry( $1 );
            $$->co_set_file_bound( $1, $1 );
            YYLOG("Added gated_clock\n");
        }
        else
        {
            $$=NULL;
        }
    }
|
  comb_variable
    {
        if ($1)
        {
            $$ = new c_co_module_body_entry( $1 );
            $$->co_set_file_bound( $1, $1 );
            YYLOG("Added comb var\n");
        }
        else
        {
            $$=NULL;
        }
    }
|
  net_variable
    {
        if ($1)
        {
            $$ = new c_co_module_body_entry( $1 );
            $$->co_set_file_bound( $1, $1 );
            YYLOG("Added net var\n");
        }
        else
        {
            $$=NULL;
        }
    }
|
  code_label
    {
        if ($1)
        {
            $$ = new c_co_module_body_entry( $1 );
            $$->co_set_file_bound( $1, $1 );
            YYLOG("Added code_label\n");
        }
        else
        {
            $$=NULL;
        }
    }
|
  TOKEN_TEXT_TIMING error ';'
    {
        yyerror("Unexpected 'timing' clause in a module - should it be in a module prototype - in line"); 
        yyerrok;
        $$=NULL;
    }
|
  TOKEN_USER_ID error
    {
        yyerror($1->file_posn,"Syntax error in module body -  code block is <name> [<documentation>] : { <statements> * }"); 
        yyerrok;
        $$=NULL;
    }
|
  error ';'
    {
        yyerror("Syntax error in module body - expected variable declaration (clocked|comb|net - clocked/comb may be assert|cover first), default clock/reset, or code block"); 
        yyerrok;
        $$=NULL;
    }
|
  error
    {
        yyerror("Syntax error in module body - expected variable declaration (clocked|comb|net - clocked/comb may be assert|cover first), default clock/reset, or code block"); 
        $$=NULL;
    }
;

    /*g Schematic symbols
     */

/*g code_label
 */
code_label:
TOKEN_USER_ID optional_documentation ':' '{' statement_list '}' optional_documentation
    {
        $$=new c_co_code_label( $1, $2, $5 );
        $$->co_set_file_bound( $1->file_posn, $6 );
        YYLOG("Completed labeled code block\n");
    }
;

/*g Optional documentation
 */
optional_documentation:
 /*Empty*/
    {
        $$=NULL;
    }
|
    TOKEN_STRING
    {
        $$=$1;
        YYLOG("Documentation supplied\n");
    }
;

/*g Instantiation and port map
 */
instantiation:
TOKEN_USER_ID TOKEN_USER_ID bracketed_port_map_list ';'
    {
        $$ = new c_co_instantiation( $1, $2, $3, NULL );
        if ($$)
        {
            $$->co_set_file_bound( $1->file_posn, $4 );
        }
    }
|
TOKEN_USER_ID TOKEN_USER_ID '[' expression ']' bracketed_port_map_list ';'
    {
        $$ = new c_co_instantiation( $1, $2, $6, $4 );
        if ($$)
        {
            $$->co_set_file_bound( $1->file_posn, $7 );
        }
    }
| TOKEN_USER_ID TOKEN_USER_ID error
    {
        yyerror($1->file_posn,"Syntax error in module instantiation: <module type> <instance name> ( <port map> ) ;");
        yyerrok;
        $$=NULL;
    }
;

bracketed_port_map_list:
'(' ')'
    {
        $$=NULL;
    }
|
'(' port_map_list ')'
    {
        $$=$2;
    }
|
'(' error
    {
        yyerror("Syntax error in port map list: ");
        yyerrok;
        $$=NULL;
    }
|
error ')'
    {
        yyerror("Syntax error in port map list");
        yyerrok;
        $$=NULL;
    }
;

port_map_list:
port_map
    {
         $$=$1;
    }
|
port_map_list ',' port_map
    {
        if ($1)
        {
            $$ = $1->chain_tail($3);
        }
        else
        {
            $$=NULL;
        }
    }
;

port_map:
TOKEN_USER_ID '=' expression /* Used for parameters only */
    {
        $$ = new c_co_port_map( $1, $3 );
        $$->co_set_file_bound( $1->file_posn, $3 );
    }
|
lvar "<=" expression /* Used for inputs only */
    {
        $$ = new c_co_port_map( $1, $3 );
        $$->co_set_file_bound( $1, $3 );
    }
|
TOKEN_USER_ID "<-" TOKEN_USER_ID /* Used for clocks only */
  {
      $$ = new c_co_port_map( $1, $3 );
      $$->co_set_file_bound( $1->file_posn, $3->file_posn );
  }
|
lvar "=>" lvar /* Used for outputs only */
    {
      $$ = new c_co_port_map( $1, $3 );
      $$->co_set_file_bound( $1, $3 );
    }
|
TOKEN_USER_ID error
    {
        yyerror($1->file_posn, "Syntax error in module port map: expected clock|input|output <name> '<-'|'='|'=>' <net name|expression>");
        yyerrok;
        $$=NULL;
    }
;

/*g Variable and clock defintions
 */
    /*g clock_defn
     */
clock_defn:
 TOKEN_TEXT_DEFAULT TOKEN_TEXT_CLOCK TOKEN_USER_ID ';'
    {
        $$ = new c_co_clock_reset_defn( clock_reset_type_default_clock, $3, clock_edge_rising );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, NULL );
        $$->co_set_file_bound( $1->file_posn, $4 );
    }
|
 TOKEN_TEXT_DEFAULT TOKEN_TEXT_CLOCK TOKEN_TEXT_RISING TOKEN_USER_ID ';'
    {
        $$ = new c_co_clock_reset_defn( clock_reset_type_default_clock, $4, clock_edge_rising );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, NULL );
        $$->co_set_file_bound( $1->file_posn, $5 );
    }
|
 TOKEN_TEXT_DEFAULT TOKEN_TEXT_CLOCK TOKEN_TEXT_FALLING TOKEN_USER_ID ';'
    {
        $$ = new c_co_clock_reset_defn( clock_reset_type_default_clock, $4, clock_edge_falling );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, NULL );
        $$->co_set_file_bound( $1->file_posn, $5 );
    }
;

    /*g gated_clock_defn
     */
gated_clock_defn:
    TOKEN_TEXT_GATED_CLOCK var_clock_spec reset_spec TOKEN_USER_ID optional_documentation ';'
    {
        $$ = new c_co_signal_declaration( $4, $2, $3, $5 );
    }
 ;

    /*g reset_defn
     */
reset_defn:
 TOKEN_TEXT_DEFAULT TOKEN_TEXT_RESET TOKEN_USER_ID ';'
    {
        $$=new c_co_clock_reset_defn( clock_reset_type_default_reset, $3, reset_active_high );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, NULL );
        $$->co_set_file_bound( $1->file_posn, $4 );
    }
|
 TOKEN_TEXT_DEFAULT TOKEN_TEXT_RESET TOKEN_TEXT_ACTIVE_LOW TOKEN_USER_ID ';'
    {
        $$=new c_co_clock_reset_defn( clock_reset_type_default_reset, $4, reset_active_low );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, $3, NULL );
        $$->co_set_file_bound( $1->file_posn, $5 );
    }
|
 TOKEN_TEXT_DEFAULT TOKEN_TEXT_RESET TOKEN_TEXT_ACTIVE_HIGH TOKEN_USER_ID ';'
    {
        $$=new c_co_clock_reset_defn( clock_reset_type_default_reset, $4, reset_active_high );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, $3, NULL );
        $$->co_set_file_bound( $1->file_posn, $5 );
    }
;

    /*g clocked_variable, comb_variable, net_variable
     */
clocked_variable:
TOKEN_TEXT_CLOCKED var_clock_spec var_reset_spec clocked_decl_spec type_specifier TOKEN_USER_ID '=' nested_assignment_list optional_documentation ';'
    {
        $$ = new c_co_signal_declaration( $6, $5, signal_usage_type_rtl, $2, $3, $8, $4, $9 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $10 );
    }
|
TOKEN_TEXT_ASSERT TOKEN_TEXT_CLOCKED var_clock_spec var_reset_spec type_specifier TOKEN_USER_ID '=' nested_assignment_list optional_documentation ';'
    {
        $$ = new c_co_signal_declaration( $6, $5, signal_usage_type_assert, $3, $4, $8, NULL, $9 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $10 );
    }
|
TOKEN_TEXT_COVER TOKEN_TEXT_CLOCKED var_clock_spec var_reset_spec type_specifier TOKEN_USER_ID '=' nested_assignment_list optional_documentation ';'
    {
        $$ = new c_co_signal_declaration( $6, $5, signal_usage_type_cover, $3, $4, $8, NULL, $9 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $10 );
    }
| TOKEN_TEXT_CLOCKED error ';' { yyerror( $1->file_posn, "Syntax error in clocked signal declaration"); yyerrok; $$=NULL;}
| TOKEN_TEXT_ASSERT TOKEN_TEXT_CLOCKED error ';' { yyerror( $1->file_posn, "Syntax error in assertion clocked signal declaration"); yyerrok; $$=NULL;}
| TOKEN_TEXT_COVER TOKEN_TEXT_CLOCKED error ';' { yyerror( $1->file_posn, "Syntax error in coverage clocked signal declaration"); yyerrok; $$=NULL;}
;

comb_variable:
TOKEN_TEXT_COMB type_specifier TOKEN_USER_ID optional_documentation ';'
    {
        $$ = new c_co_signal_declaration( $3, $2, signal_usage_type_rtl, $4 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $5 );
    }
|
TOKEN_TEXT_ASSERT TOKEN_TEXT_COMB type_specifier TOKEN_USER_ID optional_documentation ';'
    {
        $$ = new c_co_signal_declaration( $4, $3, signal_usage_type_assert, $5 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $6 );
    }
|
TOKEN_TEXT_COVER TOKEN_TEXT_COMB type_specifier TOKEN_USER_ID optional_documentation ';'
    {
        $$ = new c_co_signal_declaration( $4, $3, signal_usage_type_cover, $5 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $6 );
    }
| TOKEN_TEXT_COMB error ';' { yyerror( $1->file_posn, "Syntax error in combinatorial signal declaration"); yyerrok; $$=NULL;}
| TOKEN_TEXT_ASSERT TOKEN_TEXT_COMB error ';' { yyerror( $1->file_posn, "Syntax error in assertion combinatorial signal declaration"); yyerrok; $$=NULL;}
| TOKEN_TEXT_COVER TOKEN_TEXT_COMB error ';' { yyerror( $1->file_posn, "Syntax error in coverage combinatorial signal declaration"); yyerrok; $$=NULL;}
;

net_variable:
TOKEN_TEXT_NET type_specifier TOKEN_USER_ID optional_documentation ';'
    {
         $$ = new c_co_signal_declaration( signal_declaration_type_net_local, $3, $2, $4 );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         $$->co_set_file_bound( $1->file_posn, $5 );
    }
| TOKEN_TEXT_NET error ';' { yyerror( $1->file_posn, "Syntax error in net signal declaration"); yyerrok; $$=NULL;}
;

    /*g nested_assignment_list
     */
nested_assignment_list:
nested_assignment_bracketed_list
    {
         $$=$1;
    }
|
expression
    {
        $$ = new c_co_nested_assignment( 0, $1 );
        $$->co_set_file_bound( $1, $1 );
    }
;

nested_assignment_bracketed_list:
'{' nested_assignments '}'
    {
         $$=$2;
    }
;

nested_assignments:
nested_assignment
    {
         $$=$1;
    }
|
nested_assignments ',' nested_assignment
    {
        if ($1)
        {
            $$ = $1->chain_tail($3);
        }
        else
        {
            $$=NULL;
        }
    }
;

nested_assignment:
nested_assignment_bracketed_list
    {
        if ($1)
        {
            $$ = new c_co_nested_assignment( $1 );
            $$->co_set_file_bound( $1, $1 );
        }
        else
        {
            $$=NULL;
        }
    }
|
TOKEN_USER_ID '=' nested_assignment_list
    {
        $$ = new c_co_nested_assignment( $1, $3 );
        if ($$)
        {
            $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
            if ($3)
                $$->co_set_file_bound( $1->file_posn, $3 );
        }
    }
|
'*' '=' expression
    {
        $$ = new c_co_nested_assignment( 1, $3 );
        if ($$)
        {
            if ($3)
                $$->co_set_file_bound( $1, $3 );
        }
    }
|
'[' ']' '=' nested_assignment_list
    {
        $$ = new c_co_nested_assignment( $4 );
        if ($$)
        {
            if ($4)
                $$->co_set_file_bound( $1, $4 );
        }
    }
|
'[' TOKEN_USER_ID ']' '=' nested_assignment_list
    {
        $$ = new c_co_nested_assignment( $5 );
        if ($$)
        {
            if ($5)
                $$->co_set_file_bound( $1, $5 );
        }
    }
|
'[' TOKEN_USER_ID ';' expression ';' expression ';' expression ']' '=' nested_assignment_list
    {
        $$ = new c_co_nested_assignment( $11 );
        if ($$)
        {
            if ($11)
                $$->co_set_file_bound( $1, $11 );
        }
    }
;


    /*g clock_spec, var_clock_spec, reset_spec, var_reset_spec
     */
clock_spec: // Should be preceeded by 'TOKEN_TEXT_CLOCK'
TOKEN_USER_ID { $$ = new c_co_clock_reset_defn( clock_reset_type_clock, $1, clock_edge_rising ); $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL ); $$->co_set_file_bound( $1->file_posn, $1->file_posn ); }
| TOKEN_TEXT_RISING TOKEN_USER_ID { $$ = new c_co_clock_reset_defn( clock_reset_type_clock, $2, clock_edge_rising ); $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL ); $$->co_set_file_bound( $1->file_posn, $2->file_posn ); }
| TOKEN_TEXT_FALLING TOKEN_USER_ID { $$ = new c_co_clock_reset_defn( clock_reset_type_clock, $2, clock_edge_falling ); $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL ); $$->co_set_file_bound( $1->file_posn, $2->file_posn ); }
;

var_clock_spec:
/* empty */ { $$=NULL; }
| TOKEN_TEXT_CLOCK clock_spec { $$ = $2; }
;

reset_spec: // Should be preceeded by 'TOKEN_TEXT_RESET'
TOKEN_USER_ID  { $$ = new c_co_clock_reset_defn( clock_reset_type_reset, $1, reset_active_high ); $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL ); $$->co_set_file_bound( $1->file_posn, $1->file_posn ); }
| TOKEN_TEXT_ACTIVE_LOW TOKEN_USER_ID  { $$ = new c_co_clock_reset_defn( clock_reset_type_reset,  $2, reset_active_low ); $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, NULL ); $$->co_set_file_bound( $1->file_posn, $2->file_posn ); }
| TOKEN_TEXT_ACTIVE_HIGH TOKEN_USER_ID  { $$ = new c_co_clock_reset_defn( clock_reset_type_reset, $2, reset_active_high ); $$->co_link_symbol_list( co_compile_stage_parse, $1, $2, NULL ); $$->co_set_file_bound( $1->file_posn, $2->file_posn ); }
;

var_reset_spec:
/* empty */ { $$ = NULL; }
| TOKEN_TEXT_RESET reset_spec  { $$ = $2; }
;

clocked_decl_spec:
/* empty */ { $$ = NULL; }
| TOKEN_TEXT_DECL_ASYNC_READ clocked_decl_spec
    {
        $$ = new c_co_declspec(declspec_type_async_read,$1);
        if ($$)
        {
            $$->chain_tail($2);
        }
    }
| TOKEN_TEXT_DECL_APPROVED clocked_decl_spec
    {
        $$ = new c_co_declspec(declspec_type_approved,$1);
        if ($$)
        {
            $$->chain_tail($2);
        }
    }
;

/*g Expression
 */
optional_expression_list:
'(' expression_list ')'
{
    $$=$2;
}
|
{
    $$=NULL;
}
;

expression_list:
expression
    {
         $$=$1;
    }
|
expression_list ',' expression
    {
        if ($1)
        {
            $$ = $1->chain_tail($3);
        }
        else
        {
            $$=NULL;
        }
    }
;

expression:
TOKEN_SIZED_INT
    {
        $$=new c_co_expression( $1 );
        $$->co_set_file_bound( $1->file_posn, $1->file_posn );
    }
| lvar
    {
        if ($1)
        {
            $$=new c_co_expression( $1 );
            $$->co_set_file_bound( $1, $1 );
        }
        else
        {
            $$=NULL;
        }
    }
| TOKEN_TEXT_SIZEOF '(' expression ')'
    {
         $$=new c_co_expression( expr_subtype_sizeof, $3 );
         if ($$)
         {
             $$->co_set_file_bound( $1->file_posn, $4 );
             $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         }
    }
|
TOKEN_TEXT_BUNDLE '(' expression_list ')'
    {
        $$=new c_co_expression( expr_type_bundle, expr_subtype_none, $3 );
        if ($3)
        {
            $$->co_set_file_bound( $1->file_posn, $3 );
        }
    }
|
expression '?' expression ':' expression
    {
         $$=new c_co_expression( expr_subtype_query, $1, $3, $5 );
         if ($5)
         {
             $$->co_set_file_bound( $1, $5 );
         }
    }
| expression '+' expression { $$=new c_co_expression( expr_subtype_arithmetic_plus, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression '-' expression { $$=new c_co_expression( expr_subtype_arithmetic_minus, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression '*' expression { $$=new c_co_expression( expr_subtype_arithmetic_multiply, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression '/' expression { $$=new c_co_expression( expr_subtype_arithmetic_divide, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression '%' expression { $$=new c_co_expression( expr_subtype_arithmetic_remainder, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression '&' expression { $$=new c_co_expression( expr_subtype_binary_and, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression '|' expression { $$=new c_co_expression( expr_subtype_binary_or, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression '^' expression { $$=new c_co_expression( expr_subtype_binary_xor, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression '<' expression { $$=new c_co_expression( expr_subtype_compare_lt, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression '>' expression { $$=new c_co_expression( expr_subtype_compare_gt, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression "||" expression { $$=new c_co_expression( expr_subtype_logical_or, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression "&&" expression { $$=new c_co_expression( expr_subtype_logical_and, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression "==" expression { $$=new c_co_expression( expr_subtype_compare_eq, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression "!=" expression { $$=new c_co_expression( expr_subtype_compare_ne, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression "<=" expression { $$=new c_co_expression( expr_subtype_compare_le, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression ">=" expression { $$=new c_co_expression( expr_subtype_compare_ge, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression "<<" expression { $$=new c_co_expression( expr_subtype_logical_shift_left, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| expression ">>" expression { $$=new c_co_expression( expr_subtype_logical_shift_right, $1, $3 ); $$->co_set_file_bound( $1, $3 ); }
| '!' expression %prec NEG { $$=new c_co_expression( expr_subtype_logical_not, $2 ); $$->co_set_file_bound( $1, $2 ); }
| '~' expression %prec NEG { $$=new c_co_expression( expr_subtype_binary_not, $2 ); $$->co_set_file_bound( $1, $2 ); }
| '-' expression %prec NEG { $$=new c_co_expression( expr_subtype_negate, $2 ); $$->co_set_file_bound( $1, $2 ); }
| '(' expression ')'        { $$=new c_co_expression( expr_subtype_expression, $2 ); $$->co_set_file_bound( $1, $3 ); }
|
TOKEN_EOF
    {
        yyerror("EOF in expression");
        yyerrok;
        $$=NULL;
        cyc->parse_repeat_eof(); 
    }
;

/*g Statements
 */
statement:
 assignment_statement { $$=$1; YYLOG("Assignment statement complete\n"); }
| for_statement { $$=$1; YYLOG("For statement complete\n"); }
| if_statement { $$=$1; YYLOG("If statement complete\n");}
| switch_statement { $$=$1; YYLOG("Switch statement complete\n"); }
| '{' statement_list '}' { $$ = new c_co_statement($2); if ($$) {$$->co_set_file_bound( $1, $3 );} YYLOG("Statement block complete\n");}
| log_statement { $$=$1; YYLOG("Log statement complete\n"); }
| print_statement { $$=$1; YYLOG("Print statement complete\n"); }
| assert_statement { $$=$1; YYLOG("Assert statement complete\n"); }
| instantiation { $$ = new c_co_statement( $1 ); if ($$ && $1) {$$->co_set_file_bound( $1, $1 );}  YYLOG("Instantiation complete\n");}
| TOKEN_EOF { YYLOG("EOF in statement\n"); yyerror("EOF when statement expected - still within last code block"); yyerrok; $$=NULL; }
| error ';' { YYLOG("Error in statement, skips to ';'\n"); yyerror("Syntax error in statement, skipping to ';'"); yyerrok; $$=NULL;}
| error TOKEN_EOF { YYLOG("EOF in statement\n"); yyerror("EOF when statement expected - still within last code block"); yyerrok; $$=NULL; }
;

statement_list:
 /*empty*/
    {
        $$=NULL;
        YYLOG("End of statement list\n");
    }
|
statement_list ';'
    {
        $$ = $1;
        YYLOG("Statement list with extra ';'\n");
    }
|
statement_list statement
    {
         if ($1)
         {
              if ($2)
              {
                   $$=$1->chain_tail( $2 );
                   YYLOG("Chaining statement list\n");
              }
              else
              {
                   $$ = $1;
                   YYLOG("Unexpected chaining in statement list\n");
              }
         }
         else
         {
              $$=$2;
              YYLOG("Chaining statement list\n");
         }
    }
;

/*g assignment
 */
assignment_statement:
  lvar '=' nested_assignment_list optional_documentation ';'
    {
        $$ = new c_co_statement( $1, $3, 0, $4 );
        if ($$)
            $$->co_set_file_bound( $1, $5 );
    }
|  lvar "|=" nested_assignment_list optional_documentation ';'
    {
        $$ = new c_co_statement( $1, $3, 2, $4 );
        if ($$)
            $$->co_set_file_bound( $1, $5 );
    }
| lvar "<=" nested_assignment_list optional_documentation ';'
    { 
        $$ = new c_co_statement( $1, $3, 1, $4 );
        if ($$)
            $$->co_set_file_bound( $1, $5 );
    }
| lvar error
    {
        if ($1)
        {
            yyerror($1->start_posn,"Error in assignment statement");
        }
        else
        {
            yyerror("Error in assignment statement at unknown location");
        }
        $$=NULL;
}
;

/*g log_statement
 */
log_statement:
TOKEN_TEXT_LOG var_clock_spec var_reset_spec '(' TOKEN_STRING ')' ';' { $$ = new c_co_statement( $2, $3, $5, NULL ); $$->co_set_file_bound( $1->file_posn, $7 ); }
|
TOKEN_TEXT_LOG var_clock_spec var_reset_spec '(' TOKEN_STRING ',' log_argument_list ')' ';' { $$ = new c_co_statement( $2, $3, $5, $7 ); $$->co_set_file_bound( $1->file_posn, $9 ); }
| TOKEN_TEXT_LOG error     { yyerror($1->file_posn,"Error in 'log' statement"); $$=NULL; }
;

log_argument_list:
named_expression
    {  $$=$1; YYLOG("End of statement list\n"); }
|
log_argument_list ',' named_expression
    {
        $$=$3;
        if ($1 && $3) { $$=$1->chain_tail( $3 ); }
        else if ($1) {$$=$1;}
    }
;

named_expression:
TOKEN_STRING ',' expression
    {
        $$ = new c_co_named_expression( $1, $3 );
    }
;

/*g print_statement
 */
print_statement:
TOKEN_TEXT_PRINT var_clock_spec var_reset_spec '(' TOKEN_STRING ')' ';' { $$ = new c_co_statement( $2, $3, statement_type_print, NULL, NULL, $5, NULL, NULL ); $$->co_set_file_bound( $1->file_posn, $7 ); }
|
TOKEN_TEXT_PRINT var_clock_spec var_reset_spec '(' TOKEN_STRING ',' expression_list ')' ';' { $$ = new c_co_statement( $2, $3, statement_type_print, NULL, NULL, $5, $7, NULL ); $$->co_set_file_bound( $1->file_posn, $9 ); }
| TOKEN_TEXT_PRINT error     { yyerror($1->file_posn,"Error in 'print' statement"); $$=NULL; }
;

/*g assert_statement - asserts or cover
 */
assert_or_cover:
TOKEN_TEXT_ASSERT {$$=$1;}
|
TOKEN_TEXT_COVER  {$$=$1;}
;

assert_statement:
assert_or_cover                                            '(' expression ',' TOKEN_STRING ')' optional_expression_list ';' { $$ = new c_co_statement( NULL, NULL, (get_symbol_type($1->lex_symbol)==TOKEN_TEXT_ASSERT)?statement_type_assert:statement_type_cover, $3, $7, $5, NULL, NULL ); $$->co_set_file_bound( $1->file_posn, $8 ); }
|
assert_or_cover TOKEN_TEXT_CLOCK clock_spec var_reset_spec '(' expression ',' TOKEN_STRING ')' optional_expression_list ';' { $$ = new c_co_statement( $3, $4, (get_symbol_type($1->lex_symbol)==TOKEN_TEXT_ASSERT)?statement_type_assert:statement_type_cover, $6, $10, $8, NULL, NULL ); $$->co_set_file_bound( $1->file_posn, $11 ); }
|
assert_or_cover TOKEN_TEXT_RESET reset_spec                '(' expression ',' TOKEN_STRING ')' optional_expression_list ';' { $$ = new c_co_statement( NULL, $3, (get_symbol_type($1->lex_symbol)==TOKEN_TEXT_ASSERT)?statement_type_assert:statement_type_cover, $5, $9, $7, NULL, NULL ); $$->co_set_file_bound( $1->file_posn, $10 ); }
|
assert_or_cover                                            '(' expression ',' TOKEN_STRING ',' expression_list ')' optional_expression_list ';' { $$ = new c_co_statement( NULL, NULL, (get_symbol_type($1->lex_symbol)==TOKEN_TEXT_ASSERT)?statement_type_assert:statement_type_cover, $3, $9, $5, $7, NULL ); $$->co_set_file_bound( $1->file_posn, $10 ); }
|
assert_or_cover TOKEN_TEXT_CLOCK clock_spec var_reset_spec '(' expression ',' TOKEN_STRING ',' expression_list ')' optional_expression_list ';' { $$ = new c_co_statement( $3, $4, (get_symbol_type($1->lex_symbol)==TOKEN_TEXT_ASSERT)?statement_type_assert:statement_type_cover, $6, $12, $8, $10, NULL ); $$->co_set_file_bound( $1->file_posn, $13 ); }
|
assert_or_cover TOKEN_TEXT_RESET reset_spec                '(' expression ',' TOKEN_STRING ',' expression_list ')' optional_expression_list ';' { $$ = new c_co_statement( NULL, $3, (get_symbol_type($1->lex_symbol)==TOKEN_TEXT_ASSERT)?statement_type_assert:statement_type_cover, $5, $11, $7, $9, NULL ); $$->co_set_file_bound( $1->file_posn, $12 ); }
|
assert_or_cover '(' expression ')' optional_expression_list '{' statement_list '}' { $$ = new c_co_statement( NULL, NULL, (get_symbol_type($1->lex_symbol)==TOKEN_TEXT_ASSERT)?statement_type_assert:statement_type_cover, $3, $5, NULL, NULL, $7 ); $$->co_set_file_bound( $1->file_posn, $8 ); }
|
assert_or_cover '{' statement_list '}' { $$ = new c_co_statement( NULL, NULL, (get_symbol_type($1->lex_symbol)==TOKEN_TEXT_ASSERT)?statement_type_assert:statement_type_cover, NULL, NULL, NULL, NULL, $3 ); $$->co_set_file_bound( $1->file_posn, $4 ); }
| TOKEN_TEXT_ASSERT error     { yyerror($1->file_posn,"Error in 'assert' statement"); $$=NULL; }
| TOKEN_TEXT_COVER error     { yyerror($1->file_posn,"Error in 'cover' statement"); $$=NULL; }
;

/*g lvar
 */
lvar:
TOKEN_USER_ID
    {
        $$=new c_co_lvar( $1 );
        $$->co_set_file_bound( $1->file_posn, $1->file_posn );
    }
| lvar '.' TOKEN_USER_ID
    {
        if ($1)
        {
            $$=new c_co_lvar( $1, $3 );
            $$->co_set_file_bound( $1, $3->file_posn );
        }
        else
        {
            $$=NULL;
        }
    }
| lvar '[' expression ']'
    {
        if ($1)
        {
            $$=new c_co_lvar( $1, $3, NULL );
            $$->co_set_file_bound( $1, $4 );
        }
        else
        {
            $$=NULL;
        }
    }
| lvar '[' expression ';' expression ']'
    {
        if ($1)
        {
            $$=new c_co_lvar( $1, $5, $3 );
            $$->co_set_file_bound( $1, $6 );
        }
        else
        {
            $$=NULL;
        }
    }
|
lvar '[' error
    {
        yyerror("Error in lvar; expected index expression");
        $$=NULL;
    }
|
lvar '.' error
    {
        yyerror("Error in lvar; expected index expression");
        $$=NULL;
    }
;

/*g for_statement
 */
for_statement:
TOKEN_TEXT_FOR '(' TOKEN_USER_ID ';' expression ')' optional_documentation '{' statement_list '}'
    {
         $$=new c_co_statement( $3, $5, $9, NULL, NULL, NULL );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         $$->co_set_file_bound( $1->file_posn, $10 );
    }
|
TOKEN_TEXT_FOR '(' TOKEN_USER_ID ';' expression ')' '(' expression ';' expression ';' expression ')' optional_documentation '{' statement_list '}'
    {
         $$=new c_co_statement( $3, $5, $16, $8, $10, $12 );
         $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
         $$->co_set_file_bound( $1->file_posn, $17 );
    }
| TOKEN_TEXT_FOR error     { yyerror($1->file_posn,"Error in for statement"); $$=NULL; }
;

/*g if_statement
 */
if_statement:
TOKEN_TEXT_IF '(' expression ')' '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $6, NULL, NULL, NULL, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $7 );
    }
| TOKEN_TEXT_IF '(' expression ')' TOKEN_STRING '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $7, NULL, NULL, $5, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $8 );
    }
| TOKEN_TEXT_IF '(' expression ')' '{' statement_list '}' TOKEN_TEXT_ELSE '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $6, $10, NULL, NULL, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $8, NULL );
        $$->co_set_file_bound( $1->file_posn, $11 );
    }
| TOKEN_TEXT_IF '(' expression ')' '{' statement_list '}' TOKEN_TEXT_ELSE TOKEN_STRING '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $6, $11, NULL, NULL, $9 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $8, NULL );
        $$->co_set_file_bound( $1->file_posn, $12 );
    }
| TOKEN_TEXT_IF '(' expression ')' TOKEN_STRING '{' statement_list '}' TOKEN_TEXT_ELSE '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $7, $11, NULL, $5, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $9, NULL );
        $$->co_set_file_bound( $1->file_posn, $12 );
    }
| TOKEN_TEXT_IF '(' expression ')' TOKEN_STRING '{' statement_list '}' TOKEN_TEXT_ELSE TOKEN_STRING '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $7, $12, NULL, $5, $10 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $9, NULL );
        $$->co_set_file_bound( $1->file_posn, $13 );
    }
| TOKEN_TEXT_IF '(' expression ')' '{' statement_list '}' elsif_statement
    {
        $$=new c_co_statement( $3, $6, NULL, $8, NULL, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        if ($8)
        {
            $$->co_set_file_bound( $1->file_posn, $8 );
        }
    }
| TOKEN_TEXT_IF '(' expression ')' TOKEN_STRING '{' statement_list '}' elsif_statement
    {
        $$=new c_co_statement( $3, $7, NULL, $9, $5, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        if ($9)
        {
            $$->co_set_file_bound( $1->file_posn, $9 );
        }
    }
| TOKEN_TEXT_IF error
    {
        yyerror($1->file_posn,"Syntax error: if ( <expression>) [<documentation>] { <statements> * } [elsif [<documentation>] { <statements> * }] [else [<documentation>] { <statements> * }]");
        $$=NULL;
    }
;

elsif_statement:
TOKEN_TEXT_ELSIF '(' expression ')' '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $6, NULL, NULL, NULL, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $7 );
    }
| TOKEN_TEXT_ELSIF '(' expression ')' TOKEN_STRING '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $7, NULL, NULL, $5, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $8 );
    }
| TOKEN_TEXT_ELSIF '(' expression ')' '{' statement_list '}' TOKEN_TEXT_ELSE '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $6, $10, NULL, NULL, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $8, NULL );
        $$->co_set_file_bound( $1->file_posn, $11 );
    }
| TOKEN_TEXT_ELSIF '(' expression ')' TOKEN_STRING '{' statement_list '}' TOKEN_TEXT_ELSE '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $7, $11, NULL, $5, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $9, NULL );
        $$->co_set_file_bound( $1->file_posn, $12 );
    }
| TOKEN_TEXT_ELSIF '(' expression ')' '{' statement_list '}' TOKEN_TEXT_ELSE TOKEN_STRING '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $6, $11, NULL, NULL, $9 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $8, NULL );
        $$->co_set_file_bound( $1->file_posn, $12 );
    }
| TOKEN_TEXT_ELSIF '(' expression ')' TOKEN_STRING '{' statement_list '}' TOKEN_TEXT_ELSE TOKEN_STRING '{' statement_list '}'
    {
        $$=new c_co_statement( $3, $7, $12, NULL, $5, $10 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, $9, NULL );
        $$->co_set_file_bound( $1->file_posn, $13 );
    }
| TOKEN_TEXT_ELSIF '(' expression ')' '{' statement_list '}' elsif_statement
    {
        $$=new c_co_statement( $3, $6, NULL, $8, NULL, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        if ($8)
        {
            $$->co_set_file_bound( $1->file_posn, $8 );
        }
    }
| TOKEN_TEXT_ELSIF '(' expression ')' TOKEN_STRING '{' statement_list '}' elsif_statement
    {
        $$=new c_co_statement( $3, $7, NULL, $9, $5, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        if ($9)
        {
            $$->co_set_file_bound( $1->file_posn, $9 );
        }
    }
| TOKEN_TEXT_ELSIF error
    {
        yyerror($1->file_posn,"Syntax error: elsif (<expression>) [<documentation>] { <statements> * } [elsif { <statements> * }] [else { <statements> * }]");
        $$=NULL;
    }
;

/*g switch statements (fullswitch, partswitch, priority)
 */
switch_statement:
TOKEN_TEXT_FULLSWITCH '(' expression ')' optional_documentation '{' case_entries '}'
    {
        $$ = new c_co_statement( statement_type_full_switch_statement, $3, $7, $5 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $8 );
    }
|
TOKEN_TEXT_PARTSWITCH '(' expression ')' optional_documentation '{' case_entries '}'
    {
        $$ = new c_co_statement( statement_type_partial_switch_statement, $3, $7, $5 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $8 );
    }
|
TOKEN_TEXT_PRIORITY '(' expression ')' optional_documentation '{' case_entries '}'
    {
        $$ = new c_co_statement( statement_type_priority_statement, $3, $7, $5 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $8 );
    }
| TOKEN_TEXT_FULLSWITCH error
    {
        yyerror($1->file_posn,"Syntax error: full_switch ( <expression> ) [<documentation>] { <case entries>* }");
        yyerrok;
        $$=NULL;
    }
| TOKEN_TEXT_PARTSWITCH error
    {
        yyerror($1->file_posn,"Syntax error: part_switch ( <expression> ) [<documentation>] { <case entries>* }");
        yyerrok;
        $$=NULL;
    }
| TOKEN_TEXT_PRIORITY error
    {
        yyerror($1->file_posn,"Syntax error: priority ( <expression> ) [<documentation>] { <case entries>* }");
        yyerrok;
        $$=NULL;
    }
;

case_entries:
 /* empty */
    {
         $$ = NULL;
    }
|
case_entries case_entry
    {
         if ($1)
         {
              $$=$1->chain_tail( $2 );
         }
         else
         {
              $$=$2;
         }
    }
;

case_entry:
TOKEN_TEXT_CASE expression_list ':' '{' statement_list '}'
    {
        $$=new c_co_case_entry( $2, 0, $5 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $6 );
    }
|
TOKEN_TEXT_CASE expression_list TOKEN_STRING ':' '{' statement_list '}'
    {
        $$=new c_co_case_entry( $2, 0, $6 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $7 );
    }
| TOKEN_TEXT_CASE expression_list ':'
    {
        $$=new c_co_case_entry( $2, 1, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $3 );
    }
| TOKEN_TEXT_CASE expression_list TOKEN_STRING ':'
    {
        $$=new c_co_case_entry( $2, 1, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $4 );
    }
| TOKEN_TEXT_DEFAULT ':' '{' statement_list '}'
    {
        $$=new c_co_case_entry( NULL, 0, $4 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $5 );
    }
| TOKEN_TEXT_DEFAULT TOKEN_STRING ':' '{' statement_list '}'
    {
        $$=new c_co_case_entry( NULL, 0, $5 );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $6 );
    }
| TOKEN_TEXT_DEFAULT ':'
    {
        $$=new c_co_case_entry( NULL, 1, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $2 );
    }
| TOKEN_TEXT_DEFAULT TOKEN_STRING ':'
    {
        $$=new c_co_case_entry( NULL, 1, NULL );
        $$->co_link_symbol_list( co_compile_stage_parse, $1, NULL );
        $$->co_set_file_bound( $1->file_posn, $3 );
    }
| TOKEN_TEXT_CASE error
    {
        yyerror($1->file_posn,"Syntax error: case <expression> [,<expression>]* [<documentation>] :  [ { <statement> * } ]");
        $$=NULL;
    }
| TOKEN_TEXT_DEFAULT error
    {
        yyerror($1->file_posn,"Syntax error: default [<documentation>] :  [ { <statement> * } ]");
        $$=NULL;
    }
;

/*g End of grammar */
%%

/*a Top level
 */
static int yylex( void )
{
     int i;
     i = cyc->next_token( &yylval );
     return i;
}
static void yyerror( t_lex_file_posn err_file_posn, const char *s )
{
     if (strcmp(s, "parse error"))
     {
         cyc->set_parse_error( err_file_posn, co_compile_stage_parse, "Parse error '%s'", s );
     }
}
static void yyerror(const char *s )
{
     if (strcmp(s, "parse error"))
     {
         cyc->set_parse_error( cyc->parse_get_current_location(), co_compile_stage_parse, "Parse error '%s'", s );
     }
}
extern void cyclicity_grammar_init( c_cyclicity *cyc )
{
	cyc->set_token_table( YYNTOKENS, yytname, (const short int *)yytoknum );
}
extern int cyclicity_parse( c_cyclicity *grammar )
{
#if YYDEBUG
	yydebug = 0;
#endif
	cyc = grammar;
    last_ok_file_posn = NULL;
	return yyparse();
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


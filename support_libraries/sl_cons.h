/*a Copyright
  
  This file 'sl_cons.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_CONS
#else
#define __INC_SL_CONS

/*a Includes
 */

/*a Defines
 */

/*a Types
 */
/*t t_sl_cons_type
 */
typedef enum
{
     sl_cons_type_string,
     sl_cons_type_ptr,
     sl_cons_type_cons_list,
     sl_cons_type_integer,
     sl_cons_type_none
} t_sl_cons_type;

/*t t_sl_cons_list
 */
typedef struct t_sl_cons_list
{
     struct t_sl_cons *first;
     struct t_sl_cons *last;
} t_sl_cons_list;

/*t t_sl_cons
 */
typedef struct t_sl_cons
{
     struct t_sl_cons *next_in_list;
     int mallocked;
     t_sl_cons_type type;
     union
     {
          int integer;
          char *string;
          void *ptr;
          t_sl_cons_list cons_list;
     } data;
} t_sl_cons;


/*a External functions
 */
extern int sl_cons_reset_list( t_sl_cons_list *list );
extern int sl_cons_free_list( t_sl_cons_list *list );
extern int sl_cons_append( t_sl_cons_list *list, t_sl_cons *item );
extern t_sl_cons *sl_cons_item( t_sl_cons_list *list );
extern t_sl_cons *sl_cons_item( int integer );
extern t_sl_cons *sl_cons_item( char *string, int copy );
extern t_sl_cons *sl_cons_item( void *ptr, int mallocked );

/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


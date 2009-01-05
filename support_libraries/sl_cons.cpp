/*a Copyright
  
  This file 'sl_cons.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sl_cons.h"

/*a Defines
 */
#define EXEC_FILE_MAX_STACK_DEPTH (8)
#define EXEC_FILE_MAX_OPS_PER_CMD (10000)

/*a Types
 */

/*a Cons handling
 */
/*f sl_cons_reset_list
 */
extern int sl_cons_reset_list( t_sl_cons_list *list )
{
     list->first = NULL;
     list->last = NULL;
     return 1;
}

/*f sl_cons_free_list
 */
extern int sl_cons_free_list( t_sl_cons_list *list )
{
     t_sl_cons *cons, *next_cons;
     for (cons=list->first; cons; cons=next_cons)
     {
          next_cons = cons->next_in_list;
          if (cons->mallocked)
          {
               switch (cons->type)
               {
               case sl_cons_type_string:
                    free(cons->data.string);
                    break;
               case sl_cons_type_cons_list:
                    sl_cons_free_list(&cons->data.cons_list);
                    break;
               case sl_cons_type_ptr:
                    free(cons->data.ptr);
                    break;
               default:
                    break;
               }
          }
          free(cons);
     }
     list->first = NULL;
     list->last = NULL;
     return 1;
}

/*f sl_cons_append
 */
extern int sl_cons_append( t_sl_cons_list *list, t_sl_cons *item )
{
     item->next_in_list = NULL;
     if (list->last)
     {
          list->last->next_in_list = item;
     }
     if (!list->first)
     {
          list->first = item;
     }
     list->last = item;
     return 1;
}

/*f sl_cons_malloc_item
 */
static t_sl_cons *sl_cons_malloc_item( int extra_size )
{
     t_sl_cons *item;
     item = (t_sl_cons *)malloc( sizeof(t_sl_cons)+extra_size );
     item->next_in_list = NULL;
     item->mallocked = 0;
     item->type = sl_cons_type_none;
     return item;
}

/*f sl_cons_item ( list )
 */
extern t_sl_cons *sl_cons_item( t_sl_cons_list *list )
{
     t_sl_cons *item;
     item = sl_cons_malloc_item( 0 );
     if (!item)
          return NULL;
     item->type = sl_cons_type_cons_list;
     item->data.cons_list = *list;
     item->mallocked = 1;
     return item;
}

/*f sl_cons_item ( integer )
 */
extern t_sl_cons *sl_cons_item( int integer )
{
     t_sl_cons *item;
     item = sl_cons_malloc_item( 0 );
     if (!item)
          return NULL;
     item->type = sl_cons_type_integer;
     item->data.integer = integer;
     return item;
}

/*f sl_cons_item ( string )
 */
extern t_sl_cons *sl_cons_item( char *string, int copy )
{
     t_sl_cons *item;
     int extra = 0;

     if (copy)
     {
          extra = strlen(string)+1;
     }

     item = sl_cons_malloc_item( extra );
     if (!item)
          return NULL;

     item->type = sl_cons_type_string;
     if (copy)
     {
          item->data.string = (char *)(&(item[1]));
          strcpy( item->data.string, string );
     }
     else
     {
          item->data.string = string;
     }
     return item;
}

/*f sl_cons_item ( ptr )
 */
extern t_sl_cons *sl_cons_item( void *ptr, int mallocked )
{
     t_sl_cons *item;
     item = sl_cons_malloc_item( 0 );
     if (!item)
          return NULL;
     item->type = sl_cons_type_ptr;
     item->data.ptr = ptr;
     item->mallocked = mallocked;
     return item;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


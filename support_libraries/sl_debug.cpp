/*a Copyright
  
  This file 'sl_debug.cpp' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Includes
 */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#define SL_DEBUG_SOURCE
// The defining of SL_DEBUG ensures that copies of the inline debug functions declared in sl_debug.h are included in our library exactly once.
// It controls the definitions as 'extern' ONLY when included by this code
#include "sl_debug.h"

/*a Defines
 */

/*a Types
 */

/*a Globals
 */
static int sl_debug_on;
static t_sl_debug_level sl_debug_level;
static FILE *sl_debug_file;

/*a Functions
 */
/*f sl_debug__internal_debug
 */
extern void sl_debug__internal_debug( t_sl_debug_level level, const char *file, int line, const char *func, const char *format, va_list ap )
{
     if ( (sl_debug_on) &&
          (level>=sl_debug_level) )
     {
          if (sl_debug_file)
          {
              fprintf( sl_debug_file, "%s:%d:%s:", file, line, func );
               vfprintf( sl_debug_file, format, ap );
               fprintf( sl_debug_file, "\n" );
          }
          else
          {
              fprintf( stderr, "%s:%d:%s:", file, line, func );
               vfprintf( stderr, format, ap );
               fprintf( stderr, "\n" );
          }
     }
}

/*f sl_debug__internal_set_level
 */
extern void sl_debug__internal_set_level( t_sl_debug_level level )
{
     sl_debug_level = level;
}

/*f sl_debug__internal_set_file
 */
extern void sl_debug__internal_set_file( FILE *f )
{
     sl_debug_file = f;
}

/*f sl_debug__internal_enable
 */
extern void sl_debug__internal_enable( int enable )
{
     sl_debug_on = enable;
}

/*f sl_debug__internal_if
 */
extern int sl_debug__internal_if( t_sl_debug_level level )
{
    return (sl_debug_on) && (level>=sl_debug_level);
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


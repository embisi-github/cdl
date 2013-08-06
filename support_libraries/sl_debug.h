/*a Copyright
  
  This file 'sl_debug.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_DEBUG
#else
#define __INC_SL_DEBUG

/*a Includes
 */
#include <stdarg.h>
#include <stdio.h>

/*a Defines
 */
#define SL_DEBUG_ENABLED
#ifdef SL_DEBUG_ENABLED
#define SL_DEBUG(level,...) sl_debug(level,__FILE__,__LINE__,__PRETTY_FUNCTION__, __VA_ARGS__ )
#define SL_DEBUG_TEST(level) (sl_debug__internal_if(level))
#else
#define SL_DEBUG(level,fmt,arg...) 
#define SL_DEBUG_TEST(level) (0)
#endif

/*a Types
 */
typedef enum
{
     sl_debug_level_verbose_info = 1,
     sl_debug_level_info = 3,
     sl_debug_level_warning = 5,
     sl_debug_level_high_warning = 10
} t_sl_debug_level;

extern int sl_waveform_depth;
/*a Functions
 */
#ifdef SL_DEBUG_ENABLED
extern void sl_debug__internal_debug( t_sl_debug_level level, const char *file, int line, const char *func, const char *format, va_list ap );
extern void sl_debug__internal_set_level( t_sl_debug_level level );
extern void sl_debug__internal_set_file( FILE *f );
extern void sl_debug__internal_enable( int enable );
extern int sl_debug__internal_if( t_sl_debug_level level );
#endif

#ifdef SL_DEBUG_SOURCE
extern
#else
static inline
#endif
void sl_debug( t_sl_debug_level level, const char *file, int line, const char *func, const char *format, ... )
{
#ifdef SL_DEBUG_ENABLED
     va_list ap;
     va_start( ap, format );
     sl_debug__internal_debug( level, file, line, func, format, ap );
     va_end( ap );
#endif
}

#ifdef SL_DEBUG_SOURCE
extern
#else
static inline
#endif
void sl_debug_set_level( t_sl_debug_level level )
{
#ifdef SL_DEBUG_ENABLED
     sl_debug__internal_set_level( level );
#endif
}
#ifdef SL_DEBUG_SOURCE
extern
#else
static inline
#endif
void sl_debug_enable( int enable )
{
#ifdef SL_DEBUG_ENABLED
     sl_debug__internal_enable( enable );
#endif
}
#ifdef SL_DEBUG_SOURCE
extern
#else
static inline
#endif
void sl_debug_set_file( FILE *file )
{
#ifdef SL_DEBUG_ENABLED
     sl_debug__internal_set_file( file );
#endif
}

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


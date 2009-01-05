/*a Copyright
  
  This file 'sl_token.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "sl_token.h"

/*a Defines
 */

/*a Types
 */

/*a Static variables
 */

/*a Token handling
 */
/*f sl_token_next
 */
extern char *sl_token_next( int continuation, char *line, char *line_end )
{
     char *token;
     if (!line)
         return NULL;
     if (line>=line_end)
          return NULL;

     if (continuation)
          line += strlen(line)+1;

     for (;(line<line_end) && (line[0]==' '); line++); // skip whitespace
     if (line>=line_end)
          return NULL;
     token = line;
     if (line[0]=='"')
     {
          line++;
          for (;(line[0]!='"') && (line[0]!=0); line++); // find next quote
          if (line[0]=='"')
          {
               line++;
          }
          line[0]=0;
          return token;
     }
     for (;(line[0]!=' ') && (line[0]!=0); line++); // find next whitespace
     line[0] = 0;
     return token;
}

/*f sl_tokenize_line
 */
extern void sl_tokenize_line( char *line, char **tokens, int max_tokens, int *num_tokens )
{
     int i;
     char *line_end;

     *num_tokens = 0;
     tokens[0] = NULL;

     line_end = line+strlen(line);
     tokens[0] = sl_token_next( 0, line, line_end );
     for (i=0; tokens[i] && (i<max_tokens-1); i++)
     {
          tokens[i+1] = sl_token_next( 1, tokens[i], line_end );
     }
     *num_tokens = i;
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


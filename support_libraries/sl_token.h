/*a Copyright
  
  This file 'sl_token.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_TOKEN
#else
#define __INC_SL_TOKEN

/*a Includes
 */

/*a Defines
 */

/*a External functions
 */
extern char *sl_token_next( int continuation, char *line, char *line_end );
extern void sl_tokenize_line( char *line, char **tokens, int max_tokens, int *num_tokens );
extern char **sl_tokenize_line( char *line, int *num_tokens ); // allocates a buffer for the token pointers

/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


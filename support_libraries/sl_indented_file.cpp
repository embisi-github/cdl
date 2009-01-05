/*a Copyright
  
  This file 'sl_indented_file.cpp' copyright Gavin J Stark 2003, 2004
  
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
#include "c_sl_error.h"
#include "sl_general.h" 
#include "sl_token.h" 
#include "sl_indented_file.h"

/*a Defines
 */

/*a Types
 */

/*a Static variables
 */

/*a Indentation sensitive GUI functions
 */
/*f sl_read_indented_file_lines
 */
static void sl_read_indented_file_lines( c_sl_error *error, const char *filename, const char *user, t_sl_indented_file_cmd *file_cmds, char *text, char *text_end, char **text_out, int *line_num, int minimum_indent, t_sl_cons_list *cl )
{
     int i, j;
     int expected_indent;
     int indent;
     int syntax_error;
     char *line, *next_line;
     int ntokens;
     char *tokens[64];
     t_sl_cons_list sub_cl;
     t_sl_cons_list item_list;

     sl_cons_reset_list( cl );

     expected_indent = -1;
     for (line=text; line<text_end; line=next_line, (*line_num)++)
     {
          /*b Get line indent
           */
          for (indent=0; (line[indent]==' ') || (line[indent]=='\t') || (line[indent]=='\r'); indent++);

          /*b If commented, move on a line
           */
          if ( (line[indent]=='#') ||
               ( (line[indent]=='/') && (line[indent+1]=='/') ) )
          {
               for (next_line=line+indent; (next_line<text_end) && (next_line[0]!=0) && (next_line[0]!='\n'); next_line++);
               if (next_line<text_end)
               {
                    next_line++;
               }
               continue;
          }

          /*b If indent less than expected, return to previous level
           */
          if (expected_indent==-1)
          {
               expected_indent = indent;
               if (indent<minimum_indent)
               {
                    expected_indent = minimum_indent;
               }
          }
          if (indent>expected_indent)
          {
               error->add_error( (void *)user, error_level_serious, error_number_sl_unexpected_indent, error_id_sl_indented_file_allocate_and_read_indented_file, error_arg_type_malloc_string, filename, error_arg_type_line_number, *line_num, error_arg_type_none );
               for (next_line=line+indent; (next_line<text_end) && (next_line[0]!=0) && (next_line[0]!='\n'); next_line++);
               if (next_line<text_end)
               {
                    next_line++;
               }
               continue;
          }
          if (indent<expected_indent)
          {
               *text_out = line;
               return;
          }

          /*b Get pointer to next line, and make whitespace into spaces
           */
          for (next_line=line; (next_line<text_end) && (next_line[0]!=0) && (next_line[0]!='\n'); next_line++)
          {
               if ((next_line[0]=='\t') || (next_line[0]=='\r'))
               {
                    next_line[0] = ' ';
               }
          }
          if ((next_line<text_end) && (next_line[0]=='\n'))
          {
               next_line[0] = 0;
               next_line++;
          }

          /*b Tokenize the line
           */
          sl_tokenize_line( line, tokens, 64, &ntokens );

          /*b Find the command
           */
          for (i=0; file_cmds[i].cmd; i++)
          {
               if (!strcmp(file_cmds[i].cmd, tokens[0]))
               {
                    break;
               }
          }
          if (!file_cmds[i].cmd)
          {
               error->add_error( (void *)user, error_level_serious, error_number_sl_unknown_command, error_id_sl_indented_file_allocate_and_read_indented_file, 
                                 error_arg_type_malloc_string, tokens[0], error_arg_type_malloc_filename, filename, error_arg_type_line_number, *line_num, error_arg_type_none );
               continue;
          }

          /*b Check and import the arguments
           */
          sl_cons_reset_list (&item_list );
          sl_cons_append( &item_list, sl_cons_item( (char *)file_cmds[i].cmd, 0 ) );
          syntax_error = 0;
          for (j=0; j<(int)strlen(file_cmds[i].args); j++)
          {
               switch (file_cmds[i].args[j])
               {
               case 'x':
                    sl_cons_append( &item_list, sl_cons_item( (char *)tokens[j+1], 1 ) );
                    break;
               }
          }

          /*b If it takes items, read them
           */
          if ( file_cmds[i].takes_items )
          {
               (*line_num)++;
               sl_read_indented_file_lines( error, filename, user, file_cmds, next_line, text_end, &next_line, line_num, expected_indent+1, &sub_cl );
               sl_cons_append( &item_list, sl_cons_item( &sub_cl ) );
          }

          /*b Warn if syntax error
           */
          if (syntax_error)
          {
               error->add_error( (void *)user, error_level_serious, error_number_sl_syntax, error_id_sl_indented_file_allocate_and_read_indented_file, 
                                 error_arg_type_const_string, file_cmds[i].syntax, error_arg_type_malloc_filename, filename, error_arg_type_line_number, *line_num, error_arg_type_none );
          }

          /*b Add to the result
           */
          sl_cons_append( cl, sl_cons_item( &item_list ) );
     }
     *text_out = text_end;
}

/*f sl_allocate_and_read_indented_file
 */
extern t_sl_error_level sl_allocate_and_read_indented_file( c_sl_error *error, const char *filename, const char *user, t_sl_indented_file_cmd *file_cmds, t_sl_cons_list *cl )
{
     int line_num;
     char *text, *text_end;
     t_sl_error_level result;

     text = NULL;
     result = sl_allocate_and_read_file( error, filename, &text, user );
     if (result!=error_level_okay)
     {
          return result;
     }

     text_end = text+strlen(text);
     line_num=1;
     sl_read_indented_file_lines( error, filename, user, file_cmds, text, text_end, &text_end, &line_num, 0, cl );

     free(text);
     return error_level_okay;
}



/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


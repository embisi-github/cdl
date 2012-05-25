/*a Copyright
  
  This file 'c_lexical_analyzer.cpp' copyright Gavin J Stark 2003, 2004
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2.0.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
*/

/*a To do
To do:

*/

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sl_debug.h"
#include "c_lexical_analyzer.h"
#include "lexical_types.h"
// Need c_co_type_specifier for the grammar ... :-(
#include "c_co_type_specifier.h"
#include "cyclicity_grammar.h"
#include "c_type_value_pool.h"
#include "c_cyclicity.h"

/*a Defines
 */

/*a Types
 */
/*t t_token
 */
typedef struct t_token
{
     const char *token_name;
     int token_type;
} t_token;

/*t t_string_chain
 */
typedef struct t_string_chain
{
     struct t_string_chain *next;
     char string[1];
} t_string_chain;

/*t t_lex_string
 */
typedef struct t_lex_string
{
     struct t_lex_string *next;
     int length;
     char *text;
} t_lex_string;

/*t t_lex_symbol
 */
typedef struct t_lex_symbol
{
     struct t_lex_symbol *next;
     int type;
     char name[1];
} t_lex_symbol;

/*t t_terminal_entry_type
 */
typedef enum
{
     terminal_entry_type_whitespace,
     terminal_entry_type_single_line_comment,
     terminal_entry_type_full_comment,
     terminal_entry_type_string,
     terminal_entry_type_symbol,
     terminal_entry_type_sized_int,
     terminal_entry_type_char_pair,
     terminal_entry_type_char,
     terminal_entry_type_link_to_file,
     terminal_entry_type_termination, // ALWAYS the last in a file, indicates post-EOF
} t_terminal_entry_type;

/*t t_terminal_entry
 */
typedef struct t_terminal_entry
{
     t_terminal_entry_type terminal_entry_type;
     struct t_lex_file *lex_file; // Implicit when this is accessed throught the lex_file array, but when this terminal_entry is linked to as a use of a token this helps recover the lex_file
     int file_pos;
     int length;
     union
     {
          void *vptr;
          t_string *string;
          t_symbol *symbol;
          t_sized_int *sized_int;
          char c;
          int i;
          t_lex_file *file;
     } data;
} t_terminal_entry;

/*t t_lex_file
 */
typedef struct t_lex_file
{
     struct t_lex_file *next;

     struct t_lex_file *included_by;

     FILE *handle;
     char *filename;
     int file_size;
     char *file_data;
     int *line_starts;
     int number_lines;

     int file_terminal_pos;
     int number_terminal_entries;
     t_terminal_entry *terminal_entries;
} t_lex_file;

/*a Statics
 */
static t_token default_tokens[] = {
     {"constant", TOKEN_TEXT_CONSTANT},
     {"struct", TOKEN_TEXT_STRUCT},
     {"fsm", TOKEN_TEXT_FSM},
     {"one_hot", TOKEN_TEXT_ONE_HOT},
     {"one_cold", TOKEN_TEXT_ONE_COLD},
//     {"schematic", TOKEN_TEXT_SCHEMATIC},
//     {"symbol", TOKEN_TEXT_SYMBOL},
//     {"port", TOKEN_TEXT_PORT},
//     {"line", TOKEN_TEXT_LINE},
//     {"fill", TOKEN_TEXT_FILL},
//     {"oval", TOKEN_TEXT_OVAL},
//     {"option", TOKEN_TEXT_OPTION},
     {"preclock", TOKEN_TEXT_PRECLOCK},
     {"register", TOKEN_TEXT_REGISTER},
     {"assert", TOKEN_TEXT_ASSERT},
     {"cover", TOKEN_TEXT_COVER},
     {"include", TOKEN_TEXT_INCLUDE},
     {"typedef", TOKEN_TEXT_TYPEDEF},
     {"string", TOKEN_TEXT_STRING},
     {"bit", TOKEN_TEXT_BIT},
     {"integer", TOKEN_TEXT_INTEGER},
     {"enum", TOKEN_TEXT_ENUM},
     {"extern", TOKEN_TEXT_EXTERN},
     {"module", TOKEN_TEXT_MODULE},
     {"input", TOKEN_TEXT_INPUT},
     {"output", TOKEN_TEXT_OUTPUT},
     {"parameter", TOKEN_TEXT_PARAMETER},
     {"timing", TOKEN_TEXT_TIMING},
     {"to", TOKEN_TEXT_TO},
     {"from", TOKEN_TEXT_FROM},
     {"bundle", TOKEN_TEXT_BUNDLE},
     {"default", TOKEN_TEXT_DEFAULT},
     {"clock", TOKEN_TEXT_CLOCK},
     {"rising", TOKEN_TEXT_RISING},
     {"falling", TOKEN_TEXT_FALLING},
     {"gated_clock", TOKEN_TEXT_GATED_CLOCK},
     {"reset", TOKEN_TEXT_RESET},
     {"active_low", TOKEN_TEXT_ACTIVE_LOW},
     {"active_high", TOKEN_TEXT_ACTIVE_HIGH},
     {"clocked", TOKEN_TEXT_CLOCKED},
     {"comb", TOKEN_TEXT_COMB},
     {"net", TOKEN_TEXT_NET},
     {"for", TOKEN_TEXT_FOR},
     {"if", TOKEN_TEXT_IF},
     {"elsif", TOKEN_TEXT_ELSIF},
     {"else", TOKEN_TEXT_ELSE},
     {"full_switch", TOKEN_TEXT_FULLSWITCH},
     {"part_switch", TOKEN_TEXT_PARTSWITCH},
     {"priority", TOKEN_TEXT_PRIORITY},
     {"case", TOKEN_TEXT_CASE},
     {"break", TOKEN_TEXT_BREAK},
     {"sizeof", TOKEN_TEXT_SIZEOF},
     {"print", TOKEN_TEXT_PRINT},
     {"log", TOKEN_TEXT_LOG},
     {"config", TOKEN_TEXT_CONFIG},
     {"assert", TOKEN_TEXT_ASSERT},
     {"repeat",   TOKEN_TEXT_REPEAT},
     {"while",    TOKEN_TEXT_WHILE},
     {"generate", TOKEN_TEXT_GENERATE},
     {"matches", TOKEN_TEXT_MATCHES},
     {"__async_read__", TOKEN_TEXT_DECL_ASYNC_READ},
     {"__approved__", TOKEN_TEXT_DECL_APPROVED},
     {0, 0}
};

/*a Symbol table handling functions
 */
/*f c_lexical_analyzer::putsym
 */
t_lex_symbol *c_lexical_analyzer::putsym( const char *symname, int length, int symtype )
{
     t_lex_symbol *ptr;

     ptr = (t_lex_symbol *) malloc (sizeof (t_lex_symbol) + length);
     strncpy(ptr->name, symname, length);
     ptr->name[length]=0;
     ptr->type = symtype;
     ptr->next = sym_table;
     sym_table = ptr;
     return ptr;
}

/*f c_lexical_analyzer::getsym
 */
t_lex_symbol *c_lexical_analyzer::getsym( const char *sym_name, int length )
{
     t_lex_symbol *ptr;
     for (ptr = sym_table; ptr; ptr = ptr->next )
     {
          if ( !strncmp( ptr->name, sym_name, length ) &&
               (ptr->name[length]==0) )
          {
               return ptr;
          }
     }
     return 0;
}

/*f c_lexical_analyzer::replacesym
 */
void c_lexical_analyzer::replacesym( t_symbol *symbol, const char *new_text, int new_text_length )
{
    t_lex_symbol *lex_symbol;
    lex_symbol = getsym( new_text, new_text_length );
    if (!lex_symbol)
    {
        lex_symbol = putsym( new_text, new_text_length, symbol->lex_symbol->type );
    }
    symbol->lex_symbol = lex_symbol;
}

/*f get_symbol_type
 */
extern int get_symbol_type( t_lex_symbol *lex_symbol )
{
     if (lex_symbol)
          return lex_symbol->type;
     return -1;
}

/*a lexical_analyzer class constructors and administration functions
 */
/*f c_lexical_analyzer::c_lexical_analyzer
 */
c_lexical_analyzer::c_lexical_analyzer( c_cyclicity *cyclicity )
{
     int i;

     this->cyclicity = cyclicity;

     include_directories = NULL;
     last_include_directory = NULL;

     current_file = NULL;
     file_list = NULL;

     sym_table = NULL;

     for (i = 0; default_tokens[i].token_name != 0; i++)
     {
          putsym( default_tokens[i].token_name, strlen(default_tokens[i].token_name), default_tokens[i].token_type );
     }
}

/*f c_lexical_analyzer::~c_lexical_analyzer
 */
c_lexical_analyzer::~c_lexical_analyzer()
{
     int i;
     t_lex_file *file, *next_file;
     t_lex_symbol *lex_symbol, *next_lex_symbol;
     t_string_chain *dir, *next_dir;

     //printf("Deleting c_lexical_analyzer %p\n",this);
     for (dir=include_directories; dir; dir=next_dir)
     {
          next_dir = dir->next;
          free(dir);
     }
     for (lex_symbol = sym_table; lex_symbol; lex_symbol=next_lex_symbol)
     {
          next_lex_symbol = lex_symbol->next;
          free(lex_symbol);
          //printf("Freed symbol %p (%p)\n",lex_symbol, next_lex_symbol);
     }

     for (file=file_list; file; file=next_file)
     {
          //printf("Freeing file %p\n",file);
          if (file->filename)
          {
               free(file->filename);
          }
          if (file->file_data);
          {
               free(file->file_data);
          }
          if (file->line_starts)
          {
               free(file->line_starts);
          }
          if (file->terminal_entries)
          {
               for (i=0; i<file->number_terminal_entries; i++)
               {
                    switch (file->terminal_entries[i].terminal_entry_type)
                    {
                    case terminal_entry_type_string:
                    case terminal_entry_type_sized_int:
                    case terminal_entry_type_symbol:
                         if (file->terminal_entries[i].data.vptr)
                              free(file->terminal_entries[i].data.vptr);
                         break;
                    case terminal_entry_type_char_pair:
                    case terminal_entry_type_char:
                    case terminal_entry_type_single_line_comment:
                    case terminal_entry_type_full_comment:
                    case terminal_entry_type_whitespace:
                    case terminal_entry_type_link_to_file:
                    case terminal_entry_type_termination:
                         break;
                    }
               }
               free(file->terminal_entries);
          }
          next_file = file->next;
          free( file );
          //printf("Freed file %p (%p)\n",file, next_file);
     }
}

/*a File handling functions
 */
/*f c_lexical_analyzer::add_include_directory
 */
void c_lexical_analyzer::add_include_directory( char *directory )
{
     t_string_chain *new_str;
     new_str = (t_string_chain *)malloc(sizeof(t_string_chain) + strlen(directory) );
     if (new_str)
     {
          if (!include_directories)
          {
               include_directories = new_str;
          }
          else
          {
               last_include_directory->next = new_str;
          }
          last_include_directory = new_str;
          new_str->next = NULL;
          strcpy( new_str->string, directory );
     }
}

/*f c_lexical_analyzer::reset_files
 */
void c_lexical_analyzer::reset_files( void )
{
     current_file = NULL;
}

/*f read_file_data
        Read in the file data from the file, with a given file_length
        Return 1 for success, 0 for failure
        file_length may be -1, in which case buffer should grow as necesary
        Also set the line_starts to point to the start of each line (first character after a \n)
 */
static int read_file_data( t_lex_file *file, FILE *f, int file_length )
{
     int i, j;
     int blk_length, size_so_far;
     char *new_data;

     size_so_far = 0;
     blk_length = file_length+32; // Some room for manoeuvre
     if (file_length==-1)
     {
          blk_length = 1024;
     }
     while (1)
     {
          if (!file->file_data)
          {
               file->file_data = (char *) malloc( blk_length );
               file->file_size = blk_length;
               if (!file->file_data)
               {
                    return 0;
               }
          }
          if (blk_length+size_so_far>file->file_size)
          {
               if (blk_length<16384)
               {
                    blk_length*=2;
               }
               new_data = (char *)realloc( file->file_data, blk_length+size_so_far );
               if (!new_data)
               {
                    return 0;
               }
               file->file_data = new_data;
               file->file_size = blk_length+size_so_far;
          }
          i = fread( file->file_data+size_so_far, 1, blk_length, f );
          if (i<blk_length)
          {
               file->file_size = size_so_far+i;
               break;
          }
          size_so_far += i;
     }
     
     file->file_data[file->file_size] = 0;
     for (i=0, j=0; i<file->file_size; i++)
     {
          if (file->file_data[i]=='\n')
          {
               j++;
          }
     }
     file->number_lines = j+1;

     file->line_starts = (int *)malloc(sizeof(int)*file->number_lines);
     if (!file->line_starts)
          return 0;
     file->line_starts[0] = 0;
     for (i=0, j=0; i<file->file_size; i++)
     {
          if (file->file_data[i]=='\n')
          {
               j++;
               file->line_starts[j] = i+1;
          }
     }
     return 1;
}

/*f c_lexical_analyzer::allocate_and_read_file
 */
t_lex_file *c_lexical_analyzer::allocate_and_read_file( const char *filename, FILE *f, int length )
{
     t_lex_file *file;
     int i, j;

     /*b Allocate file structure
      */
     file = (t_lex_file *)malloc(sizeof(t_lex_file));
     file->filename = (char *)malloc(strlen(filename)+1);
     strcpy( file->filename, filename );
     file->file_size = 0;
     file->file_data = NULL;
     file->number_lines = 0;
     file->line_starts = NULL;
     file->included_by = NULL;
     file->number_terminal_entries = 0;
     file->terminal_entries = NULL;

     /*b Read file and break into tokens
      */
     if (!read_file_data( file, f, -1 ))
     {
          free(file); // ZZZ - free more
          return 0;
     }
     if (!file_break_into_tokens( file ))
     {
          free(file); // ZZZ - free more
          return 0;
     }
     file->next = file_list;
     file_list = file;

     /*b Run through tokens looking for an include directive
      */
     for (i=0; i<file->number_terminal_entries-1 ; i++)
     {
          if ( (file->terminal_entries[ i ].terminal_entry_type == terminal_entry_type_symbol) &&
               (file->terminal_entries[ i ].data.symbol->lex_symbol->type == TOKEN_TEXT_INCLUDE ) )
          {
               j = file->terminal_entries[i].file_pos;
               if ((j==0) || (file->file_data[j-1]=='\n'))
               {
                    for (j = i+1; j<file->number_terminal_entries; j++)
                    {
                         if (file->terminal_entries[ j ].terminal_entry_type == terminal_entry_type_string)
                         {
                              char *filename;
                              t_lex_file *included_file;

                              filename = file->terminal_entries[j].data.string->string;
                              included_file = include_file( file, filename, strlen(filename) );
                              SL_DEBUG( sl_debug_level_info, "Would try to include file %s (%p)", filename, included_file );
                              free(file->terminal_entries[j].data.vptr);
                              file->terminal_entries[j].terminal_entry_type = terminal_entry_type_whitespace;
                              free(file->terminal_entries[i].data.vptr);
                              file->terminal_entries[i].terminal_entry_type = terminal_entry_type_link_to_file;
                              file->terminal_entries[i].data.file = included_file;
                              if (included_file)
                                   included_file->included_by = file;
                         }
                         else if (file->terminal_entries[ j ].terminal_entry_type != terminal_entry_type_whitespace)
                         {
                              break;
                         }
                    }
               }
          }
     }

     return file;
}

/*f include_file
 */
t_lex_file *c_lexical_analyzer::include_file( t_lex_file *included_by, const char *filename, int filename_length )
{
     char buffer[256];
     t_lex_file *file;

     strncpy( buffer, filename, filename_length );
     buffer[filename_length] = 0;

     for (file=file_list; file; file=file->next)
     {
          if (!strcmp( buffer, file->filename ))
          {
               return NULL;
          }
     }
     if (!set_file( buffer ))
     {
          cyclicity->set_parse_error( co_compile_stage_tokenize, "Failed to include file '%s'", buffer );
          return NULL;
     }
     return current_file;
}

/*f file_reset_position
 */
static int file_reset_position( t_lex_file *file )
{
     SL_DEBUG( sl_debug_level_info, "Reset file %p", file );
     if (!file)
          return 0;

     file->file_terminal_pos = 0;
     return 1;
}

/*f c_lexical_analyzer::set_file
 */
int c_lexical_analyzer::set_file( FILE *f )
{
     current_file = allocate_and_read_file( "<stream>", f, -1 );
     return (file_reset_position(current_file));
}

/*f c_lexical_analyzer::set_file
 */
int c_lexical_analyzer::set_file( char *filename )
{
     int i;
     FILE *f;
     t_string_chain *dir;
     char buffer[512];

     SL_DEBUG( sl_debug_level_info, "Opening file %s", filename );
     f = fopen( filename, "r" );
     for (dir = include_directories; !f && dir; dir=dir->next )
     {
          snprintf( buffer, 512, "%s/%s", dir->string, filename );
          buffer[511] = 0;
          f = fopen( buffer, "r" );
          SL_DEBUG( sl_debug_level_info, "Tried %s : %p", buffer, f );
     }
     if (f)
     {
          fseek( f, 0,SEEK_END );
          i = ftell(f);
          fseek( f, 0, SEEK_SET );
          current_file = allocate_and_read_file( filename, f, i );
          fclose(f);
          return (file_reset_position(current_file));
     }
     return 0;
}

/*f c_lexical_analyzer::get_number_of_files
 */
int c_lexical_analyzer::get_number_of_files( void )
{
     int i;
     t_lex_file *file;
     for (i=0, file=file_list; file; i++,file=file->next);
     return i;
}

/*f c_lexical_analyzer::get_nth_file
 */
t_lex_file *c_lexical_analyzer::get_nth_file( int n )
{
     t_lex_file *file;
     for (file=file_list; (n>0) && file; n--,file=file->next);
     return file;
}

/*f c_lexical_analyzer::get_filename
 */
char *c_lexical_analyzer::get_filename( int file_number )
{
     t_lex_file *file;
     file = get_nth_file( file_number );
     if (!file)
          return NULL;
     return file->filename;
}

/*f c_lexical_analyzer::get_file_handle
 */
t_lex_file *c_lexical_analyzer::get_file_handle( int file_number )
{
     return get_nth_file( file_number );
}

/*f c_lexical_analyzer::get_file_data
 */
    int c_lexical_analyzer::get_file_data( int file_number, int *file_size, int *number_lines, char **file_data )
{
     t_lex_file *file;
     file = get_nth_file( file_number );
     if (!file)
          return 0;
     *file_size = file->file_size;
     *number_lines = file->number_lines;
     *file_data = file->file_data;
     return 1;
}

/*f c_lexical_analyzer::get_line_data
 */
int c_lexical_analyzer::get_line_data( int file_number, int line_number, char **line_start, int *line_length )
{
     t_lex_file *file;
     file = get_nth_file( file_number );
     if (!file)
          return 0;
     if ((line_number<0) || (line_number>=file->number_lines))
          return 0;

     *line_start = file->file_data + file->line_starts[line_number];
     if (line_number==file->number_lines-1)
     {
          *line_length = file->file_size - file->line_starts[line_number];
     }
     else
     {
          *line_length = file->line_starts[line_number+1] - file->line_starts[line_number];
     }
     return 1;
}

/*a Lexical analysis functions
 */
/*f c_lexical_analyzer::set_token_table
 */
void c_lexical_analyzer::set_token_table( int yyntokens, const char *const *yytname, const short *yytoknum )
{
     this->yyntokens = yyntokens;
     this->yytname = yytname;
     this->yytoknum = yytoknum;
}

/*f line_from_file_posn
 */
static int line_from_file_posn( t_lex_file *lex_file, int char_posn )
{
     int i;

     //printf("Looking for char %d in file %s\n", char_posn, lex_file->filename );
     if (char_posn<0)
          return 0;
     if (char_posn>=lex_file->file_size)
          return lex_file->number_lines-1;

     for (i=0; i<lex_file->number_lines-1; i++)
     {
          if (char_posn<lex_file->line_starts[i+1])
               return i;
     }
     return lex_file->number_lines-1;
}

/*f file_add_terminal_entry
  Add a terminal to the lexical structure for a file
  The last one in the file should be terminal_entry_type_termination which does not have a start or end - these are implied from the previous terminal (if any)
 */
static void file_add_terminal_entry( t_lex_file *file, int store, t_terminal_entry_type terminal_entry_type, int start, int end, void *data )
{
     #if 0
     printf("Add terminal %p %d %d %d %d %p\n",
            file,
            store,
            terminal_entry_type,
            start,
            end,
            data );
    #endif
     if (store)
     {
         if (terminal_entry_type==terminal_entry_type_termination)
         {
             start=0; end=0;
             if (file->number_terminal_entries>=1)
             {
                 start = file->terminal_entries[ file->number_terminal_entries-1 ].file_pos+file->terminal_entries[ file->number_terminal_entries-1 ].length;
                 end = start;
             }
         }
         file->terminal_entries[ file->number_terminal_entries ].terminal_entry_type = terminal_entry_type;
         file->terminal_entries[ file->number_terminal_entries ].lex_file = file;
         file->terminal_entries[ file->number_terminal_entries ].file_pos = start;
         file->terminal_entries[ file->number_terminal_entries ].length = end-start;
         file->terminal_entries[ file->number_terminal_entries ].data.vptr = data;
     }
     file->number_terminal_entries++;
     if (store && terminal_entry_type==terminal_entry_type_termination)
     {
         file->number_terminal_entries--; // Don't actually add one to the store for a termination - it is for 'just beyond EOF'
     }
}

/*f c_lexical_analyzer::file_break_into_tokens_internal
 */
int c_lexical_analyzer::file_break_into_tokens_internal( t_lex_file *file, int store )
{
     int i, found;
     char c, nc;
     int file_ofs, last_file_ofs;
     t_sized_int *sized_int;
     t_lex_symbol *lex_symbol;
     t_symbol *symbol;
     t_string *string;
     int bit_length, has_mask;

     /*b Check ptrs are valid
      */
     if ( !file ||
          !file->file_data )
          return 0;

     /*b Loop through all the terminals
      */
     file_ofs = 0;
     file->number_terminal_entries = 0;
     while (file_ofs<file->file_size)
     {
          last_file_ofs = file_ofs;

          /*b Full comment?
           */
          if (!strncmp(file->file_data+file_ofs, "/*", 2 ))
          {
               for (file_ofs+=2; file_ofs<file->file_size-1; file_ofs++)
               {
                    if ( (file->file_data[file_ofs]=='*') &&
                         (file->file_data[file_ofs+1]=='/') )
                    {
                         break;
                    }
                    if ( (file->file_data[file_ofs]=='/') &&
                         (file->file_data[file_ofs+1]=='*') )
                    {
                         if (store)
                         {
                              cyclicity->set_parse_error( file->terminal_entries+file->number_terminal_entries, co_compile_stage_tokenize, "/* inside comment" );
                         }
                    }
               }
               file_ofs += 2;
               file_add_terminal_entry( file, store, terminal_entry_type_full_comment, last_file_ofs, file_ofs, NULL );
               continue;
          }

          /*b Single line comment?
           */
          if (!strncmp(file->file_data+file_ofs, "//", 2 ))
          {
               for (file_ofs+=2; file_ofs<file->file_size; file_ofs++)
               {
                    if (file->file_data[file_ofs]=='\n')
                    {
                         break;
                    }
               }
               file_ofs++;
               file_add_terminal_entry( file, store, terminal_entry_type_single_line_comment, last_file_ofs, file_ofs, NULL );
               continue;
          }

          /*b Whitespace?
           */
          c = file->file_data[file_ofs];
          if ( (c==' ') || (c=='\t') || (c=='\n') || (c=='\r'))
          {
               for (file_ofs+=1; file_ofs<file->file_size; file_ofs++)
               {
                    c = file->file_data[file_ofs];
                    if ( (c!=' ') && (c!='\t') && (c!='\n') && (c!='\r') )
                    {
                         break;
                    }
               }
               file_add_terminal_entry( file, store, terminal_entry_type_whitespace, last_file_ofs, file_ofs, NULL );
               continue;
          }

          /*b Number?
           */
          if (isdigit(c))
          {
               /*b Allocate a sized_int
                */
               sized_int = NULL;
               if (store)
               {
                    sized_int = (t_sized_int *)malloc(sizeof(t_sized_int) + 32 );
                    sized_int->file_posn = &(file->terminal_entries[ file->number_terminal_entries ]);
                    sized_int->user = NULL;
                    sized_int->type_value = type_value_undefined;
                    sized_int->cyclicity = cyclicity;
               }

               /*b Read decimal, or size
                */
               if (!store)
               {
                   c = cyclicity->type_value_pool->read_value( file->file_data,
                                                               &file_ofs,
                                                               NULL,
                                                               0,
                                                               &bit_length,
                                                               &has_mask );
               }
               else
               {
                   c = cyclicity->type_value_pool->new_type_value_integer_or_bits( file->file_data, &file_ofs, &sized_int->type_value );
               }
               if (c!=0)
               {
                   if (store) /*if store not set the file->terminal_entries is NULL */ cyclicity->set_parse_error( file->terminal_entries+file->number_terminal_entries, co_compile_stage_tokenize, "Error reading sized int - too large, too big for its size, bad character, ..." );
                    file_add_terminal_entry( file, store, terminal_entry_type_whitespace, last_file_ofs, file_ofs, NULL );
                    continue;
               }
               /*b Add as terminal
                */
               file_add_terminal_entry( file, store, terminal_entry_type_sized_int, last_file_ofs, file_ofs, (void *)sized_int );
               continue;
          }

          /*b Is it a string ?
           */
          if ((c=='"') || ((c=='r') && (file->file_data[file_ofs+1]=='"')))
          {
              int triple_quoted;
              int raw;
              raw = 0;
              if (c=='r')
              {
                  raw=1;
                  file_ofs++;
              }
              triple_quoted = 0;
              if (file_ofs+3<file->file_size)
              {
                  if ( (file->file_data[file_ofs+1]=='"') && (file->file_data[file_ofs+2]=='"') )
                  {
                      file_ofs+=2;
                      triple_quoted = 1;
                  }
              }
              file_ofs++;
              while (file_ofs<file->file_size)
              {
                  if ( (!raw) && (file->file_data[file_ofs]=='\\'))
                  {
                      file_ofs+=2;
                  }
                  else if (file->file_data[file_ofs]=='"')
                  {
                      if (triple_quoted)
                      {
                          if ( (file->file_data[file_ofs+1]=='"') && (file->file_data[file_ofs+2]=='"') )
                          {
                              file_ofs+=3;
                              break;
                          }
                          file_ofs++;
                      }
                      else
                      {
                          file_ofs++;
                          break;
                      }
                  }
                  else
                  {
                      file_ofs++;
                  }
               }

               string = NULL;
               if (store)
               {
                    string = (t_string *)malloc( sizeof(t_string)+file_ofs-last_file_ofs+1 );
                    string->file_posn = &(file->terminal_entries[ file->number_terminal_entries ]);
                    string->user = NULL;
                    if (triple_quoted)
                    {
                        strncpy( string->string, file->file_data+last_file_ofs+3+raw, file_ofs-last_file_ofs ); // Skip the double quotes - who needs them!
                        string->string[file_ofs-last_file_ofs-6-raw] = 0;
                    }
                    else
                    {
                        strncpy( string->string, file->file_data+last_file_ofs+1+raw, file_ofs-last_file_ofs ); // Skip the double quotes - who needs them!
                        string->string[file_ofs-last_file_ofs-2-raw] = 0;
                    }
                    if (!raw)
                    {
                        int esc_i;
                        esc_i = 0;
                        while (string->string[esc_i])
                        {
                            if (string->string[esc_i]=='\\')
                            {
                                memmove( string->string+esc_i, string->string+esc_i+1, strlen(string->string+esc_i) );
                            }
                            else
                            {
                                esc_i = esc_i+1;
                            }
                        }
                    }
                    if (!triple_quoted)
                    {
                        if (strchr(string->string,'\n'))
                        {
                            fprintf(stderr,"Newline within single-quoted string ending at line %d of file '%s' - use triple-quotes for multiline strings -- this will error soon (Dec 2011)\n",line_from_file_posn( file, file_ofs ),file->filename );
                            //cyclicity->set_parse_error( file->terminal_entries+file->number_terminal_entries, co_compile_stage_tokenize, "Newline within commment - use triple-quotes for multiline comments" );
                        }
                    }
               }
               file_add_terminal_entry( file, store, terminal_entry_type_string, last_file_ofs, file_ofs, (void *)string );
               continue;
          }

          /*b Identifier?
           */
          if (isalpha(c) || (c=='_'))
          {
               file_ofs++;
               while ( (file_ofs<file->file_size) &&
                       ( (isalnum(file->file_data[file_ofs])) ||
                         (file->file_data[file_ofs]=='_') ) )
               {
                    file_ofs++;
               }
               lex_symbol = getsym( file->file_data+last_file_ofs, file_ofs-last_file_ofs );
               if (!lex_symbol)
               {
                    lex_symbol = putsym( file->file_data+last_file_ofs, file_ofs-last_file_ofs, TOKEN_USER_ID );
               }

               symbol = NULL;
               if (store)
               {
                    symbol = (t_symbol *)malloc(sizeof(t_symbol));
                    if (symbol)
                    {
                         symbol->lex_symbol = lex_symbol;
                         symbol->file_posn = &(file->terminal_entries[ file->number_terminal_entries ]);
                         symbol->user = NULL;
                    }
               }
               file_add_terminal_entry( file, store, terminal_entry_type_symbol, last_file_ofs, file_ofs, (void *)symbol );
               continue;
          }

          /*b Symbol pair?
           */
          nc = file->file_data[file_ofs+1];
          found = -1;
          for (i = 0; i < yyntokens; i++)
          {
               if ( (yytname[i] != 0) &&
                    (yytname[i][0] == '"') &&
                    (yytname[i][1] == c) &&
                    (yytname[i][2] == nc) &&
                    (yytname[i][3] == '"') )
               {
                    found = i;
                    break;
               }
          }
          if (found>=0)
          {
               file_ofs+=2;
               file_add_terminal_entry( file, store, terminal_entry_type_char_pair, last_file_ofs, file_ofs, (void *)found );
               continue;
          }

          /*b Any other character is a token by itself
           */
          file_ofs++;
          file_add_terminal_entry( file, store, terminal_entry_type_char, last_file_ofs, file_ofs, (void *)((int)(c)) );
          continue;

          /*b Done
           */
     }

     /*b Add termination
      */
     file_add_terminal_entry( file, store, terminal_entry_type_termination, 0, 0, NULL );

     /*b Return okay
      */
     return 1;
}

/*f c_lexical_analyzer::file_break_into_tokens
 */
int c_lexical_analyzer::file_break_into_tokens( t_lex_file *file )
{
     if (!file_break_into_tokens_internal( file, 0 ))
          return 0;
     file->terminal_entries = (t_terminal_entry *)malloc(sizeof(t_terminal_entry) * file->number_terminal_entries );
     if (!file->terminal_entries)
          return 0;
     return file_break_into_tokens_internal( file, 1 );
}

/*f c_lexical_analyzer::next_token
 */
int c_lexical_analyzer::next_token( void *arg )
{
     t_terminal_entry_type type;
     t_terminal_entry *entry;
     YYSTYPE *yyarg = (YYSTYPE *)arg;

     if (!current_file)
          return TOKEN_EOF;

     while (current_file->file_terminal_pos < current_file->number_terminal_entries) 
     {
          type = current_file->terminal_entries[ current_file->file_terminal_pos ].terminal_entry_type;
          if ( (type!=terminal_entry_type_whitespace) &&
               (type!=terminal_entry_type_single_line_comment) &&
               (type!=terminal_entry_type_full_comment) )
               break;
          current_file->file_terminal_pos++;
     }
     if (current_file->file_terminal_pos == current_file->number_terminal_entries)
     {
          if (current_file->included_by)
          {
               SL_DEBUG( sl_debug_level_info, "Returning from include file %s %p %p %d", current_file->filename , current_file, current_file->included_by, current_file->included_by->file_terminal_pos );
               current_file = current_file->included_by;
               return next_token( arg );
          }
          current_file->file_terminal_pos++;
          return TOKEN_EOF;
     }
     if (current_file->file_terminal_pos >= current_file->number_terminal_entries)
     {
          return 0;
     }

     entry = current_file->terminal_entries + current_file->file_terminal_pos;
     current_file->file_terminal_pos++;
     switch (entry->terminal_entry_type)
     {
     case terminal_entry_type_link_to_file:
          if (!entry->data.file)
               return next_token( arg );
          SL_DEBUG( sl_debug_level_info, "Including file %s %p %p %d", entry->data.file->filename , entry->data.file, current_file, current_file->file_terminal_pos );
          current_file = entry->data.file;
          current_file->file_terminal_pos = 0;
          return next_token( arg );
     case terminal_entry_type_string:
          yyarg->string = entry->data.string;
          return TOKEN_STRING;
     case terminal_entry_type_symbol:
          yyarg->symbol = entry->data.symbol;
          return entry->data.symbol->lex_symbol->type;
     case terminal_entry_type_sized_int:
          yyarg->sized_int = entry->data.sized_int;
          return TOKEN_SIZED_INT;
     case terminal_entry_type_char_pair:
          yyarg->terminal = entry;
          return yytoknum[entry->data.i];
     case terminal_entry_type_char:
          yyarg->terminal = entry;
          return entry->data.i;
     default:
          break;
     }
     fprintf(stderr,"Bug - should not reach this point in lexical analysis token return\n");
     exit(4);
}

/*f c_lexical_analyzer::repeat_eof
 */
int c_lexical_analyzer::repeat_eof( void )
{
     if (current_file->file_terminal_pos == current_file->number_terminal_entries+1)
     {
          if (!current_file->included_by)
          {
              current_file->file_terminal_pos--;
              return 1;
          }
     }
     return 0;
}

/*f c_lexical_analyzer::get_current_location
 */
t_lex_file_posn c_lexical_analyzer::get_current_location( void )
{
     if (!current_file)
          return NULL;
     if (current_file->file_terminal_pos<0) 
         return NULL;
     if (current_file->file_terminal_pos > current_file->number_terminal_entries)
         return current_file->terminal_entries + current_file->number_terminal_entries; // Points at post-EOF
     return current_file->terminal_entries + current_file->file_terminal_pos;
}

/*f c_lexical_analyzer::translate_lex_file_posn
 */
int c_lexical_analyzer::translate_lex_file_posn( t_lex_file_posn *lex_file_posn, int *file_number, int *first_line, int *last_line, int *char_offset )
{
     t_lex_file *file;
     int i, char_pos;

     *file_number = 0;
     *first_line = 0;
     *last_line = 0;
     *char_offset = 0;
     if ( (!lex_file_posn) ||
          (!(*lex_file_posn)) )
          return 0;

     for (i=0,file=file_list; file; file=file->next,i++)
     {
          if (file==(*lex_file_posn)->lex_file)
          {
               *file_number = i;
               break;
          }
     }

     if (!file)
          return 0;

     char_pos = (*lex_file_posn)->file_pos;
     *first_line = line_from_file_posn( file, char_pos );
     *last_line = line_from_file_posn( file, char_pos + (*lex_file_posn)->length );
     *char_offset = char_pos - file->line_starts[*first_line];
     return 1;
}

/*f c_lexical_analyzer::translate_lex_file_posn
 */
int c_lexical_analyzer::translate_lex_file_posn( t_lex_file_posn *lex_file_posn, int *file_number, int *file_position, int end_not_start )
{
     t_lex_file *file;
     int i, char_pos;

     if ( (!lex_file_posn) ||
          (!(*lex_file_posn)) )
          return 0;

     for (i=0,file=file_list; file; file=file->next,i++)
     {
          if (file==(*lex_file_posn)->lex_file)
          {
               *file_number = i;
               break;
          }
     }

     if (!file)
          return 0;

     char_pos = (*lex_file_posn)->file_pos;

     if (end_not_start)
     {
          *file_position = char_pos + (*lex_file_posn)->length;
     }
     else
     {
          *file_position = char_pos;
     }
     return 1;
}

/*f dump_symbols
 */
void c_lexical_analyzer::dump_symbols( int symbol_class )
{
     t_lex_symbol *s;
     for (s=sym_table; s; s=s->next)
     {
          printf("Symbol '%s' type %d\n", s->name, s->type );
          switch (s->type)
          {
          case TOKEN_USER_ID:
               printf("\tUser identifier (non-type, non-enum)\n" );
               break;
          default:
               printf("\tSystem\n");
               break;
          }
     }
}

/*f c_lexical_analyzer::find_object_including_char
 */
class c_cyc_object *c_lexical_analyzer::find_object_including_char( t_lex_file *lex_file, int file_position )
{
     int i, last_i;

     if (!lex_file)
          return NULL;

     last_i = 0;
     for (i=0; i<lex_file->number_terminal_entries; i++)
     {
          if (lex_file->terminal_entries[i].file_pos > file_position)
               break;
          last_i = i;
     }
     while (1)
     {
          switch (lex_file->terminal_entries[last_i].terminal_entry_type)
          {
          case terminal_entry_type_string:
               printf("User of string '%s' is %p\n",
                      lex_file->terminal_entries[last_i].data.string->string,
                      lex_file->terminal_entries[last_i].data.string->user );
               return lex_file->terminal_entries[last_i].data.string->user;
          case terminal_entry_type_symbol:
               printf("User of symbol '%s' is %p\n",
                      lex_file->terminal_entries[last_i].data.symbol->lex_symbol->name,
                      lex_file->terminal_entries[last_i].data.symbol->user );
               return lex_file->terminal_entries[last_i].data.symbol->user;
          case terminal_entry_type_sized_int:
               return lex_file->terminal_entries[last_i].data.sized_int->user;
          default:
               break;
          }
          if (last_i==0)
               return NULL;
          last_i--;
     }
     return NULL;
}

/*a External functions
 */
/*f c_lexical_analyzer::parse_file_text_around
 // Generate string with file text around the file posn, possibly annotated with a pointer
 */
int c_lexical_analyzer::parse_file_text_around( char *buffer, int buffer_size, t_lex_file_posn *lex_file_posn, int include_annotation )
{
    int file_number, first_line, last_line, char_offset;
    t_lex_file *file;
    int line_start, line_length;
    int display_start, display_length, post_display_length;

    buffer[0] = 0;
    if (!translate_lex_file_posn( lex_file_posn, &file_number, &first_line, &last_line, &char_offset ))
        return 0;

    for (file=file_list; file && (file_number>0); file=file->next,file_number--);
    if (!file)
        return 0; // Should never happen

    if (first_line >= file->number_lines)
        return 0; // Should never happen
    if (first_line<0)
        return 0; // A lot of really defensive stuff here
    last_line = first_line; // We actually print just the one line

    // We want display_start to point to the first character to display from the file
    // We want char_offset to be how far in to a display line the annotation should be (-1 for none)
    // We want line_start to be how far in to the display buffer the 'first_line' is
    // We want display_length to be how much to display from the file
    // We want post_display_length to be how much to display from the file AFTER the annotation line (if present)

    line_start = file->line_starts[first_line];
    if (first_line<file->number_lines-1)
    {
        line_length = file->line_starts[first_line+1] - line_start;
    }
    else
    {
        line_length = file->file_size - line_start;
    }
    if (file->file_data[line_start+line_length-1]=='\n')
    {
        line_length--;
    }
    if (line_length<0) line_length=0;

    display_start = line_start;
    display_length = line_length;
    line_start = 0;
    post_display_length = 0;

    // Adjust
    if (include_annotation)
    {
        if ((display_length+char_offset)>buffer_size-2) // It won't fit!
        {
            if (char_offset<(buffer_size-2)/2) // but just the offset to the character will - so cut off the end of the line
            {
                display_length = buffer_size-2 - char_offset;
            }
            else // Oooh - have to cut from the start of the line to fit it in; basically at this point do buffer_size/2 back from char_offset
            {
                display_start += char_offset - (buffer_size-2)/2; // Move to buffer_size/2 back from char_offset
                display_length = (buffer_size-2)/2; // Must display this length
                line_start -= char_offset - (buffer_size-2)/2; // So line_start moved back by that much too
                char_offset = (buffer_size-2)/2; // So we also moved the annotation
            }
        }
        else // It fits - can we grow it backwards by up to 3 lines? And then forwards by 2?
        {
            int up_lines;
            int down_lines;
            int space;
            up_lines=3;
            down_lines=2;
            while ((up_lines>0) && (first_line>0))
            {
                space = buffer_size-2 - display_length - char_offset;
                int prev_line_length;
                prev_line_length = file->line_starts[first_line] - file->line_starts[first_line-1];
                if (prev_line_length>=space)
                    break;
                display_length += prev_line_length;
                display_start -= prev_line_length;
                up_lines--;
                first_line--;
            }
            while ((down_lines>0) && (last_line<file->number_lines-1))
            {
                space = buffer_size-2 - display_length - char_offset - post_display_length;
                int next_line_length;
                if (last_line==file->number_lines-1)
                {
                    next_line_length = file->file_size - file->line_starts[last_line];
                }
                else
                {
                    next_line_length = file->line_starts[last_line+1] - file->line_starts[last_line];
                }
                if (next_line_length>=space)
                    break;
                post_display_length += next_line_length;
                down_lines--;
                last_line++;
            }
        }
    }
    //fprintf(stderr, "Line start %d ofs %d disp start %d disp length %d\n",line_start, char_offset, display_start, display_length );
    memcpy( buffer, file->file_data+display_start, display_length );
    buffer[display_length] = '\n';
    buffer += display_length+1;
    if (char_offset>=0)
    {
        memset( buffer, ' ', char_offset );
        buffer[ char_offset ] = '^';
        buffer[ char_offset+1 ] = 0;
        buffer += char_offset+1;
    }
    if (post_display_length)
    {
        memcpy( buffer, file->file_data+display_start+display_length, post_display_length );
        buffer += post_display_length;
    }
    buffer[0] = 0;
    return 1;
}

/*f lex_string_from_terminal ( string )
 */
extern const char *lex_string_from_terminal( struct t_string *string )
{
     if (!string)
          return "<Null string>";
     return string->string;
}

/*f lex_string_from_terminal ( symbol )
 */
extern const char *lex_string_from_terminal( struct t_symbol *symbol )
{
     if (!symbol)
          return "<Null symbol>";
     return symbol->lex_symbol->name;
}

/*f lex_string_from_terminal ( cyclicity, type_value value, buffer, buffer_size )
 */
extern void lex_string_from_terminal( c_cyclicity *cyclicity, t_type_value type_value, char *buffer, int buffer_size )
{
    buffer[0] = 0;
    if (type_value == type_value_error)
    {
        strncpy( buffer, "<error>", buffer_size );
        return;
    }
    if (type_value == type_value_undefined)
    {
        strncpy( buffer, "<undefined>", buffer_size );
        return;
    }
    if (cyclicity->type_value_pool->derefs_to_integer( type_value ))
    {
        snprintf( buffer, buffer_size, "(integer)%lld", cyclicity->type_value_pool->integer_value( type_value ));
        return;
    }
    if (cyclicity->type_value_pool->derefs_to_bit_vector( type_value ))
    {
        snprintf( buffer, buffer_size, "(bit array[%d])", cyclicity->type_value_pool->array_size( type_value ));
        return;
    }
    if (cyclicity->type_value_pool->derefs_to_bool( type_value ))
    {
        snprintf( buffer, buffer_size, "(bool) %d", cyclicity->type_value_pool->logical_value( type_value ));
        return;
    }
    snprintf( buffer, buffer_size, "type %d: value ?%d?", cyclicity->type_value_pool->get_type(type_value), -1 );
    return;
}

/*f lex_string_from_terminal ( sized_int, buffer, buffer_size )
 */
extern void lex_string_from_terminal( struct t_sized_int *sized_int, char *buffer, int buffer_size )
{
     if (!sized_int)
     {
         strncpy( buffer, "<No int>", buffer_size );
         return;
     }
     lex_string_from_terminal( sized_int->cyclicity, sized_int->type_value, buffer, buffer_size );
}

/*f lex_reset_file_posn
 */
extern void lex_reset_file_posn( t_lex_file_posn *file_posn )
{
     *file_posn = NULL;
}

/*f lex_bound_file_posn( lex_file_posn )
 */
extern void lex_bound_file_posn( t_lex_file_posn *file_posn, t_lex_file_posn *test_posn, int max_not_min )
{
     if (!(*test_posn))
          return;
     if (!(*file_posn))
     {
          *file_posn = *test_posn;
          return;
     }
     if ((*file_posn)->lex_file!=(*test_posn)->lex_file)
          return;
     if ((max_not_min) && ((*file_posn)->file_pos < (*test_posn)->file_pos))
     {
          *file_posn = *test_posn;
     }
     if ((!max_not_min) && ((*file_posn)->file_pos > (*test_posn)->file_pos))
     {
          *file_posn = *test_posn;
     }
}

/*f lex_char_relative_to_posn
 */
extern int lex_char_relative_to_posn( t_lex_file_posn *file_posn, int end_not_start, struct t_lex_file *lex_file, int file_char_position )
{
     int rel_char_pos;

     if ((!file_posn) || (!lex_file) || (!(*file_posn)))
     {
          return -1;
     }
     if ((*file_posn)->lex_file!=lex_file)
     {
          return -1;
     }

     rel_char_pos = (*file_posn)->file_pos - file_char_position;

     if (end_not_start)
     {
          rel_char_pos += (*file_posn)->length;
     }
     return rel_char_pos;
}


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/



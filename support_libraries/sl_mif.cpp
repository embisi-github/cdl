/*a Copyright
  
  This file 'sl_mif.cpp' copyright Gavin J Stark 2003, 2004, 2007
  
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
#include "sl_general.h"
#include "sl_token.h"
#include "sl_mif.h"

/*a Defines
 */
#define WHERE_I_AM {}
#define WHERE_I_AM_VERBOSE {fprintf(stderr,"%s:%s:%d\n",__FILE__,__func__,__LINE__ );}

/*a Types
 */
/*t t_mif_write_info
 */
typedef struct t_mif_write_info
{
    int bit_start;
    int bit_size;
    int bytes_per_item;
    char *data_ptr;
    t_sl_mif_file_callback_fn callback;
    void *handle;
} t_mif_write_info;

/*a Static variables
 */

/*a Write data functions
 */
/*f write_data
 */
static void write_data( void *handle, t_sl_uint64 address, t_sl_uint64 *data )
{
    t_mif_write_info *info;
    info = (t_mif_write_info *)handle;
    switch (info->bytes_per_item)
    {
    case 1:
        info->data_ptr[address*info->bytes_per_item] = data[0]>>info->bit_start;
        break;
    case 2:
        info->data_ptr[address*info->bytes_per_item+0] = data[0]>>(info->bit_start+0);
        info->data_ptr[address*info->bytes_per_item+1] = data[0]>>(info->bit_start+8);
        break;
    case 4:
        ((t_sl_uint32 *)info->data_ptr)[address*info->bytes_per_item/4] = data[0]>>(info->bit_start);
        break;
    case 8:
        ((t_sl_uint64 *)info->data_ptr)[address*info->bytes_per_item/8] = data[0]>>(info->bit_start);
        break;
    default:
        for (int i=0; i<info->bytes_per_item; i++) {
            info->data_ptr[address*info->bytes_per_item+i] = data[0]>>(info->bit_start+8*i);
        }
        break;
    }
    if (info->callback)
    {
        info->callback( info->handle, address );
    }
}

/*a MIF file handling
 */
/*f sl_mif_allocate_and_read_mif_file
 */
extern t_sl_error_level sl_mif_allocate_and_read_mif_file( c_sl_error *error, const char *filename, const char *user, int max_size, int bit_size, int bit_start, int **data_ptr, t_sl_mif_file_callback_fn callback, void *handle )
{
    return sl_mif_allocate_and_read_mif_file( error, filename, user, 0, max_size, bit_size, bit_start, 2, 0, data_ptr, callback, handle );
}

/*f sl_mif_allocate_and_read_mif_file
  Needs to support any bit length of data contents - should be specified.
  Can read just a set of the bits from the incoming data, for splitting a word memory file into sub-word memories
  Allocates the maximum size data
*/
extern t_sl_error_level sl_mif_allocate_and_read_mif_file( c_sl_error *error, const char *filename, const char *user, int address_offset, int max_size, int bit_size, int bit_start, int reset, int reset_value, int **data_ptr, t_sl_mif_file_callback_fn callback, void *handle  )
{
    int bytes_per_item;

    WHERE_I_AM;

    /*b Allocate room for the data
     */
    bytes_per_item = BITS_TO_BYTES(bit_size);
    (*data_ptr) = (int *)malloc(bytes_per_item*max_size);
    if (!*data_ptr)
    {
        return error->add_error( (void *)user, error_level_fatal, error_number_general_malloc_failed, error_id_sl_mif_allocate_and_read_mif_file,
                                 error_arg_type_malloc_string, filename,
                                 error_arg_type_malloc_filename, user,
                                 error_arg_type_none );
    }

    return sl_mif_reset_and_read_mif_file( error, filename, user,
                                           address_offset,
                                           max_size, bit_size, bit_start,
                                           reset, reset_value,
                                           *data_ptr,
                                           callback, handle );
}

/*f sl_mif_reset_and_read_mif_file
*/
extern t_sl_error_level sl_mif_reset_and_read_mif_file( c_sl_error *error, const char *filename, const char *user, int address_offset, int max_size, int bit_size, int bit_start, int reset, int reset_value, int *data, t_sl_mif_file_callback_fn callback, void *handle  )
{
    int i;
    int bytes_per_item;
    t_mif_write_info write_info;

    /*b Allocate room for the data
     */
    WHERE_I_AM;
    bytes_per_item = BITS_TO_BYTES(bit_size);

    /*b Reset if required
     */
    WHERE_I_AM;
    switch (reset)
    {
    case 1:
    WHERE_I_AM;
        for (i=0; i<max_size; i++)
        {
            switch (bytes_per_item)
            {
            case 1: ((char *)data)[i] = reset_value; break;
            case 2:
                ((char *)data)[i*2+0] = reset_value>>0;
                ((char *)data)[i*2+1] = reset_value>>8;
                break;
            default:
                for (int j=0; j<bytes_per_item; j++)
                {
                    ((char *)data)[i*bytes_per_item+j] = (reset_value>>(8*(j&3)));
                }
                break;
            }
        }
        break;
    case 2:
    WHERE_I_AM;
        for (i=0; i<max_size; i++)
        {
            switch (bytes_per_item)
            {
            case 1: ((char *)data)[i] = i+reset_value; break;
            case 2:
                ((char *)data)[i*2+0] = (i+reset_value)>>0;
                ((char *)data)[i*2+1] = (i+reset_value)>>8;
                break;
            default:
                for (int j=0; j<bytes_per_item; j++)
                {
                    ((char *)data)[i*bytes_per_item+j] = ((i+reset_value)>>(8*(j&3)));
                }
                break;
            }
        }
        break;
    case 3:
    {
    WHERE_I_AM;
        t_sl_uint64 value;
        t_sl_uint64 taps[2];
        taps[0] = 1LL<<((reset_value>>0)&0xff);
        taps[1] = 1LL<<((reset_value>>8)&0xff);
        value = (reset_value>>16)&0xffff;
        value = ~(value | (value<<16) | (value<<32) | (value<<48));
        for (i=0; i<max_size; i++)
        {
            int j;
            switch (bytes_per_item)
            {
            case 1: ((char *)data)[i] = value; break;
            case 2:
                ((char *)data)[i*2+0] = value;
                ((char *)data)[i*2+1] = value>>8;
                break;
            default:
                for (int j=0; j<bytes_per_item; j++)
                {
                    ((char *)data)[i*bytes_per_item+j] = ((value)>>(8*(j&3)));
                }
                break;
            }
            for (j=0; j<8*bytes_per_item; j++)
            {
                if (((value & taps[0])!=0) ^ ((value & taps[1])!=0)) {value = (value<<1)|1;} else {value<<=1;}
            }
        }
        break;
    }
    }

    /*b Read the MIF file
     */
    WHERE_I_AM;
    write_info.bytes_per_item = bytes_per_item;
    write_info.bit_start = bit_start;
    write_info.bit_size = bit_size;
    write_info.data_ptr = (char *)data;
    write_info.callback = callback;
    write_info.handle = handle;
    sl_mif_read_mif_file( error,
                          filename,
                          user,
                          (t_sl_uint64) address_offset,
                          (t_sl_uint64) max_size,
                          0, // No error on out of range
                          write_data,
                          &write_info );

    WHERE_I_AM;
    return error_level_okay;
}

/*f sl_mif_read_mif_file - read a MIF file with a callback for each word of data to write
 */
extern t_sl_error_level sl_mif_read_mif_file( c_sl_error *error,
                                              const char *filename,
                                              const char *user,
                                              t_sl_uint64 address_offset,
                                              t_sl_uint64 size, // size of memory - don't callback if address in file - address_offset is outside the range 0 to size-1
                                              int error_on_out_of_range, // add an error on every memory location that is out of range
                                              t_sl_mif_file_data_callback_fn callback,
                                              void *callback_handle  )
{
     char *text, *tokens[64], *line, *next_line;
     t_sl_error_level result;
     int line_num, addressed;
     t_sl_uint64 address;
     int ntokens;
     int i, j, cont;
     t_sl_uint64 values[8];

     /*b If null file, then just return
      */
     if (!filename)
          return error_level_okay;

     /*b Read in the file
      */
     text = NULL;
     result = sl_allocate_and_read_file( error, filename, &text, user );
     if (result!=error_level_okay)
     {
          return result;
     }

     /*b Parse the lines in the file and handle their contents
      */
     address = 0;
     cont = 1;
     line_num=0;
     line = text;
     while (cont)
     {
          /*b Find end-of-line, converting whitespace to spaces
           */
          for (i=0; line[i] && (line[i]!='\n'); i++)
          {
               if ( (line[i]=='\t') || (line[i]=='\r') )
               {
                    line[i] = ' ';
               }
          }
          if (line[i]!=0)
          {
               next_line = line+i+1;
               line[i] = 0;
          }
          else
          {
               cont = 0;
               next_line = line+i;
          }
          line_num++;

          /*b Tokenize the line
           */
          sl_tokenize_line( line, tokens, 64, &ntokens );
          line = next_line;

          if (ntokens==0)
               continue;

          /*b Skip comments
           */
          if ( (tokens[0][0]=='#') ||
               ( (tokens[0][0]=='/') && (tokens[0][1]=='/') ) )
               continue;

          /*b Find address in line (look for a ':' at the end of the first token or as the second token)
           */
          addressed = 0;
          i = strlen(tokens[0]);
          if ( tokens[0][i-1]==':' )
          {
               tokens[0][i-1]=0;
               addressed = 1;
          }
          else if ((tokens[1]) && (tokens[1][0]==':'))
          {
               tokens[1]++;
               addressed = 1;
          }

          /*b Store any data tokens; if there is an address then we note that first
           */
          for (i=0; i<ntokens; i++)
          {
               /*b Skip null tokens
                */
               if (tokens[i][0]==0)
                    continue;
               /*b If comment start, then stop here
                */
               if ( (tokens[i][0]=='#') ||
                    (tokens[i][0]==';') ||
                    ( (tokens[i][0]=='/') && (tokens[i][1]=='/') ) )
                   break;
               /*b Read token as ints
                */
               if (tokens[i][0]=='!')
               {
                    if (!sl_integer_from_token( tokens[i]+1, values ))
                    {
                         error->add_error( (void *)user, error_level_serious, error_number_general_bad_integer, error_id_sl_mif_allocate_and_read_mif_file,
                                           error_arg_type_malloc_string, tokens[i]+1,
                                           error_arg_type_line_number, line_num,
                                           error_arg_type_malloc_filename, filename,
                                           error_arg_type_none );
                         break;
                    }
               }
               else
               {
                   for (j=0; tokens[i][j]; j++)
                   {
                       int ch;
                       ch = tokens[i][j];
                       if ( !( ((ch>='0') && (ch<='9')) ||
                               ((ch>='a') && (ch<='f')) ||
                               ((ch>='A') && (ch<='F')) ) )
                           break;
                   }
                   if ( (tokens[i][j]) ||
                        (sscanf( tokens[i], "%llx", values)!=1) )
                    {
                         error->add_error( (void *)user, error_level_serious, error_number_general_bad_hexadecimal, error_id_sl_mif_allocate_and_read_mif_file,
                                           error_arg_type_malloc_string, tokens[i],
                                           error_arg_type_line_number, line_num,
                                           error_arg_type_malloc_filename, filename,
                                           error_arg_type_none );
                         break;
                    }
               }

               /*b If an addressed line and first value token, set address, else just move on
                */
               if (addressed)
               {
                    address = values[0];
               }

               /*b Store data (if not an address)
                */
               if  (!addressed)
               {
                   t_sl_uint64 rel_address = address - address_offset;

                   /*b Check address is in range
                    */
                   if (error_on_out_of_range && (rel_address>=size))
                   {
                       error->add_error( (void *)user, error_level_serious, error_number_general_address_out_of_range, error_id_sl_mif_allocate_and_read_mif_file,
                                         error_arg_type_uint64, address,
                                         error_arg_type_line_number, line_num,
                                         error_arg_type_malloc_filename, filename,
                                         error_arg_type_none );
                       break;
                   }

                   /*b Invoke callback to write the data
                    */
                   if ( address<size )
                   {
                       if (callback)
                       {
                           callback( callback_handle, address, values );
                       }
                       address++;
                   }
               }

               /*b Done
                */
               addressed = 0;
          }
     }

     /*b Free the text, and return
      */
     free(text);
     return error_level_okay;
}

/*f sl_mif_write_mif_file - Write a MIF file
 */
extern t_sl_error_level sl_mif_write_mif_file( c_sl_error *error,
                                               const char *filename,
                                               const char *user,
                                               int *data,
                                               int bit_size,
                                               t_sl_uint64 address_offset,
                                               t_sl_uint64 size )
{
     t_sl_uint64 address;
     int on_line;
     FILE *f;

     /*b If null file, then just return
      */
     if (!filename)
          return error_level_okay;

     /*b Open the file for writing
      */
     f = fopen( filename, "w" );
     if (!f)
     {
          return error->add_error( (void *)user, error_level_serious, error_number_general_bad_filename, error_id_sl_general_sl_allocate_and_read_file,
                                   error_arg_type_malloc_string, filename,
                                   error_arg_type_malloc_filename, user,
                                   error_arg_type_none );
     }

     /*b Write at a line for each 128 bits
      */
     address = address_offset;
     on_line = 0;
     for (; size>0; size-=1)
     {
         if (on_line>=16) // GJS was 128 - perhaps needs to be configurable?
         {
             fprintf(f,"\n");
             on_line=0;
         }
         if (bit_size<=8)
         {
             fprintf( f, "%02x ", ((const char *)data)[address] );
             on_line += 8;
         }
         else if (bit_size<=16)
         {
             fprintf( f, "%04x ", ((const unsigned short *)data)[address] );
             on_line += 16;
         }
         else if (bit_size<=32)
         {
             fprintf( f, "%08x ", ((const t_sl_uint32 *)data)[address] );
             on_line += 32;
         }
         else if (bit_size<=64)
         {
             fprintf( f, "%016llx ", ((const t_sl_uint64 *)data)[address] );
             on_line += 64;
         }
         address++;
     }
     fprintf(f,"\n");

     /*b Done
      */
     fclose(f);
     return error_level_okay;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


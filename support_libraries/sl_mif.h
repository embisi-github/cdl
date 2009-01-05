/*a Copyright
  
  This file 'sl_mif.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Supported file format
//d The file format actually supported is a collection of lines. Each line consists
//d of a set of lexical tokens seperated by white space, which may be tabs or spaces.
//d
//d If the first token of a line starts with '#', ';' or '//' then the line is deemed
//d to be a comment and ignored.
//d Then a line may be addressed or not. An addressed line starts <address> : or <address>:
//d An address line indicates that data on the line and subsequent lines is for the
//d specified address and onwards
//d On an unaddressed line or after the address in an addressed line
//d tokens are taken to be data values, unless they start with '#', ';' or '//' in which
//d case the remainder of the line is ignored as comment
//d
//d All data values and addresses are treated as hexadecimal, unless they start with '!'
//d in which case the rest of the token is read using the sl_integer_from_token call in
//d  sl_general, so all the formats supported there are supported in the MIF files.
//d
//d The first data value will be deemed to be for address 0 unless an address line preceeds it.
//d
//d Examples are:
//d 
//d 100 200 300 # This line indicates that address 0 should contain 0x100, address 1 0x200, and address 2 0x300
//d 10: 5678 # This line indicates that address 0x10 should contain 0x5678
//d 1234 # And that address 0x10+1 should contain 0x1234
//d !67 # This is a decimal data item, so 0x12 should contain 67 == 0x43
//d // This is a comment line
//d # And so is this
//d        #And so is this!
//d !1: !0x100 //Address 1 should contain 0x100 now - this overwrites theprevious value above
//d !0b00_1111_0_11_00 // Address 2 should contain binary 00111101100, or 0x1ec
//d 
//d Common faults:
//d 
//d 1:2 # The ':' must have whitespace after it for it to be interpreted correctly.
//d 
 */

/*a Wrapper
 */
#ifdef __INC_SL_MIF
#else
#define __INC_SL_MIF

/*a Includes
 */
#include "sl_general.h"
#include "c_sl_error.h"

/*a Defines
 */

/*a Types
 */
/*t t_sl_mif_file_data_callback_fn - invoked to get an external memory to write the data as it comes from the MIF file
 */
typedef void (*t_sl_mif_file_data_callback_fn)( void *handle, t_sl_uint64 address, t_sl_uint64 *data );

/*t t_sl_mif_file_callback_fn
 */
typedef void (*t_sl_mif_file_callback_fn)( void *handle, t_sl_uint64 address );

/*a External functions
 */
/*f sl_mif_write_mif_file
 */
extern t_sl_error_level sl_mif_write_mif_file( c_sl_error *error,
                                               const char *filename,
                                               const char *user,
                                               int *data,
                                               int bit_size,
                                               t_sl_uint64 address_offset,
                                               t_sl_uint64 size );

/*f sl_mif_read_mif_file
//d This is the main entrypoint. It takes a filename, which gives the MIF
//d file to load. The <i>error</i> is used for reporting errors. The
//d <i>user</i> is used in reporting of errors. The max_size parameter is
//d the size of the memory array in number of locations, and is used to
//d malloc an area of memory for the data (a pointer to which is put in
//d *data_ptr). Each memory location contains 'bit_size' bits, and only
//d that number of bits from 'bit_start' onwards are put in the memory
//d from the MIF file. Finally, the callback is called on every address
//d that is written as it is written, so the caller may know which
//d locations have been set by the MIF file.
//d This function is the same as the previous one, except that the memory
//d is specified here to start at address 'address_offset'; so only
//d addresses in the MIF file from 'address_offset' to
//d 'address_offset+max_size' will be written to the memory, starting at
//d location 0 in the memory, of course.
 */
extern t_sl_error_level sl_mif_read_mif_file( c_sl_error *error,
                                              const char *filename,
                                              const char *user,
                                              t_sl_uint64 address_offset,
                                              t_sl_uint64 size, // size of memory - don't callback if address in file - address_offset is outside the range 0 to size-1
                                              int error_on_out_of_range, // add an error on every memory location that is out of range
                                              t_sl_mif_file_data_callback_fn callback,
                                              void *callback_handle  );

/*f sl_mif_allocate_and_read_mif_file
 */
extern t_sl_error_level sl_mif_allocate_and_read_mif_file( c_sl_error *error,
                                                           const char *filename,
                                                           const char *user,
                                                           int max_size,
                                                           int bit_size,
                                                           int bit_start,
                                                           int **data_ptr,
                                                           t_sl_mif_file_callback_fn callback,
                                                           void *handle  );

/*f sl_mif_allocate_and_read_mif_file
 */
extern t_sl_error_level sl_mif_allocate_and_read_mif_file( c_sl_error *error,
                                                           const char *filename,
                                                           const char *user,
                                                           int address_offset,
                                                           int max_size, 
                                                           int bit_size,
                                                           int bit_start,
                                                           int reset,
                                                           int reset_value,
                                                           int **data_ptr,
                                                           t_sl_mif_file_callback_fn callback,
                                                           void *handle  );

/*f sl_mif_allocate_and_read_mif_file
 */
extern t_sl_error_level sl_mif_reset_and_read_mif_file( c_sl_error *error, const char *filename, const char *user, int address_offset, int max_size, int bit_size, int bit_start, int reset, int reset_value, int *data, t_sl_mif_file_callback_fn callback, void *handle  );
/*a Wrapper
 */
#endif


/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

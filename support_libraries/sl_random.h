/*a Copyright
  
  This file 'sl_random.h' copyright Gavin J Stark 2003, 2004
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Wrapper
 */
#ifdef __INC_SL_RANDOM
#else
#define __INC_SL_RANDOM

/*a Includes
 */
#include "sl_general.h"

/*a Defines
 */

/*a Types
 */
typedef enum
{
    sl_random_type_cyclic, // from seed add 'n' modulo 'm' - n==0 implies constant
    sl_random_type_nial, // average of 'n' times equal distributed between 'x' and 'y' - n=1 implies flat
    sl_random_type_one_of_n, // flat distribution of 'n' possibe values, each given
} t_sl_random_type;
typedef struct t_sl_random
{
    t_sl_random_type type; // Type of this
    t_sl_uint64 seed[2]; // Not used in cyclic
    t_sl_uint64 value; // Last value returned
    t_sl_uint64 start; // Starting value - set to 0 for one_of_n
    t_sl_uint64 range; // Range or modulus - set to equal 'extra_values+1' for one_of_n
    int extra_values; // Number of extra values - 0 except for one_of_n
    union
    {
        t_sl_uint64 cyclic; // Value to add each time
        int nial; // Number of times to average over
        t_sl_uint64 one_of_n[1]; // Array of values to choose from
    } data;
} t_sl_random;

/*a External functions
 */
/*b Random functions
 */
extern void sl_random_setstate( t_sl_random *rnd, const char *state );
extern t_sl_uint64 sl_random_number( t_sl_random *rnd );

/*a Wrapper
 */
#endif

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


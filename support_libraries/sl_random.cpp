/*a Copyright
  
  This file 'sl_random.cpp' copyright Gavin J Stark 2003, 2004, 2007, 2008
  
  This is free software; you can redistribute it and/or modify it however you wish,
  with no obligations
  
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*a Includes
 */
#include <stdlib.h>
#include <string.h>
#include "sl_random.h"

/*a Types
 */

/*a External functions
 */
/*f flat
 */
static t_sl_uint64 flat( t_sl_random *rnd )
{
    rnd->seed[0] = rnd->seed[0]*0xA983625612736133LL + 0x9281482732131231LL;
    rnd->seed[1] = rnd->seed[1]*0x76352812376fea17LL + 0x9fd91273fdec1723LL;
    return ((rnd->seed[0]<<23)^(rnd->seed[0]>>41)^rnd->seed[1])%rnd->range;
}

/*f sl_random_setstate
 */
extern void sl_random_setstate( t_sl_random *rnd, const char *state )
{
    sscanf( state, "%llx.%llx.%llx",
            &(rnd->seed[0]),
            &(rnd->seed[1]),
            &(rnd->value) );
}

/*f sl_random_number
 */
extern t_sl_uint64 sl_random_number( t_sl_random *rnd )
{
    switch (rnd->type)
    {
    case sl_random_type_cyclic:
        rnd->value = ((rnd->value+rnd->data.cyclic)%rnd->range) + rnd->start;
        break;
    case sl_random_type_nial:
    {
        int i;
        t_sl_uint64 sum;
        sum=0;
        for (i=0; i<rnd->data.nial; i++)
        {
            sum += flat(rnd);
        }
        rnd->value = sum/rnd->data.nial + rnd->start;
        break;
    }
    case sl_random_type_one_of_n:
        t_sl_uint64 n;
        n = flat(rnd);
        rnd->value = rnd->data.one_of_n[n];
        break;
    }
    return rnd->value;
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/


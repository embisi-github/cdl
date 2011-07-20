/*a Copyright

This file 'hierarchy_reg.h' copyright Gavin J Stark 2003, 2004

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, version 2.0.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.
*/

/*a Includes
 */

/*a Module
 */
extern module hierarchy_gray_code_g2b( input bit[4] gc,
                                       output bit[4] bin )
{
    timing comb input gc;
    timing comb output bin;
}

extern module hierarchy_gray_code_b2g( input bit[4] bin,
                                       output bit[4] gc )
{
    timing comb input bin;
    timing comb output gc;
}

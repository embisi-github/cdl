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
extern module hierarchy_reg( clock int_clock,
                             input bit int_reset,
                             input bit in,
                             output bit out )
{
    timing to rising clock int_clock in, int_reset;
    timing from rising clock int_clock out;
}

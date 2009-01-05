/*a Copyright

This file 'hierarchy_struct.h' copyright Gavin J Stark 2003, 2004

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

/*a Types
 */
/*t t_h_substruct
 */
typedef struct
{
    bit x;
    bit y;
} t_h_substruct;

/*t t_h_struct
 */
typedef struct
{
    bit c;
    t_h_substruct s;
} t_h_struct;

/*a Module
 */
extern module hierarchy_struct( clock int_clock,
                                input bit int_reset,
                                input t_h_struct in,
                                output t_h_struct out )
{
    timing to rising clock int_clock in, int_reset;
    timing from rising clock int_clock out;
}

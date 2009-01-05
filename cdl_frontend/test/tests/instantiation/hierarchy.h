/*a Copyright

This file 'hierarchy.h' copyright Gavin J Stark 2003, 2004

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, version 2.0.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.
*/

/*a Module prototypes
 */
extern module hierarchy( clock int_clock,
                  input bit int_reset,
                  input bit clear,
                  input bit store,
                  input bit data,
                  output bit last_value,
                  output bit value,
                  output bit next_value
                  )
{
    timing to rising clock int_clock store, clear, data, int_reset;
    timing from rising clock int_clock last_value, value, next_value;
    timing comb input store, data, clear;
    timing comb output next_value;
}

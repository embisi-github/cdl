#a Copyright
#  
#  This file 'Makefile' copyright Gavin J Stark 2003, 2004
#  
#  This program is free software; you can redistribute it and/or modify it under
#  the terms of the GNU General Public License as published by the Free Software
#  Foundation, version 2.0.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
#  for more details.

#a Global variables
CYCLICITY := ../..

all:
	(cd ${CYCLICITY}/bin ; make )
	(cd build; make)

clean:
	(cd build; make clean)

allclean:
	(cd ${CYCLICITY}/bin ; make clean )
	(cd build; make clean)

verilog:
	(cd build; make verilog)

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[	 ]*[b-z][	 ]" ***
# End: ***


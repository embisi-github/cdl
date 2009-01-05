#a Copyright
#  
#  This file 'Makefile' copyright Gavin J Stark 2003, 2004
#  
#  This is free software; you can redistribute it and/or modify it however you wish,
#  with no obligations
#  
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE.

#a Global variables
CYCLICITY = ..
PREREQS = BSEH
include ${CYCLICITY}/scripts/makefile_hdr

ALL: be_model_includes.h

be_model_includes.h: ${CYCLICITY}/backend/be_model_includes.h
	cp ${CYCLICITY}/backend/be_model_includes.h be_model_includes.h

clean:
	rm -f be_model_includes.h

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***


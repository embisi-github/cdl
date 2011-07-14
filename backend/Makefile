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
CYCLICITY = ..
PREREQS = Si
include ${CYCLICITY}/scripts/makefile_hdr

#a Source files
BACKEND_SRCS = \
		c_model_descriptor.cpp \
		md_output_markers.cpp \
		md_target_c.cpp \
		md_target_xml.cpp \
		md_target_verilog.cpp
#		md_target_vhdl.cpp

SRCS = ${BACKEND_SRCS}

#a Objects
BACKEND_OBJS = $(call objs_from_srcs,${BACKEND_SRCS})

#a Resultant 'libraries'
BACKEND_LIB = ${OS_DIR}/be_backend.o

#a Dependencies and build requirements
ALL: ${BACKEND_LIB}

${BACKEND_LIB}: ${BACKEND_OBJS}
	${LINKASOBJ} $@ ${BACKEND_OBJS}

#a Clean etc
clean:
	rm -f ${CMDLINE_PROG} ${BACKEND_OBJS} ${OS_DIR}/*.P core.*

backup:
	tar zcvf backups/backup_`date +%y%m%d%H%M`.tgz Makefile ${SRCS} ${HDRS} ${UIS}

-include $(SRCS:%.cpp=${OS_DIR}/%.P)

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***


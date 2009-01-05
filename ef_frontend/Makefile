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
PREREQS = SB
include ${CYCLICITY}/scripts/makefile_hdr

#a Source files
FRONTEND_SRCS = \
		ef_frontend.cpp

SRCS = ${FRONTEND_SRCS}

#a Objects
FRONTEND_OBJS = $(call objs_from_srcs,${FRONTEND_SRCS})

#a Resultant 'libraries'
CMDLINE_PROG = ${OS_DIR}/ef

#a Dependencies and build requirements
ALL: ${CMDLINE_PROG}

${CMDLINE_PROG}: ${FRONTEND_OBJS} ${CYCLICITY_OBJS}
	${LINKASBIN} ${CMDLINE_PROG} ${FRONTEND_OBJS} ${CYCLICITY_OBJS}

${CYCLICITY_OBJS}:
	cd ${CYCLICITY}/lib ; make

#a Clean etc
clean:
	rm -f ${LIBRARIES} ${CMDLINE_PROG} ${FRONTEND_OBJS} ${OS_DIR}/*.P core.*

backup:
	tar zcvf backups/backup_`date +%y%m%d%H%M`.tgz Makefile ${SRCS} ${HDRS}

-include $(SRCS:%.cpp=${OS_DIR}/%.P)

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***


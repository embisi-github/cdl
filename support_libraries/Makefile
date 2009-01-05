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
PREREQS = 
include ${CYCLICITY}/scripts/makefile_hdr

#a Source files
LIB_SRCS = \
		sl_debug.cpp \
		sl_option.cpp \
		c_sl_error.cpp \
		sl_cons.cpp \
		sl_token.cpp \
		sl_mif.cpp \
		sl_indented_file.cpp \
		sl_exec_file.cpp \
		sl_ef_lib_memory.cpp \
		sl_ef_lib_fifo.cpp \
		sl_ef_lib_event.cpp \
		sl_ef_lib_random.cpp \
		sl_random.cpp \
		sl_data_stream.cpp \
		sl_fifo.cpp \
		sl_hier_mem.cpp \
		sl_general.cpp

SRCS = ${LIB_SRCS}

#a Objects
LIB_OBJS = $(call objs_from_srcs,${LIB_SRCS})

#a Resultant 'libraries'
LIBRARY = ${OS_DIR}/sl_support.o

#a Dependencies and build requirements
ALL: ${LIBRARY}

${LIBRARY}: ${LIB_OBJS}
	${LINKASOBJ} ${LIBRARY} ${LIB_OBJS}

#a Clean etc
clean:
	rm -f ${LIBRARY} ${LIB_OBJS} ${OS_DIR}/*.P core.*

backup:
	tar zcvf backups/backup_`date +%y%m%d%H%M`.tgz Makefile ${SRCS} ${HDRS} ${UIS}

-include $(SRCS:%.cpp=${OS_DIR}/%.P)

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***


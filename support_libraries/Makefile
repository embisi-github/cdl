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
BASIC_LIB_SRCS := \
		sl_debug.cpp \
		sl_option.cpp \
		c_sl_error.cpp \
		sl_cons.cpp \
		sl_token.cpp \
		sl_mif.cpp \
		sl_indented_file.cpp \
		sl_ef_lib_memory.cpp \
		sl_ef_lib_fifo.cpp \
		sl_ef_lib_event.cpp \
		sl_ef_lib_random.cpp \
		sl_random.cpp \
		sl_data_stream.cpp \
		sl_fifo.cpp \
		sl_hier_mem.cpp \
		sl_pthread_barrier.cpp \
		sl_general.cpp

SRCS = ${BASIC_LIB_SRCS}

#a Objects
BASIC_LIB_OBJS  := $(call objs_from_srcs,${BASIC_LIB_SRCS})
PLAIN_LIB_OBJS  := ${BASIC_LIB_OBJS} ${OS_DIR}/sl_exec_file.o
PYTHON_LIB_OBJS := ${BASIC_LIB_OBJS} ${OS_DIR}/sl_exec_file_python.o

#a Resultant 'libraries'
PLAIN_LIBRARY  := ${OS_DIR}/sl_support.o
PYTHON_LIBRARY := ${OS_DIR}/sl_support_with_python.o
ifeq ($(BUILD_PYTHON),no)
PYTHON_LIBRARY :=
endif

#a Dependencies and build requirements
ALL: ${PLAIN_LIBRARY} ${PYTHON_LIBRARY}

${PLAIN_LIBRARY}: ${PLAIN_LIB_OBJS}
	${LINKASOBJ} ${PLAIN_LIBRARY} ${PLAIN_LIB_OBJS}

${PYTHON_LIBRARY}: ${PYTHON_LIB_OBJS}
	${LINKASOBJ} ${PYTHON_LIBRARY} ${PYTHON_LIB_OBJS}

#a Special build objects
${OS_DIR}/sl_exec_file_python.o: sl_exec_file.cpp
	$(Q)$(CC) ${CXXFLAGS} -I${PYTHON_INCLUDES} -DSL_EXEC_FILE_PYTHON -c sl_exec_file.cpp -o ${OS_DIR}/sl_exec_file_python.o

#a Clean etc
clean:
	rm -f ${PLAIN_LIBRARY} ${PYTHON_LIBRARY} ${PLAIN_LIB_OBJS} ${PLAIN_LIB_OBJS} ${PYTHON_LIB_OBJS} ${OS_DIR}/*.P core.*

backup:
	tar zcvf backups/backup_`date +%y%m%d%H%M`.tgz Makefile ${SRCS} ${HDRS} ${UIS}

-include $(SRCS:%.cpp=${OS_DIR}/%.P)

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***


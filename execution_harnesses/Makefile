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
PREREQS = sei
include ${CYCLICITY}/scripts/makefile_hdr

#a Source files
CMDLINE_SRCS = \
		eh_scripting_none.cpp \
		eh_batch_mode.cpp

PYTHON_SRCS = \
	eh_scripting_python.cpp \
	eh_py_engine.cpp

VPI_SRCS = eh_vpi.cpp

SRCS := ${CMDLINE_SRCS}
ifneq ($(BUILD_PYTHON),no)
SRCS := ${SRCS} ${PYTHON_SRCS}
endif
ifneq ($(BUILD_VPI),no)
SRCS := ${SRCS} ${VPI_SRCS}
endif

#a Objects
CMDLINE_OBJS = $(call objs_from_srcs,${CMDLINE_SRCS})
PYTHON_OBJS = $(call objs_from_srcs,${PYTHON_SRCS})
VPI_OBJS = $(call objs_from_srcs,${VPI_SRCS})

#a Resultant 'libraries'
CMDLINE_LIB := ${OS_DIR}/eh_batch_scripting_harness.o
PYTHON_LIB := ${OS_DIR}/eh_python_scripting_harness.o
VPI_LIB := ${OS_DIR}/eh_vpi_harness.o
ifeq ($(BUILD_PYTHON),no)
	PYTHON_LIB =
endif
ifeq ($(BUILD_VPI),no)
	VPI_LIB =
endif

LIBRARIES = ${PYTHON_LIB} ${CMDLINE_LIB} ${VPI_LIB}

#a Dependencies and build requirements
ALL: ${LIBRARIES}

SPECIFIC_INCLUDES := 
ifneq ($(BUILD_PYTHON),no)
SPECIFIC_INCLUDES := ${SPECIFIC_INCLUDES} -DSL_PYTHON -I${PYTHON_INCLUDES}
endif
ifneq ($(BUILD_VPI),no)
SPECIFIC_INCLUDES := ${SPECIFIC_INCLUDES} -I${VPI_INCLUDES}
endif

${CMDLINE_LIB}: $(CMDLINE_OBJS)
	${LINKASOBJ} $@ $(CMDLINE_OBJS)

${PYTHON_LIB}: $(PYTHON_OBJS)
	${LINKASOBJ} $@ ${PYTHON_OBJS}

${VPI_LIB}: $(VPI_OBJS)
	${LINKASOBJ} $@ ${VPI_OBJS}

#a Clean etc
clean:
	rm -f ${LIBRARIES} ${CMDLINE_OBJS} ${PYTHON_OBJS} ${VPI_OBJS} ${OS_DIR}/*.P core.*

backup:
	tar zcvf backups/backup_`date +%y%m%d%H%M`.tgz Makefile ${SRCS}

-include $(SRCS:%.cpp=${OS_DIR}/%.P)

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***


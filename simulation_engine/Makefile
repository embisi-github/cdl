#a Copyright
#  
#  This file 'Makefile' copyright Gavin J Stark 2003, 2004
#  
#  This is free software; you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License as published by the Free Software
#  Foundation, version 2.1.
#  
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even implied warranty of MERCHANTABILITY
#  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
#  for more details.

#a Global variables
CYCLICITY = ..
PREREQS = S

include ${CYCLICITY}/scripts/makefile_hdr

SPECIFIC_INCLUDES =
ifeq ("$(BUILD_PYTHON)","yes")
SPECIFIC_INCLUDES += -D SE_ENGINE_PYTHON -DSL_PYTHON -I${PYTHON_INCLUDES}
endif

#a Source files
ENGINE_SRCS = \
		se_cmodel_assist.cpp \
		se_internal_module.cpp \
		se_internal_module__ddr.cpp \
		se_internal_module__sram_srw.cpp \
		se_internal_module__sram_mrw.cpp \
		se_internal_module__logger.cpp \
		se_internal_module__test_harness.cpp \
		se_external_module.cpp \
		se_engine_function.cpp \
		c_se_engine__checkpoint_restore.cpp \
		c_se_engine__interrogation.cpp \
		c_se_engine__instantiation.cpp \
		c_se_engine__registration.cpp \
		c_se_engine__submodule.cpp \
		c_se_engine__simulation.cpp \
		c_se_engine__waveform.cpp \
		c_se_engine__coverage.cpp \
		c_se_engine__log.cpp \
		c_se_engine.cpp

SRCS = ${ENGINE_SRCS}

#a Objects
ENGINE_OBJS = $(call objs_from_srcs,${ENGINE_SRCS})

#a Resultant 'libraries'
SE_LIB = ${OS_DIR}/se_simulation_engine.o
LIBRARIES = ${SE_LIB}

#a Dependencies and build requirements
ALL: ${LIBRARIES}

${SE_LIB}: ${ENGINE_OBJS}
	${LINKASOBJ} $@ ${ENGINE_OBJS}
	#	${LINKDYNLIB} $@ ${ENGINE_OBJS} ${SUPPORT_WITH_PYTHON_OBJECTS}

#a Clean, depend, etc
clean:
	rm -f ${LIBRARIES} ${CMDLINE_OBJS} ${PYTHON_OBJS} ${ENGINE_OBJS} ${OS_DIR}/*.P core.*

backup:
	tar zcvf backups/backup_`date +%y%m%d%H%M`.tgz Makefile ${SRCS}

-include $(SRCS:%.cpp=${OS_DIR}/%.P)

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***


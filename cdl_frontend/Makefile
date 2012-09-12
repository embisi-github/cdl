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
PREREQS = SBi
include ${CYCLICITY}/scripts/makefile_hdr

#a Source files
CYC_SRCS = \
	cyclicity_grammar.y \
	c_type_value_pool.cpp \
	c_cyc_object.cpp \
	c_co_constructors.cpp \
	c_co_chain.cpp \
	c_co_expression.cpp \
	c_co_toplevel_cross_reference.cpp \
	c_co_toplevel_evaluate.cpp \
	c_co_module_cross_reference_pre_type_resolution.cpp \
	c_co_module_type_resolution.cpp \
	c_co_module_cross_reference_post_type_resolution.cpp \
	c_co_module_check_types.cpp \
	c_co_module_evaluate_constant_expressions.cpp \
	c_co_module_high_level_checks.cpp \
	c_co_build_model.cpp \
	c_lexical_analyzer.cpp \
	c_cyclicity.cpp

#a Objects
CYC_OBJS = $(call objs_from_srcs,${CYC_SRCS})

CMD_SRCS = ${CYC_SRCS} cyclicity.cpp
CMD_OBJS = $(call objs_from_srcs,${CMD_SRCS})

PY_SRCS = ${CYC_SRCS} py_cyclicity.cpp
PY_OBJS = $(call objs_from_srcs,${CMD_SRCS})

SRCS =  ${CYC_SRCS} py_cyclicity.cpp cyclicity.cpp

#a Resultant 'libraries'
CMDLINE_PROG = ${OS_DIR}/cdl
PY_LIB = ${OS_DIR}/py_cdl.${LIB_EXT}
ifeq ($(BUILD_PYTHON),no)
	PY_LIB =
endif

#a Dependencies and build requirements
all: libraries ALL

libraries:
	(cd ../lib ; make)

# We put in cyclicity_grammar.h here so that it is built first. Its a hack.
ALL: cyclicity_grammar.h ${CMDLINE_PROG} ${PY_LIB}

# ${BACKEND_OBJ}

${BACKEND_OBJ}: ${LANGUAGE_OBJS}
	${LINKASBIN} ${BACKEND_OBJ} ${LANGUAGE_OBJS}

${CMDLINE_PROG}: $(CMD_OBJS) $(CYCLICITY_OBJS)
	${LINKASBIN} ${CMDLINE_PROG} $(CMD_OBJS) ${CYCLICITY_OBJS} ${LD_LIBS}

${PY_LIB}: $(PY_OBJS)
	${LINKDYNLIB} ${PY_LIB} ${PY_OBJS} ${CYCLICITY_OBJS} ${LD_LIBS}


cyclicity_grammar.h: cyclicity_grammar.y
ifeq (${BISON_VERSION},2.5)
	bison -v --defines=$@ cyclicity_grammar.y
else
	bison -v -d -o ignore.me$(BISON_TAIL) cyclicity_grammar.y
	mv ignore.me.h cyclicity_grammar.h
	rm -f ignore.me ignore.me.output
endif

cyclicity_grammar.o: cyclicity_grammar.cpp cyclicity_grammar.h

cyclicity_grammar.cpp: cyclicity_grammar.y
	rm -f cyclicity_grammar.cpp
	bison --debug cyclicity_grammar.y -o cyclicity_grammar.cpp

#a Clean, depend, etc
clean:
	rm -f ${CMD_OBJS} ${PY_OBJS} ${CMDLINE_PROG} ${PY_LIB} cyclicity_grammar.h cyclicity_grammar.cpp ${OS_DIR}/*.P core.*

backup:
	tar zcvf backups/backup_`date +%y%m%d%H%M`.tgz Makefile *.y *.h *.cpp *.txt

$DEPFILES := $(patsubst %.y,${OS_DIR}/%.P,$(SRCS:%.cpp=${OS_DIR}/%.P))

-include $DEPFILES

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[	 ]*[b-z][	 ]" ***
# End: ***


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

#a Targets
ALL: subsystem ${BACKEND_OBJECTS} ${ENGINE_OBJECTS} ${SUPPORT_OBJECTS} ${HARNESS_BATCH_OBJECTS} ${HARNESS_GUI_OBJECTS} ${HARNESS_VPI_OBJECTS}

libraries:
	cd ${CYCLICITY}/support_libraries ; make

subsystem: libraries ${SUPPORT_OBJECTS}
	cd ${CYCLICITY}/backend ; make
	cd ${CYCLICITY}/simulation_engine ; make
	cd ${CYCLICITY}/execution_harnesses ; make

${BACKEND_OBJECTS}: ${CYCLICITY}/backend/${OS_DIR}/be_backend.o ${SUPPORT_OBJECTS}
	cp ${CYCLICITY}/backend/${OS_DIR}/be_backend.o ${BACKEND_OBJECTS}

${ENGINE_OBJECTS}: ${CYCLICITY}/simulation_engine/${OS_DIR}/se_simulation_engine.o ${SUPPORT_OBJECTS}
	cp ${CYCLICITY}/simulation_engine/${OS_DIR}/se_simulation_engine.o ${ENGINE_OBJECTS}

ifeq ($(BUILD_PYTHON),no)
${SUPPORT_OBJECTS}: ${CYCLICITY}/support_libraries/${OS_DIR}/sl_support.o
	cp ${CYCLICITY}/support_libraries/${OS_DIR}/sl_support.o ${SUPPORT_OBJECTS}
else
${SUPPORT_OBJECTS}: ${CYCLICITY}/support_libraries/${OS_DIR}/sl_support.o ${CYCLICITY}/support_libraries/${OS_DIR}/sl_support_with_python.o
	cp ${CYCLICITY}/support_libraries/${OS_DIR}/sl_support.o ${SUPPORT_OBJECTS}
	cp ${CYCLICITY}/support_libraries/${OS_DIR}/sl_support_with_python.o ${SUPPORT_WITH_PYTHON_OBJECTS}
endif

${HARNESS_BATCH_OBJECTS}: ${CYCLICITY}/execution_harnesses/${OS_DIR}/eh_batch_scripting_harness.o
	cp ${CYCLICITY}/execution_harnesses/${OS_DIR}/eh_batch_scripting_harness.o ${HARNESS_BATCH_OBJECTS}

${HARNESS_GUI_OBJECTS}: ${CYCLICITY}/execution_harnesses/${OS_DIR}/eh_python_scripting_harness.o
	cp ${CYCLICITY}/execution_harnesses/${OS_DIR}/eh_python_scripting_harness.o ${HARNESS_GUI_OBJECTS}

${HARNESS_VPI_OBJECTS}: ${CYCLICITY}/execution_harnesses/${OS_DIR}/eh_vpi_harness.o
	cp ${CYCLICITY}/execution_harnesses/${OS_DIR}/eh_vpi_harness.o ${HARNESS_VPI_OBJECTS}


clean:
	cd ${CYCLICITY}/backend ; make clean
	cd ${CYCLICITY}/simulation_engine ; make clean
	cd ${CYCLICITY}/support_libraries ; make clean
	rm -f ${BACKEND_OBJECTS} ${ENGINE_OBJECTS} ${SUPPORT_OBJECTS} ${HARNESS_BATCH_OBJECTS} ${HARNESS_GUI_OBJECTS}

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***


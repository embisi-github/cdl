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
PREREQS = BSE
include ${CYCLICITY}/scripts/makefile_hdr

EF = ${OS_DIR}/ef
CDL = ${OS_DIR}/cdl
BINS = ${EF} ${CDL}

#a Targets
ALL: subsystem ${BINS}

subsystem:
	cd ${CYCLICITY}/include; make
	cd ${CYCLICITY}/lib; make
	cd ${CYCLICITY}/ef_frontend ; make
	cd ${CYCLICITY}/cdl_frontend ; make

ef:
	cd ${CYCLICITY}/lib; make
	cd ${CYCLICITY}/ef_frontend ; make
	cp ${CYCLICITY}/ef_frontend/${OS_DIR}/ef ${EF}

cdl:
	cd ${CYCLICITY}/lib; make
	cd ${CYCLICITY}/cdl_frontend ; make
	cp ${CYCLICITY}/cdl_frontend/${OS_DIR}/cdl ${CDL}

${EF}: ${CYCLICITY}/ef_frontend/${OS_DIR}/ef
	cp ${CYCLICITY}/ef_frontend/${OS_DIR}/ef ${EF}

${CDL}: ${CYCLICITY}/cdl_frontend/${OS_DIR}/cdl
	cp ${CYCLICITY}/cdl_frontend/${OS_DIR}/cdl ${CDL}

clean:
	rm -f ${EF} ${CDL}

allclean:
	cd ${CYCLICITY}/bin; make clean
	cd ${CYCLICITY}/include; make clean
	cd ${CYCLICITY}/lib; make clean
	cd ${CYCLICITY}/ef_frontend ; make clean
	cd ${CYCLICITY}/cdl_frontend ; make clean
	cd ${CYCLICITY}/support_libraries ; make clean
	cd ${CYCLICITY}/simulation_frontend ; make clean
	cd ${CYCLICITY}/execution_harnesses ; make clean

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***


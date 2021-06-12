#!/bin/sh

### Constants
c_valgrind_min=1

### Actual command
scenario_cmd() {
	# Check non-pthread
	cd ${scriptdir}/noeintr_close/normal || exit

	setup_check_variables "test_noeintr_close_normal /dev/null 0"
	${c_valgrind_cmd}					\
	    ./test_noeintr_close_normal /dev/null 0		\
	    1> "${s_basename}-stdout-0.txt"
	echo "$?" > ${c_exitfile}

	setup_check_variables "test_noeintr_close_normal Makefile 0"
	${c_valgrind_cmd}					\
	    ./test_noeintr_close_normal Makefile 0		\
	    1> "${s_basename}-stdout-1.txt"
	echo "$?" > ${c_exitfile}

	setup_check_variables "test_noeintr_close_normal Makefile 1"
	${c_valgrind_cmd}					\
	    ./test_noeintr_close_normal Makefile 1		\
	    1> "${s_basename}-stdout-2.txt"
	echo "$?" > ${c_exitfile}

	# Check pthread
	cd ${scriptdir}/noeintr_close/pthread || exit

	setup_check_variables "test_noeintr_close_pthread /dev/null 0"
	${c_valgrind_cmd}					\
	    ./test_noeintr_close_pthread /dev/null 0		\
	    1> "${s_basename}-stdout-3.txt"
	echo "$?" > ${c_exitfile}

	setup_check_variables "test_noeintr_close_pthread Makefile 0"
	${c_valgrind_cmd}					\
	    ./test_noeintr_close_pthread Makefile 0		\
	    1> "${s_basename}-stdout-4.txt"
	echo "$?" > ${c_exitfile}

	setup_check_variables "test_noeintr_close_pthread Makefile 1"
	${c_valgrind_cmd}					\
	    ./test_noeintr_close_pthread Makefile 1		\
	    1> "${s_basename}-stdout-5.txt"
	echo "$?" > ${c_exitfile}
}

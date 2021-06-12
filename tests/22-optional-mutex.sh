#!/bin/sh

### Constants
c_valgrind_min=1

### Actual command
scenario_cmd() {
	setup_check_variables "test_optional_mutex_normal"
	cd ${scriptdir}/optional_mutex/normal || exit
	${c_valgrind_cmd} ./test_optional_mutex_normal
	echo "$?" > ${c_exitfile}

	setup_check_variables "test_optional_mutex_pthread"
	cd ${scriptdir}/optional_mutex/pthread || exit
	${c_valgrind_cmd} ./test_optional_mutex_pthread
	echo "$?" > ${c_exitfile}
}

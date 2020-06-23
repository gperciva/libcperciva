#!/bin/sh

### Constants
c_valgrind_min=1
test_output="${s_basename}-stdout.txt"
test_emptyloop_output="${s_basename}-stdout-emptyloop.txt"
pidfile="${s_basename}-pidfile.txt"
flag_1="${s_basename}-1.flag"

### Actual command
scenario_cmd() {
	cd ${scriptdir}/events

	setup_check_variables "test_events"
	${c_valgrind_cmd}			\
	    ./test_events 1> ${test_output}
	echo "$?" > ${c_exitfile}

	setup_check_variables "test_events output against reference"
	cmp -s ${scriptdir}/events/test-events.good ${test_output}
	echo "$?" > ${c_exitfile}

	setup_check_variables "test_events without events"
	# Run a loop without any events
	(
		${c_valgrind_cmd}					\
			./test_events ${pidfile} > ${test_emptyloop_output}
		echo "$?" > ${c_exitfile}
		# Finished writing the logfile
		touch ${flag_1}
	) &

	# Wait for the binary to initialize its signal handler
	wait_for_file ${pidfile}
	pid=$( cat ${pidfile} )

	# Signal and wait for the binary to finish writing the logfile
	kill -s USR1 ${pid}
	wait_for_file ${flag_1}

	# Compare with good values
	setup_check_variables "test-empty-events"
	cmp -s test-empty-events.good ${test_emptyloop_output}
	echo "$?" > ${c_exitfile}
}

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "noeintr.h"
#include "parsenum.h"
#include "warnp.h"

#include "open_close.h"

/* Wait duration can be interrupted by signals. */
static int
wait_ms(size_t msec)
{
	struct timespec ts;

	/* Try to wait for the desired duration. */
	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	nanosleep(&ts, NULL);

	/* Success! */
	return (0);
}

int
main(int argc, char * argv[])
{
	/* Command-line arguments. */
	const char * filename;
	int nbytes;

	/* Working variables. */
	pthread_t open_close_thread[NUM_THREADS];
	struct open_close * OC[NUM_THREADS];
	volatile int stop_open_close = 0;
	int rc;
	size_t i;

	WARNP_INIT;

	/* Process arguments. */
	if (argc != 3) {
		fprintf(stderr, "usage: %s filename nbytes\n", argv[0]);
		goto err0;
	}
	filename = argv[1];
	if (PARSENUM(&nbytes, argv[2], 0, 1)) {
		warn0("%s must be 0 or 1", argv[2]);
		goto err0;
	}

	/* Indicate that we have no open_close threads yet. */
	for (i = 0; i < NUM_THREADS; i++)
		OC[i] = NULL;

	/* Start threads to open, read 0 or 1 bytes, then close the file. */
	for (i = 0; i < NUM_THREADS; i++) {
		if ((OC[i] = open_close_start(filename, nbytes,
		    &stop_open_close, &open_close_thread[i])) == NULL)
			goto err1;
	}

	/* Wait for the thread(s) to start, do some open & close operations. */
	wait_ms(100);

	/* Stop thread(s). */
	stop_open_close = 1;
	for (i = 0; i < NUM_THREADS; i++) {
		if ((rc = pthread_join(open_close_thread[i], NULL)) != 0) {
			warn0("pthread_join: %s", strerror(rc));
			goto err1;
		}
	}

	/* Check for any errors and free memory. */
	for (i = 0; i < NUM_THREADS; i++) {
		if (open_close_cleanup(OC[i])) {
			OC[i] = NULL;
			goto err1;
		}
		OC[i] = NULL;
	}

	/* Success! */
	exit(0);

err1:
	/* Free memory. */
	for (i = 0; i < NUM_THREADS; i++)
		open_close_cleanup(OC[i]);
err0:
	/* Failure! */
	exit(1);
}

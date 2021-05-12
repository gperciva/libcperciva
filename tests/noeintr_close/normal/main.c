#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "noeintr.h"
#include "parsenum.h"
#include "warnp.h"

#include "open_close.h"

/* Forward reference; keep in sync with noeintr_close.c. */
void noeintr_close_print_evil_close_stats(void);

int
main(int argc, char * argv[])
{
	/* Command-line arguments. */
	const char * filename;
	int nbytes;

	/* Working variables. */
	int i;

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

	/* Open and close a file descriptor a bunch of times. */
	for (i = 0; i < 100; i++) {
		if (open_close(filename, nbytes)) {
			warnp("open_close");
			goto err0;
		}
	}

	/*
	 * Print stats.  It's expected that the sum of these values will not
	 * equal the number of open_close() function calls, because the whole
	 * point of noeintr_close's evil_close() is that calling it might not
	 * result in the file descriptor being closed.
	 */
	noeintr_close_print_evil_close_stats();

	/* Success! */
	exit(0);

err0:
	/* Failure! */
	exit(1);
}

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "noeintr.h"
#include "warnp.h"

#include "open_close.h"

struct open_close {
	const char * filename;
	int nbytes;
	volatile int * stop;
	size_t nopen_close;
	int rc;
};

static void *
workthread_open_close(void * cookie)
{
	struct open_close * OC = cookie;
	int fd;
	char buf[1];
	ssize_t r;

	while (*OC->stop == 0) {
		/* Open the file. */
		if ((fd = open(OC->filename, O_RDONLY)) == -1) {
			warnp("open(%s)", OC->filename);
			goto err0;
		}

		/*
		 * Read from the file.  If nbytes is 0, some platforms will
		 * still be able to detect certain problems with fd.
		 */
		if ((r = read(fd, buf, (size_t)OC->nbytes)) == -1) {
			warnp("read");
			goto err1;
		}
		if (r != (ssize_t)OC->nbytes) {
			warn0("read wanted %i byte(s); got %zi", OC->nbytes, r);
			goto err1;
		}

		/* Close the file. */
		if (noeintr_close(fd)) {
			warnp("noeintr_close");
			goto err0;
		}

		/* Update count. */
		OC->nopen_close++;
	}

	/* Success! */
	return (NULL);

err1:
	close(fd);
err0:
	/* Failure! */
	OC->rc = -1;
	return (NULL);
}

/*
 * start_repeated_open_close(filename, nbytes, stop, thr):
 * Start a thread, stored in ${thr}, to repeatedly open ${filename}, read
 * ${nbytes}, then close it.  Stop when ${*stop} is non-zero.  ${nbytes} must
 * be 0 or 1.
 */
struct open_close *
open_close_start(const char * filename, int nbytes,
    volatile int * stop, pthread_t * thr)
{
	struct open_close * OC;
	int rc;

	/* Sanity check. */
	assert((nbytes == 0) || (nbytes == 1));

	/* Allocate structure. */
	if ((OC = malloc(sizeof(struct open_close))) == NULL)
		goto err0;
	OC->filename = filename;
	OC->nbytes = nbytes;
	OC->stop = stop;
	OC->nopen_close = 0;
	OC->rc = 0;

	/* Create thread. */
	if ((rc = pthread_create(thr, NULL, workthread_open_close, OC)) != 0) {
		warn0("pthread_create: %s", strerror(rc));
		goto err1;
	}

	/* Success! */
	return (OC);

err1:
	free(OC);
err0:
	/* Failure! */
	return (NULL);
}

/*
 * open_close_cleanup(OC):
 * Check ${OC} for any previous errors, print statistics, and free memory.
 */
int
open_close_cleanup(struct open_close * OC)
{

	/* Bail if we've already cleaned up. */
	if (OC == NULL)
		return (0);

	/* Check for any prevoius errors. */
	if (OC->rc != 0)
		goto err1;

	/* Print statistics. */
	printf("Opened and closed the file %zu times.\n", OC->nopen_close);

	/* Clean up. */
	free(OC);

	/* Success! */
	return (0);

err1:
	free(OC);

	/* Failure! */
	return (-1);
}
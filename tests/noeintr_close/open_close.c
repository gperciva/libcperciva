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

/**
 * open_close(filename, nbytes):
 * Open the ${filename}, read ${bytes} (which should be 0 or 1), then close it.
 */
int
open_close(const char * filename, int nbytes)
{
	int fd;
	char buf[1];
	ssize_t r;

	/* Open the file. */
	if ((fd = open(filename, O_RDONLY)) == -1) {
		warnp("open(%s)", filename);
		goto err0;
	}

	/*
	 * Read from the file.  If nbytes is 0, some platforms will
	 * still be able to detect certain problems with fd.
	 */
	if ((r = read(fd, buf, (size_t)nbytes)) == -1) {
		warnp("read");
		goto err1;
	}
	if (r != (ssize_t)nbytes) {
		warn0("read wanted %i byte(s); got %zi", nbytes, r);
		goto err1;
	}

	/* Close the file. */
	if (noeintr_close(fd)) {
		warnp("noeintr_close");
		goto err0;
	}

	/* Success! */
	return (0);

err1:
	close(fd);
err0:
	/* Failure! */
	return (-1);
}

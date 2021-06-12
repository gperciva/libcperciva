#include <sys/socket.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "entropy.h"
#include "warnp.h"

#include "noeintr.h"

/* Sanity check compiling. */
#ifndef NOEINTR_CLOSE_PTHREAD
#error "Do not use this file directly; compile _normal.c or _pthread.c instead"
#endif

/* Handle multithreading (if applicable). */
#if NOEINTR_CLOSE_PTHREAD
#include <pthread.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#define MAGIC_COOKIE_LEN 32

static enum noeintr_close_status {
	NOEINTR_UNINITIALIZED,
	NOEINTR_INITIALIZED,
	NOEINTR_FATAL
} status = NOEINTR_UNINITIALIZED;

static int s[2] = {-1, -1};
static uint8_t cookie_write[MAGIC_COOKIE_LEN];
static struct entropy_read_cookie * entropy_read_cookie = NULL;

#ifdef NOEINTR_CLOSE_TESTING
/*
 * POSIX is silent about the state of fd if close(2) fails with errno set to
 * EINTR.  For internal testing, this function randomly selects among the
 * behaviours which are permitted by the standard.  For more info, see:
 * https://www.daemonology.net/blog/2011-12-17-POSIX-close-is-broken.html
 * https://www.austingroupbugs.net/view.php?id=529
 */
static int
evil_close(int fd)
{

	switch (random() % 3) {
	case 0:
		/* Normal close(). */
		return (close(fd));
	case 1:
		/*
		 * Fail with EINTR, without changing the open file
		 * description to which fd refers.
		 */
		errno = EINTR;
		return (-1);
	case 2:
		/* Normal close(), then fail with EINTR. */
		close(fd);
		errno = EINTR;
		return (-1);
	}

	/* UNREACHABLE. */
	abort();
}
#endif

static int
lock(void)
{
#if NOEINTR_CLOSE_PTHREAD
	int rc;

	if ((rc = pthread_mutex_lock(&mutex)) != 0) {
		/* Failure! */
		warn0("pthread_mutex_lock: %s", strerror(rc));
		return (-1);
	}
#endif

	/* Success with pthread, or non-pthread always indicate success. */
	return (0);
}

static int
unlock(void)
{
#if NOEINTR_CLOSE_PTHREAD
	int rc;

	if ((rc = pthread_mutex_unlock(&mutex)) != 0) {
		/* Failure! */
		warn0("pthread_mutex_unlock: %s", strerror(rc));
		return (-1);
	}
#endif

	/* Success with pthread, or non-pthread always indicate success. */
	return (0);
}

static void
noeintr_close_atexit(void)
{

	/*
	 * Clean up the entropy_read_cookie; this may call noeintr_close().
	 * As such, it must be handled before freeing any other resources.
	 */
	if (entropy_read_cookie != NULL)
		entropy_read_done(entropy_read_cookie);

	/* Attempt to close s[0] and s[1] using the normal close(). */
	if ((s[0] != -1) && close(s[0]))
		warnp("close");
	if ((s[1] != -1) && close(s[1]))
		warnp("close");
}

static int
noeintr_close_init(void)
{

	/* Create a socket pair. */
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, s)) {
		warnp("socketpair");
		goto err1;
	}

	/* Set up cleanup function. */
	if (atexit(noeintr_close_atexit)) {
		warnp("atexit");
		goto err2;
	}

	/* Entropy cookie; don't clean it until noeintr_close_atexit(). */
	if ((entropy_read_cookie = entropy_read_init()) == NULL) {
		warn0("entropy_read_init");
		goto err2;
	}

	/* Generate a random cookie. */
	if (entropy_read_fill(entropy_read_cookie, cookie_write,
	    MAGIC_COOKIE_LEN)) {
		warn0("entropy_read");
		goto err2;
	}

	/* Write the cookie to s[0]. */
	if (noeintr_write(s[0], cookie_write, MAGIC_COOKIE_LEN)
	    != MAGIC_COOKIE_LEN) {
		warnp("noeintr_write");
		goto err2;
	}

	/* We've finished initializing. */
	status = NOEINTR_INITIALIZED;

	/* Success! */
	return (0);

err2:
	/* Attempt to close s[0] and s[1] using the normal close(). */
	if (close(s[0]))
		warnp("close");
	s[0] = -1;
	if (close(s[1]))
		warnp("close");
	s[1] = -1;
err1:
	status = NOEINTR_FATAL;

	/* Failure! */
	return (-1);
}

/**
 * noeintr_close(fd):
 * Close the file descriptor ${fd} per the close(2) system call, but handle
 * EINTR appropriately.
 */
int
noeintr_close(int fd)
{
	uint8_t cookie_recv[MAGIC_COOKIE_LEN];
	ssize_t r;

	/* In a multithreaded environment, lock the mutex here. */
	if (lock())
		goto err0;

	/* If we failed to initialize, don't try again. */
	if (status == NOEINTR_FATAL)
		goto err1;

	/* Initialize (if needed). */
	if ((status == NOEINTR_UNINITIALIZED) && noeintr_close_init()) {
		warn0("noeintr_close_init");
		goto err1;
	}

	/* Prepare a duplicate file descriptor. */
	while (dup2(s[1], fd) == -1) {
		if (errno == EINTR) {
			/*
			 * POSIX specifies that the file descriptor fd is
			 * unchanged, so we can retry.
			 */
			continue;
		}

		warnp("dup2");
		goto err1;
	}

	/* Close the descriptor, which now refers to the same socket at s[1]. */
#ifdef NOEINTR_CLOSE_TESTING
	while (evil_close(fd)) {
#else
	while (close(fd)) {
#endif
		/*
		 * Since fd is now a copy of s[1], the only way this close()
		 * should be able to fail is with EINTR.
		 */
		if (errno != EINTR)
			goto err1;

		/*
		 * Check if EINTR was returned with the descriptor still
		 * referring to the same socket as s[1].
		 */
		errno = 0;
		while ((r = recv(fd, &cookie_recv, MAGIC_COOKIE_LEN, MSG_PEEK))
		    != MAGIC_COOKIE_LEN) {
			/*
			 * If recv() didn't read the full value but doesn't
			 * have an errno, try again.
			 */
			if (errno == 0)
				continue;

			/*
			 * If the descriptor is not open to a socket, it must
			 * have been closed.
			 */
			if ((errno == EBADF) || (errno == ENOTSOCK))
				goto done;

			/* Handle non-EINTR errors. */
			if (errno != EINTR) {
				warnp("recv");
				goto err1;
			}
		}

		/* Check if it's the cookie. */
		if ((memcmp(cookie_write, cookie_recv, MAGIC_COOKIE_LEN)
		    != 0)) {
			/*
			 * If this descriptor doesn't have the cookie we
			 * stuffed into s[0], it no longer refers to the same
			 * file as s[1]; the close call must have succeeded.
			 */
			goto done;
		}
	}

done:
	/* In a multithreaded environment, unlock the mutex here. */
	if (unlock())
		goto err0;

	/* Success! */
	return (0);

err1:
	(void)unlock();
err0:
	/* Failure! */
	return (-1);
}

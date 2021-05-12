#ifndef _OPEN_CLOSE_THREADED_H_
#define _OPEN_CLOSE_THREADED_H_

#include <pthread.h>

/* Opaque structure. */
struct open_close;

/*
 * open_close_threaded_start(filename, nbytes, stop, thr):
 * Start a thread, stored in ${thr}, to repeatedly open ${filename}, read
 * ${nbytes}, then close it.  Stop when ${*stop} is non-zero.  ${nbytes} must
 * be 0 or 1.
 */
struct open_close * open_close_threaded_start(const char *, int,
    volatile int * stop, pthread_t *);

/*
 * open_close_threaded_cleanup(OC):
 * Check ${OC} for any previous errors, print statistics, and free memory.
 */
int open_close_threaded_cleanup(struct open_close *);

#endif /* !_OPEN_CLOSE_THREADED_H_ */

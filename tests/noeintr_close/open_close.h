#ifndef _OPEN_CLOSE_H_
#define _OPEN_CLOSE_H_

#include <pthread.h>

/* Opaque structure. */
struct open_close;

/*
 * start_repeated_open_close(filename, nbytes, stop, thr):
 * Start a thread, stored in ${thr}, to repeatedly open ${filename}, read
 * ${nbytes}, then close it.  Stop when ${*stop} is non-zero.  ${nbytes} must
 * be 0 or 1.
 */
struct open_close * open_close_start(const char *, int,
    volatile int * stop, pthread_t *);

/*
 * open_close_cleanup(OC):
 * Check ${OC} for any previous errors, print statistics, and free memory.
 */
int open_close_cleanup(struct open_close *);

#endif /* !_OPEN_CLOSE_H_ */

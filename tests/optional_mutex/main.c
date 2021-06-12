#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "optional_mutex.h"
#include "warnp.h"

#define NUM_TIMES 100

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int value = 0;
static int counter = 0;

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

static void *
inc_dec_counter(void * cookie)
{
	int i;
	int rc;

	(void)cookie; /* UNUSED */

	for (i = 0; i < NUM_TIMES; i++) {
		/* Lock if we're using pthread. */
		if ((rc = optional_mutex_lock(&mutex)) != 0) {
			warn0("optional_mutex_lock: %s", strerror(rc));
			goto err0;
		}

		/* Increment, wait, decrement value. */
		value++;
		wait_ms(10);
		value--;

		/* Increment overall counter. */
		counter++;

		/* Unlock if we're using pthread. */
		if ((rc = optional_mutex_unlock(&mutex)) != 0) {
			warn0("optional_mutex_unlock: %s", strerror(rc));
			goto err0;
		}
	}

	/* Success! */
	return (NULL);

err0:
	/* Failure! */
	pthread_exit(NULL);
}

#ifdef NUM_THREADS
static int
test_pthread(void)
{
	pthread_t thr[NUM_THREADS];
	int rc;
	int i;

	/* Create threads. */
	for (i = 0; i < NUM_THREADS; i++) {
		if ((rc = pthread_create(&thr[i], NULL, inc_dec_counter, NULL))
		    != 0) {
			warn0("pthread_create: %s", strerror(rc));
			goto err0;
		}
	}

	/* Wait for threads to finish. */
	for (i = 0; i < NUM_THREADS; i++) {
		if ((rc = pthread_join(thr[i], NULL)) != 0) {
			warn0("pthread_join: %s", strerror(rc));
			goto err0;
		}
	}

	/* Success! */
	return (0);

err0:
	/* Failure! */
	return (-1);
}
#endif

int
main(int argc, char * argv[])
{
	int counter_expected;

	WARNP_INIT;
	(void)argc; /* UNUSED */

#ifdef NUM_THREADS
	counter_expected = NUM_TIMES * NUM_THREADS;
	if (test_pthread())
		goto err0;
#else
	counter_expected = NUM_TIMES;
	inc_dec_counter(NULL);
#endif

	/* Check the counter. */
	if ((value != 0) || (counter != counter_expected))
		goto err0;

	/* Success! */
	exit(0);

err0:
	/* Failure! */
	exit(1);
}

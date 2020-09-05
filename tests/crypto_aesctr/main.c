#include <sys/time.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpusupport.h"
#include "crypto_aes.h"
#include "crypto_aesctr.h"
#include "getopt.h"
#include "hexify.h"
#include "insecure_memzero.h"
#include "monoclock.h"
#include "warnp.h"

#define MAX_PLAINTEXT_LENGTH 32

#define LARGE_BUFSIZE 65536
#define MAX_CHUNK 256

/* Forward declaration. */
struct crypto_aes_key;

struct testcase {
	const char * keytext_hex;
	const char * plaintext_str;
	const char * ciphertext_hex;
};

/* Test vectors. */
static const struct testcase tests[] = {
	/* 128-bit AES-CTR. */
	{"000102030405060708090a0b0c0d0e0f",
	    " ",
	    "e6"},
	{"000102030405060708090a0b0c0d0e0f",
	    "A",
	    "87"},
	{"000102030405060708090a0b0c0d0e0f",
	    "AAAA",
	    "87e07a76"},
	{"000102030405060708090a0b0c0d0e0f",
	    "AB",
	    "87e3"},
	{"000102030405060708090a0b0c0d0e0f",
	    "hello",
	    "aec4575be8"},
	{"000102030405060708090a0b0c0d0e0f",
	    "hello world",
	    "aec4575be8af2ced1d23e5"},
	{"000102030405060708090a0b0c0d0e0f",
	    "This is 16 chars",
	    "92c95244a7e628a25e79a101c9a9aa0a"},
	{"000102030405060708090a0b0c0d0e0f",
	    "Ceci n'est pas 24 chars.",
	    "85c4585ea7e17ce71c3ba112c0bbf84b476670fdf4b2c730"},
	{"000102030405060708090a0b0c0d0e0f",
	    "This block is exactly 32 chars!!",
	    "92c95244a7ed37ed0c24a10bd2e8bd01122567f9ece0872c6918d58217870c2b"},
	/* 256-bit AES-CTR. */
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	    " ",
	    "d2"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	    "A",
	    "b3"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	    "AAAA",
	    "b3d141f7"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	    "AB",
	    "b3d2"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	    "hello",
	    "9af56cda45"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	    "hello world",
	    "9af56cda4569e8bfdb9ffe"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	    "This is 16 chars",
	    "a6f869c50a20ecf098c5ba09b54f05f3"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	    "Ceci n'est pas 24 chars.",
	    "b1f563df0a27b8b5da87ba1abc5d57b2c47d15c62bcbeccb"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
	    "This block is exactly 32 chars!!",
	    "a6f869c50a2bf3bfca98ba03ae0e12f8913e02c23399acd78695f3503ab1171c"}
};

/* Largest buffer must be last. */
static const size_t perfsizes[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
static const size_t num_perf = sizeof(perfsizes) / sizeof(perfsizes[0]);
static const size_t bytes_to_encrypt = 1 << 25;	/* approx 34 MB */

/* Print a name, then an array in hex. */
static void
print_arr(const char * name, uint8_t * arr, size_t len)
{
	size_t i;

	printf("%s", name);
	for (i = 0; i < len; i++)
		printf("%02x", arr[i]);
	printf("\n");
}

/* Print a string, then whether or not we're using hardware AESNI. */
static void
print_hardware(const char * str)
{
	int use_hardware = 0;

#ifdef CPUSUPPORT_X86_AESNI
	if (cpusupport_x86_aesni())
		use_hardware = 1;
#endif

	/* Inform the user. */
	printf("%s", str);
	if (use_hardware)
		printf(" using hardware AESNI.\n");
	else
		printf(" using software AES.\n");
}

static int
parse_testcase(const struct testcase testcase,
    struct crypto_aes_key ** key_exp_p, size_t * keylen_p,
    uint8_t plaintext_arr[static MAX_PLAINTEXT_LENGTH],
    uint8_t ciphertext_arr[static MAX_PLAINTEXT_LENGTH])
{
	uint8_t key[32];	/* We will use 16 or 32 of these bytes. */
	const size_t len = strlen(testcase.plaintext_str);

	/* Determine key length. */
	*keylen_p = strlen(testcase.keytext_hex) / 2;

	/* Sanity check. */
	assert((*keylen_p == 16) || (*keylen_p == 32));
	assert(len <= MAX_PLAINTEXT_LENGTH);

	/* Prepare the key. */
	if (unhexify(testcase.keytext_hex, key, *keylen_p)) {
		warn0("unhexify(%s)", testcase.keytext_hex);
		goto err0;
	}
	if ((*key_exp_p = crypto_aes_key_expand(key, *keylen_p)) == NULL) {
		warn0("crypto_aes_key_expand");
		goto err0;
	}

	/* Prepare the arrays. */
	memcpy(plaintext_arr, testcase.plaintext_str, len);
	if (unhexify(testcase.ciphertext_hex, ciphertext_arr, len)) {
		warn0("unhexify(%s)", testcase.ciphertext_hex);
		goto err1;
	}

	/* Clean up.  Irrelevant for a test, but it's a good habit. */
	insecure_memzero(key, 32);

	/* Success! */
	return (0);

err1:
	crypto_aes_key_free(*key_exp_p);
err0:
	/* Failure! */
	return (1);
}

static int
perftest(void)
{
	struct crypto_aes_key * key_exp;
	uint8_t key[32];
	uint8_t * largebuf;
	struct timeval begin, end;
	double delta_s;
	size_t i, j;
	size_t num_blocks;
	size_t bufsize;
	size_t maxbufsize = perfsizes[num_perf - 1];

	/* Inform user about the hardware optimization status. */
	print_hardware("Performance test of AES-CTR");
	fflush(stdout);

	/* Allocate buffer to hold largest message. */
	if ((largebuf = malloc(maxbufsize)) == NULL) {
		warnp("malloc");
		goto err0;
	}
	memset(largebuf, 0, maxbufsize);

	/* Prepare the key.  We're only performance-testing 256-bit keys. */
	for (i = 0; i < 32; i++)
		key[i] = (uint8_t)i;
	if ((key_exp = crypto_aes_key_expand(key, 32)) == NULL)
		goto err1;

	/* Warm up. */
	for (j = 0; j < 100000; j++)
		crypto_aesctr_buf(key_exp, j, largebuf, largebuf, perfsizes[0]);

	/* Reset plaintext input. */
	memset(largebuf, 0, maxbufsize);

	/* Run operations. */
	for (i = 0; i < num_perf; i++) {
		bufsize = perfsizes[i];
		num_blocks = bytes_to_encrypt / bufsize;

		/* Get beginning time. */
		if (monoclock_get_cputime(&begin)) {
			warnp("monoclock_get_cputime()");
			goto err2;
		}

		/* Encrypt all the bytes. */
		for (j = 0; j < num_blocks; j++)
			crypto_aesctr_buf(key_exp, j, largebuf, largebuf,
			    bufsize);

		/* Get ending time. */
		if (monoclock_get_cputime(&end)) {
			warnp("monoclock_get_cputime()");
			goto err2;
		}

		/* Prepare output. */
		delta_s = timeval_diff(begin, end);

		/* Print results. */
		printf("... in %zu blocks of size %zu:\t%.06f s\t%.01f MB/s\n",
		    num_blocks, bufsize, delta_s,
		    (double)bytes_to_encrypt / 1e6 / delta_s);
		fflush(stdout);
	}

	/* Clean up. */
	free(largebuf);

	/* Success! */
	return (0);

err2:
	crypto_aes_key_free(key_exp);
err1:
	free(largebuf);
err0:
	/* Failure! */
	return (1);
}

static size_t
selftest_unaligned_access(size_t keylen)
{
	struct crypto_aesctr * aesctr;
	struct crypto_aes_key * key_exp;
	uint8_t key[32];
	uint8_t * largebuf;
	uint8_t * largebuf_out1;
	uint8_t * largebuf_out2;
	size_t i;
	size_t bytes_processed;
	size_t new_chunk;
	size_t failures = 0;

	/* Prepare a large buffer with repeating 01010101_2 = 85. */
	if ((largebuf = malloc(LARGE_BUFSIZE)) == NULL)
		goto err0;
	memset(largebuf, 85, LARGE_BUFSIZE);

	/* Prepare the key: 00010203... */
	for (i = 0; i < keylen; i++)
		key[i] = (uint8_t)i;
	if ((key_exp = crypto_aes_key_expand(key, keylen)) == NULL)
		goto err1;

	/* Test with a large buffer and unaligned access. */
	printf("Computing %zu-bit AES-CTR of a large buffer two "
	    "different ways...", keylen * 8);

	/* Prepare output buffers. */
	if ((largebuf_out1 = malloc(LARGE_BUFSIZE)) == NULL)
		goto err2;
	if ((largebuf_out2 = malloc(LARGE_BUFSIZE)) == NULL)
		goto err3;

	/* Encrypt with one call. */
	crypto_aesctr_buf(key_exp, 0, largebuf, largebuf_out1, LARGE_BUFSIZE);

	/* Ensure we have a repeatable pattern of random values. */
	srandom(0);

	/* Encrypt with multiple calls. */
	if ((aesctr = crypto_aesctr_init(key_exp, 0)) == NULL)
		goto err4;
	bytes_processed = 0;
	while (bytes_processed < LARGE_BUFSIZE - MAX_CHUNK) {
		new_chunk = ((unsigned long int)random()) % MAX_CHUNK;
		crypto_aesctr_stream(aesctr, &largebuf[bytes_processed],
		    &largebuf_out2[bytes_processed], new_chunk);
		bytes_processed += new_chunk;
	}
	new_chunk = LARGE_BUFSIZE - bytes_processed;
	crypto_aesctr_stream(aesctr, &largebuf[bytes_processed],
	    &largebuf_out2[bytes_processed], new_chunk);
	crypto_aesctr_free(aesctr);

	/* Compare ciphertexts. */
	if (memcmp(largebuf_out1, largebuf_out2, LARGE_BUFSIZE)) {
		printf(" FAILED!\n");
		failures++;
	} else
		printf(" PASSED!\n");

	/* Clean up. */
	free(largebuf_out2);
	free(largebuf_out1);
	crypto_aes_key_free(key_exp);
	free(largebuf);

	return (failures);

err4:
	free(largebuf_out2);
err3:
	free(largebuf_out1);
err2:
	crypto_aes_key_free(key_exp);
err1:
	free(largebuf);
err0:
	/* Failure! */
	return (1);
}

static int
selftest(void)
{
	struct crypto_aes_key * key_exp;
	uint8_t plaintext_arr[MAX_PLAINTEXT_LENGTH];
	uint8_t ciphertext_arr[MAX_PLAINTEXT_LENGTH];
	uint8_t cbuf[MAX_PLAINTEXT_LENGTH];
	size_t keylen;
	size_t i;
	size_t len;
	size_t failures = 0;
	size_t num_tests = sizeof(tests) / sizeof(tests[0]);

	/* Inform user about the hardware optimization status. */
	print_hardware("Checking test vectors of AES");

	/* Run regular test cases. */
	for (i = 0; i < num_tests; i++) {
		/* Prepare for the test case. */
		if (parse_testcase(tests[i], &key_exp, &keylen,
		    plaintext_arr, ciphertext_arr)) {
			warn0("parse_testcase");
			goto err0;
		}
		printf("Computing %zu-bit AES-CTR of \"%s\"...",
		    keylen * 8, tests[i].plaintext_str);

		/* Plaintext length. */
		len = strlen(tests[i].plaintext_str);

		/* Encrypt with AES-CTR. */
		crypto_aesctr_buf(key_exp, 0, plaintext_arr,
		    cbuf, len);

		/* Check result. */
		if (memcmp(cbuf, ciphertext_arr, len)) {
			printf(" FAILED!\n");
			print_arr("Computed AES:\t", cbuf, len);
			print_arr("Correct AES:\t", ciphertext_arr, len);
			failures++;
		} else {
			printf(" PASSED!\n");
		}

		/* Clean up. */
		crypto_aes_key_free(key_exp);
	}

	/* Test unaligned access. */
	if (selftest_unaligned_access(16))
		failures++;
	if (selftest_unaligned_access(32))
		failures++;

	/* Report overall success to exit code. */
	if (failures)
		return (1);
	else
		return (0);

err0:
	/* Failure! */
	return (1);
}

static void
usage(void)
{

	fprintf(stderr, "usage: test_crypto_aesctr -t\n");
	fprintf(stderr, "       test_crypto_aesctr -x\n");
	exit(1);
}

int
main(int argc, char * argv[])
{
	const char * ch;

	WARNP_INIT;

	/* Process arguments. */
	while ((ch = GETOPT(argc, argv)) != NULL) {
		GETOPT_SWITCH(ch) {
		GETOPT_OPT("-t"):
			exit(perftest());
		GETOPT_OPT("-x"):
			exit(selftest());
		GETOPT_DEFAULT:
			usage();
		}
	}

	usage();
}
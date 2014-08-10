#include <stdint.h>

#include <openssl/aes.h>

#include "cpusupport.h"

#include "crypto_aes.h"

void aes_encrypt_block_aesni(const uint8_t *, uint8_t *, const AES_KEY *);

void
aes_encrypt_block(const uint8_t *in, uint8_t *out, const AES_KEY *key)
{
#ifdef CPUSUPPORT_X86_AESNI
	if (cpusupport_x86_aesni())
		return aes_encrypt_block_aesni(in, out, key);
#endif
	return AES_encrypt(in, out, key);
}

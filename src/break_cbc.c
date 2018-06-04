/*
 * break_cbc.c
 *
 * CBC analysis stuff for cryptopals.com challenges.
 */
#include <string.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include "compat.h"
#include "xor.h"
#include "aes.h"
#include "cbc.h"
#include "break_cbc.h"

#define	CBC_BITFLIPPING_PREFIX	"comment1=cooking%20MCs;userdata="
#define	CBC_BITFLIPPING_SUFFIX	";comment2=%20like%20a%20pound%20of%20bacon"


struct bytes *
cbc_bitflipping_oracle(const struct bytes *payload,
		    const struct bytes *key, const struct bytes *iv)
{
	struct bytes *before = NULL, *after = NULL, *quoted = NULL;
	struct bytes *plaintext = NULL, *ciphertext = NULL;
	int success = 0;

	/* sanity checks */
	if (payload == NULL || key == NULL || iv == NULL)
		goto cleanup;

	/* compute the payload final length, since we need to quote out the ";"
	   and "=" characters. */
	size_t len = 0;
	for (size_t i = 0; i < payload->len; i++) {
		switch (payload->data[i]) {
		case '=':
		case ';':
			len += 3;
			break;
		default:
			len += 1;
		}
	}

	/* build the quoted version of payload */
	quoted = bytes_zeroed(len);
	if (quoted == NULL)
		goto cleanup;
	uint8_t *p = quoted->data;
	for (size_t i = 0; i < payload->len; i++) {
		switch (payload->data[i]) {
		case '=':
			(void)memcpy(p, "%3D", 3);
			p += 3;
			break;
		case ';':
			(void)memcpy(p, "%3B", 3);
			p += 3;
			break;
		default:
			*p++ = payload->data[i];
		}
	}

	/* build the full plaintext to encrypt */
	before = bytes_from_str(CBC_BITFLIPPING_PREFIX);
	after  = bytes_from_str(CBC_BITFLIPPING_SUFFIX);
	const struct bytes *const parts[] = { before, quoted, after };
	plaintext = bytes_joined_const(parts, sizeof(parts) / sizeof(*parts));

	/* encrypt the plaintext using AES-CBC */
	ciphertext = aes_128_cbc_encrypt(plaintext, key, iv);
	if (ciphertext == NULL)
		goto cleanup;

	success = 1;
	/* FALLTHROUGH */
cleanup:
	bytes_free(plaintext);
	bytes_free(after);
	bytes_free(before);
	bytes_free(quoted);
	if (!success) {
		bytes_free(ciphertext);
		ciphertext = NULL;
	}
	return (ciphertext);
}


int
cbc_bitflipping_verifier(const struct bytes *ciphertext,
		    const struct bytes *key, const struct bytes *iv)
{
	struct bytes *plaintext = NULL, *target = NULL;
	int success = 0, admin = -1;

	target = bytes_from_str(";admin=true;");
	plaintext = aes_128_cbc_decrypt(ciphertext, key, iv);

	switch (bytes_find(plaintext, target, NULL)) {
	case 0:
		admin = 1;
		break;
	case 1:
		admin = 0;
		break;
	default:
		goto cleanup;
	}

	success = 1;
	/* FALLTHROUGH */
cleanup:
	bytes_free(plaintext);
	bytes_free(target);
	return (success ? admin : -1);
}


struct bytes *
cbc_bitflipping_breaker(const void *key, const void *iv)
{
#define oracle(x)	cbc_bitflipping_oracle((x), key, iv)
	const size_t blocksize = aes_128_blocksize();
	struct bytes *pad = NULL, *scrambled = NULL;
	struct bytes *admin = NULL, *payload = NULL, *ciphertext = NULL;
	int success = 0;

	/* given the prefix length, compute how much padding bytes we need to
	   add so that it is congruent to 0 modulo blocksize */
	const size_t prefixlen = strlen(CBC_BITFLIPPING_PREFIX);
	const size_t padlen = prefixlen % blocksize == 0 ? 0 :
		    blocksize - strlen(CBC_BITFLIPPING_PREFIX) % blocksize;
	pad = bytes_repeated(padlen, 'A');

	/* generate a full block on which we will hack the bytes in order to
	   bitflip the block right after it */
	const size_t sblock = (prefixlen + padlen) / blocksize;
	scrambled = bytes_repeated(blocksize, 'X');

	/* the admin=true payload. We use a comma (,), dash (-), to be flipped
	   into a semi-colon (;), respectively equal (=). */
	const size_t sci = sblock * blocksize + 0;
	const size_t eqi = sblock * blocksize + 6;
	admin = bytes_from_str(",admin-true");

	/* generate the ciphertext using the oracle */
	const struct bytes *const parts[] = { pad, scrambled, admin };
	payload = bytes_joined_const(parts, sizeof(parts) / sizeof(*parts));
	ciphertext = oracle(payload);
	if (ciphertext == NULL)
		goto cleanup;

	/* mess with the scrambled block */
	ciphertext->data[sci] ^= (',' ^ ';');
	ciphertext->data[eqi] ^= ('-' ^ '=');

	success = 1;
	/* FALLTHROUGH */
cleanup:
	bytes_free(payload);
	bytes_free(admin);
	bytes_free(scrambled);
	bytes_free(pad);
	if (!success) {
		bytes_free(ciphertext);
		ciphertext = NULL;
	}
	return (ciphertext);
#undef oracle
}


int
cbc_padding_oracle(const struct bytes *ciphertext,
		    const struct bytes *key, const struct bytes *iv)
{
	/*
	 * NOTE: implement AES-CBC decryption using OpenSSL since our own code
	 * doesn't provide a way to decrypt without removing the PKCS#7 padding.
	 */
	const EVP_CIPHER *cipher = EVP_aes_128_cbc();
	const size_t blocksize = EVP_CIPHER_block_size(cipher);
	EVP_CIPHER_CTX *ctx = NULL;
	struct bytes *plaintext = NULL;
	int padding = -1, success = 0;

	/* sanity checks */
	if (ciphertext == NULL || key == NULL)
		goto cleanup;
	if (ciphertext->len > INT_MAX || key->len > INT_MAX)
		goto cleanup;

	/* create the context */
	ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL)
		goto cleanup;

	/* setup the context cipher */
	if (EVP_DecryptInit_ex(ctx, cipher, NULL, NULL, NULL) != 1)
		goto cleanup;

	/* setup the context cipher key and iv */
	if (EVP_CIPHER_CTX_set_key_length(ctx, key->len) != 1)
		goto cleanup;
	if (iv->len != (size_t)EVP_CIPHER_CTX_iv_length(ctx))
		goto cleanup;
	if (EVP_DecryptInit_ex(ctx, NULL, NULL, key->data, iv->data) != 1)
		goto cleanup;

	/* setup the context cipher padding */
	if (EVP_CIPHER_CTX_set_padding(ctx, /* no padding */0) != 1)
		goto cleanup;

	/* NOTE: add twice the block size needed by enc update and final */
	plaintext = bytes_zeroed(ciphertext->len + blocksize * 2);
	if (plaintext == NULL)
		goto cleanup;

	/* update */
	int uplen = -1;
	int ret = EVP_DecryptUpdate(ctx, plaintext->data, &uplen, ciphertext->data, ciphertext->len);
	if (ret != 1 || uplen < 0)
		goto cleanup;

	/* finalize */
	int finlen = -1;
	ret = EVP_DecryptFinal_ex(ctx, plaintext->data + uplen, &finlen);
	if (ret != 1 || finlen < 0 || (INT_MAX - uplen) < finlen)
		goto cleanup;
	const size_t outlen = uplen + finlen;

	/* set the output buffer length */
	if (plaintext->len < outlen)
		abort();
	plaintext->len = outlen;

	/* verify if the plaintext has a PKCS#7 padding */
	padding = bytes_pkcs7_padding(plaintext, NULL);
	if (padding == -1)
		goto cleanup;

	success = 1;
	/* FALLTHROUGH */
cleanup:
	EVP_CIPHER_CTX_free(ctx);
	/* XXX: we don't provide any clue on what happened on error */
	ERR_remove_state(/* pid will be looked up */0);
	bytes_free(plaintext);
	return (success ? padding : -1);
}


struct bytes *
cbc_padding_breaker(const struct bytes *ciphertext,
		    const void *key, const struct bytes *iv)
{
#define oracle(x, iv)	cbc_padding_oracle((x), key, (iv))
	const size_t blocksize = aes_128_blocksize();
	size_t nblocks = 0;
	struct bytes *padded = NULL, *plaintext = NULL;
	int success = 0;

	/* sanity checks */
	if (ciphertext == NULL || key == NULL || iv == NULL)
		goto cleanup;
	if (ciphertext->len % blocksize != 0)
		goto cleanup;
	if (iv->len != blocksize)
		goto cleanup;

	/* total block count in the ciphertext */
	nblocks = ciphertext->len / blocksize;

	/* padded plaintext, will be filled block per block. */
	padded = bytes_zeroed(ciphertext->len);
	if (padded == NULL)
		goto cleanup;

	/*
	 * Here is how CBC decryption works:
	 *
	 *  ciphertext block c0    ciphertext block c1
	 * [...................]  [...................]
	 *         |                        |
	 *         |                        v
	 *         |               ___________________
	 *         |              |                   |
	 *         |              |    block cipher   | <- key
	 *         |              |     decryption    |
	 *         |               ___________________
	 *         |                        |
	 *         |                        v
	 *         |              intermediate state i1
	 *         |              [...................]
	 *         |                        |
	 *         |                        v
	 *         ----------------------> XOR
	 *                                  |
	 *                                  v
	 *                            plaintext p1
	 *                        [...................]
	 *
	 * We break one block c1 at a time. We alter c0, used as the IV, in
	 * order to recover the intermediate state i1 block. From that point we
	 * can compute the plaintext p1 by simply XOR'ing c0 and i1.
	 */
	for (size_t n = 0; n < nblocks; n++) {
		struct bytes *c0 = NULL, *c1 = NULL, *i1 = NULL;
		int loop_success = 0;

		/* setup c0 and c1 for the current block */
		if (n == 0) {
			/* first pass */
			c0 = bytes_dup(iv);
		} else {
			/* use the previous block as IV */
			c0 = bytes_slice(ciphertext, (n - 1) * blocksize,
				    blocksize);
		}
		c1 = bytes_slice(ciphertext, n * blocksize, blocksize);
		if (c0 == NULL || c1 == NULL)
			goto loop_cleanup;

		/* create i1 so that we can fill it byte by byte */
		i1 = bytes_zeroed(blocksize);
		if (i1 == NULL)
			goto loop_cleanup;

		/* recover i1 by breaking one byte at a time, from the last to
		   the first */
		for (size_t pad = 1; pad <= blocksize; pad++) {
			/* here we aim for the block c1 to decrypt to a
			   plaintext with a valid padding of value `pad' */
			struct bytes *alblock = bytes_dup(c0);
			if (alblock == NULL)
				goto loop_cleanup;

			/* setup padding bytes after the one we are cracking so
			   that they decrypt to `pad' */
			const uint8_t *original = c0->data + blocksize - pad;
			uint8_t *altered = alblock->data + blocksize - 1;
			uint8_t *istate  = i1->data      + blocksize - 1;
			for (size_t p = 1; p < pad; p++)
				*altered-- = *istate-- ^ pad;
			/* `altered' and `istate' point to the target byte */

			int found = 0;
			for (uint16_t byte = 0; byte <= UINT8_MAX; byte++) {
				*altered = *original ^ (uint8_t)byte;
				if (oracle(c1, alblock) != 0)
					continue;
				/*
				 * If we flip something in the byte before the
				 * target padding byte (i.e. the first padding
				 * byte), the padding should be still correct.
				 * If not, then the byte before the target byte
				 * was "accidently" taken as part of the
				 * padding.
				 */
				if (pad == 1) {
					/* there is a byte before the padding */
					*(altered - 1) += 1;
					const int ret = oracle(c1, alblock);
					*(altered - 1) -= 1;
					if (ret != 0)
						continue;
				}
				found = 1;
				/* We've hacked `alblock' in a way that
				   altered ^ istate == pad */
				*istate = *altered ^ pad;
				break;
			}
			bytes_free(alblock);
			if (!found)
				goto loop_cleanup;
		}

		/* We've successfully recovered i1, now compute the plaintext
		   block using it. */
		if (bytes_xor(i1, c0) != 0)
			goto loop_cleanup;
		if (bytes_put(padded, n * blocksize, i1) != 0) {
			fprintf(stderr, "bytes_put at n=%zu\n", n);
			goto loop_cleanup;
		}

		loop_success = 1;
		/* FALLTHROUGH */
loop_cleanup:
		bytes_free(i1);
		bytes_free(c1);
		bytes_free(c0);
		if (!loop_success)
			goto cleanup;
	}

	plaintext = bytes_pkcs7_unpadded(padded);
	success = 1;
	/* FALLTHROUGH */
cleanup:
	bytes_free(padded);
	if (!success) {
		bytes_free(plaintext);
		plaintext = NULL;
	}
	return (plaintext);
#undef oracle
}

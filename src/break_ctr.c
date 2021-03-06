/*
 * break_ctr.c
 *
 * CTR analysis stuff for cryptopals.com challenges.
 */
#include <string.h>

#include "compat.h"
#include "xor.h"
#include "ctr.h"
#include "break_cbc.h"
#include "break_ctr.h"
#include "break_single_byte_xor.h"

#define	CTR_BITFLIPPING_PREFIX	"comment1=cooking%20MCs;userdata="
#define	CTR_BITFLIPPING_SUFFIX	";comment2=%20like%20a%20pound%20of%20bacon"


struct bytes *
break_ctr_fixed_nonce(struct bytes **ciphertexts, size_t count)
{
	struct bytes *keystream = NULL;
	int success = 0;

	/* sanity checks */
	if (ciphertexts == NULL || count == 0)
		goto cleanup;

	/* find the length of the longest ciphertext(s) */
	size_t maxlen = 0;
	for (size_t i = 0; i < count; i++) {
		const struct bytes *ciphertext = ciphertexts[i];
		if (ciphertext == NULL)
			goto cleanup;
		maxlen = (ciphertext->len > maxlen ? ciphertext->len : maxlen);
	}

	keystream = bytes_zeroed(maxlen);
	if (keystream == NULL)
		goto cleanup;

	/* break the keystream one byte at a time */
	for (size_t i = 0; i < maxlen; i++) {
		struct bytes *buf = NULL, *key = NULL;
		/* we will aggregate the ith byte of each ciphertext. Start by
		   computing the length of the buffer */
		size_t buflen = 0;
		for (size_t j = 0; j < count; j++) {
			/* check that the current ciphertext is long enough to
			   hold a byte at i */
			if (ciphertexts[j]->len > i)
				buflen += 1;
		}
		/* Now that we know the length, allocate the buffer */
		buf = bytes_zeroed(buflen);
		if (buf == NULL)
			goto cleanup;
		/* populate the buffer with the ith byte of each ciphertext */
		buflen = 0;
		for (size_t j = 0; j < count; j++) {
			if (ciphertexts[j]->len > i) {
				buf->data[buflen++] = ciphertexts[j]->data[i];
			}
		}

		/* attempt to guess the ith keystream byte */
		struct bytes *result = break_single_byte_xor(
			    buf, looks_like_shuffled_english, &key, NULL);
		bytes_free(buf);
		if (result == NULL)
			goto cleanup;
		bytes_free(result);
		if (key == NULL || key->len != 1) {
			bytes_free(key);
			goto cleanup;
		}
		keystream->data[i] = key->data[0];
		bytes_free(key);
	}

	success = 1;
	/* FALLTHROUGH */
cleanup:
	if (!success) {
		bytes_free(keystream);
		keystream = NULL;
	}
	return (keystream);
}


struct bytes *
aes_128_ctr_edit_oracle(const struct bytes *ciphertext,
		    const struct bytes *key, uint64_t nonce,
		    size_t offset, const struct bytes *replacement)
{
	struct bytes *zeroes = NULL, *keystream = NULL, *rkeystream = NULL;
	struct bytes *before = NULL, *rct = NULL, *after = NULL, *output = NULL;
	int success = 0;

	/* sanity checks */
	if (ciphertext == NULL || key == NULL || replacement == NULL)
		goto cleanup;
	if (offset > ciphertext->len)
		goto cleanup;
	if (ciphertext->len - offset < replacement->len)
		goto cleanup;

	/*
	 * We build the output as:
	 *
	 *    before  offset  rct   bound  after
	 *       v      |      v      |      v
	 * [ ......... ][ .......... ][ .......... ]
	 *
	 * Here rct is the "replacement ciphertext", i.e. the encrypted version
	 * of replacement. before and after are untouched slices of the original
	 * ciphertext.
	 */

	const size_t bound = offset + replacement->len;

	/* encrypt as many 0x0 as we need in order to get the keystream */
	zeroes = bytes_zeroed(bound);
	keystream = aes_128_ctr_encrypt(zeroes, key, nonce);
	/* get the part of the keystream needed to encrypt the replacement */
	rkeystream = bytes_slice(keystream, offset, replacement->len);
	rct = bytes_dup(replacement);
	if (bytes_xor(rct, rkeystream) != 0)
		goto cleanup;

	/* find the copied parts from the ciphertext before and after the
	   replacement */
	before = bytes_slice(ciphertext, 0, offset);
	after  = bytes_slice(ciphertext, bound, ciphertext->len - bound);
	output = bytes_joined(3, before, rct, after);

	success = 1;
	/* FALLTHROUGH */
cleanup:
	bytes_free(after);
	bytes_free(before);
	bytes_free(rct);
	bytes_free(rkeystream);
	bytes_free(keystream);
	bytes_free(zeroes);
	if (!success) {
		bytes_free(output);
		output = NULL;
	}
	return (output);
}


struct bytes *
aes_128_ctr_edit_breaker(const struct bytes *ciphertext,
		    const struct bytes *key, const uint64_t nonce)
#define oracle(ct, off, rep) \
		aes_128_ctr_edit_oracle((ct), key, nonce, (off), (rep))
{
	return (oracle(ciphertext, 0, ciphertext));
}
#undef oracle


struct bytes *
ctr_bitflipping_encrypt(const struct bytes *payload,
		    const struct bytes *key, uint64_t nonce)
{
	struct bytes *before = NULL, *after = NULL, *escaped = NULL;
	struct bytes *plaintext = NULL, *ciphertext = NULL;
	int success = 0;

	/* sanity checks */
	if (payload == NULL || key == NULL)
		goto cleanup;

	/* escape the special characters from the payload */
	escaped = cbc_bitflipping_escape(payload);

	/* build the full plaintext to encrypt */
	before = bytes_from_str(CTR_BITFLIPPING_PREFIX);
	after  = bytes_from_str(CTR_BITFLIPPING_SUFFIX);
	plaintext = bytes_joined(3, before, escaped, after);

	/* encrypt the plaintext using AES-CTR */
	ciphertext = aes_128_ctr_encrypt(plaintext, key, nonce);
	if (ciphertext == NULL)
		goto cleanup;

	success = 1;
	/* FALLTHROUGH */
cleanup:
	bytes_free(plaintext);
	bytes_free(after);
	bytes_free(before);
	bytes_free(escaped);
	if (!success) {
		bytes_free(ciphertext);
		ciphertext = NULL;
	}
	return (ciphertext);
}


int
ctr_bitflipping_verifier(const struct bytes *ciphertext,
		    const struct bytes *key, uint64_t nonce)
{
	struct bytes *plaintext = NULL, *target = NULL;
	int success = 0, admin = -1;

	target = bytes_from_str(";admin=true;");
	plaintext = aes_128_ctr_decrypt(ciphertext, key, nonce);

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
ctr_bitflipping_breaker(const void *key, uint64_t nonce)
#define encrypt(x)	ctr_bitflipping_encrypt((x), key, nonce)
{
	struct bytes *payload = NULL, *ciphertext = NULL;
	int success = 0;

	const size_t prefixlen = strlen(CTR_BITFLIPPING_PREFIX);

	/* the admin=true payload. We use a comma (,), dash (-), to be flipped
	   into a semi-colon (;), respectively equal (=). */
	const size_t sci = prefixlen + 0;
	const size_t eqi = prefixlen + 6;
	payload = bytes_from_str(",admin-true");

	/* generate the ciphertext */
	ciphertext = encrypt(payload);
	if (ciphertext == NULL)
		goto cleanup;

	/* mess with the scrambled block */
	ciphertext->data[sci] ^= (',' ^ ';');
	ciphertext->data[eqi] ^= ('-' ^ '=');

	success = 1;
	/* FALLTHROUGH */
cleanup:
	bytes_free(payload);
	if (!success) {
		bytes_free(ciphertext);
		ciphertext = NULL;
	}
	return (ciphertext);
}
#undef encrypt

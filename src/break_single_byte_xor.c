/*
 * break_single_byte_xor.c
 *
 * Breaking Single-byte XOR "cipher".
 */
#include "break_plaintext.h"
#include "break_single_byte_xor.h"


struct bytes *
break_single_byte_xor(const struct bytes *ciphertext,
		break_plaintext_func_t method,
		struct bytes **key_p, double *score_p)
{
	struct bytes *decrypted = NULL, *key = NULL;
	uint8_t guess = 0;
	double score = 0;
	int success = 0;

	/* sanity checks */
	if (ciphertext == NULL || ciphertext->len == 0)
		goto cleanup;
	if (method == NULL)
		goto cleanup;

	/* create a working copy of the buffer to analyze */
	decrypted = bytes_dup(ciphertext);
	if (decrypted == NULL)
		goto cleanup;

	/* go through each possible byte and find the one most likely to yield
	   english text */
	for (uint16_t k = 0; k <= UINT8_MAX; k++) {
		/* XOR the working buffer with the previous key and the current
		   key, so that we undo the last encrypt iteration and encrypt
		   for the current iteration at once.  */
		for (size_t i = 0; i < decrypted->len; i++)
			decrypted->data[i] = (uint8_t)k ^ ciphertext->data[i];
		/* run the analysis on the "decrypted" buffer */
		double s = 0;
		if (method(decrypted, &s) != 0)
			goto cleanup;
		/* save the current guess if it looks like the best one */
		if (s > score) {
			guess = (uint8_t)k;
			score = s;
		}
	}

	/* here `guess' is our guessed key and `score' the probability that is
	   this the correct one. */
	for (size_t i = 0; i < decrypted->len; i++)
		decrypted->data[i] = (uint8_t)guess ^ ciphertext->data[i];

	if (key_p != NULL) {
		key = bytes_from_single(guess);
		if (key == NULL)
			goto cleanup;
	}

	success = 1;

	/* set `key_p' and `score_p' if needed */
	if (key_p != NULL) {
		*key_p = key;
		key = NULL;
	}
	if (score_p != NULL)
		*score_p = score;

	/* FALLTHROUGH */
cleanup:
	bytes_free(key);
	if (!success) {
		bytes_free(decrypted);
		decrypted = NULL;
	}
	return (decrypted);
}

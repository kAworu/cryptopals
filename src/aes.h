#ifndef AES_H
#define AES_H
/*
 * aes.h
 *
 * AES stuff for cryptopals.com challenges.
 *
 * Mosty just wrapping the OpenSSL API.
 */
#include "bytes.h"


/**
 * Encrypt a given plaintext via AES-128 in ECB mode under the provided key.
 *
 * Returns the ciphertext or NULL on error.
 */
struct bytes	*aes_128_ecb_encrypt(const struct bytes *plaintext,
		    const struct bytes *key);

/**
 * Decrypt a given ciphertext encrypted via AES-128 in ECB mode under the
 * provided key.
 *
 * Returns the plaintext or NULL on error.
 */
struct bytes	*aes_128_ecb_decrypt(const struct bytes *ciphertext,
		    const struct bytes *key);

/**
 * Encrypt a given plaintext via AES-128 in CBC mode under the provided key with
 * the given IV.
 *
 * Returns the ciphertext or NULL on error.
 */
struct bytes	*aes_128_cbc_encrypt(const struct bytes *plaintext,
		    const struct bytes *key, const struct bytes *iv);

/**
 * Decrypt a given ciphertext encrypted via AES-128 in CBC mode under the
 * provided key with the given IV.
 *
 * Returns the plaintext or NULL on error.
 */
struct bytes	*aes_128_cbc_decrypt(const struct bytes *ciphertext,
		    const struct bytes *key, const struct bytes *iv);

#endif /* ndef AES_H */

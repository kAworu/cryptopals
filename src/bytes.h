#ifndef BYTES_H
#define BYTES_H
/*
 * bytes.h
 *
 * Bytes manipulation stuff for cryptopals.com challenges.
 */
#include <stdint.h>


/*
 * A very simple struct holding a bunch of bytes and the byte count.
 */
struct bytes {
	size_t len;
	uint8_t data[];
};


/*
 * Create a bytes struct from a NUL-terminated string.
 *
 * Returns a pointer to a newly allocated bytes struct that should passed to
 * free(3). Returns NULL if the given pointer is NULL, or malloc(3) failed.
 */
struct bytes	*bytes_from_raw(const char *s);

/*
 * Create a bytes struct from a hex-encoded NUL-terminated string.
 *
 * Returns a pointer to a newly allocated bytes struct that should passed to
 * free(3). Returns NULL if the given pointer is NULL, decoding failed, or
 * malloc(3) failed.
 *
 * NOTE: This implementation will reject the encoded data if it contains
 * characters outside the base16 alphabet as per RFC 4648 § 3.3.
 */
struct bytes	*bytes_from_hex(const char *hex);

/*
 * Create a bytes struct from another bytes struct by copying it.
 *
 * Returns a pointer to a newly allocated bytes struct that should passed to
 * free(3). Returns NULL if the given pointer is NULL, or malloc(3) failed.
 */
struct bytes	*bytes_copy(const struct bytes *src);

/*
 * Perform a binary XOR of two bytes struct of the same length. After this
 * function returns, the first bytes struct argument holds the result.
 *
 * Returns 0 on success, -1 if any of the argument is NULL or if their length
 * doesn't match.
 */
int	bytes_xor(struct bytes *bytes, const struct bytes *mask);

/*
 * Returns the raw NUL-terminated string representation of the given bytes
 * struct.
 *
 * Returns a pointer to a newly allocated bytes struct that should passed to
 * free(3). Returns NULL if the given pointer is NULL, or malloc(3) failed.
 */
char	*bytes_to_raw(const struct bytes *bytes);

/*
 * Returns the hex representation of the given bytes struct, using uppercase
 * letters.
 *
 * Returns a pointer to a newly allocated bytes struct that should passed to
 * free(3). Returns NULL if the given pointer is NULL, or malloc(3) failed.
 */
char	*bytes_to_hex(const struct bytes *bytes);

/*
 * Returns the base64 representation of the given bytes struct.
 *
 * Returns a pointer to a newly allocated bytes struct that should passed to
 * free(3). Returns NULL if the given pointer is NULL, or malloc(3) failed.
 */
char	*bytes_to_base64(const struct bytes *bytes);

#endif /* ndef BYTES_H */

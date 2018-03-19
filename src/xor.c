/*
 * xor.c
 *
 * XOR "cipher" stuff for cryptopals.com challenges.
 */
#include <stdlib.h>

#include "xor.h"


int
bytes_xor(struct bytes *buf, const struct bytes *mask)
{
	if (buf == NULL || mask == NULL)
		return (-1);
	if (buf->len != mask->len)
		return (-1);

	for (size_t i = 0; i < buf->len; i++)
		buf->data[i] ^= mask->data[i];

	return (0);
}


int
repeating_key_xor(struct bytes *buf, const struct bytes *key)
{
	if (buf == NULL || key == NULL)
		return (-1);
	if (key->len == 0)
		return (-1);

	for (size_t i = 0; i < buf->len; i++)
		buf->data[i] ^= key->data[i % key->len];

	return (0);
}

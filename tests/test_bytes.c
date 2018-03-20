/*
 * test_bytes.c
 */
#include <stdlib.h>

#include "munit.h"
#include "bytes.h"


static MunitResult
test_bytes_from_raw(const MunitParameter *params, void *data)
{
	const char *input = "foobar";

	for (size_t i = 0; i <= sizeof(input); i++) {
		struct bytes *buf = bytes_from_raw(input, i);
		if (buf == NULL)
			munit_error("bytes_from_raw");

		munit_assert_size(buf->len, ==, i);
		munit_assert_memory_equal(buf->len, buf->data, input);

		bytes_free(buf);
	}

	/* when NULL is given */
	munit_assert_null(bytes_from_raw(NULL, 1));

	return (MUNIT_OK);
}


static MunitResult
test_bytes_from_single(const MunitParameter *params, void *data)
{
	const uint8_t vectors[] = {
		0x0, 0x1, 0xa0, 0xef, 0xfe, 0xff
	};

	for (size_t i = 0; i <= sizeof(vectors) / sizeof(*vectors); i++) {
		const uint8_t byte = vectors[i];
		struct bytes *buf = bytes_from_single(byte);
		if (buf == NULL)
			munit_error("bytes_from_single");

		munit_assert_size(buf->len, ==, 1);
		munit_assert_memory_equal(1, buf->data, &byte);

		bytes_free(buf);
	}

	return (MUNIT_OK);
}


static MunitResult
test_bytes_from_str(const MunitParameter *params, void *data)
{
	const struct {
		char *input;
		size_t expected;
	} vectors[] = {
		{ .input = "",       .expected = 0 },
		{ .input = "f",      .expected = 1 },
		{ .input = "fo",     .expected = 2 },
		{ .input = "foo",    .expected = 3 },
		{ .input = "foob",   .expected = 4 },
		{ .input = "fooba",  .expected = 5 },
		{ .input = "foobar", .expected = 6 },
	};

	for (int i = 0; i < (sizeof(vectors) / sizeof(*vectors)); i++) {
		const char *input = vectors[i].input;
		const size_t expected = vectors[i].expected;

		struct bytes *buf = bytes_from_str(input);
		if (buf == NULL)
			munit_error("bytes_from_str");

		munit_assert_size(buf->len, ==, expected);
		munit_assert_memory_equal(buf->len, buf->data, input);

		bytes_free(buf);
	}

	/* when NULL is given */
	munit_assert_null(bytes_from_str(NULL));

	return (MUNIT_OK);
}


/* Test Vectors from RFC 4648 */
static MunitResult
test_bytes_from_hex(const MunitParameter *params, void *data)
{
	const struct {
		char *input;
		char *expected;
	} vectors[] = {
		{ .input = "",             .expected = "" },
		{ .input = "66",           .expected = "f" },
		{ .input = "666F",         .expected = "fo" },
		{ .input = "666F6F",       .expected = "foo" },
		{ .input = "666F6F62",     .expected = "foob" },
		{ .input = "666F6F6261",   .expected = "fooba" },
		{ .input = "666F6F626172", .expected = "foobar" },
		/* Added this one for case-insensitiveness compliance. */
		{ .input = "666f6f626172", .expected = "foobar" },
	};

	for (int i = 0; i < (sizeof(vectors) / sizeof(*vectors)); i++) {
		const char *input = vectors[i].input;
		const char *expected = vectors[i].expected;

		struct bytes *buf = bytes_from_hex(input);
		if (buf == NULL)
			munit_error("bytes_from_hex");

		munit_assert_size(buf->len, ==, strlen(expected));
		munit_assert_memory_equal(buf->len, buf->data, expected);

		bytes_free(buf);
	}

	/* when NULL is given */
	munit_assert_null(bytes_from_hex(NULL));

	/* test with a string not hex-encoded */
	munit_assert_null(bytes_from_hex("!0x"));

	return (MUNIT_OK);
}


/* Test Vectors from RFC 4648 */
static MunitResult
test_bytes_from_base64(const MunitParameter *params, void *data)
{
	const struct {
		char *input;
		char *expected;
	} vectors[] = {
		{ .input = "",         .expected = "" },
		{ .input = "Zg==",     .expected = "f" },
		{ .input = "Zm8=",     .expected = "fo" },
		{ .input = "Zm9v",     .expected = "foo" },
		{ .input = "Zm9vYg==", .expected = "foob" },
		{ .input = "Zm9vYmE=", .expected = "fooba" },
		{ .input = "Zm9vYmFy", .expected = "foobar" },
	};

	for (int i = 0; i < (sizeof(vectors) / sizeof(*vectors)); i++) {
		const char *input = vectors[i].input;
		const char *expected = vectors[i].expected;

		struct bytes *buf = bytes_from_base64(input);
		if (buf == NULL)
			munit_error("bytes_from_base64");

		munit_assert_size(buf->len, ==, strlen(expected));
		munit_assert_memory_equal(buf->len, buf->data, expected);

		bytes_free(buf);
	}

	/* when NULL is given */
	munit_assert_null(bytes_from_base64(NULL));

	/* test with a string not base64-encoded */
	munit_assert_null(bytes_from_base64("!base64"));

	return (MUNIT_OK);
}


static MunitResult
test_bytes_copy(const MunitParameter *params, void *data)
{
	const struct {
		char *input;
	} vectors[] = {
		{ .input = "" },
		{ .input = "f" },
		{ .input = "fo" },
		{ .input = "foo" },
		{ .input = "foob" },
		{ .input = "fooba" },
		{ .input = "foobar" },
	};

	for (int i = 0; i < (sizeof(vectors) / sizeof(*vectors)); i++) {
		const char *input = vectors[i].input;

		struct bytes *buf = bytes_from_str(input);
		if (buf == NULL)
			munit_error("bytes_from_str");

		struct bytes *cpy = bytes_copy(buf);
		if (cpy == NULL)
			munit_error("bytes_copy");

		munit_assert_size(buf->len, ==, cpy->len);
		munit_assert_memory_equal(buf->len, buf->data, cpy->data);

		bytes_free(cpy);
		bytes_free(buf);
	}

	/* when NULL is given */
	munit_assert_null(bytes_copy(NULL));

	return (MUNIT_OK);
}


/* first part of Set 1 / Challenge 6 */
static MunitResult
test_bytes_hamming_distance(const MunitParameter *params, void *data)
{
	const char *a = "this is a test";
	const char *b = "wokka wokka!!!";

	struct bytes *abuf = bytes_from_str(a);
	struct bytes *bbuf = bytes_from_str(b);
	if (abuf == NULL || bbuf == NULL)
		munit_error("bytes_from_str");

	int retval = bytes_hamming_distance(abuf, bbuf);
	munit_assert_int(retval, ==, 37);

	bytes_free(bbuf);
	bytes_free(abuf);
	return (MUNIT_OK);
}


static MunitResult
test_bytes_to_str(const MunitParameter *params, void *data)
{
	const struct {
		char *input;
		char *expected;
	} vectors[] = {
		{ .input = "",       .expected = "" },
		{ .input = "f",      .expected = "f" },
		{ .input = "fo",     .expected = "fo" },
		{ .input = "foo",    .expected = "foo" },
		{ .input = "foob",   .expected = "foob" },
		{ .input = "fooba",  .expected = "fooba" },
		{ .input = "foobar", .expected = "foobar" },
	};

	for (int i = 0; i < (sizeof(vectors) / sizeof(*vectors)); i++) {
		const char *input = vectors[i].input;
		const char *expected = vectors[i].expected;

		struct bytes *buf = bytes_from_str(input);
		if (buf == NULL)
			munit_error("bytes_from_str");

		char *result = bytes_to_str(buf);
		if (result == NULL)
			munit_error("bytes_to_str");

		munit_assert_string_equal(result, expected);

		free(result);
		bytes_free(buf);
	}

	/* when NULL is given */
	munit_assert_null(bytes_to_str(NULL));

	return (MUNIT_OK);
}


static MunitResult
test_bytes_to_hex(const MunitParameter *params, void *data)
{
	const struct {
		char *input;
		char *expected;
	} vectors[] = {
		{ .input = "",       .expected = "" },
		{ .input = "f",      .expected = "66" },
		{ .input = "fo",     .expected = "666F" },
		{ .input = "foo",    .expected = "666F6F" },
		{ .input = "foob",   .expected = "666F6F62" },
		{ .input = "fooba",  .expected = "666F6F6261" },
		{ .input = "foobar", .expected = "666F6F626172" },
	};

	for (int i = 0; i < (sizeof(vectors) / sizeof(*vectors)); i++) {
		const char *input = vectors[i].input;
		const char *expected = vectors[i].expected;

		struct bytes *buf = bytes_from_str(input);
		if (buf == NULL)
			munit_error("bytes_from_str");

		char *result = bytes_to_hex(buf);
		if (result == NULL)
			munit_error("bytes_to_hex");

		munit_assert_string_equal(result, expected);

		free(result);
		bytes_free(buf);
	}

	/* when NULL is given */
	munit_assert_null(bytes_to_hex(NULL));

	return (MUNIT_OK);
}


/* Test Vectors from RFC 4648 */
static MunitResult
test_bytes_to_base64(const MunitParameter *params, void *data)
{
	const struct {
		char *input;
		char *expected;
	} vectors[] = {
		{ .input = "",       .expected = "" },
		{ .input = "f",      .expected = "Zg==" },
		{ .input = "fo",     .expected = "Zm8=" },
		{ .input = "foo",    .expected = "Zm9v" },
		{ .input = "foob",   .expected = "Zm9vYg==" },
		{ .input = "fooba",  .expected = "Zm9vYmE=" },
		{ .input = "foobar", .expected = "Zm9vYmFy" },
	};

	for (int i = 0; i < (sizeof(vectors) / sizeof(*vectors)); i++) {
		const char *input = vectors[i].input;
		const char *expected = vectors[i].expected;

		struct bytes *buf = bytes_from_str(input);
		if (buf == NULL)
			munit_error("bytes_from_str");

		char *result = bytes_to_base64(buf);
		if (result == NULL)
			munit_error("bytes_to_base64");

		munit_assert_string_equal(result, expected);

		free(result);
		bytes_free(buf);
	}

	/* when NULL is given */
	munit_assert_null(bytes_to_base64(NULL));

	return (MUNIT_OK);
}


/* Set 1 / Challenge 1 */
static MunitResult
test_bytes_hex_to_base64(const MunitParameter *params, void *data)
{
	const char *hex = "49276d206b696c6c696e6720796f757220627261696e206c696b65206120706f69736f6e6f7573206d757368726f6f6d";
	const char *expected = "SSdtIGtpbGxpbmcgeW91ciBicmFpbiBsaWtlIGEgcG9pc29ub3VzIG11c2hyb29t";

	struct bytes *buf = bytes_from_hex(hex);
	if (buf == NULL)
		munit_error("bytes_from_hex");

	char *result = bytes_to_base64(buf);
	if (result == NULL)
		munit_error("bytes_to_base64");

	munit_assert_string_equal(result, expected);

	free(result);
	bytes_free(buf);
	return (MUNIT_OK);
}


/* The test suite. */
MunitTest test_bytes_suite_tests[] = {
	{ "bytes_from_raw",         test_bytes_from_raw,         NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_from_single",      test_bytes_from_single,      NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_from_str",         test_bytes_from_str,         NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_from_hex",         test_bytes_from_hex,         NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_from_base64",      test_bytes_from_base64,      NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_copy",             test_bytes_copy,             NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_hamming_distance", test_bytes_hamming_distance, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_to_str",           test_bytes_to_str,           NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_to_hex",           test_bytes_to_hex,           NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_to_base64",        test_bytes_to_base64,        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "bytes_hex_to_base64",    test_bytes_hex_to_base64,    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{
		.name       = NULL,
		.test       = NULL,
		.setup      = NULL,
		.tear_down  = NULL,
		.options    = MUNIT_TEST_OPTION_NONE,
		.parameters = NULL,
	},
};

/*
 * test_break_dh.c
 */
#include "munit.h"
#include "helpers.h"
#include "dh.h"
#include "break_dh.h"
#include "test_dh.h"


static MunitResult
mitm_dh_helper(const MunitParameter *params, void *data, enum dh_mitm_type type)
{
	/* XXX: this test does a little too much, it test both the exchange w/
	   MITM and the echo message */
	struct mpi *p = mpi_from_hex(nist_p_hex);
	struct mpi *g = mpi_from_hex(nist_g_hex);
	if (p == NULL || g == NULL)
		munit_error("mpi_from_hex");

	struct dh *alice = dh_new();
	if (alice == NULL)
		munit_error("dh_new");

	struct dh *mallory = dh_mitm_new(type, /* bob */dh_new());
	if (mallory == NULL)
		munit_error("dh_mitm_new");

	int ret = alice->exchange(alice, mallory, p, g);
	if (ret != 0)
		munit_error("dh exchange");

	struct bytes *message = bytes_from_str("All we have to decide is what to do with the time that is given us.");
	if (message == NULL)
		munit_error("bytes_from_str");

	ret = alice->challenge(alice, mallory, message);
	munit_assert_int(ret, ==, 0);

	const struct dh_mitm_opaque *dhinfo = mallory->opaque;
	munit_assert_not_null(dhinfo);
	munit_assert_size(dhinfo->count, ==, 1);
	munit_assert_not_null(dhinfo->messages);
	munit_assert_not_null(dhinfo->messages[0]);
	munit_assert_size(dhinfo->messages[0]->len, ==, message->len);
	munit_assert_memory_equal(message->len,
		    dhinfo->messages[0]->data, message->data);

	bytes_free(message);
	mallory->free(mallory);
	alice->free(alice);
	mpi_free(g);
	mpi_free(p);
	return (MUNIT_OK);
}


/* Set 5 / Challenge 34 (second part, MITM attack) */
static MunitResult
test_mitm_dh_p_as_a(const MunitParameter *params, void *data)
{
	return (mitm_dh_helper(params, data, DH_MITM_P_AS_A));
}


/* Set 5 / Challenge 35 (first part, g = 1) */
static MunitResult
test_mitm_dh_1_as_g(const MunitParameter *params, void *data)
{
	return (mitm_dh_helper(params, data, DH_MITM_1_AS_G));
}


/* Set 5 / Challenge 35 (second part, g = p) */
static MunitResult
test_mitm_dh_p_as_g(const MunitParameter *params, void *data)
{
	return (mitm_dh_helper(params, data, DH_MITM_P_AS_G));
}


/* Set 5 / Challenge 35 (third part, g = p - 1) */
static MunitResult
test_mitm_dh_p_minus_1_as_g(const MunitParameter *params, void *data)
{
	return (mitm_dh_helper(params, data, DH_MITM_P_MINUS_1_AS_G));
}


/* The test suite. */
MunitTest test_break_dh_suite_tests[] = {
	{ "mitm-0", test_mitm_dh_p_as_a, srand_reset, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "mitm-1", test_mitm_dh_1_as_g, srand_reset, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "mitm-2", test_mitm_dh_p_as_g, srand_reset, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "mitm-3", test_mitm_dh_p_minus_1_as_g, srand_reset, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{
		.name       = NULL,
		.test       = NULL,
		.setup      = NULL,
		.tear_down  = NULL,
		.options    = MUNIT_TEST_OPTION_NONE,
		.parameters = NULL,
	},
};

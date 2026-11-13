// SPDX-License-Identifier: GPL-2.0-only OR MIT
/*
 * Behaviour of the software camera indicator state. The same file builds as
 * a KUnit suite in the kernel and as a plain program (-DDCP_IND_USERSPACE).
 */
#ifdef DCP_IND_USERSPACE
#include "kunit-shim.h"
#else
#include <kunit/test.h>
#include <linux/module.h>
#endif

#include "dcp-indicator.h"

static void indicator_starts_off(struct kunit *test)
{
	struct dcp_indicator ind = {};

	KUNIT_EXPECT_EQ(test, ind.state, DCP_IND_OFF);
}

/* Confirmed only after a present that carried the square, never before. */
static void indicator_confirmed_only_after_present(struct kunit *test)
{
	struct dcp_indicator ind = {};

	KUNIT_ASSERT_EQ(test, dcp_ind_begin(&ind), 0);
	KUNIT_EXPECT_EQ(test, ind.state, DCP_IND_PENDING);
	/* A present without the square decides nothing. */
	dcp_ind_present_done(&ind, false);
	KUNIT_EXPECT_EQ(test, ind.state, DCP_IND_PENDING);
	dcp_ind_present_done(&ind, true);
	KUNIT_EXPECT_EQ(test, ind.state, DCP_IND_LIT);
}

/* Presents never start an indicator that nobody asked for. */
static void indicator_idle_ignores_presents(struct kunit *test)
{
	struct dcp_indicator ind = {};

	dcp_ind_present_done(&ind, true);
	KUNIT_EXPECT_EQ(test, ind.state, DCP_IND_OFF);
	KUNIT_ASSERT_EQ(test, dcp_ind_begin(&ind), 0);
	dcp_ind_present_done(&ind, true);
	dcp_ind_end(&ind);
	dcp_ind_present_done(&ind, true);
	KUNIT_EXPECT_EQ(test, ind.state, DCP_IND_OFF);
}

/* One owner at a time; ending frees it for the next stream. */
static void indicator_single_owner(struct kunit *test)
{
	struct dcp_indicator ind = {};

	KUNIT_ASSERT_EQ(test, dcp_ind_begin(&ind), 0);
	dcp_ind_present_done(&ind, true);
	KUNIT_EXPECT_EQ(test, dcp_ind_begin(&ind), -EBUSY);
	KUNIT_EXPECT_EQ(test, ind.state, DCP_IND_LIT);
	dcp_ind_end(&ind);
	KUNIT_EXPECT_EQ(test, dcp_ind_begin(&ind), 0);
}

/* Only the kernel's own framebuffer may sit on the reserved plane. */
static void indicator_plane_reservation(struct kunit *test)
{
	/* An ordinary plane is never restricted. */
	KUNIT_EXPECT_TRUE(test, dcp_ind_plane_ok(false, true, false));
	KUNIT_EXPECT_TRUE(test, dcp_ind_plane_ok(false, true, true));
	/* The reserved plane: disabled or the kernel framebuffer only. */
	KUNIT_EXPECT_TRUE(test, dcp_ind_plane_ok(true, false, false));
	KUNIT_EXPECT_TRUE(test, dcp_ind_plane_ok(true, true, true));
	KUNIT_EXPECT_FALSE(test, dcp_ind_plane_ok(true, false, true));
}

static struct kunit_case indicator_cases[] = {
	KUNIT_CASE(indicator_starts_off),
	KUNIT_CASE(indicator_confirmed_only_after_present),
	KUNIT_CASE(indicator_idle_ignores_presents),
	KUNIT_CASE(indicator_single_owner),
	KUNIT_CASE(indicator_plane_reservation),
	{}
};

static struct kunit_suite indicator_suite = {
	.name = "apple-dcp-indicator",
	.test_cases = indicator_cases,
};

#ifdef DCP_IND_USERSPACE
KUNIT_SHIM_MAIN(indicator_suite)
#else
kunit_test_suite(indicator_suite);
MODULE_DESCRIPTION("Software camera indicator state tests");
MODULE_LICENSE("Dual MIT/GPL");
#endif

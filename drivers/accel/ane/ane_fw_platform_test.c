// SPDX-License-Identifier: GPL-2.0-only OR MIT
#include <kunit/test.h>
#include <linux/module.h>

#include "ane_fw_platform.h"

static void ane_preload_ranges_test(struct kunit *test)
{
	const struct ane_fw_platform soc = {
		.preload_dva = { 0x10000000000, 0x100000c0000 },
		.preload_size = { 0xc0000, 0x2ac000 },
	};
	u64 phys[] = { 0x100000000, 0x200000000 };
	u64 size[] = { 0xc0000, 0x2ac000 };
	u64 dva[] = { 0x10000000000, 0x100000c0000 };

	KUNIT_EXPECT_TRUE(test, ane_fw_preload_ranges_valid(&soc, phys, size, dva));
	/* Physical placement may change independently on every boot. */
	phys[0] = 0x300000000;
	KUNIT_EXPECT_TRUE(test, ane_fw_preload_ranges_valid(&soc, phys, size, dva));
	phys[0] = phys[1] + SZ_16K;
	KUNIT_EXPECT_FALSE(test, ane_fw_preload_ranges_valid(&soc, phys, size, dva));
	phys[0] = phys[1] - size[0];
	KUNIT_EXPECT_TRUE(test, ane_fw_preload_ranges_valid(&soc, phys, size, dva));
	phys[0]++;
	KUNIT_EXPECT_FALSE(test, ane_fw_preload_ranges_valid(&soc, phys, size, dva));
	phys[0] = 0;
	KUNIT_EXPECT_FALSE(test, ane_fw_preload_ranges_valid(&soc, phys, size, dva));
	phys[0] = U64_MAX - SZ_16K + 1;
	KUNIT_EXPECT_FALSE(test, ane_fw_preload_ranges_valid(&soc, phys, size, dva));
	phys[0] = 0x100000000;
	size[1] -= SZ_16K;
	KUNIT_EXPECT_FALSE(test, ane_fw_preload_ranges_valid(&soc, phys, size, dva));
	size[1] = 0x2ac000;
	dva[0] = 0x100000c0000;
	dva[1] = 0x10000000000;
	KUNIT_EXPECT_FALSE(test, ane_fw_preload_ranges_valid(&soc, phys, size, dva));
}

/*
 * iBoot loads one of two T6050 firmware builds: TEXT 0xc0000 / DATA 0x2ac000
 * (M5 Pro J714s) or TEXT 0xec000 / DATA 0x280000 (M5 Max K114c, read from its
 * live ADT). Both end at the same DVA. Each has its own pinned TEXT digest.
 */
static void ane_t6050_preload_layouts_test(struct kunit *test)
{
	static const u8 j714s_pin[] = { 0x93, 0xae, 0x3a, 0x37 };
	static const u8 k114c_pin[] = { 0xd1, 0x66, 0xdb, 0x76 };
	const struct ane_fw_platform *soc = &ane_t6050_platform;
	u64 phys[] = { 0x10000de8000, 0x100022c4000 };
	u64 j714s_size[] = { 0xc0000, 0x2ac000 }, j714s_dva[] = { 0x10000000000, 0x100000c0000 };
	u64 k114c_size[] = { 0xec000, 0x280000 }, k114c_dva[] = { 0x10000000000, 0x100000ec000 };
	u64 mixed_size[] = { 0xec000, 0x2ac000 };
	const u8 *pin;

	KUNIT_EXPECT_TRUE(test, ane_fw_preload_ranges_valid(soc, phys, j714s_size, j714s_dva));
	KUNIT_EXPECT_TRUE(test, ane_fw_preload_ranges_valid(soc, phys, k114c_size, k114c_dva));
	KUNIT_EXPECT_FALSE(test, ane_fw_preload_ranges_valid(soc, phys, k114c_size, j714s_dva));
	KUNIT_EXPECT_FALSE(test, ane_fw_preload_ranges_valid(soc, phys, mixed_size, k114c_dva));

	pin = ane_fw_preload_text_pin(soc, j714s_size, j714s_dva);
	KUNIT_ASSERT_NOT_NULL(test, pin);
	KUNIT_EXPECT_MEMEQ(test, pin, j714s_pin, sizeof(j714s_pin));
	pin = ane_fw_preload_text_pin(soc, k114c_size, k114c_dva);
	KUNIT_ASSERT_NOT_NULL(test, pin);
	KUNIT_EXPECT_MEMEQ(test, pin, k114c_pin, sizeof(k114c_pin));
	KUNIT_EXPECT_NULL(test, ane_fw_preload_text_pin(soc, mixed_size, k114c_dva));
	/* H17 (T8140) has one build only. */
	KUNIT_EXPECT_NULL(test, ane_fw_preload_text_pin(&ane_h17_platform, k114c_size, k114c_dva));
}

/*
 * H16 and H17 devices (any firmware platform) take the backend's sleep path,
 * never the rtclient one; only the H13/H14 rtclient (no platform) does.
 */
static void ane_platform_sleep_path_test(struct kunit *test)
{
	static const struct ane_fw_platform h16 = { .soc = 0x6041 }, h17 = { .soc = 0x8140 };

	KUNIT_EXPECT_EQ(test, ane_fw_sleep_path(&h16), ANE_SLEEP_FW_BACKEND);
	KUNIT_EXPECT_EQ(test, ane_fw_sleep_path(&h17), ANE_SLEEP_FW_BACKEND);
	KUNIT_EXPECT_EQ(test, ane_fw_sleep_path(NULL), ANE_SLEEP_RTCLIENT);
}

/*
 * Only the staged backend's platform (H17, Exclave-owned firmware) binds
 * the device for the backend; H16 keeps its own probe; the rtclient has no
 * platform. A bound H17 device still takes the backend's sleep path.
 */
static void ane_platform_staged_bind_test(struct kunit *test)
{
	static const struct ane_fw_platform h16 = { .soc = 0x6041, .owner = ANE_FW_OWNER_AP },
		h17 = { .soc = 0x8140, .owner = ANE_FW_OWNER_EXCLAVE };

	KUNIT_EXPECT_TRUE(test, ane_fw_platform_staged(&h17));
	KUNIT_EXPECT_FALSE(test, ane_fw_platform_staged(&h16));
	KUNIT_EXPECT_FALSE(test, ane_fw_platform_staged(NULL));
	KUNIT_EXPECT_EQ(test, ane_fw_sleep_path(&h17), ANE_SLEEP_FW_BACKEND);
}

static struct kunit_case ane_fw_platform_cases[] = {
	KUNIT_CASE(ane_platform_staged_bind_test),
	KUNIT_CASE(ane_platform_sleep_path_test),
	KUNIT_CASE(ane_preload_ranges_test),
	KUNIT_CASE(ane_t6050_preload_layouts_test),
	{}
};

static struct kunit_suite ane_fw_platform_suite = {
	.name = "ane-fw-platform",
	.test_cases = ane_fw_platform_cases,
};

kunit_test_suite(ane_fw_platform_suite);

MODULE_DESCRIPTION("KUnit tests for ANE firmware preload range validation");
MODULE_LICENSE("Dual MIT/GPL");

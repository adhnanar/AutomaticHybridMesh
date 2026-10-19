/* TASK025 pure-store regression tests (run in locked and host-allow builds). */
#include <string.h>
#include <zephyr/ztest.h>
#include "sim_flash.h"

#define TOKEN 0xA5C3E1F7u

ZTEST(os_store, test_blank_is_denied_never_implicitly_armed)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;

	memset(&f, 0, sizeof(f));
	sim_erase(&f);
	s = sim_ops(&f);
	zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_BLANK);
	zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_E_NOT_ARMED);
	zassert_equal(f.writes, 0, "firmware must not write a blank store");
	zassert_equal(f.erases, 1);
}

ZTEST(os_store, test_armed_then_consume_then_reboot_stays_consumed)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;

	sim_provision(&f, TOKEN);
	s = sim_ops(&f);
	zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_ARMED);
	zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_OK);
	zassert_equal(f.writes, 1, "exactly one program, no erase");
	zassert_equal(f.erases, 0);
	/* "reboot": a fresh ops view over the same flash bytes */
	s = sim_ops(&f);
	zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_CONSUMED_STATE);
	zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_E_NOT_ARMED);
	zassert_equal(f.writes, 1);
}

ZTEST(os_store, test_arm_image_validation)
{
	uint8_t img[FRURT_OS_STORE_ARM_LEN];

	zassert_equal(frurt_os_store_arm_image(0, img), FRURT_OS_STORE_E_PARAM);
	zassert_equal(frurt_os_store_arm_image(0xFFFFFFFFu, img), FRURT_OS_STORE_E_PARAM);
	zassert_equal(frurt_os_store_arm_image(TOKEN, NULL), FRURT_OS_STORE_E_PARAM);
	zassert_equal(frurt_os_store_arm_image(TOKEN, img), FRURT_OS_STORE_OK);
	/* CRC-32 check value for "123456789" is 0xCBF43926 */
	zassert_equal(frurt_os_store_crc32((const uint8_t *)"123456789", 9), 0xCBF43926u);
}

ZTEST(os_store, test_corrupt_header_variants_denied)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;

	for (int byte = 0; byte < 16; byte++) {
		sim_provision(&f, TOKEN);
		f.mem[byte] ^= 0x01; /* flip one header bit (damage, not a legal program) */
		s = sim_ops(&f);
		zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_CORRUPT, "byte %d", byte);
		zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_E_NOT_ARMED);
		zassert_equal(f.writes, 0);
	}
}

ZTEST(os_store, test_wrong_version_and_bad_token_denied)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;
	uint8_t img[16];

	sim_provision(&f, TOKEN);
	memset(f.mem, 0xFF, 16);
	(void)frurt_os_store_arm_image(TOKEN, img);
	img[4] = 2; /* version 2 with a recomputed CRC must still be unknown => deny */
	uint32_t crc = frurt_os_store_crc32(img, 12);

	memcpy(img + 12, &crc, 4);
	memcpy(f.mem, img, 16);
	s = sim_ops(&f);
	zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_CORRUPT);
}

ZTEST(os_store, test_read_error_is_fail_closed)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;

	sim_provision(&f, TOKEN);
	f.read_fail = 1;
	s = sim_ops(&f);
	zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_IO_ERROR);
	zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_E_NOT_ARMED);
	zassert_equal(f.writes, 0);
	zassert_equal(frurt_os_store_state(NULL), FRURT_OS_STORE_IO_ERROR);
}

ZTEST(os_store, test_interrupted_write_no_change_denies_this_boot_and_stays_armed)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;

	sim_provision(&f, TOKEN);
	f.fault = SIM_FAIL_NOCHANGE;
	f.fault_at_write = 1;
	s = sim_ops(&f);
	zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_E_WRITE);
	/* nothing was committed, so the caller must not transmit; still ARMED is safe */
	zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_ARMED);
}

ZTEST(os_store, test_torn_write_reads_as_consumed_never_armed)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;

	sim_provision(&f, TOKEN);
	f.fault = SIM_TORN_ERROR;
	f.fault_at_write = 1;
	s = sim_ops(&f);
	zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_E_WRITE);
	zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_CONSUMED_STATE);
	zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_E_NOT_ARMED);
}

ZTEST(os_store, test_silent_torn_write_fails_verify)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;

	sim_provision(&f, TOKEN);
	f.fault = SIM_TORN_SILENT;
	f.fault_at_write = 1;
	s = sim_ops(&f);
	zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_E_VERIFY);
	zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_CONSUMED_STATE);
}

ZTEST(os_store, test_stuck_write_claiming_success_fails_verify)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;

	sim_provision(&f, TOKEN);
	f.fault = SIM_STUCK;
	f.fault_at_write = 1;
	s = sim_ops(&f);
	zassert_equal(frurt_os_store_consume(&s), FRURT_OS_STORE_E_VERIFY);
}

ZTEST(os_store, test_partial_programming_every_bit_pattern_is_not_armed)
{
	struct sim_flash f;
	struct frurt_os_store_ops s;

	/* any single cleared bit in the consume word means consumed */
	for (int bit = 0; bit < 32; bit++) {
		sim_provision(&f, TOKEN);
		f.mem[FRURT_OS_STORE_OFF_CONSUME + bit / 8] &= (uint8_t)~(1u << (bit % 8));
		s = sim_ops(&f);
		zassert_equal(frurt_os_store_state(&s), FRURT_OS_STORE_CONSUMED_STATE, "bit %d", bit);
	}
}

ZTEST_SUITE(os_store, NULL, NULL, NULL, NULL, NULL);

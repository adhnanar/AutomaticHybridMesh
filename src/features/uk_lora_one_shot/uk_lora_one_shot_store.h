/*
 * Frurt UK868 one-shot DURABLE authorization record (TASK025).
 *
 * Pure C, no Zephyr dependency. Fail-closed rules:
 *  - Firmware can never ARM. Arming is an external provisioning step that
 *    writes (after erasing) the 16-byte image from frurt_os_store_arm_image().
 *  - Blank, corrupt, unreadable, unknown-version or already-consumed storage
 *    all mean DENY. Nothing here is initialised implicitly.
 *  - The only firmware write is ONE 32-bit program of the consume word
 *    (0xFFFFFFFF -> marker). Programming only clears bits, so any torn or
 *    interrupted write leaves a value != 0xFFFFFFFF, which reads as CONSUMED.
 *    No erase is ever done by firmware (no wear, no erase-interrupted state).
 *  - frurt_os_store_consume() returns success only after the marker is read
 *    back exactly; the radio may be touched only after that.
 *
 * Layout (little-endian words, 24 bytes, must not share an erase page with
 * anything that firmware erases):
 *   +0  magic   FRURT_OS_STORE_MAGIC
 *   +4  version FRURT_OS_STORE_VERSION
 *   +8  token   external arm token, not 0 and not 0xFFFFFFFF
 *   +12 crc32   over bytes 0..11
 *   +16 consume 0xFFFFFFFF = armed/unconsumed; anything else = consumed
 *   +20 spare   must stay erased (0xFFFFFFFF); not interpreted
 *
 * Remaining hardware gate (NOT closed by this module): the flash driver must
 * provide word-granular programming with monotonic bit clearing and no
 * read-back of a stale 1 after a power cut. See docs/TASK025-*.md.
 */
#ifndef UK_LORA_ONE_SHOT_STORE_H_
#define UK_LORA_ONE_SHOT_STORE_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Production firmware must NOT define this. Until a real flash adapter is
 * reviewed against the acceptance gate, the allow build refuses to compile. */
#ifndef FRURT_UK_ONE_SHOT_STORE_ACCEPTED
#define FRURT_UK_ONE_SHOT_STORE_ACCEPTED 0
#endif

#define FRURT_OS_STORE_MAGIC    0x52554652u /* "FRUR" */
#define FRURT_OS_STORE_VERSION  1u
#define FRURT_OS_STORE_CONSUMED 0x534E4F43u /* "CONS" */
#define FRURT_OS_STORE_ERASED   0xFFFFFFFFu
#define FRURT_OS_STORE_SIZE     24u
#define FRURT_OS_STORE_ARM_LEN  16u
#define FRURT_OS_STORE_OFF_CONSUME 16u

enum frurt_os_store_state {
	FRURT_OS_STORE_ARMED = 0,
	FRURT_OS_STORE_CONSUMED_STATE = 1,
	FRURT_OS_STORE_BLANK = 2,   /* never provisioned */
	FRURT_OS_STORE_CORRUPT = 3, /* bad magic/version/crc/token */
	FRURT_OS_STORE_IO_ERROR = 4,
};

enum frurt_os_store_result {
	FRURT_OS_STORE_OK = 0,
	FRURT_OS_STORE_E_NOT_ARMED = -1,
	FRURT_OS_STORE_E_WRITE = -2,    /* write error: state unknown => treat as consumed */
	FRURT_OS_STORE_E_VERIFY = -3,   /* readback mismatch */
	FRURT_OS_STORE_E_PARAM = -4,
};

struct frurt_os_store_ops {
	/* Both return 0 on success. write() programs (clears bits) only, word aligned. */
	int (*read)(void *ctx, uint32_t off, void *buf, size_t len);
	int (*write)(void *ctx, uint32_t off, const void *buf, size_t len);
	void *ctx;
};

uint32_t frurt_os_store_crc32(const uint8_t *data, size_t len);

/* Build the 16-byte arm image (magic, version, token, crc). token must not be 0/0xFFFFFFFF. */
int frurt_os_store_arm_image(uint32_t token, uint8_t out[FRURT_OS_STORE_ARM_LEN]);

/* Classify storage. Read-only. */
enum frurt_os_store_state frurt_os_store_state(const struct frurt_os_store_ops *s);

/* Durably consume the shot. OK only if it was ARMED and the marker reads back exactly. */
int frurt_os_store_consume(const struct frurt_os_store_ops *s);

#ifdef __cplusplus
}
#endif

#endif /* UK_LORA_ONE_SHOT_STORE_H_ */

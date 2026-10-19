#include "uk_lora_one_shot_store.h"

static void put32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}

static uint32_t get32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

uint32_t frurt_os_store_crc32(const uint8_t *data, size_t len)
{
	uint32_t crc = 0xFFFFFFFFu;

	for (size_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int b = 0; b < 8; b++) {
			crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
		}
	}
	return ~crc;
}

int frurt_os_store_arm_image(uint32_t token, uint8_t out[FRURT_OS_STORE_ARM_LEN])
{
	if (out == NULL || token == 0u || token == FRURT_OS_STORE_ERASED) {
		return FRURT_OS_STORE_E_PARAM;
	}
	put32(out, FRURT_OS_STORE_MAGIC);
	put32(out + 4, FRURT_OS_STORE_VERSION);
	put32(out + 8, token);
	put32(out + 12, frurt_os_store_crc32(out, 12));
	return FRURT_OS_STORE_OK;
}

enum frurt_os_store_state frurt_os_store_state(const struct frurt_os_store_ops *s)
{
	uint8_t b[FRURT_OS_STORE_SIZE];
	int all_erased = 1;

	if (s == NULL || s->read == NULL || s->write == NULL ||
	    s->read(s->ctx, 0, b, sizeof(b)) != 0) {
		return FRURT_OS_STORE_IO_ERROR;
	}
	for (size_t i = 0; i < FRURT_OS_STORE_ARM_LEN; i++) {
		if (b[i] != 0xFF) {
			all_erased = 0;
		}
	}
	if (all_erased) {
		return FRURT_OS_STORE_BLANK;
	}
	uint32_t token = get32(b + 8);

	if (get32(b) != FRURT_OS_STORE_MAGIC || get32(b + 4) != FRURT_OS_STORE_VERSION ||
	    token == 0u || token == FRURT_OS_STORE_ERASED ||
	    get32(b + 12) != frurt_os_store_crc32(b, 12)) {
		return FRURT_OS_STORE_CORRUPT;
	}
	return get32(b + FRURT_OS_STORE_OFF_CONSUME) == FRURT_OS_STORE_ERASED
		       ? FRURT_OS_STORE_ARMED
		       : FRURT_OS_STORE_CONSUMED_STATE;
}

int frurt_os_store_consume(const struct frurt_os_store_ops *s)
{
	uint8_t w[4];
	uint8_t rb[4];

	if (frurt_os_store_state(s) != FRURT_OS_STORE_ARMED) {
		return FRURT_OS_STORE_E_NOT_ARMED;
	}
	put32(w, FRURT_OS_STORE_CONSUMED);
	if (s->write(s->ctx, FRURT_OS_STORE_OFF_CONSUME, w, sizeof(w)) != 0) {
		return FRURT_OS_STORE_E_WRITE;
	}
	if (s->read(s->ctx, FRURT_OS_STORE_OFF_CONSUME, rb, sizeof(rb)) != 0 ||
	    get32(rb) != FRURT_OS_STORE_CONSUMED ||
	    frurt_os_store_state(s) != FRURT_OS_STORE_CONSUMED_STATE) {
		return FRURT_OS_STORE_E_VERIFY;
	}
	return FRURT_OS_STORE_OK;
}

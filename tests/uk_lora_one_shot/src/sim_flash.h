/* Host-only NOR-flash simulator for the TASK025 durable one-shot store tests.
 * Programming can only clear bits (AND). Fault injection covers power cuts. */
#ifndef SIM_FLASH_H_
#define SIM_FLASH_H_

#include <stdint.h>
#include <string.h>
#include "uk_lora_one_shot_store.h"

enum sim_fault {
	SIM_OK = 0,
	SIM_FAIL_NOCHANGE, /* power cut before any bit changed; write reports error */
	SIM_TORN_ERROR,    /* half the bits programmed, write reports error */
	SIM_TORN_SILENT,   /* half the bits programmed, write claims success */
	SIM_STUCK,         /* write claims success but nothing changed */
};

struct sim_flash {
	uint8_t mem[64];
	unsigned int writes;
	unsigned int reads;
	enum sim_fault fault;
	unsigned int fault_at_write; /* 1-based write index that is faulty, 0 = none */
	int read_fail;               /* every read fails when set */
	unsigned int erases;         /* only the provisioning helper may erase */
};

static inline int sim_read(void *c, uint32_t off, void *buf, size_t len)
{
	struct sim_flash *f = c;

	f->reads++;
	if (f->read_fail || off + len > sizeof(f->mem)) {
		return -1;
	}
	memcpy(buf, f->mem + off, len);
	return 0;
}

static inline int sim_write(void *c, uint32_t off, const void *buf, size_t len)
{
	struct sim_flash *f = c;
	const uint8_t *src = buf;
	int fault = (f->fault_at_write == f->writes + 1u) ? f->fault : SIM_OK;

	f->writes++;
	if (off + len > sizeof(f->mem) || (off & 3u) || (len & 3u)) {
		return -1;
	}
	if (fault == SIM_FAIL_NOCHANGE) {
		return -2;
	}
	if (fault == SIM_STUCK) {
		return 0;
	}
	for (size_t i = 0; i < len; i++) {
		uint8_t v = src[i];

		if (fault == SIM_TORN_ERROR || fault == SIM_TORN_SILENT) {
			v |= 0xF0; /* only the low nibble was programmed */
		}
		f->mem[off + i] &= v;
	}
	return fault == SIM_TORN_ERROR ? -3 : 0;
}

static inline struct frurt_os_store_ops sim_ops(struct sim_flash *f)
{
	return (struct frurt_os_store_ops){ sim_read, sim_write, f };
}

static inline void sim_erase(struct sim_flash *f)
{
	memset(f->mem, 0xFF, sizeof(f->mem));
	f->erases++;
}

/* External provisioning step (not firmware): erase, then program the arm image. */
static inline void sim_provision(struct sim_flash *f, uint32_t token)
{
	uint8_t img[FRURT_OS_STORE_ARM_LEN];

	memset(f, 0, sizeof(*f));
	sim_erase(f);
	f->erases = 0;
	(void)frurt_os_store_arm_image(token, img);
	memcpy(f->mem, img, sizeof(img));
}

#endif

/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef NORTHBRIDGE_INTEL_SANDYBRIDGE_OC_PROFILE_H
#define NORTHBRIDGE_INTEL_SANDYBRIDGE_OC_PROFILE_H

#include <stdint.h>
#include <types.h>

#define OC_PROFILE_MAGIC	0x4f435031u	/* "OCP1" */
#define OC_PROFILE_VERSION	1
#define OC_PROFILE_NAME		"oc_memory_profile.bin"

/* Failed boots tolerated before the profile is skipped automatically. */
#define OC_PROFILE_RETRY_LIMIT	2

/*
 * A preprogrammed memory profile, read from CBFS when
 * CONFIG(NATIVE_RAMINIT_OC_PROFILE) is set.  A zero field leaves that value to
 * the devicetree and SPD path.  A valid profile also skips the XMP acceptance
 * rules and the automatic voltage cap, so the values are used as given (the
 * voltage is still clamped to the board's documented range).
 */
struct oc_memory_profile {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint16_t checksum;
	uint16_t voltage_mv;
	uint16_t max_mem_clock_mhz;
	uint8_t tcl;
	uint8_t trcd;
	uint8_t trp;
	uint8_t tras;
	uint16_t trfc;
	uint8_t cmd_rate;
	uint8_t reserved[3];
} __packed;

/* Returns the validated profile, or NULL when there is none. */
const struct oc_memory_profile *oc_profile_get(void);

/* Clears the failed-boot counter after a successful memory training. */
void oc_profile_boot_ok(void);

#endif /* NORTHBRIDGE_INTEL_SANDYBRIDGE_OC_PROFILE_H */

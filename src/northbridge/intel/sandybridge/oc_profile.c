/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <console/console.h>
#include <stddef.h>
#include <string.h>

#include "oc_profile.h"

static struct oc_memory_profile profile;
static bool loaded;
static bool valid;

static uint16_t profile_checksum(const struct oc_memory_profile *p)
{
	uint16_t sum = 0;
	const uint16_t *words = (const uint16_t *)p;

	for (size_t i = 0; i < sizeof(*p) / sizeof(*words); i++) {
		if (i == offsetof(struct oc_memory_profile, checksum) / sizeof(*words))
			continue;
		sum += words[i];
	}

	return sum;
}

const struct oc_memory_profile *oc_profile_get(void)
{
	if (loaded)
		return valid ? &profile : NULL;
	loaded = true;

	size_t size = 0;
	void *mapped = cbfs_map(OC_PROFILE_NAME, &size);
	if (!mapped || size < sizeof(profile))
		return NULL;

	memcpy(&profile, mapped, sizeof(profile));

	if (profile.magic != OC_PROFILE_MAGIC || profile.version != OC_PROFILE_VERSION ||
	    profile.size != sizeof(profile) || profile.checksum != profile_checksum(&profile)) {
		printk(BIOS_WARNING, "OC profile: %s is not a valid profile, ignored\n",
		       OC_PROFILE_NAME);
		return NULL;
	}

	valid = true;
	printk(BIOS_INFO, "OC profile: %u mV, %u MHz, tCL %u tRCD %u tRP %u tRAS %u "
	       "tRFC %u, command rate %u\n", profile.voltage_mv, profile.max_mem_clock_mhz,
	       profile.tcl, profile.trcd, profile.trp, profile.tras, profile.trfc,
	       profile.cmd_rate);

	return &profile;
}

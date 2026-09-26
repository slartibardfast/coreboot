/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __NORTHBRIDGE_INTEL_SANDYBRIDGE_DRAM_VOLTAGE_H__
#define __NORTHBRIDGE_INTEL_SANDYBRIDGE_DRAM_VOLTAGE_H__

#include <types.h>

/* One installed DIMM's operating-voltage evidence, as decoded from its SPD
   data. floor_contribution_mv is the DIMM's minimum operating voltage (1250,
   1350 or 1500 per the SPD operating-voltage bits); xmp_voltage_mv is 0 when
   the DIMM ships no XMP data. */
struct dram_dimm_voltage {
	int floor_contribution_mv;
	int xmp_voltage_mv;
};

/* Clamp a memory profile's requested DRAM voltage into the board's
   documented rail span. */
int dram_clamp_profile_voltage_mv(int requested_mv, int board_min_mv,
				  int board_max_mv);

/* The DRAM voltage the automatic path selects for a population of DIMMs:
   the JEDEC common denominator when a plain DIMM is installed, otherwise
   the lowest XMP request when one is usable, otherwise the population
   floor. Behaviour specified in
   src/drivers/i2c/nct3933u/dram-rail.allium (Population.selection_mv). */
int dram_auto_voltage_mv(const struct dram_dimm_voltage *dimms, size_t count,
			 int auto_min_mv, int auto_max_mv);

#endif /* __NORTHBRIDGE_INTEL_SANDYBRIDGE_DRAM_VOLTAGE_H__ */

/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <types.h>

#include "dram_voltage.h"

int dram_clamp_profile_voltage_mv(int requested_mv, int board_min_mv,
				  int board_max_mv)
{
	if (requested_mv < board_min_mv)
		return board_min_mv;
	if (requested_mv > board_max_mv)
		return board_max_mv;
	return requested_mv;
}

int dram_auto_voltage_mv(const struct dram_dimm_voltage *dimms, size_t count,
			 int auto_min_mv, int auto_max_mv)
{
	/* Clamped as the population is walked */
	int voltage_min = auto_min_mv;
	int voltage_max = auto_max_mv;

	/* Highest XMP-requested voltage, clamped down as the population is
	 * walked. If this is higher than the maximum permitted voltage, it
	 * means no XMP profile is used. */
	int voltage_xmp_min = voltage_max + 1;
	size_t i;

	for (i = 0; i < count; i++) {
		/* Clamp voltage_min to the highest minimum operation voltage
		 * when using the standard SPD profile. A plain DDR3 DIMM
		 * forces the JEDEC standard voltage maximum. */
		if (dimms[i].floor_contribution_mv >= 1500)
			voltage_min = 1500;
		else if (dimms[i].floor_contribution_mv > voltage_min)
			voltage_min = dimms[i].floor_contribution_mv;

		if (dimms[i].xmp_voltage_mv) {
			if (dimms[i].xmp_voltage_mv < voltage_xmp_min)
				voltage_xmp_min = dimms[i].xmp_voltage_mv;
			if (dimms[i].xmp_voltage_mv < voltage_max)
				voltage_max = dimms[i].xmp_voltage_mv;
		} else {
			/* Non-XMP DIMM. Revert to JEDEC standard voltage
			 * maximum */
			voltage_max = 1500;
		}
	}

	/* If the lowest XMP requested voltage is higher than the maximum,
	 * we can't use any XMP profiles and thus revert to SPD minimum voltage */
	if (voltage_xmp_min > voltage_max)
		return voltage_min;

	/* Lower than 1.5V XMP profiles also exist. Make sure we won't undervolt any
	 * non-DDR3L DIMMs if such low-voltage XMP DIMMs are installed. */
	if (voltage_xmp_min > voltage_min)
		return voltage_xmp_min;
	return voltage_min;
}

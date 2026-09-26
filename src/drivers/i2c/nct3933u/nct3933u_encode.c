/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <types.h>

#include "nct3933u_encode.h"

enum cb_err nct3933u_encode_voltage(int default_mv, int step_uv,
				    int voltage_mv, uint8_t *reg8)
{
	const int offset = (voltage_mv * 1000 - default_mv * 1000) / step_uv;

	/* Make sure the requested voltage is within the possible range */
	if ((offset > 127) || (offset < -127))
		return CB_ERR;

	/* Convert to sign-magnitude format used by the chip */
	*reg8 = (offset < 0) ? (-offset | 0x80) : offset;

	return CB_SUCCESS;
}

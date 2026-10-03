/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "fan_curve_encode.h"

bool fan_curve_valid(const uint8_t *temps, const uint8_t *duties)
{
	int i;

	if (duties[0] < NCT6776_FLOOR_MIN_DUTY)
		return false;

	for (i = 0; i < NCT6776_SF4_POINTS; i++) {
		if (temps[i] > NCT6776_TEMP_MAX_C)
			return false;
		if (i > 0) {
			if (temps[i] < temps[i - 1])
				return false;
			if (duties[i] < duties[i - 1])
				return false;
		}
	}

	/* The last point is the full-speed ceiling; the critical net sits
	   above everything the curve can command. */
	return duties[NCT6776_SF4_POINTS - 1] == NCT6776_DUTY_MAX;
}

bool fan_curve_floor_safe(uint8_t floor_duty)
{
	return floor_duty >= NCT6776_FLOOR_MIN_DUTY;
}

bool fan_curve_slew_safe(uint8_t up_ticks, uint8_t down_ticks)
{
	return down_ticks > up_ticks;
}

enum cb_err fan_curve_encode_sf4(uint16_t output_base, const uint8_t *temps,
				 const uint8_t *duties,
				 uint8_t critical_temp_c, uint8_t step_up_ticks,
				 uint8_t step_down_ticks,
				 struct nct6776_fan_program *prog)
{
	int i;

	if (!fan_curve_valid(temps, duties))
		return CB_ERR;

	if (critical_temp_c > NCT6776_TEMP_MAX_C)
		return CB_ERR;

	if (critical_temp_c <= temps[NCT6776_SF4_POINTS - 1])
		return CB_ERR;

	if (!fan_curve_slew_safe(step_up_ticks, step_down_ticks))
		return CB_ERR;

	prog->output_base = output_base;
	prog->mode_field = NCT6776_MODE_SF4;
	for (i = 0; i < NCT6776_SF4_POINTS; i++) {
		prog->point_temp[i] = temps[i];
		prog->point_duty[i] = duties[i];
	}
	prog->step_up_ticks = step_up_ticks;
	prog->step_down_ticks = step_down_ticks;
	prog->critical_temp_c = critical_temp_c;
	return CB_SUCCESS;
}

enum cb_err fan_curve_source_reg(uint8_t index, uint16_t *reg16)
{
	if (index >= NCT6776_SOURCE_COUNT)
		return CB_ERR;

	*reg16 = NCT6776_REG_SOURCE_BASE + index;
	return CB_SUCCESS;
}

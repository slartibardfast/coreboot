/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "isl6367_encode.h"

enum cb_err isl6367_encode_offset(int offset_mv, uint8_t *reg8)
{
	int steps;

	if (offset_mv < ISL6367_OFFSET_MIN_MV || offset_mv > ISL6367_OFFSET_MAX_MV)
		return CB_ERR;

	steps = offset_mv / ISL6367_STEP_MV;
	*reg8 = steps < 0 ? (uint8_t)(256 + steps) : (uint8_t)steps;
	return CB_SUCCESS;
}

void isl6367_decode_offset(uint8_t reg8, int *offset_mv)
{
	int steps = reg8 < 128 ? reg8 : reg8 - 256;

	*offset_mv = steps * ISL6367_STEP_MV;
}

enum cb_err isl6367_encode_fixed(int voltage_mv, uint8_t *reg8)
{
	int raw;

	if (voltage_mv < ISL6367_FIX_MIN_MV || voltage_mv > ISL6367_FIX_MAX_MV)
		return CB_ERR;

	raw = ISL6367_FIX_BASE_BYTE
	      + (voltage_mv - ISL6367_FIX_BASE_MV) / ISL6367_STEP_MV;
	*reg8 = raw < 256 ? (uint8_t)raw : (uint8_t)(raw - 256);
	return CB_SUCCESS;
}

void isl6367_decode_fixed(uint8_t reg8, int *voltage_mv)
{
	int steps_from_base = (reg8 - ISL6367_FIX_BASE_BYTE + 256) % 256;

	*voltage_mv = ISL6367_FIX_BASE_MV + steps_from_base * ISL6367_STEP_MV;
}

enum cb_err isl6367_encode_llc(int stored_value, uint8_t *d3_bits,
			       uint8_t *d4_bit)
{
	if (stored_value < 1 || stored_value > ISL6367_LLC_MAX_VALUE)
		return CB_ERR;

	*d3_bits = stored_value == 2 ? 3 : stored_value == 3 ? 2 :
		   stored_value == 4 ? 1 : 0;
	*d4_bit = stored_value == 1 ? 0 : 1;
	return CB_SUCCESS;
}

bool isl6367_offset_within_safe(int offset_mv)
{
	return offset_mv >= ISL6367_OFFSET_MIN_MV &&
	       offset_mv <= ISL6367_OFFSET_SAFE_MAX_MV;
}

bool isl6367_fixed_within_safe(int voltage_mv)
{
	return voltage_mv >= ISL6367_FIX_MIN_MV &&
	       voltage_mv <= ISL6367_FIX_SAFE_MAX_MV;
}

/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "oc_mailbox_encode.h"

uint32_t oc_mailbox_interface_word(uint8_t cmd, uint8_t plane, bool run_busy)
{
	return cmd | ((uint32_t)plane << 8) | (run_busy ? 1u << 31 : 0);
}

uint32_t oc_mailbox_data_word(int32_t offset_units, uint8_t target_mode,
			      uint16_t voltage_target, uint8_t max_ratio)
{
	return ((uint32_t)(offset_units & 0x7ff) << 21) |
	       ((uint32_t)(target_mode & 1) << 20) |
	       ((uint32_t)(voltage_target & 0xfff) << 8) |
	       (uint32_t)(max_ratio & 0xff);
}

int32_t oc_mailbox_offset_units(uint32_t data_word)
{
	uint32_t raw = (data_word >> 21) & 0x7ff;

	return raw & 0x400 ? (int32_t)raw - 2048 : (int32_t)raw;
}

/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <device/device.h>
#include <device/pnp.h>
#include <superio/hwm5_conf.h>
#include <superio/nuvoton/common/hwm.h>
#include <types.h>

#include "chip.h"
#include "fan_curve_encode.h"
#include "nct6776.h"

/* The source-register index for a PWM output's bank base: outputs 0x100,
   0x200, 0x300, 0x800, 0x900 and 0xa00 pair with the six temperature
   outputs in bank 6; 0xb00 has no paired source register. Returns -1 when
   the base is out of the paired set. */
static int hwm_source_index(uint16_t output_base)
{
	static const uint16_t bases[] = { 0x100, 0x200, 0x300, 0x800, 0x900,
					  0xa00 };

	for (size_t i = 0; i < ARRAY_SIZE(bases); i++) {
		if (bases[i] == output_base)
			return (int)i;
	}
	return -1;
}

static void hwm_write_source(const u16 base, const struct nct6776_fan_curve *c)
{
	int index = hwm_source_index(c->output_base);
	uint16_t reg16;
	enum cb_err err;

	if (index < 0) {
		printk(BIOS_WARNING, "nct6776: output base 0x%04x has no "
		       "paired source register\n", c->output_base);
		return;
	}

	err = fan_curve_source_reg((uint8_t)index, &reg16);
	if (err != CB_SUCCESS)
		return;

	nuvoton_hwm_select_bank(base, reg16 >> 8);
	pnp_write_hwm5_index(base, reg16 & 0xff, c->source);

	if (c->temp_offset) {
		/* The temperature offsets sit in bank 4, one register per
		   temperature output, in signed degrees C. */
		nuvoton_hwm_select_bank(base, 4);
		pnp_write_hwm5_index(base, (uint8_t)(0x54 + index),
				     (uint8_t)c->temp_offset);
	}
}

static void hwm_write_mode_field(const u16 base, uint16_t output_base,
				 uint8_t mode_field)
{
	u8 reg;

	nuvoton_hwm_select_bank(base, output_base >> 8);
	reg = pnp_read_hwm5_index(base, NCT6776_REG_BANK_FAN_MODE);
	reg = (reg & 0x8f) | (mode_field << 4);
	pnp_write_hwm5_index(base, NCT6776_REG_BANK_FAN_MODE, reg);
}

static void hwm_program_sf4(const u16 base, const struct nct6776_fan_curve *c)
{
	struct nct6776_fan_program prog;
	enum cb_err err;
	int i;

	err = fan_curve_encode_sf4(c->output_base, c->point_temp,
				   c->point_duty, c->critical_temp_c,
				   c->step_up_ticks, c->step_down_ticks,
				   &prog);
	if (err != CB_SUCCESS) {
		printk(BIOS_ERR, "nct6776: devicetree fan curve for output "
		       "0x%04x refused by the spec, output left untouched\n",
		       c->output_base);
		return;
	}

	nuvoton_hwm_select_bank(base, prog.output_base >> 8);
	for (i = 0; i < NCT6776_SF4_POINTS; i++) {
		pnp_write_hwm5_index(base,
				     (uint8_t)(NCT6776_REG_BANK_AUTO_TEMP + i),
				     prog.point_temp[i]);
		pnp_write_hwm5_index(base,
				     (uint8_t)(NCT6776_REG_BANK_AUTO_PWM + i),
				     prog.point_duty[i]);
	}
	pnp_write_hwm5_index(base, NCT6776_REG_BANK_STEP_UP,
			     prog.step_up_ticks);
	pnp_write_hwm5_index(base, NCT6776_REG_BANK_STEP_DOWN,
			     prog.step_down_ticks);
	pnp_write_hwm5_index(base, NCT6776_REG_BANK_CRITICAL_TEMP,
			     prog.critical_temp_c);

	hwm_write_mode_field(base, c->output_base, prog.mode_field);
}

static void hwm_program_cruise(const u16 base, const struct nct6776_fan_curve *c)
{
	u8 reg;

	nuvoton_hwm_select_bank(base, c->output_base >> 8);
	pnp_write_hwm5_index(base, NCT6776_REG_BANK_TARGET, c->target_temp_c);
	/* The tolerance rides the fan-mode register's low nibble. */
	reg = pnp_read_hwm5_index(base, NCT6776_REG_BANK_FAN_MODE);
	reg = (reg & 0xf0) | (c->tolerance_c & 0x0f);
	pnp_write_hwm5_index(base, NCT6776_REG_BANK_FAN_MODE, reg);
	hwm_write_mode_field(base, c->output_base,
			     NCT6776_MODE_THERMAL_CRUISE);
}

static void hwm_program_manual(const u16 base, const struct nct6776_fan_curve *c)
{
	nuvoton_hwm_select_bank(base, c->output_base >> 8);
	/* The live duty rides the PWM register, not the SmartFan IV point
	   table. */
	pnp_write_hwm5_index(base, NCT6776_REG_BANK_PWM, c->duty);
	hwm_write_mode_field(base, c->output_base, NCT6776_MODE_MANUAL);
}

static void hwm_program_output(const u16 base, const struct nct6776_fan_curve *c)
{
	if (c->mode == NCT6776_FAN_IGNORE)
		return;

	hwm_write_source(base, c);

	switch (c->mode) {
	case NCT6776_FAN_SF4:
		hwm_program_sf4(base, c);
		break;
	case NCT6776_FAN_THERMAL_CRUISE:
		hwm_program_cruise(base, c);
		break;
	case NCT6776_FAN_MANUAL:
		hwm_program_manual(base, c);
		break;
	default:
		break;
	}
}

void nct6776_hwm_init(const u16 base, const struct superio_nuvoton_nct6776_config *conf)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(conf->fan); i++)
		hwm_program_output(base, &conf->fan[i]);
}

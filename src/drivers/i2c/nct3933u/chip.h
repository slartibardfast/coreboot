/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DRIVERS_I2C_NCT3933U_CHIP_H__
#define __DRIVERS_I2C_NCT3933U_CHIP_H__

/* default_mv refers to the actual output voltage when the DAC is set to
   output 0. step_uv determines how much the voltage changes per DAC step.
   If step_uv is positive, voltage increases with positive values.
   If it's negative, voltage decreases with positive values. */
struct drivers_i2c_nct3933u_config {
	int step_delay_us;
	int default_mv[3];
	int step_uv[3];
};
#endif /* __DRIVERS_I2C_NCT3933U_CHIP_H__ */

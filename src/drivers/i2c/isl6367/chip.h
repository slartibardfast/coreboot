/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DRIVERS_I2C_ISL6367_CHIP_H__
#define __DRIVERS_I2C_ISL6367_CHIP_H__

/* The vcore settings this controller instance applies at boot. Zero or
   unset values leave the corresponding silicon registers untouched, so a
   board that declares the chip changes nothing until it opts in. Exactly
   one of the offset and fixed setpoints takes effect: the fixed target
   wins, since both ride the same register. */
struct drivers_i2c_isl6367_config {
	int vcore_offset_mv;	/* signed trim on the SVID voltage, mV */
	int vcore_fix_mv;	/* absolute target, mV */
	int loadline_level;	/* vendor setup stored value 1-5 */
};
#endif /* __DRIVERS_I2C_ISL6367_CHIP_H__ */

/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DRIVERS_I2C_NCT3933U_NCT3933U_ENCODE_H__
#define __DRIVERS_I2C_NCT3933U_NCT3933U_ENCODE_H__

#include <types.h>

/* Pure millivolt-to-register mapping: the signed step offset from
   default_mv, refused when it leaves the chip's 127-step span, written in
   sign-magnitude form otherwise. Behaviour specified in
   dram-rail.allium (RailProgrammed, RailRequestRefused, DacEncoding). */
enum cb_err nct3933u_encode_voltage(int default_mv, int step_uv,
				    int voltage_mv, uint8_t *reg8);

#endif /* __DRIVERS_I2C_NCT3933U_NCT3933U_ENCODE_H__ */

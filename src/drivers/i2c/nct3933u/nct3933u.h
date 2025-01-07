/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DRIVERS_I2C_NCT3933U_NCT3933U_H__
#define __DRIVERS_I2C_NCT3933U_NCT3933U_H__
#include <device/device.h>

enum cb_err nct3933u_set_voltage(const struct device *const dev, int voltage_mv);
#endif /* __DRIVERS_I2C_NCT3933U_NCT3933U_H__ */

/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DRIVERS_I2C_ISL6367_ISL6367_H__
#define __DRIVERS_I2C_ISL6367_ISL6367_H__

#include <device/device.h>

#include "isl6367_encode.h"

/* ISL6367 registers on the PCH SMBus, from the recovered map (see
   docs/z77-vcore-control.md in the governing host repository). */
#define ISL6367_REG_INIT_D1	0xD1
#define ISL6367_REG_LLC		0xD3
#define ISL6367_REG_LLC_EN	0xD4
#define ISL6367_REG_ARM		0xD6
#define ISL6367_REG_INIT_D8	0xD8
#define ISL6367_REG_INIT_D9	0xD9
#define ISL6367_REG_OFFSET_FIX	0xDB

enum cb_err isl6367_apply(const struct device *const dev);

#if CONFIG(DRIVERS_I2C_ISL6367_VCORE)
/* Applies the devicetree's vcore settings to the aliased controller. Called
   from romstage, before the CPU's SVID takes over. */
enum cb_err isl6367_vcore_apply(void);
#endif

#endif /* __DRIVERS_I2C_ISL6367_ISL6367_H__ */

## SPDX-License-Identifier: GPL-2.0-or-later

ramstage-$(CONFIG_DRIVERS_I2C_ISL6367) += isl6367.c
ramstage-$(CONFIG_DRIVERS_I2C_ISL6367) += isl6367_encode.c
romstage-$(CONFIG_DRIVERS_I2C_ISL6367_VCORE) += isl6367.c
romstage-$(CONFIG_DRIVERS_I2C_ISL6367_VCORE) += isl6367_encode.c

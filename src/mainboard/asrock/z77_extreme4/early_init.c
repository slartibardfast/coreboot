/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootblock_common.h>
#include <console/console.h>
#include <rules.h>
#include <superio/nuvoton/nct6776/nct6776.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <types.h>

void bootblock_mainboard_early_init(void)
{
	/* Enable early serial */
	if (CONFIG(CONSOLE_SERIAL))
		nuvoton_enable_serial(NCT6776_SP1, CONFIG_TTYS0_BASE);
}

#if ENV_SEPARATE_ROMSTAGE
#include <drivers/i2c/isl6367/isl6367.h>

void mainboard_early_init(bool s3resume);

void mainboard_early_init(bool s3resume)
{
	/* The core voltage settings ride the same preservation assumption as
	   the DRAM rail: on an S3 resume the silicon kept them, on any other
	   boot the board may have lost power, so apply them fresh. */
	if (!s3resume && CONFIG(DRIVERS_I2C_ISL6367_VCORE)) {
		if (isl6367_vcore_apply() == CB_SUCCESS)
			printk(BIOS_DEBUG, "Applied ISL6367 vcore settings\n");
		else
			printk(BIOS_WARNING, "ISL6367 vcore settings failed\n");
	}
}
#endif /* ENV_SEPARATE_ROMSTAGE */

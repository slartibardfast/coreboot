/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <console/console.h>
#include <device/device.h>
#include <device/smbus_host.h>
#include "chip.h"
#include "nct3933u.h"

#if CONFIG(DRIVERS_I2C_NCT3933U_DRAM)
#include <static.h>
#endif /* CONFIG(DRIVERS_I2C_NCT3933U_DRAM) */


#if !DEVTREE_EARLY
static void nct3933u_init(struct device *const dev);
#endif /* !DEVTREE_EARLY */


#if CONFIG(DRIVERS_I2C_NCT3933U_DRAM)
/* Called from raminit when NCT3933U is used to set DRAM voltage. */
enum cb_err set_dram_voltage(int voltage_mv);
enum cb_err set_dram_voltage(int voltage_mv)
{
	return nct3933u_set_voltage(_dev_v_dram_ptr, voltage_mv);
}
#endif /* CONFIG(DRIVERS_I2C_NCT3933U_DRAM) */

enum cb_err nct3933u_set_voltage(const struct device *const dev, int voltage_mv)
{
	/* Get address and channel from the devicetree */
	int i2c_address = dev->upstream->dev->path.i2c.device;
	int channel = dev->path.generic.id;

	/* Get board-specific voltage constants from devicetree */
	const struct drivers_i2c_nct3933u_config *cfg = dev->chip_info;
	int default_uv = 1000 * cfg->default_mv[channel - 1];
	int step_uv = cfg->step_uv[channel - 1];

	int offset = (voltage_mv * 1000 - default_uv) / step_uv;

	/* Make sure the requested voltage is within the possible range */
	if ((offset > 127) || (offset < -127))
		return CB_ERR;

	/* Convert to sign-magnitude format used by the chip */
	const uint8_t reg8 = (offset < 0) ? (-offset | 0x80) : offset;

	return do_smbus_write_byte(smbus_base(), i2c_address, channel, reg8);
}

#if !DEVTREE_EARLY
static void nct3933u_init(struct device *const dev)
{
	printk(BIOS_DEBUG, "nct3933u init\n");
}

static struct device_operations nct3933u_ops = {
	.read_resources = noop_read_resources,
	.set_resources  = noop_set_resources,
	.init           = nct3933u_init,
};

static void nct3933u_enable(struct device *const dev)
{
	dev->ops = &nct3933u_ops;
}

struct chip_operations drivers_i2c_nct3933u_ops = {
	.name = "NCT3933U",
	.enable_dev = nct3933u_enable
};
#endif /* !DEVTREE_EARLY */

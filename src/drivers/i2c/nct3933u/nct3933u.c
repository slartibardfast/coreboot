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
	uint8_t reg8;
	enum cb_err err;

	err = nct3933u_encode_voltage(cfg->default_mv[channel - 1],
				      cfg->step_uv[channel - 1],
				      voltage_mv, &reg8);
	if (err != CB_SUCCESS)
		return err;

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

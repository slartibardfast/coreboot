/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <device/device.h>
#include <device/smbus_host.h>
#include "chip.h"
#include "isl6367.h"

#if CONFIG(DRIVERS_I2C_ISL6367_VCORE)
#include <static.h>
#endif

/* The vendor firmware's init sequence precedes any setting write; the
   register roles and values come from the recovered map. */
static enum cb_err isl6367_init_writes(const int addr)
{
	enum cb_err err;

	err = do_smbus_write_byte(smbus_base(), addr, ISL6367_REG_INIT_D1, 0x00);
	if (err != CB_SUCCESS)
		return err;
	err = do_smbus_write_byte(smbus_base(), addr, ISL6367_REG_ARM, 0x02);
	if (err != CB_SUCCESS)
		return err;
	err = do_smbus_write_byte(smbus_base(), addr, ISL6367_REG_INIT_D8, 0x01);
	if (err != CB_SUCCESS)
		return err;
	err = do_smbus_write_byte(smbus_base(), addr, ISL6367_REG_INIT_D9, 0x01);
	return err;
}

static enum cb_err isl6367_write_llc(const int addr, const int stored_value)
{
	uint8_t d3_bits, d4_bit, tmp;
	enum cb_err err;
	int val;

	err = isl6367_encode_llc(stored_value, &d3_bits, &d4_bit);
	if (err != CB_SUCCESS)
		return err;

	val = do_smbus_read_byte(smbus_base(), addr, ISL6367_REG_LLC);
	if (val < 0)
		return val;
	err = do_smbus_write_byte(smbus_base(), addr, ISL6367_REG_LLC,
				  (val & ~0x03) | d3_bits);
	if (err != CB_SUCCESS)
		return err;

	val = do_smbus_read_byte(smbus_base(), addr, ISL6367_REG_LLC_EN);
	if (val < 0)
		return val;
	tmp = (val & ~0x01) | d4_bit;
	return do_smbus_write_byte(smbus_base(), addr, ISL6367_REG_LLC_EN, tmp);
}

static enum cb_err isl6367_write_vcore(const int addr,
	const struct drivers_i2c_isl6367_config *cfg)
{
	uint8_t reg8;
	enum cb_err err;

	/* Both setpoints ride one register; the fixed target wins when the
	   board declares both. */
	if (cfg->vcore_fix_mv) {
		err = isl6367_encode_fixed(cfg->vcore_fix_mv, &reg8);
		if (err != CB_SUCCESS)
			return err;
	} else if (cfg->vcore_offset_mv) {
		err = isl6367_encode_offset(cfg->vcore_offset_mv, &reg8);
		if (err != CB_SUCCESS)
			return err;
	} else {
		return CB_SUCCESS;
	}

	return do_smbus_write_byte(smbus_base(), addr, ISL6367_REG_OFFSET_FIX,
				   reg8);
}

enum cb_err isl6367_apply(const struct device *const dev)
{
	const struct drivers_i2c_isl6367_config *cfg = dev->chip_info;
	const int addr = dev->upstream->dev->path.i2c.device;
	enum cb_err err;

	/* Stored value zero everywhere means leave the silicon untouched. */
	if (!cfg->loadline_level && !cfg->vcore_offset_mv && !cfg->vcore_fix_mv)
		return CB_SUCCESS;

	err = isl6367_init_writes(addr);
	if (err != CB_SUCCESS)
		return err;

	if (cfg->loadline_level) {
		err = isl6367_write_llc(addr, cfg->loadline_level);
		if (err != CB_SUCCESS)
			return err;
	}

	return isl6367_write_vcore(addr, cfg);
}

#if CONFIG(DRIVERS_I2C_ISL6367_VCORE)
enum cb_err isl6367_vcore_apply(void)
{
	return isl6367_apply(_dev_v_core_ptr);
}
#endif

#if !DEVTREE_EARLY
static void isl6367_init(struct device *const dev)
{
	printk(BIOS_DEBUG, "isl6367 init\n");
}

static struct device_operations isl6367_ops = {
	.read_resources = noop_read_resources,
	.set_resources  = noop_set_resources,
	.init           = isl6367_init,
};

static void isl6367_enable(struct device *const dev)
{
	dev->ops = &isl6367_ops;
}

struct chip_operations drivers_i2c_isl6367_ops = {
	.name = "ISL6367",
	.enable_dev = isl6367_enable
};
#endif /* !DEVTREE_EARLY */

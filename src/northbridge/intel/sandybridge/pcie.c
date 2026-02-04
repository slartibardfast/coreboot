/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <device/pci.h>
#include <device/pciexp.h>
#include <device/pci_ids.h>
#include <console/console.h>
#include <assert.h>

static const char *pcie_acpi_name(const struct device *dev)
{
	assert(dev);

	if (dev->path.type != DEVICE_PATH_PCI)
		return NULL;

	assert(dev->upstream);
	if (dev->upstream->secondary == 0)
		switch (dev->path.pci.devfn) {
		case PCI_DEVFN(1, 0):
			return "PEGP";
		case PCI_DEVFN(1, 1):
			return "PEG1";
		case PCI_DEVFN(1, 2):
			return "PEG2";
		case PCI_DEVFN(6, 0):
			return "PEG6";
		};

	struct device *const port = dev->upstream->dev;
	assert(port);
	assert(port->upstream);

	if (dev->path.pci.devfn == PCI_DEVFN(0, 0) &&
	    port->upstream->secondary == 0 &&
	    (port->path.pci.devfn == PCI_DEVFN(1, 0) ||
	     port->path.pci.devfn == PCI_DEVFN(1, 1) ||
	     port->path.pci.devfn == PCI_DEVFN(1, 2) ||
	     port->path.pci.devfn == PCI_DEVFN(6, 0)))
		return "DEV0";

	return NULL;
}

/*
 * Mark large prefetchable BARs on PEG children for above-4G allocation.
 * This enables ReBAR support on Sandy/Ivy Bridge by allowing resized BARs
 * that exceed available 32-bit MMIO space to be placed above 4GB.
 */
static void peg_read_resources(struct device *dev)
{
	/* First do standard PCI bridge resource reading */
	pci_bus_read_resources(dev);

	/* Then mark large prefetch BARs for above-4G allocation */
	struct bus *bus = dev->downstream;
	if (!bus)
		return;

	for (struct device *child = bus->children; child; child = child->sibling) {
		for (struct resource *res = child->resource_list; res; res = res->next) {
			/* Only memory BARs */
			if (!(res->flags & IORESOURCE_MEM))
				continue;
			/* Only prefetchable (VRAM, not MMIO registers) */
			if (!(res->flags & IORESOURCE_PREFETCH))
				continue;
			/* Only 64-bit capable BARs */
			if (res->limit <= 0xffffffffULL)
				continue;
			/* Mark BARs > 256MB for above-4G - won't fit reliably below */
			if (res->size > 256 * MiB) {
				res->flags |= IORESOURCE_ABOVE_4G;
				printk(BIOS_DEBUG, "PEG: %s %02lx size %llx marked above 4G\n",
				       dev_path(child), res->index, res->size);
			}
		}
	}
}

static struct device_operations device_ops = {
	.read_resources		= peg_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_bus_enable_resources,
	.scan_bus		= pciexp_scan_bridge,
	.reset_bus		= pci_bus_reset,
	.init			= pci_dev_init,
	.ops_pci		= &pci_dev_ops_pci,
	.acpi_name		= pcie_acpi_name,
};

static const unsigned short pci_device_ids[] = {
	0x0101, 0x0105, 0x0109, 0x010d,
	0x0151, 0x0155, 0x0159, 0x015d,
	0,
};

static const struct pci_driver pch_pcie __pci_driver = {
	.ops		= &device_ops,
	.vendor		= PCI_VID_INTEL,
	.devices	= pci_device_ids,
};

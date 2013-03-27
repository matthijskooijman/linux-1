/*
 * platform.c - DesignWare HS OTG Controller platform driver
 *
 * Copyright (C) 2004-2013 Synopsys, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include "core.h"
#include "hcd.h"

static const char dwc2_driver_name[] = "dwc2";

static struct dwc2_core_params dwc2_module_params = {
	.otg_cap			= -1,
	.otg_ver			= -1,
	.dma_enable			= -1,
	.dma_desc_enable		= -1,
	.speed				= -1,
	.enable_dynamic_fifo		= -1,
	.en_multiple_tx_fifo		= -1,
	.host_rx_fifo_size		= -1,
	.host_nperio_tx_fifo_size	= -1,
	.host_perio_tx_fifo_size	= -1,
	.max_transfer_size		= -1,
	.max_packet_count		= -1,
	.host_channels			= -1,
	.phy_type			= -1,
	.phy_utmi_width			= -1,
	.phy_ulpi_ddr			= -1,
	.phy_ulpi_ext_vbus		= -1,
	.i2c_enable			= -1,
	.ulpi_fs_ls			= -1,
	.host_support_fs_ls_low_power	= -1,
	.host_ls_low_power_phy_clk	= -1,
	.ts_dline			= -1,
	.reload_ctl			= -1,
	.ahb_single			= -1,
};

/**
 * dwc2_driver_remove() - Called when the DWC_otg core is unregistered with the
 * DWC_otg driver
 *
 * @dev: Platform device
 *
 * This routine is called, for example, when the rmmod command is executed. The
 * device may or may not be electrically present. If it is present, the driver
 * stops device processing. Any resources used on behalf of this device are
 * freed.
 */
static int dwc2_driver_remove(struct platform_device *dev)
{
	struct dwc2_hsotg *hsotg = platform_get_drvdata(dev);

	dev_dbg(&dev->dev, "%s(%p)\n", __func__, dev);

	dwc2_hcd_remove(hsotg);

	return 0;
}

/**
 * dwc2_load_property() - Load a single property from the devicetree
 * node into the given variable.
 *
 * @dev: Platform device
 * @res: The variable to put the loaded value into
 * @name: The name of the devicetree property to load
 */
static void dwc2_load_property(struct platform_device *dev, int *res, const char *name)
{
	int len;
	const u32 *val;

	val = of_get_property(dev->dev.of_node, name, &len);
	if (!val)
		return;

	if (len != sizeof(*val)) {
		dev_warn(&dev->dev, "Invalid value in devicetree for %s property, should be a single integer\n", name);
		return;
	}

	*res = be32_to_cpu(*val);

	dev_dbg(&dev->dev, "Loaded %s parameter from devicetree: %d\n", name, *res);
}

/**
 * dwc2_load_properties() - Load all devicetree properties into the core
 * params.
 *
 * @dev: Platform device
 * @params: The core parameters to load the values into
 */
static void dwc2_load_properties(struct platform_device *dev, struct dwc2_core_params *params)
{
	dev_dbg(&dev->dev, "Loading parameters from devicetree node %s\n", dev->dev.of_node->name);
	dwc2_load_property(dev, &params->dma_enable, "dma-enable");
	dwc2_load_property(dev, &params->otg_cap, "otg-cap");
	dwc2_load_property(dev, &params->otg_ver, "otg-ver");
	dwc2_load_property(dev, &params->dma_enable, "dma-enable");
	dwc2_load_property(dev, &params->dma_desc_enable, "dma-desc-enable");
	dwc2_load_property(dev, &params->speed, "speed");
	dwc2_load_property(dev, &params->enable_dynamic_fifo, "enable-dynamic-fifo");
	dwc2_load_property(dev, &params->en_multiple_tx_fifo, "en-multiple-tx-fifo");
	dwc2_load_property(dev, &params->host_rx_fifo_size, "host-rx-fifo-size");
	dwc2_load_property(dev, &params->host_nperio_tx_fifo_size, "host-nperio-tx-fifo-size");
	dwc2_load_property(dev, &params->host_perio_tx_fifo_size, "host-perio-tx-fifo-size");
	dwc2_load_property(dev, &params->max_transfer_size, "max-transfer-size");
	dwc2_load_property(dev, &params->max_packet_count, "max-packet-count");
	dwc2_load_property(dev, &params->host_channels, "host-channels");
	dwc2_load_property(dev, &params->phy_type, "phy-type");
	dwc2_load_property(dev, &params->phy_utmi_width, "phy-utmi-width");
	dwc2_load_property(dev, &params->phy_ulpi_ddr, "phy-ulpi-ddr");
	dwc2_load_property(dev, &params->phy_ulpi_ext_vbus, "phy-ulpi-ext-vbus");
	dwc2_load_property(dev, &params->i2c_enable, "i2c-enable");
	dwc2_load_property(dev, &params->ulpi_fs_ls, "ulpi-fs-ls");
	dwc2_load_property(dev, &params->host_support_fs_ls_low_power, "host-support-fs-ls-low-power");
	dwc2_load_property(dev, &params->host_ls_low_power_phy_clk, "host-ls-low-power-phy-clk");
	dwc2_load_property(dev, &params->ts_dline, "ts-dline");
	dwc2_load_property(dev, &params->reload_ctl, "reload-ctl");
	dwc2_load_property(dev, &params->ahb_single, "ahb-single");
}

/**
 * dwc2_driver_probe() - Called when the DWC_otg core is bound to the DWC_otg
 * driver
 *
 * @dev: Platform device
 *
 * This routine creates the driver components required to control the device
 * (core, HCD, and PCD) and initializes the device. The driver components are
 * stored in a dwc2_hsotg structure. A reference to the dwc2_hsotg is saved
 * in the device private data. This allows the driver to access the dwc2_hsotg
 * structure on subsequent calls to driver methods for this device.
 */
static int dwc2_driver_probe(struct platform_device *dev)
{
	struct dwc2_hsotg *hsotg;
	struct resource *res;
	int retval;
	int irq;

	dev_dbg(&dev->dev, "%s(%p)\n", __func__, dev);

	hsotg = devm_kzalloc(&dev->dev, sizeof(*hsotg), GFP_KERNEL);
	if (!hsotg)
		return -ENOMEM;

	hsotg->dev = &dev->dev;

	irq = platform_get_irq(dev, 0);
	if (irq < 0) {
		dev_err(&dev->dev, "missing IRQ resource\n");
		return -EINVAL;
	}

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&dev->dev, "missing memory base resource\n");
		return -EINVAL;
	}

	if (dev->dev.of_node)
		dwc2_load_properties(dev, &dwc2_module_params);

	/* TODO: Does this need a request, or just a ioremap? */
	hsotg->regs = devm_ioremap_resource(&dev->dev, res);
	if (IS_ERR(hsotg->regs))
		return PTR_ERR(hsotg->regs);

	dev_dbg(&dev->dev, "mapped PA %08lx to VA %p\n",
		(unsigned long)res->start, hsotg->regs);

	retval = dwc2_hcd_init(hsotg, irq, &dwc2_module_params);
	if (retval)
		return retval;

	platform_set_drvdata(dev, hsotg);
	dev_dbg(&dev->dev, "hsotg=%p\n", hsotg);

	dev_dbg(&dev->dev, "registering common handler for irq%d\n", irq);
	retval = devm_request_irq(&dev->dev, irq, dwc2_handle_common_intr,
				  IRQF_SHARED, dev_name(&dev->dev),
				  hsotg);
	if (retval)
		dwc2_hcd_remove(hsotg);

	return retval;
}

static const struct of_device_id dwc2_of_match_table[] = {
	{ .compatible = "snps,dwc2" },
	{},
};
MODULE_DEVICE_TABLE(of, dwc2_otg_match_table);

static struct platform_driver dwc2_platform_driver = {
	.driver = {
		.name = (char *)dwc2_driver_name,
		.of_match_table = dwc2_of_match_table,
	},
	.probe = dwc2_driver_probe,
	.remove = dwc2_driver_remove,
};

module_platform_driver(dwc2_platform_driver);

MODULE_DESCRIPTION("DESIGNWARE HS OTG Platform Glue");
MODULE_AUTHOR("Synopsys, Inc.");
MODULE_LICENSE("Dual BSD/GPL");

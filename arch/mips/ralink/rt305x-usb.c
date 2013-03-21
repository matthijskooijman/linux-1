/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Copyright (C) 2008-2011 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/delay.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <linux/usb/ehci_pdriver.h>
#include <linux/usb/ohci_pdriver.h>

#include <asm/mach-ralink/ralink_regs.h>
#include <asm/mach-ralink/rt305x.h>

static atomic_t rt3352_usb_pwr_ref = ATOMIC_INIT(0);

static int rt3352_usb_power_on(struct platform_device *pdev)
{

	if (atomic_inc_return(&rt3352_usb_pwr_ref) == 1) {
		u32 t;

		t = rt_sysc_r32(RT3352_SYSC_REG_USB_PS);

		/* enable clock for port0's and port1's phys */
		t = rt_sysc_r32(RT3352_SYSC_REG_CLKCFG1);
		t |= RT3352_CLKCFG1_UPHY0_CLK_EN | RT3352_CLKCFG1_UPHY1_CLK_EN;
		rt_sysc_w32(t, RT3352_SYSC_REG_CLKCFG1);
		mdelay(500);

		/* pull USBHOST and USBDEV out from reset */
		t = rt_sysc_r32(RT3352_SYSC_REG_RSTCTRL);
		t &= ~(RT3352_RSTCTRL_UHST | RT3352_RSTCTRL_UDEV);
		rt_sysc_w32(t, RT3352_SYSC_REG_RSTCTRL);
		mdelay(500);

		/* enable host mode */
		t = rt_sysc_r32(RT3352_SYSC_REG_SYSCFG1);
		t |= RT3352_SYSCFG1_USB0_HOST_MODE;
		rt_sysc_w32(t, RT3352_SYSC_REG_SYSCFG1);

		t = rt_sysc_r32(RT3352_SYSC_REG_USB_PS);
	}

	return 0;
}

static void rt3352_usb_power_off(struct platform_device *pdev)
{
	if (atomic_dec_return(&rt3352_usb_pwr_ref) == 0) {
		u32 t;

		/* put USBHOST and USBDEV into reset */
		t = rt_sysc_r32(RT3352_SYSC_REG_RSTCTRL);
		t |= RT3352_RSTCTRL_UHST | RT3352_RSTCTRL_UDEV;
		rt_sysc_w32(t, RT3352_SYSC_REG_RSTCTRL);
		udelay(10000);

		/* disable clock for port0's and port1's phys*/
		t = rt_sysc_r32(RT3352_SYSC_REG_CLKCFG1);
		t &= ~(RT3352_CLKCFG1_UPHY0_CLK_EN | RT3352_CLKCFG1_UPHY1_CLK_EN);
		rt_sysc_w32(t, RT3352_SYSC_REG_CLKCFG1);
		udelay(10000);
	}
}

static struct usb_ehci_pdata rt3352_ehci_data = {
	.power_on	= rt3352_usb_power_on,
	.power_off	= rt3352_usb_power_off,
};

static struct usb_ohci_pdata rt3352_ohci_data = {
	.power_on	= rt3352_usb_power_on,
	.power_off	= rt3352_usb_power_off,
};

static void ralink_add_usb(char *name, void *pdata, u64 *mask)
{
	struct device_node *node;
	struct platform_device *pdev;

	node = of_find_compatible_node(NULL, NULL, name);
	if (!node)
		return;

	pdev = of_find_device_by_node(node);
	if (!pdev)
		goto error_out;

	if (pdata)
		pdev->dev.platform_data = pdata;
	if (mask) {
		pdev->dev.dma_mask = mask;
		pdev->dev.coherent_dma_mask = *mask;
	}

error_out:
	of_node_put(node);
}

static u64 rt3352_ohci_dmamask = DMA_BIT_MASK(32);
static u64 rt3352_ehci_dmamask = DMA_BIT_MASK(32);
static u64 rt3050_dwc2_dmamask = DMA_BIT_MASK(32);

void ralink_usb_platform(void)
{
	if (soc_is_rt3352() || soc_is_rt5350()) {
		ralink_add_usb("ohci-platform",
				&rt3352_ohci_data, &rt3352_ohci_dmamask);
		ralink_add_usb("ehci-platform",
				&rt3352_ehci_data, &rt3352_ehci_dmamask);
	}
	if (soc_is_rt305x() || soc_is_rt3350()) {
		ralink_add_usb("ralink,rt3050-otg",
				NULL, &rt3050_dwc2_dmamask);
	}
}

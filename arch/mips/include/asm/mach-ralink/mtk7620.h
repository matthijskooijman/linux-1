/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Parts of this file are based on Ralink's 2.6.21 BSP
 *
 * Copyright (C) 2008-2011 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 */

#ifndef _RT305X_REGS_H_
#define _RT305X_REGS_H_

#define MTK7620_SYSC_BASE		0x10000000

#define SYSC_REG_CHIP_NAME0		0x00
#define SYSC_REG_CHIP_NAME1		0x04
#define SYSC_REG_CHIP_REV		0x0c
#define SYSC_REG_SYSTEM_CONFIG0		0x10
#define SYSC_REG_SYSTEM_CONFIG1		0x14
#define SYSC_REG_CPLL_CONFIG0		0x54
#define SYSC_REG_CPLL_CONFIG1		0x58

#define MTK7620N_CHIP_NAME0		0x33365452
#define MTK7620N_CHIP_NAME1		0x20203235

#define MTK7620A_CHIP_NAME0		0x3637544d
#define MTK7620A_CHIP_NAME1		0x20203032

#define CHIP_REV_PKG_MASK		0x1
#define CHIP_REV_PKG_SHIFT		16
#define CHIP_REV_VER_MASK		0xf
#define CHIP_REV_VER_SHIFT		8
#define CHIP_REV_ECO_MASK		0xf

#define MTK7620_CPLL_SW_CONFIG_SHIFT	31
#define MTK7620_CPLL_SW_CONFIG_MASK	0x1
#define MTK7620_CPLL_CPU_CLK_SHIFT	24
#define MTK7620_CPLL_CPU_CLK_MASK	0x1

#define MTK7620_GPIO_MODE_I2C		BIT(0)
#define MTK7620_GPIO_MODE_UART0_SHIFT	2
#define MTK7620_GPIO_MODE_UART0_MASK	0x7
#define MTK7620_GPIO_MODE_UART0(x)	((x) << MTK7620_GPIO_MODE_UART0_SHIFT)
#define MTK7620_GPIO_MODE_UARTF		0x0
#define MTK7620_GPIO_MODE_PCM_UARTF	0x1
#define MTK7620_GPIO_MODE_PCM_I2S	0x2
#define MTK7620_GPIO_MODE_I2S_UARTF	0x3
#define MTK7620_GPIO_MODE_PCM_GPIO	0x4
#define MTK7620_GPIO_MODE_GPIO_UARTF	0x5
#define MTK7620_GPIO_MODE_GPIO_I2S	0x6
#define MTK7620_GPIO_MODE_GPIO		0x7
#define MTK7620_GPIO_MODE_UART1		BIT(5)
#define MTK7620_GPIO_MODE_MDIO		BIT(8)
#define MTK7620_GPIO_MODE_RGMII1	BIT(9)
#define MTK7620_GPIO_MODE_RGMII2	BIT(10)
#define MTK7620_GPIO_MODE_SPI		BIT(11)
#define MTK7620_GPIO_MODE_SPI_REF_CLK	BIT(12)
#define MTK7620_GPIO_MODE_WLED		BIT(13)
#define MTK7620_GPIO_MODE_JTAG		BIT(15)
#define MTK7620_GPIO_MODE_EPHY		BIT(15)
#define MTK7620_GPIO_MODE_WDT		BIT(22)

#endif

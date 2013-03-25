/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2009 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 */

#include <linux/string.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>

#include <asm/bootinfo.h>
#include <asm/addrspace.h>

#include "common.h"

#define MB	(1024 * 1024)

unsigned long ramips_mem_base;
unsigned long ramips_mem_size_min;
unsigned long ramips_mem_size_max;

#ifdef CONFIG_SOC_RT305X

#include <asm/mach-ralink/rt305x.h>

static unsigned long rt5350_get_mem_size(void)
{
	void __iomem *sysc = (void __iomem *) KSEG1ADDR(RT305X_SYSC_BASE);
	unsigned long ret;
	u32 t;

	t = __raw_readl(sysc + SYSC_REG_SYSTEM_CONFIG);
	t = (t >> RT5350_SYSCFG0_DRAM_SIZE_SHIFT) &
	RT5350_SYSCFG0_DRAM_SIZE_MASK;

	switch (t) {
	case RT5350_SYSCFG0_DRAM_SIZE_2M:
		ret = 2 * 1024 * 1024;
		break;
	case RT5350_SYSCFG0_DRAM_SIZE_8M:
		ret = 8 * 1024 * 1024;
		break;
	case RT5350_SYSCFG0_DRAM_SIZE_16M:
		ret = 16 * 1024 * 1024;
		break;
	case RT5350_SYSCFG0_DRAM_SIZE_32M:
		ret = 32 * 1024 * 1024;
		break;
	case RT5350_SYSCFG0_DRAM_SIZE_64M:
		ret = 64 * 1024 * 1024;
		break;
	default:
		panic("rt5350: invalid DRAM size: %u", t);
		break;
	}

	return ret;
}

#endif

static void __init detect_mem_size(void)
{
	unsigned long size;

#ifdef CONFIG_SOC_RT305X
	if (soc_is_rt5350()) {
		size = rt5350_get_mem_size();
	} else
#endif
	{
		void *base;

		base = (void *) KSEG1ADDR(detect_mem_size);
		for (size = ramips_mem_size_min; size < ramips_mem_size_max;
		     size <<= 1 ) {
			if (!memcmp(base, base + size, 1024))
				break;
		}
	}

	pr_info("memory detected: %uMB\n", (unsigned int) size / MB);

	add_memory_region(ramips_mem_base, size, BOOT_MEM_RAM);
}

int __init early_init_dt_detect_memory(unsigned long node, const char *uname,
				     int depth, void *data)
{
	unsigned long l;
	__be32 *mem;

	/* We are scanning "memorydetect" nodes only */
	if (depth != 1 || strcmp(uname, "memorydetect") != 0)
		return 0;

	mem = of_get_flat_dt_prop(node, "ralink,memory", &l);
	if (mem == NULL)
		return 0;

	if ((l / sizeof(__be32)) != 3)
		panic("invalid memorydetect node\n");

	ramips_mem_base = dt_mem_next_cell(dt_root_addr_cells, &mem);
	ramips_mem_size_min = dt_mem_next_cell(dt_root_size_cells, &mem);
	ramips_mem_size_max = dt_mem_next_cell(dt_root_size_cells, &mem);

	pr_info("memory window: 0x%llx, min: %uMB, max: %uMB\n",
		(unsigned long long) ramips_mem_base,
		(unsigned int) ramips_mem_size_min / MB,
		(unsigned int) ramips_mem_size_max / MB);

	detect_mem_size();

	return 0;
}

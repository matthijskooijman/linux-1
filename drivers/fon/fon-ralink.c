/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mtd/mtd.h>
#include <linux/of_platform.h>

struct boardconfig
{
	unsigned char mac[6];
	unsigned char key[32];
	unsigned char serial[11];
};

static char fon_sku[64] = "FON2303";
static struct boardconfig fon_config;

static struct proc_dir_entry *procdir, *procsku, *procmac, *procserial, *prockey;

static int procsku_read(char *buf, char **start, off_t offset,
					int len, int *eof, void *unused)
{
	return sprintf(buf, fon_sku);
}

static int prockey_read(char *buf, char **start, off_t offset,
					int len, int *eof, void *unused)
{
	int i, rlen = 0;

	for(i = 0; i < 32; i++)
		rlen += sprintf(&buf[rlen], "%02X", fon_config.key[i]);

	return rlen;
}

static int procserial_read(char *buf, char **start, off_t offset,
					int len, int *eof, void *unused)
{
	return sprintf(buf, fon_config.serial);
}

static int procmac_read(char *buf, char **start, off_t offset,
					int len, int *eof, void *unused)
{
	return sprintf(buf, "%02X-%02X-%02X-%02X-%02X-%02X",
			fon_config.mac[0], fon_config.mac[1],
			fon_config.mac[2], fon_config.mac[3],
			fon_config.mac[4], fon_config.mac[5]);
}

static int fon_proc_init(void)
{
	procdir = proc_mkdir("fon", NULL);
	if (!procdir)
		return -1;

	procsku = create_proc_entry("sku", 0, procdir);
	if (procsku)
		procsku->read_proc = procsku_read;

	procmac = create_proc_entry("mac", 0, procdir);
	if (procmac)
		procmac->read_proc = procmac_read;

	procserial = create_proc_entry("serial", 0, procdir);
	if (procserial)
		procserial->read_proc = procserial_read;

	prockey = create_proc_entry("key", 0, procdir);
	if (prockey)
		prockey->read_proc = prockey_read;

	return 0;
}

static int fon_config_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node, *mtd_np = NULL;
	size_t retlen;
	int size, offset = 0;
	struct mtd_info *mtd;
	const char *part;
	const __be32 *list;
	phandle phandle;

	list = of_get_property(np, "fon,config", &size);
	if (!list || (size < (3 * sizeof(*list)))) {
		dev_err(&pdev->dev, "failed to load eeprom property\n");
		return -ENOENT;
	}

	phandle = be32_to_cpup(list++);
	if (phandle)
		mtd_np = of_find_node_by_phandle(phandle);
	if (!mtd_np) {
		dev_err(&pdev->dev, "failed to load mtd phandle\n");
		return -ENOENT;
	}

	part = of_get_property(mtd_np, "label", NULL);
	if (!part)
		part = mtd_np->name;

	mtd = get_mtd_device_nm(part);
	if (IS_ERR(mtd)) {
		dev_err(&pdev->dev, "failed to get mtd device \"%s\"\n", part);
		return PTR_ERR(mtd);
	}

	offset = be32_to_cpup(list++);
	mtd->_read(mtd, offset, sizeof(fon_sku), &retlen, (u_char *) fon_sku);

	/* fon20n has no sku in the flash */
	if (fon_sku[0] == '\0')
		sprintf(fon_sku, "FON2303");

	offset = be32_to_cpup(list++);
	mtd->_read(mtd, offset, sizeof(struct boardconfig), &retlen, (u_char *) &fon_config);

	fon_config.serial[10] = '\0';

	return fon_proc_init();
}

static const struct of_device_id fon_config_match[] = {
	{ .compatible = "fon,config" },
	{},
};
MODULE_DEVICE_TABLE(of, fon_config_match);

static struct platform_driver fon_config_driver = {
	.probe = fon_config_probe,
	.driver = {
		.name = "fon,config",
		.owner = THIS_MODULE,
		.of_match_table = fon_config_match,
	},
};

module_platform_driver(fon_config_driver);

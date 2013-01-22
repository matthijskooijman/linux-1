/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Copyright (C) 2009-2011 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

enum ralink_gpio_reg {
	GPIO_REG_INT = 0,
	GPIO_REG_EDGE,
	GPIO_REG_RENA,
	GPIO_REG_FENA,
	GPIO_REG_DATA,
	GPIO_REG_DIR,
	GPIO_REG_POL,
	GPIO_REG_SET,
	GPIO_REG_RESET,
	GPIO_REG_TOGGLE,
	GPIO_REG_MAX
};

struct ralink_gpio_chip {
	struct gpio_chip chip;
	u8 regs[GPIO_REG_MAX];

	spinlock_t lock;
	void __iomem *membase;
};

static inline struct ralink_gpio_chip *to_ralink_gpio(struct gpio_chip *chip)
{
	struct ralink_gpio_chip *rg;

	rg = container_of(chip, struct ralink_gpio_chip, chip);
	return rg;
}

static inline void rt_gpio_w32(struct ralink_gpio_chip *rg, u8 reg, u32 val)
{
	iowrite32(val, rg->membase + rg->regs[reg]);
}

static inline u32 rt_gpio_r32(struct ralink_gpio_chip *rg, u8 reg)
{
	return ioread32(rg->membase + rg->regs[reg]);
}

static void ralink_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct ralink_gpio_chip *rg = to_ralink_gpio(chip);

	rt_gpio_w32(rg, (value) ? GPIO_REG_SET : GPIO_REG_RESET, BIT(offset));
}

static int ralink_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct ralink_gpio_chip *rg = to_ralink_gpio(chip);

	return !!(rt_gpio_r32(rg, GPIO_REG_DATA) & BIT(offset));
}

static int ralink_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct ralink_gpio_chip *rg = to_ralink_gpio(chip);
	unsigned long flags;
	u32 t;

	spin_lock_irqsave(&rg->lock, flags);
	t = rt_gpio_r32(rg, GPIO_REG_DIR);
	t &= ~BIT(offset);
	rt_gpio_w32(rg, GPIO_REG_DIR, t);
	spin_unlock_irqrestore(&rg->lock, flags);

	return 0;
}

static int ralink_gpio_direction_output(struct gpio_chip *chip,
					unsigned offset, int value)
{
	struct ralink_gpio_chip *rg = to_ralink_gpio(chip);
	unsigned long flags;
	u32 t;

	spin_lock_irqsave(&rg->lock, flags);
	ralink_gpio_set(chip, offset, value);
	t = rt_gpio_r32(rg, GPIO_REG_DIR);
	t |= BIT(offset);
	rt_gpio_w32(rg, GPIO_REG_DIR, t);
	spin_unlock_irqrestore(&rg->lock, flags);

	return 0;
}

static int ralink_gpio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	struct ralink_gpio_chip *gc;
	const __be32 *ngpio;

	if (!res) {
		dev_err(&pdev->dev, "failed to find resource\n");
		return -ENOMEM;
	}

	gc = devm_kzalloc(&pdev->dev,
				sizeof(struct ralink_gpio_chip), GFP_KERNEL);
	if (!gc)
		return -ENOMEM;

	gc->membase = devm_ioremap_resource(&pdev->dev, res);
	if (!gc->membase) {
		dev_err(&pdev->dev, "cannot remap I/O memory region\n");
		return -ENOMEM;
	}

	if (of_property_read_u8_array(np, "ralink,register-map",
			gc->regs, GPIO_REG_MAX)) {
		dev_err(&pdev->dev, "failed to read register definition\n");
		return -EINVAL;
	}

	ngpio = of_get_property(np, "ralink,num-gpios", NULL);
	if (!ngpio) {
		dev_err(&pdev->dev, "failed to read number of pins\n");
		return -EINVAL;
	}

	spin_lock_init(&gc->lock);

	gc->chip.label = dev_name(&pdev->dev);
	gc->chip.of_node = np;
	gc->chip.base = -1;
	gc->chip.ngpio = be32_to_cpu(*ngpio);
	gc->chip.direction_input = ralink_gpio_direction_input;
	gc->chip.direction_output = ralink_gpio_direction_output;
	gc->chip.get = ralink_gpio_get;
	gc->chip.set = ralink_gpio_set;

	/* set polarity to low for all lines */
	rt_gpio_w32(gc, GPIO_REG_POL, 0);

	dev_info(&pdev->dev, "registering %d gpios\n", gc->chip.ngpio);

	return gpiochip_add(&gc->chip);
}

static const struct of_device_id ralink_gpio_match[] = {
	{ .compatible = "ralink,rt2880-gpio" },
	{},
};
MODULE_DEVICE_TABLE(of, ralink_gpio_match);

static struct platform_driver ralink_gpio_driver = {
	.probe = ralink_gpio_probe,
	.driver = {
		.name = "rt2880_gpio",
		.owner = THIS_MODULE,
		.of_match_table = ralink_gpio_match,
	},
};

static int __init ralink_gpio_init(void)
{
	return platform_driver_register(&ralink_gpio_driver);
}

subsys_initcall(ralink_gpio_init);

// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for Firefly GPIO on Firefly / Forlinx boards.
 *
 * Copyright (C) 2016, Zhongshan T-chip Intelligent Technology Co.,ltd.
 * Copyright (C) 2024, Ported to mainline gpiod API.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/err.h>

struct firefly_gpio_info {
	struct gpio_desc *gpio;
	struct gpio_desc *irq_gpio;
	int irq;
};

static irqreturn_t firefly_gpio_irq_handler(int irq, void *dev_id)
{
	struct device *dev = dev_id;

	dev_info(dev, "Firefly GPIO IRQ triggered!\n");
	return IRQ_HANDLED;
}

static int firefly_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct firefly_gpio_info *gpio_info;
	int ret;

	dev_info(dev, "Firefly GPIO Test Program Probe\n");

	gpio_info = devm_kzalloc(dev, sizeof(*gpio_info), GFP_KERNEL);
	if (!gpio_info)
		return -ENOMEM;

	/* Get the output GPIO descriptor */
	gpio_info->gpio = devm_gpiod_get(dev, "firefly", GPIOD_OUT_LOW);
	if (IS_ERR(gpio_info->gpio)) {
		ret = PTR_ERR(gpio_info->gpio);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get firefly-gpio: %d\n", ret);
		return ret;
	}

	dev_info(dev, "Firefly GPIO output configured\n");

	/* Get the interrupt GPIO descriptor */
	gpio_info->irq_gpio = devm_gpiod_get(dev, "firefly-irq", GPIOD_IN);
	if (IS_ERR(gpio_info->irq_gpio)) {
		ret = PTR_ERR(gpio_info->irq_gpio);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get firefly-irq-gpio: %d\n", ret);
		return ret;
	}

	gpio_info->irq = gpiod_to_irq(gpio_info->irq_gpio);
	if (gpio_info->irq < 0) {
		dev_err(dev, "Failed to get IRQ from firefly-irq-gpio: %d\n",
			gpio_info->irq);
		return gpio_info->irq;
	}

	ret = devm_request_irq(dev, gpio_info->irq, firefly_gpio_irq_handler,
			       IRQF_TRIGGER_LOW, "firefly-gpio", dev);
	if (ret) {
		dev_err(dev, "Failed to request IRQ %d: %d\n",
			gpio_info->irq, ret);
		return ret;
	}

	dev_info(dev, "Firefly GPIO IRQ configured (irq=%d)\n", gpio_info->irq);

	return 0;
}

static const struct of_device_id firefly_match_table[] = {
	{ .compatible = "firefly,rk3588-gpio", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, firefly_match_table);

static struct platform_driver firefly_gpio_driver = {
	.driver = {
		.name = "firefly-gpio",
		.of_match_table = firefly_match_table,
	},
	.probe = firefly_gpio_probe,
};

module_platform_driver(firefly_gpio_driver);

MODULE_AUTHOR("maocl <service@t-firefly.com>");
MODULE_DESCRIPTION("Firefly GPIO driver (gpiod API)");
MODULE_ALIAS("platform:firefly-gpio");
MODULE_LICENSE("GPL");

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(CONFIG_THINGY53_LOGGING)
#include <zephyr/init.h>

#include <zephyr/usb/usb_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(thingy53_setup);

static int usb_cdc_init(const struct device *dev)
{
	int err = usb_enable(NULL);

	if (err) {
		LOG_ERR("Failed to enable USB");
	}

	return err;
}

SYS_INIT(usb_cdc_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
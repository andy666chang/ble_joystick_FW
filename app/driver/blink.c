/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#if CONFIG_LOG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(blink, LOG_LEVEL_INF);
#endif

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static void blink(void)
{
	int ret;
	bool led_state = true;

	if (!gpio_is_ready_dt(&led)) {
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return;
		}

		led_state = !led_state;

		#if CONFIG_LOG
		LOG_DBG("LED state: %s", led_state ? "ON" : "OFF");
		#else
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		#endif

		k_msleep(SLEEP_TIME_MS);
	}
}

#define STACKSIZE 1024
#define PRIORITY 7

K_THREAD_DEFINE(blink_id, STACKSIZE, blink, NULL, NULL, NULL,
    PRIORITY, 0, 0);

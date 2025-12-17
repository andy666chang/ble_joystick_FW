/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include "driver/hog.h"

#if CONFIG_LOG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
#endif

int main(void)
{
    #if CONFIG_LOG
    LOG_INF("Board: %s", CONFIG_BOARD);
    LOG_INF("Target: %s", CONFIG_BOARD_TARGET);
    #else
    printf("Board: %s\n", CONFIG_BOARD);
    printf("Target: %s\n", CONFIG_BOARD_TARGET);
    #endif

    // 初始化藍牙並啟動 HID 服務
    ble_init();

	return 0;
}

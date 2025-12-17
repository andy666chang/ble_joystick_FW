/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化藍牙並啟動 HID 服務
 * 
 * @return int 0 成功, 負值為錯誤碼
 */
int ble_init(void);

/**
 * @brief 初始化 HID 滑鼠服務
 * 
 */
void hid_mouse_init(void);

/**
 * @brief 發送 HID 滑鼠報告
 * 
 * @param buttons 按鍵狀態 (使用 BIT(0), BIT(1) 等)
 * @param x X 軸移動距離
 * @param y Y 軸移動距離
 * @return int 0 成功, 負值為錯誤碼
 */
int hids_send_mouse_report(uint8_t buttons, int8_t x, int8_t y);

#ifdef __cplusplus
}
#endif

/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hid_mouse, LOG_LEVEL_INF);

enum {
	HIDS_REMOTE_WAKE = BIT(0),
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info {
	uint16_t version; /* version number of base USB HID Specification */
	uint8_t code; /* country HID Device hardware is localized for. */
	uint8_t flags;
} __packed;

struct hids_report {
	uint8_t id; /* report id */
	uint8_t type; /* report type */
} __packed;

static struct hids_info info = {
	.version = 0x0000,
	.code = 0x00,
	.flags = HIDS_NORMALLY_CONNECTABLE,
};

enum {
	HIDS_INPUT = 0x01,
	HIDS_OUTPUT = 0x02,
	HIDS_FEATURE = 0x03,
};

static struct hids_report input = {
	.id = 0x01,
	.type = HIDS_INPUT,
};

static uint8_t notify;
static uint8_t ctrl_point;
static uint8_t report_map[] = {
	0x05, 0x01, /* Usage Page (Generic Desktop Ctrls) */
	0x09, 0x02, /* Usage (Mouse) */
	0xA1, 0x01, /* Collection (Application) */
	0x85, 0x01, /*	 Report Id (1) */
	0x09, 0x01, /*   Usage (Pointer) */
	0xA1, 0x00, /*   Collection (Physical) */
	0x05, 0x09, /*     Usage Page (Button) */
	0x19, 0x01, /*     Usage Minimum (0x01) */
	0x29, 0x03, /*     Usage Maximum (0x03) */
	0x15, 0x00, /*     Logical Minimum (0) */
	0x25, 0x01, /*     Logical Maximum (1) */
	0x95, 0x03, /*     Report Count (3) */
	0x75, 0x01, /*     Report Size (1) */
	0x81, 0x02, /*     Input (Data,Var,Abs,No Wrap,Linear,...) */
	0x95, 0x01, /*     Report Count (1) */
	0x75, 0x05, /*     Report Size (5) */
	0x81, 0x03, /*     Input (Const,Var,Abs,No Wrap,Linear,...) */
	0x05, 0x01, /*     Usage Page (Generic Desktop Ctrls) */
	0x09, 0x30, /*     Usage (X) */
	0x09, 0x31, /*     Usage (Y) */
	0x15, 0x81, /*     Logical Minimum (129) */
	0x25, 0x7F, /*     Logical Maximum (127) */
	0x75, 0x08, /*     Report Size (8) */
	0x95, 0x02, /*     Report Count (2) */
	0x81, 0x06, /*     Input (Data,Var,Rel,No Wrap,Linear,...) */
	0xC0,       /*   End Collection */
	0xC0,       /* End Collection */
};


static ssize_t read_info(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_map,
				 sizeof(report_map));
}

static ssize_t read_report(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_report));
}

static void input_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_input_report(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > sizeof(ctrl_point)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

#if CONFIG_SAMPLE_BT_USE_AUTHENTICATION
/* Require encryption using authenticated link-key. */
#define SAMPLE_BT_PERM_READ BT_GATT_PERM_READ_AUTHEN
#define SAMPLE_BT_PERM_WRITE BT_GATT_PERM_WRITE_AUTHEN
#else
/* Require encryption. */
#define SAMPLE_BT_PERM_READ BT_GATT_PERM_READ_ENCRYPT
#define SAMPLE_BT_PERM_WRITE BT_GATT_PERM_WRITE_ENCRYPT
#endif

/* HID Service Declaration */
BT_GATT_SERVICE_DEFINE(hog_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_info, NULL, &info),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_report_map, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       SAMPLE_BT_PERM_READ,
			       read_input_report, NULL, NULL),
	BT_GATT_CCC(input_ccc_changed,
		    SAMPLE_BT_PERM_READ | SAMPLE_BT_PERM_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
			   read_report, NULL, &input),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE,
			       NULL, write_ctrl_point, &ctrl_point),
);

struct mouse_report_t {
	uint8_t buttons; // Bit 0:左鍵, Bit 1:右鍵, Bit 2:中鍵
	int8_t x;       // X 軸位移 (-127 ~ 127)
	int8_t y;       // Y 軸位移 (-127 ~ 127)
} __packed;

/**
 * @brief 發送 HID 滑鼠報告
 * 
 * @param buttons 按鍵狀態 (使用 BIT(0), BIT(1) 等)
 * @param x X 軸移動距離
 * @param y Y 軸移動距離
 * @return int 0 成功, 負值為錯誤碼
 */
int hids_send_mouse_report(uint8_t buttons, int8_t x, int8_t y) {
	if (!notify) {
		return -EACCES; // 主機尚未開啟通知
	}

	struct mouse_report_t report = {
		.buttons = buttons,
		.x = x,
		.y = y,
	};

	LOG_DBG("Sending mouse report: buttons=0x%02x, x=%d, y=%d",
		report.buttons, report.x, report.y);

	/* &hog_svc.attrs[5] 是 HIDS Report 的特徵值聲明
	 * Zephyr 會自動尋找其後的 Value 屬性進行 Notify
	 */
	return bt_gatt_notify(NULL, &hog_svc.attrs[5], &report, sizeof(report));
}

#include <math.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static void hid_mouse_service(void) {
	int err = 0, ret = 0;

	// Button Init
	const struct gpio_dt_spec btns[] = {
		GPIO_DT_SPEC_GET(DT_ALIAS(left), gpios),
		// GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios),
		GPIO_DT_SPEC_GET(DT_ALIAS(right), gpios),
		GPIO_DT_SPEC_GET(DT_ALIAS(middle), gpios),
		// GPIO_DT_SPEC_GET(DT_ALIAS(rst), gpios),
	};

	for (size_t i = 0; i < ARRAY_SIZE(btns); i++) {
		gpio_pin_configure_dt(&btns[i], GPIO_INPUT);
	}

	// ADC Init
	// Data of ADC io-channels specified in devicetree.
	static const struct adc_dt_spec adc_channels[] = {
		DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
					DT_SPEC_AND_COMMA)
	};

	// Configure channels individually prior to sampling.
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			LOG_ERR("ADC controller device %s not ready", adc_channels[i].dev->name);
			return;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			LOG_ERR("Could not setup channel #%d (%d)", i, err);
			return;
		}
	}
	


	while (1) {
		if (notify) {
			uint8_t btn_state = 0;
			int8_t axis[2] = {0};

			// Read btn to btn_state
			for (size_t i = 0; i < ARRAY_SIZE(btns); i++) {
				if (gpio_pin_get_dt(&btns[i])) {
					btn_state |= BIT(i);
				}
			}

			// Read ADC values for X and Y
			uint16_t buf;
			struct adc_sequence sequence = {
				.buffer = &buf,
				/* buffer size in bytes, not number of samples */
				.buffer_size = sizeof(buf),
			};
			for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
				int32_t val_mv;

				(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

				err = adc_read_dt(&adc_channels[i], &sequence);
				if (err < 0) {
					LOG_ERR("Could not read (%d)", err);
					continue;
				}

				val_mv = (int32_t)buf;

				err = adc_raw_to_millivolts_dt(&adc_channels[i],
						       &val_mv);
				LOG_DBG("[%d] = %"PRId32" mV", i, val_mv);

				axis[i] = (int8_t)((val_mv - 1600) / 20);

				if ( fabs(axis[i]) < 5 ) {
					axis[i] = 0;
				}
			}

			// 呼叫 API 發送數據 (例如：按鍵狀態, X=0, Y=0)
			int err = hids_send_mouse_report(btn_state, axis[0], axis[1]);
			if (err) {
				LOG_ERR("Failed to send mouse report (err %d)", err);
			}
		}
		k_sleep(K_MSEC(100));
	}
}

#define STACKSIZE 1024
#define PRIORITY 7

K_THREAD_DEFINE(hid_mouse_id, STACKSIZE, hid_mouse_service, NULL, NULL, NULL,
    PRIORITY, 0, K_TICKS_FOREVER);

void hid_mouse_init(void) {
	k_thread_start(hid_mouse_id);
}

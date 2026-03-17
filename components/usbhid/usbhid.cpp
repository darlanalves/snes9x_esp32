/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "errno.h"
#include "driver/gpio.h"

#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/hid_usage_mouse.h"

#include "control.h"
#include "timing.h"

uint64_t devicestate = 0;

typedef struct
{
  uint64_t         mask;
  uint64_t         pressed_state;
//  uint64_t         released_state;
  int              key_state;
  uint32_t         key_timer;
} keymap_usb_t;

#if 0
#define BUTTON_MAP_USB {\
  [BUTTON_A]     = { 0b0000000000000000000000000000000000000000001000000000000000000000, 0b0000000000000000000000000000000000000000001000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_B]     = { 0b0000000000000000000000000000000000000000010000000000000000000000, 0b0000000000000000000000000000000000000000010000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_X]     = { 0b0000000000000000000000000000000000000000000100000000000000000000, 0b0000000000000000000000000000000000000000000100000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_Y]     = { 0b0000000000000000000000000000000000000000100000000000000000000000, 0b0000000000000000000000000000000000000000100000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_ST]    = { 0b0000000000000000000000000000000000000000000000000010000000000000, 0b0000000000000000000000000000000000000000000000000010000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_SEL]   = { 0b0000000000000000000000000000000000000000000000000001000000000000, 0b0000000000000000000000000000000000000000000000000000100000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_L]     = { 0b0000000000000000000000000000000000000000000000000000000100000000, 0b0000000000000000000000000000000000000000000000000000000100000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_R]     = { 0b0000000000000000000000000000000000000000000000000000001000000000, 0b0000000000000000000000000000000000000000000000000000001000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_SW]    = { 0b1000000000000000000000000000000000000000000000000000000000000000, 0b1000000000000000000000000000000000000000000000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_LEFT]  = { 0b0000000000000000000000000111111100000000000000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0b0000000000000000000000000111111100000000000000000000000000000000, 0, 0 }, \
  [BUTTON_RIGHT] = { 0b0000000000000000000000001000000000000000000000000000000000000000, 0b0000000000000000000000001000000000000000000000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  [BUTTON_UP]    = { 0b0000000000000000000000000000000001111111000000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0b0000000000000000000000000000000001111111000000000000000000000000, 0, 0 }, \
  [BUTTON_DOWN]  = { 0b0000000000000000000000000000000010000000000000000000000000000000, 0b0000000000000000000000000000000010000000000000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 } \
};
#endif

#define BUTTON_MAP_USB {\
  { 0b0000000000000000000000000000000000000000001000000000000000000000, 0b0000000000000000000000000000000000000000001000000000000000000000, 0, 0 }, \
  { 0b0000000000000000000000000000000000000000010000000000000000000000, 0b0000000000000000000000000000000000000000010000000000000000000000, 0, 0 }, \
  { 0b0000000000000000000000000000000000000000000100000000000000000000, 0b0000000000000000000000000000000000000000000100000000000000000000, 0, 0 }, \
  { 0b0000000000000000000000000000000000000000100000000000000000000000, 0b0000000000000000000000000000000000000000100000000000000000000000, 0, 0 }, \
  { 0b0000000000000000000000000111111100000000000000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  { 0b0000000000000000000000001000000000000000000000000000000000000000, 0b0000000000000000000000001000000000000000000000000000000000000000, 0, 0 }, \
  { 0b0000000000000000000000000000000001111111000000000000000000000000, 0b0000000000000000000000000000000000000000000000000000000000000000, 0, 0 }, \
  { 0b0000000000000000000000000000000010000000000000000000000000000000, 0b0000000000000000000000000000000010000000000000000000000000000000, 0, 0 }, \
  { 0b0000000000000000000000000000000000000000000000000010000000000000, 0b0000000000000000000000000000000000000000000000000010000000000000, 0, 0 }, \
  { 0b0000000000000000000000000000000000000000000000000001000000000000, 0b0000000000000000000000000000000000000000000000000000100000000000, 0, 0 }, \
  { 0b0000000000000000000000000000000000000000000000000000000100000000, 0b0000000000000000000000000000000000000000000000000000000100000000, 0, 0 }, \
  { 0b0000000000000000000000000000000000000000000000000000001000000000, 0b0000000000000000000000000000000000000000000000000000001000000000, 0, 0 }, \
  { 0b1000000000000000000000000000000000000000000000000000000000000000, 0b1000000000000000000000000000000000000000000000000000000000000000, 0, 0 } \
}; \

keymap_usb_t keymap_usb[NUM_BTNS] = BUTTON_MAP_USB;
static const char *USBTAG = "example";

QueueHandle_t app_event_queue = NULL;

/**
 * @brief APP event group
 *
 * Application logic can be different. There is a one among other ways to distingiush the
 * event by application event group.
 * In this example we have two event groups:
 * APP_EVENT            - General event, which is APP_QUIT_PIN press event (Generally, it is IO0).
 * APP_EVENT_HID_HOST   - HID Host Driver event, such as device connection/disconnection or input report.
 */
typedef enum {
    APP_EVENT = 0,
    APP_EVENT_HID_HOST
} app_event_group_t;

/**
 * @brief APP event queue
 *
 * This event is used for delivering the HID Host event from callback to a task.
 */
typedef struct {
    app_event_group_t event_group;
    /* HID Host - Device related info */
    struct {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void *arg;
    } hid_host_device;
} app_event_queue_t;

/**
 * @brief HID Protocol string names
 */
static const char *hid_proto_name_str[] = {
    "NONE",
    "KEYBOARD",
    "MOUSE"
};

/**
 * @brief USB HID Host Generic Interface report callback handler
 *
 * 'generic' means anything else than mouse or keyboard
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
static void hid_host_generic_report_callback(const uint8_t *const data, const int length)
{
    devicestate = 0;

    for (int i = 0; i < length; i++)
        devicestate |= ((uint64_t) data[length - i - 1]) << (i * 8);

    for (int i = 0; i < NUM_BTNS; i++) {
        if ((devicestate & keymap_usb[i].mask) == keymap_usb[i].pressed_state) {
            if (keymap_usb[i].key_state == 0) {
                keymap_usb[i].key_state = 1;
                keymap_usb[i].key_timer = millis();
            }
        } else {
            keymap_usb[i].key_state = 0;
        }
    }

}

/**
 * @brief USB HID Host interface callback
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host interface event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg)
{
    uint8_t data[64] = { 0 };
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                                  data,
                                                                  64,
                                                                  &data_length));

        hid_host_generic_report_callback(data, data_length);

        break;
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(USBTAG, "HID Device, protocol '%s' DISCONNECTED",
                 hid_proto_name_str[dev_params.proto]);
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGI(USBTAG, "HID Device, protocol '%s' TRANSFER_ERROR",
                 hid_proto_name_str[dev_params.proto]);
        break;
    default:
        ESP_LOGE(USBTAG, "HID Device, protocol '%s' Unhandled event",
                 hid_proto_name_str[dev_params.proto]);
        break;
    }
}

/**
 * @brief USB HID Host Device event
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host Device event
 * @param[in] arg                Pointer to arguments, does not used
 */
void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event,
                           void *arg)
{
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    const hid_host_device_config_t dev_config = {
        .callback = hid_host_interface_callback,
        .callback_arg = NULL
    };

    switch (event) {
    case HID_HOST_DRIVER_EVENT_CONNECTED:
        ESP_LOGI(USBTAG, "HID Device, protocol '%s' CONNECTED",
                 hid_proto_name_str[dev_params.proto]);

        ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT));
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
            }
        }
        ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
        break;
    default:
        break;
    }
}

/**
 * @brief Start USB Host install and handle common USB host library events while app pin not low
 *
 * @param[in] arg  Not used
 */
static void usb_lib_task(void *arg)
{
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));
    xTaskNotifyGive((TaskHandle_t) arg);

    while (true) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        // In this example, there is only one client registered
        // So, once we deregister the client, this call must succeed with ESP_OK
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_ERROR_CHECK(usb_host_device_free_all());
            break;
        }
    }

    ESP_LOGI(USBTAG, "USB shutdown");
    // Clean up USB Host
    vTaskDelay(10); // Short delay to allow clients clean-up
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelete(NULL);
}

/**
 * @brief BOOT button pressed callback
 *
 * Signal application to exit the HID Host task
 *
 * @param[in] arg Unused
 */
static void gpio_isr_cb(void *arg)
{
    BaseType_t xTaskWoken = pdFALSE;
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT,
    };

    if (app_event_queue) {
        xQueueSendFromISR(app_event_queue, &evt_queue, &xTaskWoken);
    }

    if (xTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief HID Host Device callback
 *
 * Puts new HID Device event to the queue
 *
 * @param[in] hid_device_handle HID Device handle
 * @param[in] event             HID Device event
 * @param[in] arg               Not used
 */
void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_driver_event_t event,
                              void *arg)
{
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT_HID_HOST,
        // HID Host Device related info
        .hid_host_device = {
            .handle = hid_device_handle,
            .event = event,
            .arg = arg
        }
    };

    if (app_event_queue) {
        xQueueSend(app_event_queue, &evt_queue, 0);
    }
}

uint64_t usb_ctrl_get_devicestate() {
  return devicestate;
}

bool usb_ctrl_is_pressed(int btn) {
  if (btn > NUM_BTNS + 1 /* ANY */)
    return false;

  if (btn == BUTTON_SW)
    return false;

  if (btn == BUTTON_ANY) {
    for (int i = 0; i < NUM_BTNS; i++)
      if (keymap_usb[i].key_state == 1)
        return true;
    
    return false;
  } else {
    return keymap_usb[btn].key_state == 1;
  }
}

void hid_main_task(void *parameter)
{
    BaseType_t task_created;
    app_event_queue_t evt_queue;
    ESP_LOGI(USBTAG, "HID Host example");

    /*
    * Create usb_lib_task to:
    * - initialize USB Host library
    * - Handle USB Host events while APP pin in in HIGH state
    */
    task_created = xTaskCreatePinnedToCore(usb_lib_task,
                                           "usb_events",
                                           4096,
                                           xTaskGetCurrentTaskHandle(),
                                           2, NULL, 0);
    assert(task_created == pdTRUE);

    // Wait for notification from usb_lib_task to proceed
    ulTaskNotifyTake(false, 1000);

    /*
    * HID host driver configuration
    * - create background task for handling low level event inside the HID driver
    * - provide the device callback to get new HID Device connection event
    */
    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = hid_host_device_callback,
        .callback_arg = NULL
    };

    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

    // Create queue
    app_event_queue = xQueueCreate(10, sizeof(app_event_queue_t));

    ESP_LOGI(USBTAG, "Waiting for HID Device to be connected");

    while (1) {
        // Wait queue
        if (xQueueReceive(app_event_queue, &evt_queue, portMAX_DELAY)) {
            if (APP_EVENT == evt_queue.event_group) {
                // User pressed button
                usb_host_lib_info_t lib_info;
                ESP_ERROR_CHECK(usb_host_lib_info(&lib_info));
                if (lib_info.num_devices == 0) {
                    // End while cycle
                    break;
                } else {
                    ESP_LOGW(USBTAG, "To shutdown example, remove all USB devices and press button again.");
                    // Keep polling
                }
            }

            if (APP_EVENT_HID_HOST ==  evt_queue.event_group) {
                hid_host_device_event(evt_queue.hid_host_device.handle,
                                      evt_queue.hid_host_device.event,
                                      evt_queue.hid_host_device.arg);
            }
        }
    }

    ESP_LOGI(USBTAG, "HID Driver uninstall");
    ESP_ERROR_CHECK(hid_host_uninstall());
    xQueueReset(app_event_queue);
    vQueueDelete(app_event_queue);
}

void init_hid() {
  xTaskCreatePinnedToCore(
    hid_main_task,   /* Function to implement the task */
    "hid_main_task", /* Name of the task */
    1024,          /* Stack size in words */
    NULL,          /* Task input parameter */
    18,             /* Priority of the task */
    NULL,          /* Task handle. */
    0);            /* Core where the task should run */
}
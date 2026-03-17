/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED

#include "soc/soc_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "lcd_mipi_dsi.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

#define TXW350099C0_CMD_SHLR_BIT    (1ULL << 0)
#define TXW350099C0_CMD_UPDN_BIT    (1ULL << 1)

#define LCD_H_RES (640)
#define LCD_V_RES (480)
#define MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV (2500)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCD panel initialization commands.
 *
 */
typedef struct {
    int cmd;                /*<! The specific LCD command */
    const void *data;       /*<! Buffer that holds the command specific data */
    size_t data_bytes;      /*<! Size of `data` in memory, in bytes */
    unsigned int delay_ms;  /*<! Delay in milliseconds after this command */
} txw350099c0_lcd_init_cmd_t;

/**
 * @brief LCD panel vendor configuration.
 *
 * @note  This structure needs to be passed to the `vendor_config` field in `esp_lcd_panel_dev_config_t`.
 *
 */
typedef struct {
    const txw350099c0_lcd_init_cmd_t *init_cmds;         /*!< Pointer to initialization commands array. Set to NULL if using default commands.
                                                     *   The array should be declared as `static const` and positioned outside the function.
                                                     *   Please refer to `vendor_specific_init_default` in source file.
                                                     */
    uint16_t init_cmds_size;                        /*<! Number of commands in above array */
    struct {
        esp_lcd_dsi_bus_handle_t dsi_bus;               /*!< MIPI-DSI bus configuration */
        const esp_lcd_dpi_panel_config_t *dpi_config;   /*!< MIPI-DPI panel configuration */
    } mipi_config;
    struct {
        unsigned int use_mipi_interface: 1;         /*<! Set to 1 if using MIPI interface, default is RGB interface */
        unsigned int mirror_by_cmd: 1;              /*<! The `mirror()` function will be implemented by LCD command if set to 1. This flag is only valid for the RGB interface.
                                                     *   Otherwise, the function will be implemented by software.
                                                     */

        union {
            unsigned int auto_del_panel_io: 1;
            unsigned int enable_io_multiplex: 1;
        };  /*<! Delete the panel IO instance automatically if set to 1. All `*_by_cmd` flags will be invalid.
             *   If the panel IO pins are sharing other pins of the RGB interface to save GPIOs,
             *   Please set it to 1 to release the panel IO and its pins (except CS signal).
             *   This flag is only valid for the RGB interface.
             */
    } flags;
} panel_vendor_config_t;

/**
 * @brief MIPI DSI bus configuration structure
 *
 */
#define PANEL_BUS_DSI_2CH_CONFIG()                \
    {                                                     \
        .bus_id = 0,                                      \
        .num_data_lanes = 2,                              \
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,      \
        .lane_bit_rate_mbps = 370,                       \
    }

/**
 * @brief MIPI DBI panel IO configuration structure
 *
 */
#define PANEL_IO_DBI_CONFIG() \
    {                                 \
        .virtual_channel = 0,         \
        .lcd_cmd_bits = 8,            \
        .lcd_param_bits = 8,          \
    }

/**
 * @brief MIPI DPI configuration structure
 *
 * @note  refresh_rate = (dpi_clock_freq_mhz * 1000000) / (h_res + hsync_pulse_width + hsync_back_porch + hsync_front_porch)
 *                                                      / (v_res + vsync_pulse_width + vsync_back_porch + vsync_front_porch)
 *
 */
#define PANEL_CONFIG(px_format, freq)                            \
    {                                                            \
        .virtual_channel = 0,                                    \
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,             \
        .dpi_clock_freq_mhz = (int) (35 * freq / 60),            \
        .pixel_format = px_format,                               \
        .num_fbs = 2,                                            \
        .video_timing = {                                        \
            .h_size = LCD_H_RES,                                 \
            .v_size = LCD_V_RES,                                 \
            .hsync_pulse_width = 120,                             \
            .hsync_back_porch = 120,                             \
            .hsync_front_porch = 120,                            \
            .vsync_pulse_width = 5,                              \
            .vsync_back_porch = 13,                              \
            .vsync_front_porch = 17,                             \
        },                                                       \
        .flags = {                                               \
          .use_dma2d = true,                                     \
        },                                                       \
    }

typedef struct {
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    const txw350099c0_lcd_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    struct {
        unsigned int reset_level: 1;
    } flags;
    // To save the original functions of MIPI DPI panel
    esp_err_t (*del)(esp_lcd_panel_t *panel);
    esp_err_t (*init)(esp_lcd_panel_t *panel);
} txw350099c0_panel_t;

/**************************************** END OF HEADER *************************************/

static const char *PANEL_TAG = "txw350099c0";

static esp_err_t panel_txw350099c0_send_init_cmds(txw350099c0_panel_t *txw350099c0);

static esp_err_t panel_txw350099c0_del(esp_lcd_panel_t *panel);
static esp_err_t panel_txw350099c0_init(esp_lcd_panel_t *panel);
static esp_err_t panel_txw350099c0_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_txw350099c0_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_txw350099c0_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_txw350099c0_disp_on_off(esp_lcd_panel_t *panel, bool on_off);

esp_err_t esp_lcd_new_panel(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                    esp_lcd_panel_handle_t *ret_panel, int lcd_scale)
{
/*
    ESP_LOGI(PANEL_TAG, "version: %d.%d.%d", ESP_LCD_TXW350099C0_VER_MAJOR, ESP_LCD_TXW350099C0_VER_MINOR,
             ESP_LCD_TXW350099C0_VER_PATCH);
*/
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, PANEL_TAG, "invalid arguments");
    panel_vendor_config_t *vendor_config = (panel_vendor_config_t *)panel_dev_config->vendor_config;
    ESP_RETURN_ON_FALSE(vendor_config && vendor_config->mipi_config.dpi_config && vendor_config->mipi_config.dsi_bus, ESP_ERR_INVALID_ARG, PANEL_TAG,
                        "invalid vendor config");

    esp_err_t ret = ESP_OK;
    txw350099c0_panel_t *txw350099c0 = (txw350099c0_panel_t *)calloc(1, sizeof(txw350099c0_panel_t));
    ESP_RETURN_ON_FALSE(txw350099c0, ESP_ERR_NO_MEM, PANEL_TAG, "no mem for txw350099c0 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        ESP_GOTO_ON_ERROR(gpio_reset_pin(panel_dev_config->reset_gpio_num), err, PANEL_TAG, "failed to setup reset GPIO");
        gpio_set_direction(panel_dev_config->reset_gpio_num, GPIO_MODE_OUTPUT);
        gpio_set_level(panel_dev_config->reset_gpio_num, 0);
    }

    txw350099c0->io = io;
    txw350099c0->init_cmds = vendor_config->init_cmds;
    txw350099c0->init_cmds_size = vendor_config->init_cmds_size;
    txw350099c0->reset_gpio_num = panel_dev_config->reset_gpio_num;
    txw350099c0->flags.reset_level = panel_dev_config->flags.reset_active_high;

    // Create MIPI DPI panel
    ESP_GOTO_ON_ERROR(lcd_new_panel_dpi(vendor_config->mipi_config.dsi_bus, vendor_config->mipi_config.dpi_config, ret_panel, lcd_scale), err, PANEL_TAG,
                      "create MIPI DPI panel failed");
    ESP_LOGD(PANEL_TAG, "new MIPI DPI panel @%p", *ret_panel);

    // Save the original functions of MIPI DPI panel
    txw350099c0->del = (*ret_panel)->del;
    txw350099c0->init = (*ret_panel)->init;
    // Overwrite the functions of MIPI DPI panel
    (*ret_panel)->del = panel_txw350099c0_del;
    (*ret_panel)->init = panel_txw350099c0_init;
    (*ret_panel)->reset = panel_txw350099c0_reset;
    (*ret_panel)->mirror = panel_txw350099c0_mirror;
    (*ret_panel)->invert_color = panel_txw350099c0_invert_color;
    (*ret_panel)->disp_on_off = panel_txw350099c0_disp_on_off;
    (*ret_panel)->user_data = txw350099c0;
    ESP_LOGD(PANEL_TAG, "new txw350099c0 panel @%p", txw350099c0);

    return ESP_OK;

err:
    if (txw350099c0) {
        free(txw350099c0);
    }
    return ret;
}
static const txw350099c0_lcd_init_cmd_t vendor_specific_init_default[] = {
//  {cmd, { data }, data_size, delay_ms}
  //{0x35, (uint8_t []) {0x00}, 1, 0},  // TE
  // Set EXTC
  //{0xB9, (uint8_t []) {0xF1, 0x12, 0x83}, 3, 0},
  // Set DSI
  //{0xBA, (uint8_t []) {0x31, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x25, \
                          0x00, 0x90, 0x0A, 0x00, 0x00, 0x01, 0x4F, 0x01, 0x00, 0x00, 0x37}, 27, 0},
  {0x11, (uint8_t []) {0x00}, 0, 120},

  {0x29, (uint8_t []) {0x00}, 0, 50},  // disp on

};

static esp_err_t panel_txw350099c0_send_init_cmds(txw350099c0_panel_t *txw350099c0)
{
    esp_lcd_panel_io_handle_t io = txw350099c0->io;
    const txw350099c0_lcd_init_cmd_t *init_cmds = NULL;
    uint16_t init_cmds_size = 0;

    // vendor specific initialization, it can be different between manufacturers
    // should consult the LCD supplier for initialization sequence code
    if (txw350099c0->init_cmds) {
        init_cmds = txw350099c0->init_cmds;
        init_cmds_size = txw350099c0->init_cmds_size;
    } else {
        init_cmds = vendor_specific_init_default;
        init_cmds_size = sizeof(vendor_specific_init_default) / sizeof(txw350099c0_lcd_init_cmd_t);
    }

    for (int i = 0; i < init_cmds_size; i++) {
        // Send command
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_bytes),
                            PANEL_TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(init_cmds[i].delay_ms));
    }
    ESP_LOGD(PANEL_TAG, "send init commands success");

    return ESP_OK;
}

static esp_err_t panel_txw350099c0_del(esp_lcd_panel_t *panel)
{
    txw350099c0_panel_t *txw350099c0 = (txw350099c0_panel_t *)panel->user_data;

    if (txw350099c0->reset_gpio_num >= 0) {
        gpio_reset_pin(txw350099c0->reset_gpio_num);
        gpio_set_direction(txw350099c0->reset_gpio_num, GPIO_MODE_OUTPUT);
        gpio_set_level(txw350099c0->reset_gpio_num, 0);
    }
    // Delete MIPI DPI panel
    txw350099c0->del(panel);
    free(txw350099c0);
    ESP_LOGD(PANEL_TAG, "del txw350099c0 panel @%p", txw350099c0);

    return ESP_OK;
}

static esp_err_t panel_txw350099c0_init(esp_lcd_panel_t *panel)
{
    txw350099c0_panel_t *txw350099c0 = (txw350099c0_panel_t *)panel->user_data;

    ESP_RETURN_ON_ERROR(panel_txw350099c0_send_init_cmds(txw350099c0), PANEL_TAG, "send init commands failed");
    ESP_RETURN_ON_ERROR(txw350099c0->init(panel), PANEL_TAG, "init MIPI DPI panel failed");

    return ESP_OK;
}

static esp_err_t panel_txw350099c0_reset(esp_lcd_panel_t *panel)
{
    txw350099c0_panel_t *txw350099c0 = (txw350099c0_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = txw350099c0->io;

    // Perform hardware reset
    if (txw350099c0->reset_gpio_num >= 0) {
        gpio_reset_pin(txw350099c0->reset_gpio_num);
        gpio_set_direction(txw350099c0->reset_gpio_num, GPIO_MODE_OUTPUT);
        gpio_set_level(txw350099c0->reset_gpio_num, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_direction(txw350099c0->reset_gpio_num, GPIO_MODE_INPUT);
        // floating means 1, since external pullup is connected
        // !! Do not set to 1, since IOVCC is 1.8V !!
        gpio_set_pull_mode(txw350099c0->reset_gpio_num, GPIO_FLOATING);
        vTaskDelay(pdMS_TO_TICKS(150));
    } else if (io) { // Perform software reset
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0), PANEL_TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return ESP_OK;
}

static esp_err_t panel_txw350099c0_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    txw350099c0_panel_t *txw350099c0 = (txw350099c0_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = txw350099c0->io;
    uint8_t madctl_val = 0x01;

    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_STATE, PANEL_TAG, "invalid panel IO");

    // Control mirror through LCD command
    if (mirror_x) {
        madctl_val |= TXW350099C0_CMD_SHLR_BIT;
    } else {
        madctl_val &= ~TXW350099C0_CMD_SHLR_BIT;
    }
    if (mirror_y) {
        madctl_val |= TXW350099C0_CMD_UPDN_BIT;
    } else {
        madctl_val &= ~TXW350099C0_CMD_UPDN_BIT;
    }

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t []) {
        madctl_val
    }, 1), PANEL_TAG, "send command failed");

    return ESP_OK;
}

static esp_err_t panel_txw350099c0_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    txw350099c0_panel_t *txw350099c0 = (txw350099c0_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = txw350099c0->io;
    uint8_t command = 0;

    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_STATE, PANEL_TAG, "invalid panel IO");

    if (invert_color_data) {
        command = LCD_CMD_INVON;
    } else {
        command = LCD_CMD_INVOFF;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), PANEL_TAG, "send command failed");

    return ESP_OK;
}

static esp_err_t panel_txw350099c0_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    ESP_LOGE(PANEL_TAG, "display on/off is not supported");

    return ESP_ERR_NOT_SUPPORTED;
}

#ifdef __cplusplus
}
#endif

#endif // SOC_MIPI_DSI_SUPPORTED

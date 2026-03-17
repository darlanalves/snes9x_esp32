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

#define P055H020V2_CMD_SHLR_BIT    (1ULL << 0)
#define P055H020V2_CMD_UPDN_BIT    (1ULL << 1)

#define LCD_H_RES (720)
#define LCD_V_RES (1280)
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
} p055h020v2_lcd_init_cmd_t;

/**
 * @brief LCD panel vendor configuration.
 *
 * @note  This structure needs to be passed to the `vendor_config` field in `esp_lcd_panel_dev_config_t`.
 *
 */
typedef struct {
    const p055h020v2_lcd_init_cmd_t *init_cmds;         /*!< Pointer to initialization commands array. Set to NULL if using default commands.
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
        .lane_bit_rate_mbps = 1000,                       \
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
        .dpi_clock_freq_mhz = (int) (64 * freq / 60),            \
        .pixel_format = px_format,                               \
        .num_fbs = 2,                                            \
        .video_timing = {                                        \
            .h_size = LCD_H_RES,                                 \
            .v_size = LCD_V_RES,                                 \
            .hsync_pulse_width = 20,                             \
            .hsync_back_porch = 40,                             \
            .hsync_front_porch = 40,                            \
            .vsync_pulse_width = 3,                              \
            .vsync_back_porch = 10,                              \
            .vsync_front_porch = 16,                             \
        },                                                       \
        .flags = {                                               \
          .use_dma2d = true,                                     \
        },                                                       \
    }

typedef struct {
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    const p055h020v2_lcd_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    struct {
        unsigned int reset_level: 1;
    } flags;
    // To save the original functions of MIPI DPI panel
    esp_err_t (*del)(esp_lcd_panel_t *panel);
    esp_err_t (*init)(esp_lcd_panel_t *panel);
} p055h020v2_panel_t;

/**************************************** END OF HEADER *************************************/

static const char *PANEL_TAG = "p055h020v2";

static esp_err_t panel_p055h020v2_send_init_cmds(p055h020v2_panel_t *p055h020v2);

static esp_err_t panel_p055h020v2_del(esp_lcd_panel_t *panel);
static esp_err_t panel_p055h020v2_init(esp_lcd_panel_t *panel);
static esp_err_t panel_p055h020v2_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_p055h020v2_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_p055h020v2_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_p055h020v2_disp_on_off(esp_lcd_panel_t *panel, bool on_off);

esp_err_t esp_lcd_new_panel(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                    esp_lcd_panel_handle_t *ret_panel, int lcd_scale)
{
/*
    ESP_LOGI(PANEL_TAG, "version: %d.%d.%d", ESP_LCD_P055H020V2_VER_MAJOR, ESP_LCD_P055H020V2_VER_MINOR,
             ESP_LCD_P055H020V2_VER_PATCH);
*/
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, PANEL_TAG, "invalid arguments");
    panel_vendor_config_t *vendor_config = (panel_vendor_config_t *)panel_dev_config->vendor_config;
    ESP_RETURN_ON_FALSE(vendor_config && vendor_config->mipi_config.dpi_config && vendor_config->mipi_config.dsi_bus, ESP_ERR_INVALID_ARG, PANEL_TAG,
                        "invalid vendor config");

    esp_err_t ret = ESP_OK;
    p055h020v2_panel_t *p055h020v2 = (p055h020v2_panel_t *)calloc(1, sizeof(p055h020v2_panel_t));
    ESP_RETURN_ON_FALSE(p055h020v2, ESP_ERR_NO_MEM, PANEL_TAG, "no mem for p055h020v2 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        ESP_GOTO_ON_ERROR(gpio_reset_pin(panel_dev_config->reset_gpio_num), err, PANEL_TAG, "failed to setup reset GPIO");
        gpio_set_direction(panel_dev_config->reset_gpio_num, GPIO_MODE_OUTPUT);
        gpio_set_level(panel_dev_config->reset_gpio_num, 0);
    }

    p055h020v2->io = io;
    p055h020v2->init_cmds = vendor_config->init_cmds;
    p055h020v2->init_cmds_size = vendor_config->init_cmds_size;
    p055h020v2->reset_gpio_num = panel_dev_config->reset_gpio_num;
    p055h020v2->flags.reset_level = panel_dev_config->flags.reset_active_high;

    // Create MIPI DPI panel
    ESP_GOTO_ON_ERROR(lcd_new_panel_dpi(vendor_config->mipi_config.dsi_bus, vendor_config->mipi_config.dpi_config, ret_panel, lcd_scale), err, PANEL_TAG,
                      "create MIPI DPI panel failed");
    ESP_LOGD(PANEL_TAG, "new MIPI DPI panel @%p", *ret_panel);

    // Save the original functions of MIPI DPI panel
    p055h020v2->del = (*ret_panel)->del;
    p055h020v2->init = (*ret_panel)->init;
    // Overwrite the functions of MIPI DPI panel
    (*ret_panel)->del = panel_p055h020v2_del;
    (*ret_panel)->init = panel_p055h020v2_init;
    (*ret_panel)->reset = panel_p055h020v2_reset;
    (*ret_panel)->mirror = panel_p055h020v2_mirror;
    (*ret_panel)->invert_color = panel_p055h020v2_invert_color;
    (*ret_panel)->disp_on_off = panel_p055h020v2_disp_on_off;
    (*ret_panel)->user_data = p055h020v2;
    ESP_LOGD(PANEL_TAG, "new p055h020v2 panel @%p", p055h020v2);

    return ESP_OK;

err:
    if (p055h020v2) {
        free(p055h020v2);
    }
    return ret;
}

static const p055h020v2_lcd_init_cmd_t vendor_specific_init_default[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {0xB9, (uint8_t []) {0xF1,0x12,0x83}, 0x03 , 0},  // enter user cmd
    {0xB1, (uint8_t []) {0x00,0x00,0x00,0xDA,0x80}, 0x05, 0},  // ?
    {0xB2, (uint8_t []) {0xC8,0x02,0xF0}, 0x03, 0},  // SETDISP (resolution)
    {0xB3, (uint8_t []) {0x10,0x10,0x28,0x28,0x03,0xFF,0x00,0x00,0x00,0x00}, 0x0A, 0},  // SETRGBIF sync timing
    {0xB4, (uint8_t []) {0x80}, 0x01, 0},  // SETCYC inversion pattern
    {0xB5, (uint8_t []) {0x0A,0x0A}, 0x02, 0},  // SETBGP VREF
    {0xB6, (uint8_t []) {0x92,0x92}, 0x02, 0},//  B_VCOM
    {0xB8, (uint8_t []) {0x25,0x22,0xF0,0x13}, 0x04, 0}, // SETPOWER_EXT
    {0xBA, (uint8_t []) {0x31,0x81,0x05,0xF9,0x0E,0x0E,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x25,0x00,0x90,0x0A,0x00,0x00,0x01,0x4F,0x01,0x00,0x00,0x37}, 27, 0},  // SETMIPI
    {0xBC, (uint8_t []) {0x47}, 0x01, 0},  // SETVDC
    {0xBF, (uint8_t []) {0x02,0x11,0x00}, 0x03, 0},   // ?
    {0xC0, (uint8_t []) {0x73,0x73,0x50,0x50,0x00,0x00,0x12,0x70,0x00}, 0x09, 0},  // SETSCR source driving
    {0xC1, (uint8_t []) {0x54,0xC0,0x32,0x32,0x77,0xF1,0xFF,0xFF,0xCC,0xCC,0x77,0x77}, 12, 0},  // SETPOWER
    {0xC6, (uint8_t []) {0x82,0x00,0xBF,0xFF,0x00,0xFF}, 0x06, 0},  // ?
    {0xC7, (uint8_t []) {0xB8,0x00,0x0A,0x00,0x00,0x02}, 0x06, 0},  // SETIO
    {0xC8, (uint8_t []) {0x10,0x40,0x1E,0x02}, 0x04, 0},  //  SETCABC
    {0xCC, (uint8_t []) {0x0B}, 0x01, 0},  // SETPANEL  (direction, RGB order)
    {0xE0, (uint8_t []) {0x00,0x0C,0x0F,0x3F,0x3F,0x3F,0x3F,0x42,0x05,0x0B,0x0B,0x0D,0x0E,0x0C,0x0F,0x17,0x1F,0x00,0x0C,0x0F,0x3F,0x3F,0x3F,0x3F,0x42,0x05,0x0B,0x0B,0x0D,0x0E,0x0C,0x0F,0x17,0x1F}, 34, 0}, // SETGAMMA
    {0xE3, (uint8_t []) {0x07,0x07,0x0B,0x0B,0x0B,0x0B,0x00,0x00,0x00,0x00,0xFF,0x00,0xC0,0x10}, 14, 0},// SLPOTP   //SETEQ
    {0xE9, (uint8_t []) {0xC8,0x10,0x03,0x05,0x01,0x80,0x81,0x12,0x31,0x23,0x37,0x81,0x80,0x81,0x37,0x00,0x00,0x81,0x00,0x00,0x00,0x00,0x00,0x81,0x00,0x00,0x00,0x00,0xF8,0xBA,0x02,0x46,0x08,0x88,0x88,0x84,0x88,0x88,0x88,0xF8,0xBA,0x13,0x57,0x18,0x88,0x88,0x85,0x88,0x88,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, 63, 0},  // SETGIP1, internal display timing
    {0xEA, (uint8_t []) {0x96,0x12,0x01,0x01,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x8F,0xAB,0x75,0x31,0x58,0x88,0x88,0x81,0x88,0x88,0x88,0x8F,0xAB,0x64,0x20,0x48,0x88,0x88,0x80,0x88,0x88,0x88,0x23,0x00,0x00,0x00,0x6B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x80,0x81,0x00,0x00,0x00,0x00,0x00,0x00}, 63, 0},  // SETGIP2, internal display timing
    {0xEF, (uint8_t []) {0xFF,0xFF,0x01}, 0x03, 0}, // exit user cmd
    {0x11, (uint8_t []) {0x00}, 0x00, 250},//Sleep Out
    {0x29, (uint8_t []) {0x00}, 0x00, 50},//Display On

};

static esp_err_t panel_p055h020v2_send_init_cmds(p055h020v2_panel_t *p055h020v2)
{
    esp_lcd_panel_io_handle_t io = p055h020v2->io;
    const p055h020v2_lcd_init_cmd_t *init_cmds = NULL;
    uint16_t init_cmds_size = 0;

    // vendor specific initialization, it can be different between manufacturers
    // should consult the LCD supplier for initialization sequence code
    if (p055h020v2->init_cmds) {
        init_cmds = p055h020v2->init_cmds;
        init_cmds_size = p055h020v2->init_cmds_size;
    } else {
        init_cmds = vendor_specific_init_default;
        init_cmds_size = sizeof(vendor_specific_init_default) / sizeof(p055h020v2_lcd_init_cmd_t);
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

static esp_err_t panel_p055h020v2_del(esp_lcd_panel_t *panel)
{
    p055h020v2_panel_t *p055h020v2 = (p055h020v2_panel_t *)panel->user_data;

    if (p055h020v2->reset_gpio_num >= 0) {
        gpio_reset_pin(p055h020v2->reset_gpio_num);
        gpio_set_direction(p055h020v2->reset_gpio_num, GPIO_MODE_OUTPUT);
        gpio_set_level(p055h020v2->reset_gpio_num, 0);
    }
    // Delete MIPI DPI panel
    p055h020v2->del(panel);
    free(p055h020v2);
    ESP_LOGD(PANEL_TAG, "del p055h020v2 panel @%p", p055h020v2);

    return ESP_OK;
}

static esp_err_t panel_p055h020v2_init(esp_lcd_panel_t *panel)
{
    p055h020v2_panel_t *p055h020v2 = (p055h020v2_panel_t *)panel->user_data;

    ESP_RETURN_ON_ERROR(panel_p055h020v2_send_init_cmds(p055h020v2), PANEL_TAG, "send init commands failed");
    ESP_RETURN_ON_ERROR(p055h020v2->init(panel), PANEL_TAG, "init MIPI DPI panel failed");

    return ESP_OK;
}

static esp_err_t panel_p055h020v2_reset(esp_lcd_panel_t *panel)
{
    p055h020v2_panel_t *p055h020v2 = (p055h020v2_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = p055h020v2->io;

    // Perform hardware reset
    if (p055h020v2->reset_gpio_num >= 0) {
        gpio_reset_pin(p055h020v2->reset_gpio_num);
        gpio_set_direction(p055h020v2->reset_gpio_num, GPIO_MODE_OUTPUT);
        gpio_set_level(p055h020v2->reset_gpio_num, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_direction(p055h020v2->reset_gpio_num, GPIO_MODE_INPUT);
        // floating means 1, since external pullup is connected
        // !! Do not set to 1, since IOVCC is 1.8V !!
        gpio_set_pull_mode(p055h020v2->reset_gpio_num, GPIO_FLOATING);
        vTaskDelay(pdMS_TO_TICKS(250));
    } else if (io) { // Perform software reset
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0), PANEL_TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return ESP_OK;
}

static esp_err_t panel_p055h020v2_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    p055h020v2_panel_t *p055h020v2 = (p055h020v2_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = p055h020v2->io;
    uint8_t madctl_val = 0x01;

    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_STATE, PANEL_TAG, "invalid panel IO");

    // Control mirror through LCD command
    if (mirror_x) {
        madctl_val |= P055H020V2_CMD_SHLR_BIT;
    } else {
        madctl_val &= ~P055H020V2_CMD_SHLR_BIT;
    }
    if (mirror_y) {
        madctl_val |= P055H020V2_CMD_UPDN_BIT;
    } else {
        madctl_val &= ~P055H020V2_CMD_UPDN_BIT;
    }

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t []) {
        madctl_val
    }, 1), PANEL_TAG, "send command failed");

    return ESP_OK;
}

static esp_err_t panel_p055h020v2_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    p055h020v2_panel_t *p055h020v2 = (p055h020v2_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = p055h020v2->io;
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

static esp_err_t panel_p055h020v2_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    ESP_LOGE(PANEL_TAG, "display on/off is not supported");

    return ESP_ERR_NOT_SUPPORTED;
}

#ifdef __cplusplus
}
#endif

#endif // SOC_MIPI_DSI_SUPPORTED

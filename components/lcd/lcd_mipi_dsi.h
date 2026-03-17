/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_lcd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create LCD panel for MIPI DSI DPI interface
 *
 * @param[in] bus MIPI DSI bus handle, returned from `esp_lcd_new_dsi_bus`
 * @param[in] panel_config DSI data panel configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @param[in] lcd_scaling scaling factor
 * @return
 *      - ESP_OK: Create MIPI DSI data panel successfully
 *      - ESP_ERR_INVALID_ARG: Create MIPI DSI data panel failed because of invalid argument
 *      - ESP_ERR_NO_MEM: Create MIPI DSI data panel failed because of out of memory
 *      - ESP_ERR_NOT_SUPPORTED: Create MIPI DSI data panel failed because of unsupported feature
 *      - ESP_FAIL: Create MIPI DSI data panel failed because of other error
 */
esp_err_t lcd_new_panel_dpi(esp_lcd_dsi_bus_handle_t bus, const esp_lcd_dpi_panel_config_t *panel_config, esp_lcd_panel_handle_t *ret_panel, int lcd_scaling);

/**
 * @brief Get the address of the frame buffer(s) that allocated by the driver
 *
 * @param[in] dpi_panel MIPI DPI panel handle, returned from esp_lcd_new_panel_dpi()
 * @param[in] fb_num Number of frame buffer(s) to get. This value must be the same as the number of the followed parameters.
 * @param[out] fb0 Address of the frame buffer 0 (first frame buffer)
 * @param[out] ... List of other frame buffers if any
 * @return
 *      - ESP_ERR_INVALID_ARG: Get frame buffer address failed because of invalid argument
 *      - ESP_OK: Get frame buffer address successfully
 */
esp_err_t lcd_dpi_panel_get_frame_buffer(esp_lcd_panel_handle_t dpi_panel, uint32_t fb_num, void **fb0, ...);

/**
 * @brief Set color conversion configuration for DPI panel
 *
 * @param[in] dpi_panel MIPI DPI panel handle, returned from esp_lcd_new_panel_dpi()
 * @param[in] config Color conversion configuration
 * @return
 *      - ESP_OK: Set color conversion configuration successfully
 *      - ESP_ERR_INVALID_ARG: Set color conversion configuration failed because of invalid argument
 *      - ESP_FAIL: Set color conversion configuration failed because of other error
 */
esp_err_t lcd_dpi_panel_set_color_conversion(esp_lcd_panel_handle_t dpi_panel, const esp_lcd_color_conv_config_t *config);

/**
 * @brief Register LCD DPI panel callbacks
 *
 * @param[in] dpi_panel LCD DPI panel handle, which is returned from esp_lcd_new_panel_dpi()
 * @param[in] cbs structure with all LCD panel callbacks
 * @param[in] user_ctx User private data, passed directly to callback's user_ctx
 * @return
 *      - ESP_ERR_INVALID_ARG: Register callbacks failed because of invalid argument
 *      - ESP_OK: Register callbacks successfully
 */
esp_err_t lcd_dpi_panel_register_event_callbacks(esp_lcd_panel_handle_t dpi_panel, const esp_lcd_dpi_panel_event_callbacks_t *cbs, void *user_ctx);

#ifdef __cplusplus
}
#endif

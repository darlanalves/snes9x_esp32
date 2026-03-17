/*
 * Snes9x running on the ESP32-P4-Function-EV-Board
 *
 * This file contains some code from: https://github.com/espressif
 * /esp-idf/blob/master/examples/peripherals/lcd/mipi_dsi/main/mipi_dsi_lcd_example_main.c
 *
 * Copyright (C) 2025 Daniel Kammer (daniel.kammer@web.de)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "driver/ledc.h"

#include "esp_cache.h"

#include "esp_log.h"

#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"

#include "lcd_mipi_dsi.h"
#include "lcd_drv.h"

#include "../engine/hwcfg.h"

//================================== HW setup ==================================
#include DISPLAY_DRIVER

#define MIPI_DSI_PHY_PWR_LDO_CHAN (3)

#define BL_LEDC_TIMER (LEDC_TIMER_0)
#define BL_LEDC_CHANNEL (LEDC_CHANNEL_0)
#define BL_LEDC_MODE (LEDC_LOW_SPEED_MODE)

/* =========================================== LCD ==================================================*/
//#define LCD_ROTATION

bool m_init = false;
SemaphoreHandle_t vsync_event;

// MIPI driver
static char *LCD_TAG = "DISP_DRV";
static esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
static esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;

uint16_t *tx_buf[2];

volatile int line_block_cnt = 2;  // WA for missing vsync, should be 0

volatile uint32_t mipi_status_info;

// framebuffer
volatile uint16_t *m_fb;

volatile int fb_buffer_width = 0;
volatile int fb_buffer_height = 0;

volatile int ofs_src_start;
volatile int ofs_x;
volatile int ofs_y;
volatile int ofs_src;
volatile int ofs_src_line_inc;
volatile int linebuf_width;
volatile int m_scale_factor;
volatile bool m_prefer_vsync_over_fps;

/* ==================================================================================================== */
void calculate_image_offset() {
  #ifdef LCD_ROTATION
  ofs_x = (LCD_V_RES - fb_buffer_width * m_scale_factor) / 2;

  ofs_y = (LCD_H_RES - fb_buffer_height * m_scale_factor) / 2;

  ofs_src_start = -ofs_x / m_scale_factor;

  ofs_src_line_inc = 1;  // increment on each line

  linebuf_width = fb_buffer_height;

  if (ofs_y < 0) {
    ofs_src_line_inc -= ofs_y / m_scale_factor * fb_buffer_width;
    linebuf_width = LCD_H_RES / m_scale_factor;
    ofs_y = 0;
  }

  #else
  ofs_x = (LCD_H_RES - fb_buffer_width * m_scale_factor) / 2;

  ofs_y = (LCD_V_RES - fb_buffer_height * m_scale_factor) / 2;

  ofs_src_start = -ofs_y / m_scale_factor * fb_buffer_width;

  ofs_src_line_inc = fb_buffer_width;  // increment on each line

  linebuf_width = fb_buffer_width;

  if (ofs_x < 0) {
    ofs_src_start -= ofs_x / m_scale_factor;
    linebuf_width = LCD_H_RES / m_scale_factor;
    ofs_x = 0;
  }
  #endif

  ofs_src = ofs_src_start;
}

IRAM_ATTR static bool notify_refresh_ready(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx) {
    /*
    // If the emulation loop has not yet acked the vsync event, then now it's too late
    // and the emulator has to wait another frame. This prevents vertical tearing 
    // (horiz. tearing when roatated) but will cause low framerates. (Everyone gets to choose)
    */
    if (m_prefer_vsync_over_fps)
      xSemaphoreTakeFromISR(vsync_event, NULL);

  int fb_num = ((uint32_t) user_ctx) & 0xff;

  if ((((uint32_t) user_ctx) & 0xff00) != 0) {
    // if a buffer unterrun of the MIPI tx buffer occured, the MIPI driver will issue a 
    // sync event and the whole frame has to be scanned out again, however the first
    // line will have already been scanned out.
    line_block_cnt = 1;
    ofs_src = ofs_src_start + ofs_src_line_inc;
  }

  uint16_t *fb_line = (uint16_t *) tx_buf[fb_num];
  assert(fb_line);

  #ifdef LCD_ROTATION
  if ((ofs_src >= 0) && (ofs_src < fb_buffer_width)) {
  #else
  if ((ofs_src >= 0) && (ofs_src < fb_buffer_height * fb_buffer_width)) {
  #endif

    #ifdef LCD_ROTATION
    uint16_t *dst = (uint16_t*) &fb_line[ofs_y];
    uint16_t *src = (uint16_t*) &m_fb[ofs_src + (linebuf_width - 1) * fb_buffer_width];
    #else
    uint16_t *dst = (uint16_t*) &fb_line[ofs_x];
    uint16_t *src = (uint16_t*) &m_fb[ofs_src];
    #endif

    if (m_scale_factor==3) {
      for (int x = 0; x < linebuf_width; x++) {
        *dst = *src;
        dst++;
        *dst = *src;
        dst++;
        *dst = *src;
        dst++;

        #ifdef LCD_ROTATION
        src -= fb_buffer_width;
        #else
        src++;
        #endif
      }
    } else if (m_scale_factor==2) {
      for (int x = 0; x < linebuf_width; x++) {
        *dst = *src;
        dst++;
        *dst = *src;
        dst++;

        #ifdef LCD_ROTATION
        src -= fb_buffer_width;
        #else
        src++;
        #endif
      }
    } else if (m_scale_factor==1) {
      for (int x = 0; x < linebuf_width; x++) {
        *dst = *src;
        dst++;

        #ifdef LCD_ROTATION
        src -= fb_buffer_width;
        #else
        src++;
        #endif
      }
    }
  } else {
    uint32_t *dst = (uint32_t*) &fb_line[0];

    for (int x = 0; x < LCD_H_RES / 2; x++) {
      *dst = 0;  // black
      dst++;
    }
  }

  esp_cache_msync((void *) fb_line, LCD_H_RES * 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);

  ofs_src += ofs_src_line_inc;

  line_block_cnt++;

  if (line_block_cnt == LCD_V_RES / m_scale_factor) {
    line_block_cnt = 0;
    ofs_src = ofs_src_start;

    xSemaphoreGiveFromISR(vsync_event, NULL);
  }

  return (pdFALSE);
}

void lcd_set_vsync(bool vsync) {
  m_prefer_vsync_over_fps = vsync;
}

void lcd_init(lcd_config_t lcd_config) {
  if (m_init)
    return;

  fb_buffer_width = lcd_config.buffer_width;
  fb_buffer_height = lcd_config.buffer_height;
  m_scale_factor = lcd_config.scale_factor;
  m_prefer_vsync_over_fps = lcd_config.prefer_vsync_over_fps;

  uint16_t refresh_rate = lcd_config.target_fps;

  if ((refresh_rate < 50) || (refresh_rate > 60))
    refresh_rate = 50;

  if ((m_scale_factor < 1) || (m_scale_factor > 3))
    m_scale_factor = 1;

  calculate_image_offset();

  //assert(gpio_reset_pin(PIN_LCD_TE) == ESP_OK);
  //gpio_set_direction(PIN_LCD_TE, GPIO_MODE_INPUT);

#ifdef PIN_LCD_BL_PWM
  ESP_LOGI(LCD_TAG, "Turn on LCD backlight");

  gpio_reset_pin((gpio_num_t) PIN_LCD_BL_PWM);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = BL_LEDC_MODE,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .timer_num        = BL_LEDC_TIMER,
        .freq_hz          = (100000),
        .clk_cfg          = LEDC_USE_XTAL_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = PIN_LCD_BL_PWM,
        .speed_mode     = BL_LEDC_MODE,
        .channel        = BL_LEDC_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = BL_LEDC_TIMER,
        .duty           = 5, // max: 255, max_battery: 127
        .hpoint         = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_LOGI(LCD_TAG, "Backlight init complete");
#endif

  // Turn on the power for MIPI DSI PHY, so it can go from "No Power" state to "Shutdown" state
#ifdef MIPI_DSI_PHY_PWR_LDO_CHAN
  ESP_LOGI(LCD_TAG, "MIPI DSI PHY Powered on");
  esp_ldo_channel_config_t ldo_mipi_phy_config = {
    .chan_id = MIPI_DSI_PHY_PWR_LDO_CHAN,
    .voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
  };
  assert(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy) == ESP_OK);
#endif

  ESP_LOGI(LCD_TAG, "Initialize MIPI DSI bus");
  esp_lcd_dsi_bus_config_t bus_config = PANEL_BUS_DSI_2CH_CONFIG();
  assert(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus) == ESP_OK);

  ESP_LOGI(LCD_TAG, "Install panel IO");
  esp_lcd_dbi_io_config_t dbi_config = PANEL_IO_DBI_CONFIG();
  assert(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io) == ESP_OK);

  ESP_LOGI(LCD_TAG, "Install LCD driver of %s", PANEL_TAG);
  ESP_LOGI(LCD_TAG, "Setting refresh rate to: %d", refresh_rate);
  esp_lcd_dpi_panel_config_t dpi_config = PANEL_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565, refresh_rate);
  panel_vendor_config_t vendor_config = {
    .mipi_config = {
      .dsi_bus = mipi_dsi_bus,
      .dpi_config = &dpi_config,
    },
    .flags = {
      .use_mipi_interface = 1,
    },
  };

  const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = PIN_LCD_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16,
    .flags = { .reset_active_high = 0 }, 
    .vendor_config = &vendor_config,
  };

  assert(esp_lcd_new_panel(mipi_dbi_io, &panel_config, &panel_handle, m_scale_factor) == ESP_OK);
  assert(lcd_dpi_panel_get_frame_buffer(panel_handle, 2, (void **) &tx_buf[0], (void **) &tx_buf[1]) == ESP_OK);

  // white borders
  #ifdef LCD_ROTATION
  int buf_len = LCD_H_RES;
  #else
  int buf_len = LCD_V_RES;
  #endif

  for (int i = 0; i < 2; i++)
    for (int j = 0; j < buf_len; j++)
      tx_buf[i][j] = 0;  // border color

  esp_lcd_dpi_panel_event_callbacks_t cbs = {
    .on_refresh_done = notify_refresh_ready,
  };
  assert(lcd_dpi_panel_register_event_callbacks(panel_handle, &cbs, (void*) &mipi_status_info) == ESP_OK);

  assert(esp_lcd_panel_reset(panel_handle) == ESP_OK);

  assert(esp_lcd_panel_init(panel_handle) == ESP_OK);

  vsync_event = xSemaphoreCreateBinary();

  assert(vsync_event);
  xSemaphoreGive(vsync_event);

  #ifdef PIN_LCD_BL_EN
  gpio_set_direction(PIN_LCD_BL_EN, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_LCD_BL_EN, 1);
  #endif

  #if 0
  esp_lcd_dpi_panel_set_pattern(panel_handle, MIPI_DSI_PATTERN_BAR_VERTICAL);
  while (1) {
    ESP_LOGE(LCD_TAG, "showing test pattern");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }    
  #endif

  m_init = true;
}

void lcd_deinit(void) {
  if (!m_init)
    return;

  assert(esp_lcd_panel_del(panel_handle) == ESP_OK);
  assert(esp_lcd_panel_io_del(mipi_dbi_io) == ESP_OK);
  assert(esp_lcd_del_dsi_bus(mipi_dsi_bus) == ESP_OK);
  panel_handle = NULL;
  mipi_dbi_io = NULL;
  mipi_dsi_bus = NULL;

  if (ldo_mipi_phy) {
    assert(esp_ldo_release_channel(ldo_mipi_phy) == ESP_OK);
    ldo_mipi_phy = NULL;
  }

  assert(gpio_reset_pin(PIN_LCD_RST) == ESP_OK);
  gpio_set_direction(PIN_LCD_RST, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_LCD_RST, 0);

  #ifdef PIN_LCD_BL_PWM
  assert(gpio_reset_pin(PIN_LCD_BL_PWM) == ESP_OK);
  #endif

  m_init = false;
}

void lcd_set_backlight(float percent) {
  if (!m_init)
    return;

#ifdef PIN_LCD_BL_PWM
  int level = percent * 2.55 / 5.;  // fcipaq TODO debug
  
  if ((level < 0) || (level > 255))
    return;

  // ledcWrite(1 /* PWM channel */, level /* duty cycle */);
  ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_CHANNEL, BL_LEDC_CHANNEL, level));
  ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_CHANNEL, BL_LEDC_CHANNEL));
#endif
}

bool lcd_set_brightness(int level) {
#ifdef PIN_LCD_BL_PWM
  if ((level < 0) || (level > 255))
    return false;

  // ledcWrite(1 /* PWM channel */, level /* duty cycle */);
  ESP_ERROR_CHECK(ledc_set_duty(BL_LEDC_CHANNEL, BL_LEDC_CHANNEL, level));
  ESP_ERROR_CHECK(ledc_update_duty(BL_LEDC_CHANNEL, BL_LEDC_CHANNEL));
#endif
  return true;
}

uint16_t lcd_get_fb_width() {
  return fb_buffer_width;
}

uint16_t lcd_get_fb_height() {
  return fb_buffer_height;  
}

uint16_t* lcd_get_fb() {
  return m_fb;
}

void lcd_set_fb(uint16_t* fb) {
  m_fb = fb;
}

void lcd_wait_vsync() {

  while (xSemaphoreTake(vsync_event, 0) != pdTRUE)
    ;  // sync to LCD

}
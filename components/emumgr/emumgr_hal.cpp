/*
 * emumgr - Emulation manager for Snes9x on the Pico Held 2
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

 #include <cstdio>
 #include <cstring>

 #include <picoengine.h>

 /* SD support */
#include <dirent.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include <sys/stat.h>
#include "sd_pwr_ctrl_by_on_chip_ldo.h"

extern "C" {
#include "lcd_drv.h"
}

#include "heapstrings.h"

#include "emumgr.h"
#include "emumgr_hal.h"

#include "hwcfg.h"

#include "usbhid.h"

#define ALPHABETIZE

#define MOUNTPOINT "/sd"

// sd
volatile int sd_is_init;
sdmmc_card_t *card = NULL;
sdmmc_host_t host;
FIL fil;
uint32_t filesize = 0;
bool ldo_init;

extern uint16_t* framebuffer[2];

/* ============================================ HAL ===================================================*/
bool em_hal_init() {
  return true;
}

/* ============================================ Input ===================================================*/
bool em_ctrl_is_pressed(int btn) {
#ifndef DEVBOARD  
  return ctrl_is_pressed(btn);
#else
  return usb_ctrl_is_pressed(btn);
#endif
}

bool em_usb_ctrl_is_pressed(int btn) {
#ifndef DEVBOARD  
  return usb_ctrl_is_pressed(btn);
#else
  return false;
#endif
}

void em_ctrl_button_state_update() {
#ifndef DEVBOARD
  ctrl_button_state_update();
#endif
}

/* ============================================ LCD ===================================================*/
int em_lcd_get_width() {
  return lcd_get_fb_width();
}

int em_lcd_get_height() {
  return lcd_get_fb_height();
}

void em_lcd_set_vsync(bool vsync) {
  lcd_set_vsync(vsync);
}

void em_get_backbuffer(gbuffer_t* fb_back) {
  if (lcd_get_fb() == framebuffer[0])
    gbuf_create(fb_back, lcd_get_fb_width(), lcd_get_fb_height(), framebuffer[1]);
  else
    gbuf_create(fb_back, lcd_get_fb_width(), lcd_get_fb_height(), framebuffer[0]);
}

void em_request_pageflip() {
  lcd_wait_vsync();

  if (lcd_get_fb() == framebuffer[0])
    lcd_set_fb((uint16_t *) framebuffer[1]);
  else
    lcd_set_fb((uint16_t *) framebuffer[0]);
}

void em_lcd_backlight_set(int value) {
  lcd_set_brightness(value);
}

void em_lcd_refresh() {
}

/* ============================================ PSRAM ===================================================*/

void* em_psram_malloc(uint32_t size) {
  return heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void em_psram_free(void* buffer) {
  free(buffer);
}

/* ============================================ filesystem ===================================================*/

bool em_get_directory(char *path, char* buffer) {
    if (!sd_is_init)
      return false;

    FRESULT res;
    FF_DIR dir;
    FILINFO fno;

    int item_cnt = 0;

    res = f_opendir(&dir, path);                   /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);           /* Read a directory item */
            if ((fno.fname[0] == 0) || (item_cnt >= DIRECTORY_MAX_ITEMS)) break;          /* Error or end of dir/RAM */
            if (!(fno.fattrib & AM_DIR)) {
#ifdef ALPHABETIZE
              // alphabetize  
              for (int i = 0; i < item_cnt; i++) {
                bool leave = false;                
                char* item_buf = get_item_aligned(i, buffer);
                int len = strlen(item_buf) > strlen(fno.fname) ? strlen(fno.fname) : strlen(item_buf);
                for (int j = 0; j < len; j++) {
                  // case "==" unnecessary since two file names cannot be the same

                  // skip item
                  if (symbol_rank(fno.fname[j]) > symbol_rank(item_buf[j]))
                    break;

                  // that's the spot
                  if (symbol_rank(fno.fname[j]) < symbol_rank(item_buf[j])) {
                    insert_item_aligned(i, fno.fname, buffer);
                    leave = true;
                    break;
                  }
                }
                if (leave)
                  break;
              }
#else
              add_item_aligned(fno.fname, buffer);
#endif
              item_cnt++;
            }
        }
        f_closedir(&dir);
    } else {
        printf("Error reading directory: \"%s\". (%u)\n", path, res);
        return false;
    }

    //buffer[buf_pos] = terminatorstring;
    return true;
}

uint32_t em_f_size() {
  if (!sd_is_init)
    return 0;

  return f_size(&fil);
}

bool em_f_mkdir(char* path) {
  if (!sd_is_init)
    return false;
  
  f_mkdir(path);

  return true;
}

bool em_f_open(const char* filename, uint8_t access_mode) {
  if (!sd_is_init)
    return false;

  FRESULT fr;
  uint8_t mode;

  if (access_mode == EM_FA_R)
    mode = FA_READ;

  if (access_mode == EM_FA_RWC)
    mode = FA_READ | FA_WRITE | FA_CREATE_ALWAYS;

  f_close(&fil);
  fr = f_open(&fil, (char*) filename, mode);

  if (fr == FR_OK)
    return true;
  else
    return false;

}

bool em_f_read(uint8_t* buf, int len, uint32_t* bytes_read) {
  if (!sd_is_init)
    return false;

  FRESULT fr;
  UINT br;
  fr = f_read(&fil, buf, len, (UINT*) &br);
  *bytes_read = br;

  if (fr == FR_OK)
    return true;
  else
    return false;
}

bool em_f_write(uint8_t* buf, int len, uint32_t* bytes_written) {
  if (!sd_is_init)
    return false;

  FRESULT fr;
  UINT bw;
  fr = f_write(&fil, buf, len, (UINT*) &bw);
  *bytes_written = bw;

  if (fr == FR_OK)
    return true;
  else
    return false;
}

bool em_f_close() {
  f_close(&fil);
  return true;
}

/* ------------------------------ SD card -----------------------------------*/
bool em_sd_init() {
  if (sd_is_init)
    return true;

  esp_err_t ret;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };

  host = SDMMC_HOST_DEFAULT();

  if (!ldo_init) {
    sd_pwr_ctrl_ldo_config_t ldo_config = {
      .ldo_chan_id = 4,
    };

    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        //ESP_LOGE(HAL_TAG, "Failed to create a new on-chip LDO power control driver");
        return false;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
    host.slot = SDMMC_HOST_SLOT_1;
    host.max_freq_khz = 40000;

    ldo_init = true;
    printf("ldo init complete.\n");

  }

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 4;
  slot_config.clk = PIN_SD_SCK;
  slot_config.cmd = PIN_SD_CMD_MOSI;
  slot_config.d0 = PIN_SD_DAT0_MISO;
  slot_config.d1 = PIN_SD_DAT1;
  slot_config.d2 = PIN_SD_DAT2;
  slot_config.d3 = PIN_SD_DAT3_CS;

  ret = esp_vfs_fat_sdmmc_mount(MOUNTPOINT, &host, &slot_config, &mount_config, &card);
  if (ret != ESP_OK)
    return false;

  sd_is_init = true;

  return true;

}

bool em_sd_deinit() {
  if (!sd_is_init)
    return true;
  
  esp_vfs_fat_sdcard_unmount(MOUNTPOINT, card);
  sd_is_init = false;
  return true;
}

/* ============================================ flash ===================================================*/
#if 0
static void call_flash_range_erase(void *param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

bool em_flash(void* address) {
  int rc = flash_safe_execute(call_flash_range_erase, address, UINT32_MAX /* max delay*/ );

  //hard_assert(rc == PICO_OK);

  if (rc == PICO_OK)
    return true;
  else
    return false;
}

int em_get_flash_sector_size() {
    return FLASH_SECTOR_SIZE;
}

uint32_t em_get_flash_base_addr() {
    return XIP_BASE;
}

void em_flush_cache() {
  flash_flush_cache();    
  xip_cache_clean_all();
}
#endif

/*
 * Snes9x running on the ESP32-P4-Function-EV-Board
 *
 * Copyright (C) 2023 Daniel Kammer (daniel.kammer@web.de)
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

/* ============================== INCLUDES ===================================*/
// debug
#include <errno.h>

// basics
#include <inttypes.h>
#include <cassert>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// platform
#include "esp_heap_caps.h"
#include "esp_log.h"

// HW support
#include "i2s_audio.h"
#include "usbhid.h"

#include "emumgr.h"
#include "emumgr_hal.h"

extern "C" {
#include "lcd_drv.h"
}

#include "timing.h"

#include "hwcfg.h"
#include "picoengine.h"

// SNES
extern "C" {
#include "snes9x/snes9x.h"
#include "snes9x/soundux.h"
#include "snes9x/memmap.h"
#include "snes9x/apu.h"
#include "snes9x/display.h"
#include "snes9x/cpuexec.h"
#include "snes9x/srtc.h"
#include "snes9x/save.h"
#include "snes9x/gfx.h"
}

//#include <fonts/fontfiles/f13x16.h>
#include <fonts/fontfiles/picoheld_pixel_14_numbers.h>
#include <fonts/fontfiles/picoheld_pixel_5x7_rosa.h>

#define OSD_FONT picoheld_pixel_14_numbers

/* ============================== EMU COMPILE TIME SWITCHES ===================================*/
// video
#define PREFER_VSYNC_OVER_FPS (0) 
#define ENABLE_FRAMEDROPPING
#define TARGET_FPS (52)
#define TARGET_FRAME_DURATION (1000000 / TARGET_FPS)  // add a little margin

// sound
#define AUDIO_SAMPLE_RATE (22040 * TARGET_FPS / 60) // keep same sampling rate constant at lower fps //32040
#define AUDIO_BUFFER_NUM_FRAMES (5)
#define AUDIO_BUFFER_LENGTH (AUDIO_SAMPLE_RATE / TARGET_FPS)

/* ============================== GLOBAL VARIABLES ===================================*/

static char *TAG = "SNES9x_ESP32-P4";

uint16_t *framebuffer[2];

typedef struct {
  // video
  bool  buffer_tainted = false;

  // sound
  int16_t* audio_buf[2];
  SemaphoreHandle_t make_sound;
  i2s_audio *audio_output;

  // snes9x
  uint8_t* SubScreen;

} emulator_control_t;

emulator_control_t emulator_control;
extern emulator_state_t emulator_state;

// snes stuff
bool overclock_cycles = false;
int one_c = 4, slow_one_c = 5, two_c = 6;
extern SGFX GFX;       // snes9x API

/* ------------------------------ SNES callbacks -----------------------------------*/
static bool reset_handler(bool hard) {
  S9xReset();
  return true;
}

bool S9xInitDisplay(void) {
  GFX.Pitch = SNES_WIDTH * 2;  // 16 bbp
  GFX.ZPitch = SNES_WIDTH;
  GFX.Screen = (uint8_t *) framebuffer[0];

  // seems crazy but works for Super Mario World et. al. and saves 112 KB of RAM
  emulator_control.SubScreen = (uint8_t*) heap_caps_malloc(GFX.Pitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_SPIRAM); // 112 KB
  GFX.SubScreen = GFX.Screen;
  GFX.ZBuffer = (uint8_t *) heap_caps_malloc(GFX.ZPitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_INTERNAL);   // 56 KB
  GFX.SubZBuffer = (uint8_t *) heap_caps_malloc(GFX.ZPitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_SPIRAM);  // 56 KB
  return GFX.Screen && GFX.SubScreen && GFX.ZBuffer && GFX.SubZBuffer;
}

void S9xDeinitDisplay(void) {
}

uint32_t S9xReadJoypad(int32_t port) {
  if (port > 1)
    return 0;

  uint32_t joypad = 0;

  if (port == 0) {
    if (em_ctrl_is_pressed(BUTTON_LEFT)) joypad |= SNES_LEFT_MASK;
    if (em_ctrl_is_pressed(BUTTON_RIGHT)) joypad |= SNES_RIGHT_MASK;
    if (em_ctrl_is_pressed(BUTTON_UP))   joypad |= SNES_UP_MASK;
    if (em_ctrl_is_pressed(BUTTON_DOWN)) joypad |= SNES_DOWN_MASK;

    if (em_ctrl_is_pressed(BUTTON_ST))   joypad |= SNES_START_MASK;
    if (em_ctrl_is_pressed(BUTTON_SEL))  joypad |= SNES_SELECT_MASK;

    if (em_ctrl_is_pressed(BUTTON_L))    joypad |= SNES_TL_MASK;
    if (em_ctrl_is_pressed(BUTTON_R))    joypad |= SNES_TR_MASK;

    if (em_ctrl_is_pressed(BUTTON_A))    joypad |= SNES_A_MASK;
    if (em_ctrl_is_pressed(BUTTON_B))    joypad |= SNES_B_MASK;
    if (em_ctrl_is_pressed(BUTTON_X))    joypad |= SNES_X_MASK;
    if (em_ctrl_is_pressed(BUTTON_Y))    joypad |= SNES_Y_MASK;
  } else if (port == 1) {
    if (em_usb_ctrl_is_pressed(BUTTON_LEFT)) joypad |= SNES_LEFT_MASK;
    if (em_usb_ctrl_is_pressed(BUTTON_RIGHT)) joypad |= SNES_RIGHT_MASK;
    if (em_usb_ctrl_is_pressed(BUTTON_UP))   joypad |= SNES_UP_MASK;
    if (em_usb_ctrl_is_pressed(BUTTON_DOWN)) joypad |= SNES_DOWN_MASK;

    if (em_usb_ctrl_is_pressed(BUTTON_ST))   joypad |= SNES_START_MASK;
    if (em_usb_ctrl_is_pressed(BUTTON_SEL))  joypad |= SNES_SELECT_MASK;

    if (em_usb_ctrl_is_pressed(BUTTON_L))    joypad |= SNES_TL_MASK;
    if (em_usb_ctrl_is_pressed(BUTTON_R))    joypad |= SNES_TR_MASK;

    if (em_usb_ctrl_is_pressed(BUTTON_A))    joypad |= SNES_A_MASK;
    if (em_usb_ctrl_is_pressed(BUTTON_B))    joypad |= SNES_B_MASK;
    if (em_usb_ctrl_is_pressed(BUTTON_X))    joypad |= SNES_X_MASK;
    if (em_usb_ctrl_is_pressed(BUTTON_Y))    joypad |= SNES_Y_MASK;
  }

  return joypad;
}

bool S9xReadMousePosition(int32_t which1, int32_t *x, int32_t *y, uint32_t *buttons) {
  return false;
}

bool S9xReadSuperScopePosition(int32_t *x, int32_t *y, uint32_t *buttons) {
  return false;
}

bool JustifierOffscreen(void) {
  return true;
}

void JustifierButtons(uint32_t *justifiers) {
  (void)justifiers;
}

static void update_keymap(int id) {
}

static bool screenshot_handler(const char *filename, int width, int height) {
  //return rg_display_save_frame(filename, currentUpdate, width, height);
  return true;
}

void emu_panic(char* str)
{
  for (int i = 0; i < 10; i++) {
    printf(str);
    //sleep_ms(1000);
  }
  pwr_shutdown();

}

/* ------------------------- SETUP ------------------------------*/
void snes_init() {
  Settings.CyclesPercentage = 100;
  Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
  Settings.FrameTimePAL = 20000;
  Settings.FrameTimeNTSC = 16667;
  Settings.ControllerOption = SNES_JOYPAD;
  Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;
  Settings.SoundPlaybackRate = AUDIO_SAMPLE_RATE;
  Settings.DisableSoundEcho = false;
  Settings.InterpolatedSound = true;
  Settings.ForcePAL = true;  
#ifdef USE_BLARGG_APU
  Settings.SoundInputRate = AUDIO_SAMPLE_RATE;
#endif

  if (!S9xInitMemory())
    emu_panic("Memory init failed!");

  if (!S9xInitAPU())
    emu_panic("APU init failed!");

  if (!S9xInitSound(0, 0))
    emu_panic("Sound init failed!");

  if (!S9xInitGFX())
    emu_panic("Graphics init failed!");

#ifdef USE_BLARGG_APU
    //S9xSetSamplesAvailableCallback(S9xAudioCallback);
#else
  S9xSetPlaybackRate(Settings.SoundPlaybackRate);
#endif
}

void audio_init() {
  emulator_control.make_sound = xSemaphoreCreateBinary();
  assert(emulator_control.make_sound);
  xSemaphoreGive(emulator_control.make_sound);
  xSemaphoreTake(emulator_control.make_sound, 0);

  for (int i = 0; i < 2; i++) {
    emulator_control.audio_buf[i] = (int16_t *) heap_caps_calloc(1, AUDIO_BUFFER_LENGTH * AUDIO_BUFFER_NUM_FRAMES * 2 * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(emulator_control.audio_buf[i]);
  }

  // needs to be called prior to anything else, because "i2s_driver_install" sets up some
  // "default" GPIOs, potentially overwriting the GPIO config.
  #ifndef DEVBOARD
  emulator_control.audio_output = new i2s_audio();
  (emulator_control.audio_output)->start(PIN_I2S_BCK, PIN_I2S_WS, PIN_I2S_DOUT, AUDIO_SAMPLE_RATE, emulator_control.audio_buf[0], AUDIO_BUFFER_LENGTH * AUDIO_BUFFER_NUM_FRAMES * 2);
  #endif

}

void gfx_init() {
  for (int i = 0; i < 2; i++) {
    framebuffer[i] = (uint16_t *) heap_caps_calloc(1, SNES_WIDTH * SNES_HEIGHT_EXTENDED * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(framebuffer[i]);
 
    uint16_t* fb = framebuffer[i];

    // clear buffer
    for (int i = 0; i < SNES_WIDTH * SNES_HEIGHT_EXTENDED; i++)
      fb[i] = 0;
  }

  if (!S9xInitDisplay())
    emu_panic("Display init failed!");

  lcd_set_fb((uint16_t *)framebuffer[1]);

}

void setup(void) {
  ESP_LOGI(TAG, "entering setup");

  pwr_init();

  audio_init();  // call before the rest

  timing_init();

  gfx_init();

  ctrl_init();

  lcd_set_fb((uint16_t *)framebuffer[0]);
  lcd_config_t lcd_config = {
    .buffer_width = SNES_WIDTH,
    .buffer_height = SNES_HEIGHT_EXTENDED,
    .scale_factor = 3,
    .target_fps = TARGET_FPS,
    .prefer_vsync_over_fps = PREFER_VSYNC_OVER_FPS,
  };

  ESP_LOGE(TAG, "LCD init");
  lcd_init(lcd_config);

  init_hid();

  snes_init();

}

void clear_screen_bufs(bool force) {
  if (!emulator_control.buffer_tainted && !force)
    return;

  
  for (int i = 0; i < SNES_WIDTH * 8; i++) {
    framebuffer[0][i] = 0;
    framebuffer[1][i] = 0;
  }
  for (int i = SNES_WIDTH * 224; i < SNES_WIDTH * SNES_HEIGHT_EXTENDED; i++) {
    framebuffer[0][i] = 0;
    framebuffer[1][i] = 0;
  }

  emulator_control.buffer_tainted = false;
}

/* ------------------------- EMULATION ------------------------------*/
void emulation_loop(void *parameter) {

  // enable sound
  pwr_amp(1);

  bool menuCancelled = false;
  bool menuPressed = false;

  int frame_no = 0;
  int fps = 0;
  unsigned long fps_timer = millis();

  gbuffer_t fb_back;
  em_get_backbuffer(&fb_back);
  emumgr_menu(fb_back);
  clear_screen_bufs(true);

#ifdef ENABLE_FRAMEDROPPING
  int framedrop_occurred = 0;
  int32_t framedrop_balance = 0;
  uint32_t framedrop_timer = micros();
#endif

  while (1) {
    em_ctrl_button_state_update();

#ifndef DEVBOARD
    if (em_ctrl_is_pressed(BUTTON_SW)) {
#else
    if (em_ctrl_is_pressed(BUTTON_L) && em_ctrl_is_pressed(BUTTON_R)) {
#endif
      em_get_backbuffer(&fb_back);
      emumgr_menu(fb_back);
      if (PPU.ScreenHeight != SNES_HEIGHT_EXTENDED)
        clear_screen_bufs(true);
      IPPU.RenderThisFrame = false;
#ifdef ENABLE_FRAMEDROPPING
      framedrop_balance = TARGET_FRAME_DURATION;
      framedrop_timer = micros();
#endif
      frame_no = 0;
      fps_timer = millis();
    } 
    
    S9xMainLoop();

    xSemaphoreGive(emulator_control.make_sound);

    /* ---------- LCD ------------*/
    if (IPPU.RenderThisFrame) {
       em_get_backbuffer(&fb_back);
       if (emulator_state.show_fps) {
        char debug_print[20];
        itoa(fps, debug_print, 10);
        int r = (511 * (TARGET_FPS - fps)) / TARGET_FPS;
        int g = 511 - r;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        font_write_string(15, 12, 0x0, debug_print, (font_t*) OSD_FONT, fb_back);
        font_write_string(13, 10, rgb_col_888_565(r, g, 0), debug_print, (font_t*) OSD_FONT, fb_back);
      }

      draw_battery(219, 10, pwr_get_bat_level(4), fb_back);

      lcd_wait_vsync();

      if (lcd_get_fb() == (void *)framebuffer[1]) {
        if (PPU.ScreenHeight == SNES_HEIGHT_EXTENDED) {
          GFX.Screen = (uint8_t *)framebuffer[1];
          emulator_control.buffer_tainted = true;
        } else {
          clear_screen_bufs(false);
          GFX.Screen = (uint8_t *) &framebuffer[1][SNES_WIDTH * 8];
        }
          
        lcd_set_fb((uint16_t *)framebuffer[0]);
      } else {
        if (PPU.ScreenHeight == SNES_HEIGHT_EXTENDED) {
          GFX.Screen = (uint8_t *)framebuffer[0];
          emulator_control.buffer_tainted = true;
        } else {
          clear_screen_bufs(false);
          GFX.Screen = (uint8_t *) &framebuffer[0][SNES_WIDTH * 8];
        }

        lcd_set_fb((uint16_t *)framebuffer[1]);
      }

      GFX.SubScreen = emulator_state.psram_subscreen ? emulator_control.SubScreen : GFX.Screen;
      frame_no++;
    }

    /* ---------- LCD END------------*/

    if (millis() - fps_timer > 1000) {
      fps = (int) (frame_no * 1000 / (millis() - fps_timer));
      ESP_LOGI(TAG, "fps: %d", fps);
      frame_no = 0;
      fps_timer = millis();
#ifdef ENABLE_FRAMEDROPPING
      if (framedrop_occurred) {
        ESP_LOGI(TAG, "Frame drop occurred: %d frame(s) dropped.", framedrop_occurred);
        framedrop_occurred = 0;
      }
#endif
    }

    IPPU.RenderThisFrame = true;
#ifdef ENABLE_FRAMEDROPPING
    framedrop_balance += (micros() - framedrop_timer) - TARGET_FRAME_DURATION;
    framedrop_timer = micros();

    // Are we too slow
    if (framedrop_balance > TARGET_FRAME_DURATION / 2) // fcipaq: TODO: what amount is best?
      IPPU.RenderThisFrame = false;
    
    if (framedrop_balance < 200)  // margin
      framedrop_balance = 0;
#endif

  }  
}

void audio_loop() {

  int cur_audio_buf = 0;

  uint32_t audio_timer = millis();
  int audio_frame_cnt = 0;
  int audio_frame_no = 0;

  while(1) {
    while (xSemaphoreTake(emulator_control.make_sound, portMAX_DELAY) != pdTRUE)
      ;
    
    if (emulator_state.volume != 0)
      S9xMixSamples(&(emulator_control.audio_buf[cur_audio_buf][AUDIO_BUFFER_LENGTH * 2 * audio_frame_no]), AUDIO_BUFFER_LENGTH * 2);

    int16_t* tmp_audio_buf = &(emulator_control.audio_buf[cur_audio_buf][AUDIO_BUFFER_LENGTH * 2 * audio_frame_no]);

    if (emulator_state.volume == 0) {
      for (int i = 0; i < AUDIO_BUFFER_LENGTH * 2; i++) {
        *tmp_audio_buf = 0;
        tmp_audio_buf++;
      }
    } else {
      for (int i = 0; i < AUDIO_BUFFER_LENGTH * 2; i++) {
        *tmp_audio_buf /= 11 - emulator_state.volume;
        tmp_audio_buf++;
      }
    }

    audio_frame_no++;
    if (audio_frame_no == AUDIO_BUFFER_NUM_FRAMES) {
#ifndef DEVBOARD
      (emulator_control.audio_output)->set_next_buffer(emulator_control.audio_buf[cur_audio_buf]);
#endif
      cur_audio_buf++;
      cur_audio_buf = cur_audio_buf % 2;
      audio_frame_no = 0;
    }

    audio_frame_cnt++;

    if (millis() - audio_timer > 1000) {
      if (emulator_state.volume != 0)
        ESP_LOGI(TAG, "audio fps: %d", (int)(audio_frame_cnt * 1000 / (millis() - audio_timer)));
      audio_frame_cnt = 0;
      audio_timer = millis();
    }
  }  
}

/* ------------------------- MAIN ------------------------------*/

// freeRTOS calls app_main from C
extern "C" {
  extern void app_main();
}

void app_main(void) {
  
  setup();

  xTaskCreatePinnedToCore(
    emulation_loop,   /* Function to implement the task */
    "emulation_loop", /* Name of the task */
    4096,          /* Stack size in words */
    NULL,          /* Task input parameter */
    18,             /* Priority of the task */
    NULL,          /* Task handle. */
    1);            /* Core where the task should run */

  audio_loop();
}
#pragma once

typedef struct {
  int buffer_width;
  int buffer_height;
  int scale_factor;
  float target_fps;
  int prefer_vsync_over_fps;
} lcd_config_t;

void lcd_wait_vsync();
void lcd_set_vsync(bool vsync);
bool lcd_set_brightness(int level);
void lcd_set_fb(uint16_t* fb_back);
uint16_t* lcd_get_fb();
void lcd_set_fb_width(int width);
void lcd_set_fb_height(int height);
uint16_t lcd_get_fb_width();
uint16_t lcd_get_fb_height();
bool lcd_set_brightness(int level);
void lcd_deinit(void);
void lcd_init(lcd_config_t lcd_config);
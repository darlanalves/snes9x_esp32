#include "control.h"
#include "hal/adc_types.h"
#include "hwcfg.h"
#include <driver/gpio.h>
#include "adc.h"
#include "timing.h"

#define BUTTON_PRESSED_WHEN_LO (0)
#define BUTTON_PRESSED_WHEN_HI (1)
#define BUTTON_DIGITAL (0)
#define BUTTON_ANALOG (1)

typedef struct
{
  int              pin_id;
  gpio_num_t       gpio_num;
  int              key_state;
  gpio_pull_mode_t pull_mode;
  int              pressed_state;
  bool             signal_type;
  int              analog_threshold;
  uint32_t         key_timer;
} keymap_t;

keymap_t keymap[NUM_BTNS] = BUTTON_MAP;

void ctrl_init() {
  adc_init();

  for (int i = 0; i < NUM_BTNS; i++) {
    if (keymap[i].gpio_num != GPIO_NUM_NC) {
      if (keymap[i].signal_type == BUTTON_DIGITAL) {
        gpio_set_direction(keymap[i].gpio_num, GPIO_MODE_INPUT);
        gpio_set_pull_mode(keymap[i].gpio_num, keymap[i].pull_mode);
      } else {
        adc_setup_gpio(keymap[i].gpio_num, ADC_ATTEN_DB_12);
      }
    }
  }
}

bool ctrl_is_pressed(int btn) {
  if (btn > NUM_BTNS + 1 /* ANY */)
    return false;

  if (btn == BUTTON_ANY) {
    for (int i = 0; i < NUM_BTNS; i++)
      if (keymap[i].key_state == 1)
        return true;
    
    return false;
  } else {
    return keymap[btn].key_state == 1;
  }
}

uint32_t ctrl_button_state_update() {
  uint32_t res = 0;

  for (int i = 0; i < NUM_BTNS; i++) {

    if (keymap[i].gpio_num != GPIO_NUM_NC) {
      bool result = false;

      if (keymap[i].signal_type == BUTTON_DIGITAL)
        result = (gpio_get_level(keymap[i].gpio_num) == keymap[i].pressed_state);
      else
        result = adc_digital_read(keymap[i].gpio_num, keymap[i].analog_threshold);

      if (result) {
        if (keymap[i].key_state == 0) {
          keymap[i].key_state = 1;
          keymap[i].key_timer = millis();
        }
        res |= 1u << i;
      } else {
        keymap[i].key_state = 0;
      }
    }
  }

  return res;
}

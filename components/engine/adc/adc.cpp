#include "hal/adc_types.h"
#include "hal/gpio_types.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

static adc_oneshot_unit_handle_t adc_handle[2] = { NULL, NULL };
bool adc_init_complete = 0;

bool adc_setup_gpio(int gpio_num, adc_atten_t atten) {

  adc_unit_t unit;
  adc_channel_t channel;

  adc_oneshot_io_to_channel(gpio_num, &unit, &channel);

  adc_oneshot_chan_cfg_t config = {
    .atten    = atten,
    .bitwidth = ADC_BITWIDTH_DEFAULT,
  };

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle[unit == ADC_UNIT_1 ? 0 : 1], channel, &config));

  return true;

}

bool adc_digital_read(int gpio, int threshold) {
  if (gpio == GPIO_NUM_NC)
    return false;
  
  adc_unit_t unit;
  adc_channel_t channel;
  int raw;

  adc_oneshot_io_to_channel(gpio, &unit, &channel);

  adc_oneshot_read(adc_handle[unit == ADC_UNIT_1 ? 0 : 1], channel, &raw);

  return (raw > threshold) ? true : false;
}

int adc_analog_read(int gpio) {
  if (gpio == GPIO_NUM_NC)
    return 0;

  adc_unit_t unit;
  adc_channel_t channel;
  int raw;

  adc_oneshot_io_to_channel(gpio, &unit, &channel);

  adc_oneshot_read(adc_handle[unit == ADC_UNIT_1 ? 0 : 1], channel, &raw);

  return raw;
}

bool adc_init() {
  
  if (adc_init_complete)
    return true; 

  adc_oneshot_unit_init_cfg_t unit_cfg = {
    .unit_id = ADC_UNIT_1,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
  };

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle[0]));

  unit_cfg = {
    .unit_id = ADC_UNIT_2,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
  };

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle[1]));

  adc_init_complete = true;

  return true;
}

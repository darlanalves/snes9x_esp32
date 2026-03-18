#include "hal/adc_types.h"
#include "hal/gpio_types.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_chip_info.h" 

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

adc_cali_handle_t adc_cali_handle[2][8]; // 2 units, each 8 channels max.
#endif

static adc_oneshot_unit_handle_t adc_handle[2] = { NULL, NULL };

bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  printf("calibration scheme version is %s", "Curve Fitting");
  adc_cali_curve_fitting_config_t cali_config = {
      .unit_id = unit,
      .chan = channel,
      .atten = atten,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
  if (ret == ESP_OK)
      calibrated = true;

  *out_handle = handle;
  if (ret == ESP_OK) {
      printf("Calibration Success\n");
  } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
      printf("eFuse not burnt, skip software calibration\n");
  } else {
      printf("Invalid arg or no memory\n");
  }
#else
  printf("ADC calibration unsupported on selected ESP32-P4 chip revision\n");
#endif

  return calibrated;
}

bool adc_setup_gpio(int gpio_num, adc_atten_t atten) {

  adc_unit_t unit;
  adc_channel_t channel;

  adc_oneshot_io_to_channel(gpio_num, &unit, &channel);

  adc_oneshot_chan_cfg_t config = {
    .atten    = atten,
    .bitwidth = ADC_BITWIDTH_DEFAULT,
  };

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle[unit], channel, &config));

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  adc_calibration_init(unit, channel, atten, &adc_cali_handle[unit][channel]);
#endif

  return true;

}

bool adc_digital_read(int gpio, int threshold) {
  if (gpio == GPIO_NUM_NC)
    return false;
  
  adc_unit_t unit;
  adc_channel_t channel;
  int raw;

  adc_oneshot_io_to_channel(gpio, &unit, &channel);

  adc_oneshot_read(adc_handle[unit], channel, &raw);

  return (raw > threshold) ? true : false;
}

int adc_analog_read(int gpio) {
  if (gpio == GPIO_NUM_NC)
    return 0;

  adc_unit_t unit;
  adc_channel_t channel;
  int raw;

  adc_oneshot_io_to_channel(gpio, &unit, &channel);

  adc_oneshot_read(adc_handle[unit], channel, &raw);

  int voltage = raw;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  adc_cali_raw_to_voltage(adc_cali_handle[unit][channel], raw, &voltage);
#endif

  return voltage;
}

bool adc_init() {
  
  static bool adc_init_complete = false;

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

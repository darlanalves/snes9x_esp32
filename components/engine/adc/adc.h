#pragma once

bool adc_init();
bool adc_setup_gpio(int gpio_num, adc_atten_t atten);
bool adc_digital_read(int gpio, int threshold);
int adc_analog_read(int gpio);

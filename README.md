# This is Snes9x running on Pico Held 2 ESP32-P4 edition

(ESP32-P4-Function-EV-Board is also supported)

This is based on [this](https://github.com/ducalex/retro-go/) repo.

Features:

- Snes9x emualtion
- load/save state support with thumbnails
- brightness/volume adjustment
- battery status indicator
- some settings available
- Limited USB-HID controller support

Building:

- Pick your hardware platform (in components/engine/hwcfg.h)
- Just build and flash with ESP-IDF (idf.py flash)

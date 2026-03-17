# todo: split esp32 part in seperate CMakeLists.txt file
idf_component_register(SRCS ./emumgr.cpp ./emumgr_hal.cpp ./heapstrings.cpp
                       INCLUDE_DIRS ./.. ../../assets ../engine  ../engine/utils  ../engine/graphics  ../engine/control  ../engine/power  ../lcd
                       REQUIRES main esp_event esp_partition esp_adc nvs_flash fatfs spiffs driver # anything except for main is only reqired for esp32
                       PRIV_REQUIRES )

# only required for esp32
set (CMAKE_CXX_FLAGS "-march=rv32imafc -mabi=ilp32f -Wno-misleading-indentation -Wno-narrowing -Wno-format -Wno-maybe-uninitialized")
set (CMAKE_C_FLAGS "-march=rv32imafc -mabi=ilp32f -Wno-format -Wno-unused-const-variable -Wno-incompatible-pointer-types -Wno-maybe-uninitialized")

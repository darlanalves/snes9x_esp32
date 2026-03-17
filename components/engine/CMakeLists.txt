idf_component_register(SRCS
                         ./control/control.cpp
                         ./utils/timing.cpp
                         ./fonts/fonts.cpp
                         ./graphics/blitter.cpp
                         ./graphics/colors.cpp
                         ./graphics/gbuffers.cpp
                         ./graphics/primitives.cpp
                         ./power/power.cpp
                         ./adc/adc.cpp
                       INCLUDE_DIRS
                         .
                         ./control
                         ./fonts
                         ./graphics
                         ./utils
                         ./power
                         ./adc
                       REQUIRES 
                         driver esp_adc
                       PRIV_REQUIRES )

set (CMAKE_CXX_FLAGS "-march=rv32imafc -mabi=ilp32f") #"-Wno-misleading-indentation -Wno-narrowing -Wno-format -mlongcalls -fpermissive")
set (CMAKE_C_FLAGS "-march=rv32imafc -mabi=ilp32f -Wno-format -Wno-unused-const-variable")

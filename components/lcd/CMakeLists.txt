idf_component_register(SRCS ./lcd_panel_dpi.c ./lcd_drv.c
                       INCLUDE_DIRS ./..
                       REQUIRES esp_lcd esp_mm
                       PRIV_REQUIRES )

set (CMAKE_CXX_FLAGS "-march=rv32imafc -mabi=ilp32f") #"-Wno-misleading-indentation -Wno-narrowing -Wno-format -mlongcalls -fpermissive")
set (CMAKE_C_FLAGS "-march=rv32imafc -mabi=ilp32f -Wno-format -Wno-unused-const-variable")

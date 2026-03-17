idf_component_register(SRCS ./main.cpp
                       INCLUDE_DIRS ./.. ./../components/lcd ./../components/snes9x ./../components/i2s_audio ./../components/usbhid ./../components/engine ./../components/emumgr
                       REQUIRES 
                       PRIV_REQUIRES engine fatfs esp_timer driver i2s_audio usbhid )

target_compile_options(${COMPONENT_LIB} PRIVATE -DCOMPILEDATE="$(COMPILEDATE)" -DGITREV="$(GITREV)")

#set (CMAKE_CXX_FLAGS "")
#set (CMAKE_C_FLAGS "-mlongcalls")

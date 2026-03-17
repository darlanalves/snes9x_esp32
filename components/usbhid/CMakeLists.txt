idf_component_register(SRCS "usbhid.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES usb espressif__usb_host_hid esp_driver_gpio engine
                       )

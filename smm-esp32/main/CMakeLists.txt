idf_component_register(
    SRCS 
        "smm.c"
        "initsys.c"
        "textmenu.c"
        "menu/menu.c"
        "menu/main/main.c"
        "menu/main/wifi.c"
        "menu/main/sampler.c"
        "wifimgr/menu.c"
        "wifimgr/wifimgr.c"
        "webserver/webserver.c"
        "webserver/endpoint_ws.c"
        "webserver/endpoint_serve_file.c"
        "webserver/endpoint_sampler_channels.c"
        "webserver/endpoint_sampler_ctl.c"
        "webserver/endpoint_calibration.c"
        "sampler/sampler.c"
        "sampler/sampler_ringbuf.c"
        "calibration/calibration.c"
    INCLUDE_DIRS "."
) #src files
spiffs_create_partition_image(storage ../../web-frontend/dist/smm-frontend FLASH_IN_PROJECT) #spi partition
set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS "-Wno-comment -Wno-stringop-overread -Wno-stringop-overflow -Wno-format -Wno-unused-const-variable") #disable some annoying compilation flags

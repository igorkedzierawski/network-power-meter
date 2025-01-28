#include "calibration.h"

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

esp_err_t smm_calibration_get(calibration_registers_t *registers) {
    esp_err_t err;
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
    } else {

        union {
            uint32_t u32;
            float f32;
        } converter;

        converter.f32 = 1;
        err = nvs_get_u32(my_handle, "Va", &converter.u32);
        registers->voltage.multiplier = converter.f32;
        converter.f32 = 0;
        err = nvs_get_u32(my_handle, "Vb", &converter.u32);
        registers->voltage.offset = converter.f32;

        converter.f32 = 1;
        err = nvs_get_u32(my_handle, "I1a", &converter.u32);
        registers->current1.multiplier = converter.f32;
        converter.f32 = 0;
        err = nvs_get_u32(my_handle, "I1b", &converter.u32);
        registers->current1.offset = converter.f32;

        converter.f32 = 1;
        err = nvs_get_u32(my_handle, "I2a", &converter.u32);
        registers->current2.multiplier = converter.f32;
        converter.f32 = 0;
        err = nvs_get_u32(my_handle, "I2b", &converter.u32);
        registers->current2.offset = converter.f32;

        converter.f32 = 50;
        err = nvs_get_u32(my_handle, "H", &converter.u32);
        registers->firstHarmonicFrequency = converter.f32;

        nvs_close(my_handle);
    }
    return ESP_OK;
}

esp_err_t smm_calibration_set(calibration_registers_t *registers) {
    esp_err_t err;
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
    } else {

        union {
            uint32_t u32;
            float f32;
        } converter;

        converter.f32 = registers->voltage.multiplier;
        err = nvs_set_u32(my_handle, "Va", converter.u32);
        converter.f32 = registers->voltage.offset;
        err = nvs_set_u32(my_handle, "Vb", converter.u32);

        converter.f32 = registers->current1.multiplier;
        err = nvs_set_u32(my_handle, "I1a", converter.u32);
        converter.f32 = registers->current1.offset;
        err = nvs_set_u32(my_handle, "I1b", converter.u32);

        converter.f32 = registers->current2.multiplier;
        err = nvs_set_u32(my_handle, "I2a", converter.u32);
        converter.f32 = registers->current2.offset;
        err = nvs_set_u32(my_handle, "I2b", converter.u32);

        converter.f32 = registers->firstHarmonicFrequency;
        err = nvs_set_u32(my_handle, "H", converter.u32);

        nvs_close(my_handle);
    }
    return ESP_OK;
}

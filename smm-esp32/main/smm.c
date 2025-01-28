#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_check.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "lcd.h"
#include "mcp3208.h"
#include "initsys.h"
#include "akdmenu.h"
#include "sampler/sampler.h"

#include "iot_button.h"
#include "textmenu.h"
#include "menu/menu.h"
#include "wifimgr/wifimgr.h"
#include "esp_mac.h"
#include "nvs_flash.h"

#include "esp_spiffs.h"
#include "calibration/calibration.h"

static const char *TAG = "SMM";

lcd_handle_t lcd_handle;
spi_device_handle_t mcp_handle;

void render_line(uint8_t line) {
    akdmenu_host_t *hostptr = menu_get_host();
    char buf[17];
    for (uint8_t i = 0; i < hostptr->rows; i++) {
        uint8_t buf_index = hostptr->line_dbuf_origin + i;
        akdmenu_item_t *item = hostptr->line_dbuf[buf_index];
        bool selected = (buf_index == hostptr->rows), non_null = (item != NULL);
        const char *format = non_null && selected ? "*%-16s" : " %-16s";
        const char *text = non_null ? item->meta.text : "??";
        snprintf(buf, sizeof(buf), format, text);
        lcd_set_cursor(&lcd_handle, 0, i);
        lcd_write_str(&lcd_handle, buf);
    }
}

void init_all(void) {
    ESP_ERROR_CHECK(smm_initsys_init_hspi());
    ESP_ERROR_CHECK(smm_initsys_init_i2c_num_0());

    ESP_ERROR_CHECK(smm_initsys_init_netif());
    ESP_ERROR_CHECK(smm_initsys_init_default_event_loop());
    ESP_ERROR_CHECK(smm_initsys_init_nvs());
    ESP_ERROR_CHECK(smm_initsys_init_wifi());

    ESP_ERROR_CHECK(smm_initsys_init_adc(&mcp_handle));
    ESP_ERROR_CHECK(smm_initsys_init_lcd(&lcd_handle));

    ESP_ERROR_CHECK(smm_initsys_init_spiffs());

    ESP_ERROR_CHECK(smm_initsys_init_sampler(&mcp_handle));

    ESP_ERROR_CHECK(smm_initsys_init_webserver());

    ESP_ERROR_CHECK(smm_initsys_init_menu(render_line));
    textmenu_init();
    ESP_ERROR_CHECK(smm_initsys_init_buttons());
}

void app_main(void) {
    init_all();
    if (!smm_initsys_check_all_initialized()) {
        ESP_LOGE(TAG, "not every subsystem was initialized");
        return;
    }

    lcd_clear_screen(&lcd_handle);
    lcd_no_cursor(&lcd_handle);
    lcd_backlight(&lcd_handle);

    ESP_LOGI("TAG", "DONE[%d] => %d", i++, (int)wifimgr_set_credentials(WIFI_IF_AP, "MiernikMocy", "12345678"));

    ESP_ERROR_CHECK(wifimgr_set_if_enabled(WIFI_IF_AP, true));

    char ipbuffer[46] = {0};
    wifimgr_get_ip(WIFI_IF_STA, ipbuffer, 45);

    ESP_ERROR_CHECK_WITHOUT_ABORT(sampler_select_channels(3));
}

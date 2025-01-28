#ifndef _SMM_INITSYS_H_
#define _SMM_INITSYS_H_

#include "driver/spi_master.h"
#include "lcd.h"
#include "akdmenu.h"

esp_err_t smm_initsys_init_default_event_loop(void);

esp_err_t smm_initsys_init_netif(void);

esp_err_t smm_initsys_init_nvs(void);

esp_err_t smm_initsys_init_wifi(void);

esp_err_t smm_initsys_init_hspi(void);

esp_err_t smm_initsys_init_adc(spi_device_handle_t *handle);

esp_err_t smm_initsys_init_i2c_num_0(void);

esp_err_t smm_initsys_init_lcd(lcd_handle_t *handle);

esp_err_t smm_initsys_init_buttons(void);

esp_err_t smm_initsys_init_menu(akdmenu_redraw_handler_v redraw_handler);

esp_err_t smm_initsys_init_spiffs(void);

esp_err_t smm_initsys_init_sampler(spi_device_handle_t *mcp_handle);

esp_err_t smm_initsys_init_webserver(void);

bool smm_initsys_check_all_initialized(void);

#endif

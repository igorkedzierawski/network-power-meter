#include "initsys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "config.h"

#include "lcd.h"
#include "mcp3208.h"
#include "wifimgr/wifimgr.h"
#include "akdmenu.h"
#include "iot_button.h"
#include "menu/menu.h"
#include "webserver/webserver.h"

#include "esp_mac.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_check.h"
#include "sampler/sampler.h"

static const char *TAG = "INITSYS";

#define SMM_INITSYS_PREAMBULE(subsystem)                                                              \
    do {                                                                                              \
        if ((initialized & (1 << SMM_INITSYS_SS_##subsystem)) == (1 << SMM_INITSYS_SS_##subsystem)) { \
            ESP_LOGW(TAG, "Subsystem " #subsystem " is already initialized.");                        \
            return ESP_OK;                                                                            \
        } else if ((initialized & SMM_INITSYS_PR_##subsystem) != SMM_INITSYS_PR_##subsystem) {        \
            ESP_LOGE(TAG, "Subsystem " #subsystem " has uninitialized prerequisites.");               \
            return ESP_FAIL;                                                                          \
        }                                                                                             \
        ESP_LOGI(TAG, "Initializing " #subsystem " subsystem.");                                      \
    } while (0)

#define SMM_INITSYS_MARK_INITIALIZED(subsystem) \
    (initialized |= (1 << SMM_INITSYS_SS_##subsystem))

#define SMM_INITSYS_M1(a) (1 << (SMM_INITSYS_SS_##a))
#define SMM_INITSYS_M2(a, b) (SMM_INITSYS_M1(a) | SMM_INITSYS_M1(b))
#define SMM_INITSYS_M3(a, b, c) (SMM_INITSYS_M2(a, b) | SMM_INITSYS_M1(c))
#define SMM_INITSYS_M4(a, b, c, d) (SMM_INITSYS_M3(a, b, c) | SMM_INITSYS_M1(d))
#define SMM_INITSYS_M5(a, b, c, d, e) (SMM_INITSYS_M4(a, b, c, d) | SMM_INITSYS_M1(e))
#define SMM_INITSYS_M6(a, b, c, d, e, f) (SMM_INITSYS_M5(a, b, c, d, e) | SMM_INITSYS_M1(f))
#define SMM_INITSYS_M7(a, b, c, d, e, f, g) (SMM_INITSYS_M6(a, b, c, d, e, f) | SMM_INITSYS_M1(g))
#define SMM_INITSYS_M8(a, b, c, d, e, f, g, h) (SMM_INITSYS_M7(a, b, c, d, e, f, g) | SMM_INITSYS_M1(h))
#define SMM_INITSYS_M9(a, b, c, d, e, f, g, h, i) (SMM_INITSYS_M8(a, b, c, d, e, f, g, h) | SMM_INITSYS_M1(i))
#define SMM_INITSYS_M10(a, b, c, d, e, f, g, h, i, j) (SMM_INITSYS_M9(a, b, c, d, e, f, g, h, i) | SMM_INITSYS_M1(j))

#define SMM_INITSYS_M_GET(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME

#define SMM_INITSYS_PREREQ_MASK(...) SMM_INITSYS_M_GET(__VA_ARGS__, SMM_INITSYS_M10, SMM_INITSYS_M9, SMM_INITSYS_M8, SMM_INITSYS_M7, SMM_INITSYS_M6, SMM_INITSYS_M5, SMM_INITSYS_M4, SMM_INITSYS_M3, SMM_INITSYS_M2, SMM_INITSYS_M1)(__VA_ARGS__)

typedef enum {
    SMM_INITSYS_SS_WIFI = 0,
    SMM_INITSYS_SS_DEFAULT_EVENT_LOOP,
    SMM_INITSYS_SS_NETIF,
    SMM_INITSYS_SS_NVS,
    SMM_INITSYS_SS_HSPI,
    SMM_INITSYS_SS_I2C_NUM_0,
    SMM_INITSYS_SS_ADC,
    SMM_INITSYS_SS_LCD,
    SMM_INITSYS_SS_BUTTONS,
    SMM_INITSYS_SS_MENU,
    SMM_INITSYS_SS_SPIFFS,
    SMM_INITSYS_SS_SAMPLER,
    SMM_INITSYS_SS_WEBSERVER,
    SMM_INITSYS_SS_END_OF_SUBSYSTEMS
} smm_initsys_subsys;

typedef enum {
    SMM_INITSYS_PR_WIFI = SMM_INITSYS_PREREQ_MASK(DEFAULT_EVENT_LOOP, NETIF, NVS),
    SMM_INITSYS_PR_DEFAULT_EVENT_LOOP = 0,
    SMM_INITSYS_PR_NETIF = 0,
    SMM_INITSYS_PR_NVS = 0,
    SMM_INITSYS_PR_HSPI = 0,
    SMM_INITSYS_PR_I2C_NUM_0 = 0,
    SMM_INITSYS_PR_ADC = SMM_INITSYS_PREREQ_MASK(HSPI),
    SMM_INITSYS_PR_LCD = SMM_INITSYS_PREREQ_MASK(I2C_NUM_0),
    SMM_INITSYS_PR_BUTTONS = 0,
    SMM_INITSYS_PR_MENU = 0,
    SMM_INITSYS_PR_SPIFFS = 0,
    SMM_INITSYS_PR_SAMPLER = SMM_INITSYS_PREREQ_MASK(ADC),
    SMM_INITSYS_PR_WEBSERVER = 0,
} smm_initsys_prereq;

static uint32_t initialized = 0;

esp_err_t smm_initsys_init_default_event_loop(void) {
    SMM_INITSYS_PREAMBULE(DEFAULT_EVENT_LOOP);

    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "failed");

    SMM_INITSYS_MARK_INITIALIZED(DEFAULT_EVENT_LOOP);
    return ESP_OK;
}

esp_err_t smm_initsys_init_netif(void) {
    SMM_INITSYS_PREAMBULE(NETIF);

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "failed");

    SMM_INITSYS_MARK_INITIALIZED(NETIF);
    return ESP_OK;
}

esp_err_t smm_initsys_init_nvs(void) {
    SMM_INITSYS_PREAMBULE(NVS);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "failed");
        ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "failed");
    } else if (ret != ESP_OK) {
        ESP_RETURN_ON_ERROR(ret, TAG, "nvs_flash_init() failed");
    }

    SMM_INITSYS_MARK_INITIALIZED(NVS);
    return ESP_OK;
}

esp_err_t smm_initsys_init_wifi(void) {
    SMM_INITSYS_PREAMBULE(WIFI);
    
    ESP_RETURN_ON_ERROR(wifimgr_init(), TAG, "failed");

    SMM_INITSYS_MARK_INITIALIZED(WIFI);
    return ESP_OK;
}

esp_err_t smm_initsys_init_hspi(void) {
    SMM_INITSYS_PREAMBULE(HSPI);

    ESP_LOGI(TAG, "HSPI: MISO=%d MOSI=%d CLK=%d",
             SMM_CONFIG_HSPI_MISO_PIN, SMM_CONFIG_HSPI_MOSI_PIN, SMM_CONFIG_HSPI_CLK_PIN);

    esp_err_t ret;
    spi_bus_config_t hspi_config = {
        .miso_io_num = SMM_CONFIG_HSPI_MISO_PIN,
        .mosi_io_num = SMM_CONFIG_HSPI_MOSI_PIN,
        .sclk_io_num = SMM_CONFIG_HSPI_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
    };

    ret = spi_bus_initialize(HSPI_HOST, &hspi_config, SPI_DMA_DISABLED);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed");
        ESP_ERROR_CHECK(ret);
        return ESP_ERR_INVALID_STATE;
    }

    SMM_INITSYS_MARK_INITIALIZED(HSPI);
    return ESP_OK;
}

esp_err_t smm_initsys_init_adc(spi_device_handle_t *handle) {
    SMM_INITSYS_PREAMBULE(ADC);

    ESP_LOGI(TAG, "ADC(MCP3208): host=%d CS=%d clk_f=%dHz",
             SMM_CONFIG_MCP3208_HOST_ID, SMM_CONFIG_MCP3208_CS_PIN,
             SMM_CONFIG_MCP3208_CLK_FREQ_HZ);

    esp_err_t ret;
    ret = mcp3208_attach_device(SMM_CONFIG_MCP3208_HOST_ID, SMM_CONFIG_MCP3208_CS_PIN,
                                SMM_CONFIG_MCP3208_CLK_FREQ_HZ, handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mcp3208_attach_device failed");
        ESP_ERROR_CHECK(ret);
        return ESP_ERR_INVALID_STATE;
    }

    SMM_INITSYS_MARK_INITIALIZED(ADC);
    return ESP_OK;
}

esp_err_t smm_initsys_init_i2c_num_0(void) {
    SMM_INITSYS_PREAMBULE(I2C_NUM_0);

    ESP_LOGI(TAG, "I2C_NUM_0: mode=MASTER SDA=%d(pu=%d) SCL=%d(pu=%d) clk_f=%dHz",
             SMM_CONFIG_I2C_NUM_0_SDA_PIN, SMM_CONFIG_I2C_NUM_0_SDA_PULLUP_EN,
             SMM_CONFIG_I2C_NUM_0_SCL_PIN, SMM_CONFIG_I2C_NUM_0_SCL_PULLUP_EN,
             SMM_CONFIG_I2C_NUM_0_CLK_FREQ_HZ);

    esp_err_t ret;
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SMM_CONFIG_I2C_NUM_0_SDA_PIN,
        .scl_io_num = SMM_CONFIG_I2C_NUM_0_SCL_PIN,
        .master.clk_speed = SMM_CONFIG_I2C_NUM_0_CLK_FREQ_HZ,
        .sda_pullup_en = SMM_CONFIG_I2C_NUM_0_SDA_PULLUP_EN,
        .scl_pullup_en = SMM_CONFIG_I2C_NUM_0_SCL_PULLUP_EN,
    };

    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed");
        ESP_ERROR_CHECK(ret);
        return ESP_ERR_INVALID_STATE;
    }
    ret = i2c_param_config(I2C_NUM_0, &i2c_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed");
        ESP_ERROR_CHECK(ret);
        return ESP_ERR_INVALID_STATE;
    }

    SMM_INITSYS_MARK_INITIALIZED(I2C_NUM_0);
    return ESP_OK;
}

esp_err_t smm_initsys_init_lcd(lcd_handle_t *handle) {
    SMM_INITSYS_PREAMBULE(LCD);

    ESP_LOGI(TAG, "LCD display driver: i2c_port=%d addr=0x%02X cols=%d rows=%d",
             SMM_CONFIG_LCD_I2C_PORT, SMM_CONFIG_LCD_ADDRESS,
             SMM_CONFIG_LCD_COLUMNS, SMM_CONFIG_LCD_ROWS);

    esp_err_t ret;
    lcd_handle_t lcd_config = {
        .i2c_port = SMM_CONFIG_LCD_I2C_PORT,
        .address = SMM_CONFIG_LCD_ADDRESS,
        .columns = SMM_CONFIG_LCD_COLUMNS,
        .rows = SMM_CONFIG_LCD_ROWS,
        .display_function = LCD_4BIT_MODE | LCD_2LINE | LCD_5x8DOTS,
        .display_control = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF,
        .display_mode = LCD_ENTRY_INCREMENT | LCD_ENTRY_DISPLAY_NO_SHIFT,
        .cursor_column = 0,
        .cursor_row = 0,
        .backlight = LCD_BACKLIGHT,
        .initialized = false,
    };

    memcpy(handle, &lcd_config, sizeof(lcd_config));
    ret = lcd_init(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "lcd_init failed");
        ESP_ERROR_CHECK(ret);
        return ESP_ERR_INVALID_STATE;
    }
    ret = lcd_clear_screen(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "lcd_clear_screen failed");
        ESP_ERROR_CHECK(ret);
        return ESP_ERR_INVALID_STATE;
    }

    SMM_INITSYS_MARK_INITIALIZED(LCD);
    return ESP_OK;
}

esp_err_t smm_initsys_init_buttons(void) {
    SMM_INITSYS_PREAMBULE(BUTTONS);

    ESP_LOGI(TAG, "Installing buttons driver: btnUP=%d btnDN=%d",
             SMM_CONFIG_BUTTONS_BTN_UP_PIN, SMM_CONFIG_BUTTONS_BTN_DOWN_PIN);

    esp_err_t ret;
    button_config_t button_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .short_press_time = SMM_CONFIG_BUTTONS_SHORT_PRESS_TIME,
        .gpio_button_config = {
            .active_level = 0,
        },
    };
    button_event_config_t event_cfg = {
        .event_data.long_press.press_time = SMM_CONFIG_BUTTONS_LONG_PRESS_TIME,
    };

    struct button_def {
        gpio_num_t gpio;
        menu_click_type_e single_click;
        menu_click_type_e double_click;
        menu_click_type_e long_press;
    } button_defs[] = {
        {
            .gpio = SMM_CONFIG_BUTTONS_BTN_UP_PIN,
            .single_click = MENU_CLICK_TYPE_UP_SINGLE,
            .double_click = MENU_CLICK_TYPE_UP_DOUBLE,
            .long_press = MENU_CLICK_TYPE_UP_LONG,
        },
        {
            .gpio = SMM_CONFIG_BUTTONS_BTN_DOWN_PIN,
            .single_click = MENU_CLICK_TYPE_DOWN_SINGLE,
            .double_click = MENU_CLICK_TYPE_DOWN_DOUBLE,
            .long_press = MENU_CLICK_TYPE_DOWN_LONG,
        },
    };
    const size_t button_count = sizeof(button_defs) / sizeof(*button_defs);

    for (uint8_t i = 0; i < button_count; i++) {
        struct button_def button_def = button_defs[i];

        button_cfg.gpio_button_config.gpio_num = button_def.gpio;
        button_handle_t button = iot_button_create(&button_cfg);
        if (button == NULL) {
            ESP_LOGE(TAG, "iot_button_create failed (button %d)", i);
            ESP_ERROR_CHECK(ESP_ERR_INVALID_STATE);
            return ESP_ERR_INVALID_STATE;
        }

        event_cfg.event = BUTTON_SINGLE_CLICK;
        ret = iot_button_register_event_cb(
            button, event_cfg, menu_submit_click, (void *)button_def.single_click);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "iot_button_register_event_cb smm_btnevt_callback (button %d)", button_def.single_click);
            ESP_ERROR_CHECK(ret);
            return ESP_ERR_INVALID_STATE;
        }

        event_cfg.event = BUTTON_DOUBLE_CLICK;
        iot_button_register_event_cb(
            button, event_cfg, menu_submit_click, (void *)button_def.double_click);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "iot_button_register_event_cb smm_btnevt_callback (button %d)", button_def.double_click);
            ESP_ERROR_CHECK(ret);
            return ESP_ERR_INVALID_STATE;
        }

        event_cfg.event = BUTTON_LONG_PRESS_START;
        iot_button_register_event_cb(
            button, event_cfg, menu_submit_click, (void *)button_def.long_press);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "iot_button_register_event_cb smm_btnevt_callback (button %d)", button_def.long_press);
            ESP_ERROR_CHECK(ret);
            return ESP_ERR_INVALID_STATE;
        }
    }

    SMM_INITSYS_MARK_INITIALIZED(BUTTONS);
    return ESP_OK;
}

esp_err_t smm_initsys_init_menu(akdmenu_redraw_handler_v redraw_handler) {
    SMM_INITSYS_PREAMBULE(MENU);

    menu_init(redraw_handler);

    SMM_INITSYS_MARK_INITIALIZED(MENU);
    return ESP_OK;
}

esp_err_t smm_initsys_init_spiffs(void) {
    SMM_INITSYS_PREAMBULE(SPIFFS);

    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    SMM_INITSYS_MARK_INITIALIZED(SPIFFS);
    return ESP_OK;
}

esp_err_t smm_initsys_init_sampler(spi_device_handle_t *mcp_handle) {
    SMM_INITSYS_PREAMBULE(SAMPLER);

    ESP_LOGI(TAG, "Initializing Sampler");

    sampler_config_t sampler_cfg = {
        .read_cmds[0] = MCP3208_CMD_SGL_CH3, //voltage
        .read_cmds[1] = MCP3208_CMD_SGL_CH6, //current1
        .read_cmds[2] = MCP3208_CMD_SGL_CH0, //current2
        .mcp_handle = mcp_handle,
        .core_id = 1,
        .timing = {
            .timer_freq_hz = 4705882, // ~4.7MHz
            .sampling_interval = 1621, // 4.7MHz / (1620*10) ~= 2903.073Hz
            .idle_watchdog = 26110 * 20, // about 20s (@sampling_interval=26110Hz)
        },
    };
    ESP_RETURN_ON_ERROR(sampler_init(&sampler_cfg), TAG, "failed");

    SMM_INITSYS_MARK_INITIALIZED(SAMPLER);
    return ESP_OK;
}

esp_err_t smm_initsys_init_webserver(void) {
    SMM_INITSYS_PREAMBULE(WEBSERVER);

    ESP_RETURN_ON_ERROR(smm_webserver_start(), TAG, "failed");

    SMM_INITSYS_MARK_INITIALIZED(WEBSERVER);
    return ESP_OK;
}

bool smm_initsys_check_all_initialized(void) {
    for (smm_initsys_subsys subsys = 0; subsys != SMM_INITSYS_SS_END_OF_SUBSYSTEMS; subsys++) {
        uint32_t mask = 1 << subsys;
        if ((initialized & mask) != mask) {
            return false;
        }
    }
    return true;
}

#include "wifimgr.h"

#include <string.h>

#include "esp_system.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "stdint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"

#include "esp_wifi.h"
#include "nvs_flash.h"

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_check.h"
#include "esp_random.h"

static const char *TAG = "WIFIMGR";

static struct {
    struct {
        char ssid[32];
        char password[64];
        esp_netif_t *instance;
        bool enabled;
    } ap;
    struct {
        char ssid[32];
        char password[64];
        esp_netif_t *instance;
        bool enabled;
    } sta;
} wifimgr = {
    .ap = {
        .ssid = {0},
        .password = {0},
        .instance = NULL,
        .enabled = false,
    },
    .sta = {
        .ssid = {0},
        .password = {0},
        .instance = NULL,
        .enabled = false,
    },
};

static void wifi_event_handler(void *arg, esp_event_base_t ebase, int32_t eid, void *edata);
ESP_EVENT_DEFINE_BASE(WIFIMGR_EVENTS);

esp_err_t wifimgr_init(void) {
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config.nvs_enable = 0;

    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL),
        TAG, "failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL),
        TAG, "failed");

    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_config), TAG, "failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "failed");
    ESP_RETURN_ON_FALSE((wifimgr.ap.instance = esp_netif_create_default_wifi_ap()) != NULL, ESP_FAIL, TAG, "failed");
    ESP_RETURN_ON_FALSE((wifimgr.sta.instance = esp_netif_create_default_wifi_sta()) != NULL, ESP_FAIL, TAG, "failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "failed");
    ESP_RETURN_ON_ERROR(wifimgr_set_if_enabled(WIFI_IF_AP, false), TAG, "failed");
    ESP_RETURN_ON_ERROR(wifimgr_set_if_enabled(WIFI_IF_STA, false), TAG, "failed");

    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t ebase, int32_t eid, void *edata) {
    if (ebase == WIFI_EVENT && eid == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)edata;
        ESP_LOGI(TAG, "TAG_AP Station " MACSTR " joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    ESP_LOGI(TAG, "Event!! %d %d", (int)ebase, (int)eid);
    if (ebase == WIFI_EVENT && eid == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)edata;
        ESP_LOGI(TAG, "TAG_AP Station " MACSTR " joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (ebase == WIFI_EVENT && eid == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)edata;
        ESP_LOGI(TAG, "TAG_AP Station " MACSTR " left, AID=%d, reason:%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    } else if (ebase == IP_EVENT && eid == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)edata;
        ESP_LOGI(TAG, "TAG_STA Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// note that this is ridiculous but it does work (i.e. change ap/sta status without* interrupting the other)...
esp_err_t wifimgr_set_if_enabled(wifi_interface_t iface, bool enabled) {
    if (iface == WIFI_IF_AP) {
        wifi_config_t config = {
            .ap = {
                .channel = 1,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg = {
                    .required = false,
                },
            },
        };
        if (enabled) {
            ESP_LOGI(TAG, "%s AP", wifimgr.ap.enabled ? "Reenabling" : "Enabling");
            size_t ssid_len = strnlen(wifimgr.ap.ssid, 32), password_len = strnlen(wifimgr.ap.password, 64);
            memcpy(config.ap.ssid, wifimgr.ap.ssid, ssid_len);
            memcpy(config.ap.password, wifimgr.ap.password, password_len);
            config.ap.ssid_len = ssid_len;
            config.ap.max_connection = 4;
            config.ap.ssid_hidden = 0;
        } else {
            ESP_LOGI(TAG, "%s AP", wifimgr.ap.enabled ? "Redisabling" : "Disabling");
            esp_fill_random(config.ap.ssid, 32);
            esp_fill_random(config.ap.password, 63);
            config.ap.ssid_len = 32;
            config.ap.max_connection = 0;
            config.ap.ssid_hidden = 1;
        }
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &config), TAG, "Failed to set config for AP");
        ESP_ERROR_CHECK(esp_event_post(WIFIMGR_EVENTS, enabled ? WIFIMGR_EVENT_AP_ENABLED : WIFIMGR_EVENT_AP_DISABLED, NULL, 0, portMAX_DELAY));
        wifimgr.ap.enabled = enabled;
        return ESP_OK;
    } else if (iface == WIFI_IF_STA) {
        wifi_config_t config = {
            .sta = {
                .scan_method = WIFI_ALL_CHANNEL_SCAN,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            },
        };
        if (enabled) {
            ESP_LOGI(TAG, "%s STA", wifimgr.sta.enabled ? "Reenabling" : "Enabling");
            size_t ssid_len = strnlen(wifimgr.sta.ssid, 32), password_len = strnlen(wifimgr.sta.password, 64);
            memcpy(config.sta.ssid, wifimgr.sta.ssid, ssid_len);
            memcpy(config.sta.password, wifimgr.sta.password, password_len);
            config.sta.failure_retry_cnt = 5;
            config.sta.bssid_set = 0;
        } else {
            ESP_LOGI(TAG, "%s STA", wifimgr.sta.enabled ? "Redisabling" : "Disabling");
            esp_fill_random(config.ap.ssid, 32);
            esp_fill_random(config.ap.password, 63);
            esp_fill_random(config.sta.bssid, 6);
            config.sta.failure_retry_cnt = 0;
            config.sta.bssid_set = 1;
        }
        if (!enabled) {
            ESP_RETURN_ON_ERROR(esp_wifi_disconnect(), TAG, "Failed to disconnect from AP");
        }
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), TAG, "Failed to set config for STA");
        if (enabled) {
            ESP_RETURN_ON_ERROR(esp_wifi_connect(), TAG, "Failed to connect to AP");
        }
        wifimgr.sta.enabled = enabled;
        return ESP_OK;
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

bool wifimgr_is_enabled(wifi_interface_t iface) {
    if (iface == WIFI_IF_AP) {
        return wifimgr.ap.enabled;
    } else if (iface == WIFI_IF_STA) {
        return wifimgr.sta.enabled;
    }
    return false;
}

esp_err_t wifimgr_set_credentials(wifi_interface_t iface, const char *ssid, const char *password) {
    if (ssid == NULL && password == NULL) {
        return ESP_OK;
    }

    char *target_ssid, *target_password;
    if (iface == WIFI_IF_AP) {
        target_ssid = wifimgr.ap.ssid;
        target_password = wifimgr.ap.password;
    } else if (iface == WIFI_IF_STA) {
        target_ssid = wifimgr.sta.ssid;
        target_password = wifimgr.sta.password;
    } else {
        return ESP_ERR_INVALID_ARG;
    }

    if (ssid != NULL) {
        memcpy(target_ssid, ssid, strnlen(ssid, 32));
    }
    if (password != NULL) {
        memcpy(target_password, password, strnlen(password, 64));
    }

    return ESP_OK;
}

esp_err_t wifimgr_get_credentials(wifi_interface_t iface, char *ssid_buf, char *password_buf) {
    char *source_ssid, *source_password;
    if (iface == WIFI_IF_AP) {
        source_ssid = wifimgr.ap.ssid;
        source_password = wifimgr.ap.password;
    } else if (iface == WIFI_IF_STA) {
        source_ssid = wifimgr.sta.ssid;
        source_password = wifimgr.sta.password;
    } else {
        return ESP_ERR_INVALID_ARG;
    }

    if (ssid_buf != NULL) {
        memcpy(ssid_buf, source_ssid, strnlen(source_ssid, 32));
    }
    if (password_buf != NULL) {
        memcpy(password_buf, source_password, strnlen(source_password, 64));
    }

    return ESP_OK;
}

esp_err_t wifimgr_get_ip(wifi_interface_t iface, char *buf, size_t n) {
    if (iface == WIFI_IF_AP) {
        if (!wifimgr.ap.enabled) {
            return ESP_ERR_INVALID_STATE;
        }
        esp_netif_ip_info_t info;
        ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(wifimgr.ap.instance, &info), TAG, "Failed to get AP IP");
        snprintf(buf, n, IPSTR, IP2STR(&info.ip));
        return ESP_OK;
    } else if (iface == WIFI_IF_STA) {
        if (!wifimgr.sta.enabled) {
            return ESP_ERR_INVALID_STATE;
        }
        esp_netif_ip_info_t info;
        ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(wifimgr.sta.instance, &info), TAG, "Failed to get STA IP");
        snprintf(buf, n, IPSTR, IP2STR(&info.ip));
        return ESP_OK;
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

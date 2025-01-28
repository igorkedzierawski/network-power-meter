#ifndef _WIFIMGR_H_
#define _WIFIMGR_H_

#include "freertos/FreeRTOS.h"

#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_event.h"

/*
WIFIMGR:
    - cała infrastruktura potrzebna do działania wifi jest włączona przed
        jakimkolwiek użyciem WIFIMGR (czyli netif, eventloop i wifidriver)
    - obsługiwane mogą być jednocześnie tryby AP i STA
    - domyślnie wszystko będzie zgaszone
    - są możliwości zapisu/odczytu ustawień do/z NVS:
        - namespace: WIFIMGR
        - wymagana jest inicjacja domyślnego NVS
        - robione jest to za sprawą funkcji:
            - _save -- zapisuje aktualne ustawienia
            - _load -- zamyka wszystko i wczytuje ustawienia z nvs
            - _is_present -- sprawdza czy ustawienia mogą być wczytane
*/

ESP_EVENT_DECLARE_BASE(WIFIMGR_EVENTS);

enum {
    WIFIMGR_EVENT_AP_ENABLED,
    WIFIMGR_EVENT_AP_DISABLED,
    WIFIMGR_EVENT_AP_STATION_JOINED,
    WIFIMGR_EVENT_AP_STATION_LEFT,
    WIFIMGR_EVENT_AP_GATEWAY_IP_CHANGED,
    TIMER_EVENT_EXPIRY,
    TIMER_EVENT_STOPPED
};

esp_err_t wifimgr_init(void);

esp_err_t wifimgr_set_if_enabled(wifi_interface_t iface, bool enabled);

bool wifimgr_is_enabled(wifi_interface_t iface);

esp_err_t wifimgr_set_credentials(wifi_interface_t iface, const char *ssid, const char *password);

esp_err_t wifimgr_get_credentials(wifi_interface_t iface, char *ssid_buf, char *password_buf);

esp_err_t wifimgr_get_ip(wifi_interface_t iface, char *buf, size_t n);

#endif
#include "menu.h"

#include "akdmenu.h"
#include "../menu/menu.h"
#include "wifimgr.h"
#include "../textmenu.h"

#include <string.h>

#include "esp_mac.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"

// CTLBEGIN
// CTLBEGIN
// CTLBEGIN

struct wifi_ap {
    akdmenu_menu_t menu;
    akdmenu_entry_t display_ssid;
    akdmenu_entry_t display_ip;
    akdmenu_entry_t disable;
    akdmenu_entry_t enable;
};
static struct wifi_ap wifi_ap;


char BUFFER[60];
#define BUF_0 BUFFER
#define BUF_1 (BUFFER+15)
#define BUF_2 (BUFFER+30)
#define BUF_3 (BUFFER+45)

const char DISPLAY_GWIP_L0[] = "Gateway IP:";
const char DISPLAY_SSID_L0[] = "Network SSID:";
static void ap_display_ssid(uint8_t cmd) {
    memset(BUFFER, 0, 60);
    snprintf(BUF_0, 16, "%-16.15s", "Network SSID:");
    wifimgr_get_credentials(WIFI_IF_AP, BUF_1, NULL);
    textmenu_print_wraplines(&wifi_ap.menu, BUFFER, strnlen(BUFFER, 60));
}

static void ap_display_gwip(uint8_t cmd) {
    memset(BUFFER, 0, 60);
    snprintf(BUF_0, 16, "%-16.15s", "Gateway IP:");
    wifimgr_get_ip(WIFI_IF_AP, BUF_1, 45);
    textmenu_print_wraplines(&wifi_ap.menu, BUFFER, strnlen(BUFFER, 60));
}

static void ap_enable(uint8_t cmd) {
    wifimgr_set_if_enabled(WIFI_IF_AP, true);
}

static void ap_disable(uint8_t cmd) {
    wifimgr_set_if_enabled(WIFI_IF_AP, false);
}

// CTLEND
// CTLEND
// CTLEND

static struct wifi_ap wifi_ap = {
    .menu = akdmenu_decl_menu("Tryb AP :) ", true),
    .display_ssid = akdmenu_decl_entry("Wysw. SSID", true, ap_display_ssid),
    .display_ip = akdmenu_decl_entry("Wysw. GWIP", true, ap_display_gwip),
    .disable = akdmenu_decl_entry("Turn OFF", true, ap_disable),
    .enable = akdmenu_decl_entry("Turn ON", true, ap_enable),
};

static void eh_WIFI_EVENT_AP_START() {
    akdmenu_item_set_visible(&wifi_ap.display_ssid, true);
    akdmenu_item_set_visible(&wifi_ap.enable, false);
    akdmenu_item_set_visible(&wifi_ap.display_ip, true);
    akdmenu_item_set_visible(&wifi_ap.disable, true);
    akdmenu_host_t *host = menu_get_host();
    if (host != NULL) {
        akdmenu_host_notify_change(host, NULL, AKDMENU_CHANGE_VISIBILITY);
    }
}

static void eh_WIFI_EVENT_AP_STOP() {
    akdmenu_item_set_visible(&wifi_ap.enable, true);
    akdmenu_item_set_visible(&wifi_ap.display_ssid, false);
    akdmenu_item_set_visible(&wifi_ap.display_ip, false);
    akdmenu_item_set_visible(&wifi_ap.disable, false);
    akdmenu_host_t *host = menu_get_host();
    if (host != NULL) {
        akdmenu_host_notify_change(host, NULL, AKDMENU_CHANGE_VISIBILITY);
    }
}

static esp_err_t wire_up_ap(void) {
    akdmenu_menu_append(&wifi_ap.menu, &wifi_ap.display_ssid);
    akdmenu_menu_append(&wifi_ap.menu, &wifi_ap.display_ip);
    akdmenu_menu_append(&wifi_ap.menu, &wifi_ap.disable);
    akdmenu_menu_append(&wifi_ap.menu, &wifi_ap.enable);
    akdmenu_menu_rewind(&wifi_ap.menu);
    // wire_up_multiline_menu();
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(
            WIFIMGR_EVENTS, WIFIMGR_EVENT_AP_ENABLED, &eh_WIFI_EVENT_AP_START, NULL, NULL),
        "POG", "failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(
            WIFIMGR_EVENTS, WIFIMGR_EVENT_AP_DISABLED, &eh_WIFI_EVENT_AP_STOP, NULL, NULL),
        "POG", "failed");
    if(wifimgr_is_enabled(WIFI_IF_AP)) {
        eh_WIFI_EVENT_AP_START();
    } else {
        eh_WIFI_EVENT_AP_STOP();
    }
    return 0;
}

static struct wifi_sta {
    akdmenu_menu_t menu;
    akdmenu_entry_t display_ssid;
    akdmenu_entry_t display_ip;
    akdmenu_entry_t disconnect;
    akdmenu_entry_t enable;
    akdmenu_entry_t wps_connect;
    struct {
        akdmenu_menu_t menu;
        akdmenu_entry_t e1;
        akdmenu_entry_t e2;
        akdmenu_entry_t e3;
        akdmenu_entry_t e4;
    } connect_with;
} wifi_sta = {
    .menu = akdmenu_decl_menu("Tryb STA: ", true),
    .display_ssid = akdmenu_decl_entry("Wysw. SSID", true, NULL),
    .display_ip = akdmenu_decl_entry("Wysw. IP", true, NULL),
    .disconnect = akdmenu_decl_entry("Rozlacz", true, NULL),
    .enable = akdmenu_decl_entry("Uruchom", true, NULL),
    .wps_connect = akdmenu_decl_entry("Polacz(WPS)", true, NULL),
    .connect_with = {
        .menu = akdmenu_decl_menu("Polacz z:", true),
        .e1 = akdmenu_decl_entry("1...", true, NULL),
        .e2 = akdmenu_decl_entry("2...", true, NULL),
        .e3 = akdmenu_decl_entry("3...", true, NULL),
        .e4 = akdmenu_decl_entry("4...", true, NULL),
    },
};

static void wire_up_sta(void) {
    akdmenu_menu_append(&wifi_sta.menu, &wifi_sta.display_ssid);
    akdmenu_menu_append(&wifi_sta.menu, &wifi_sta.display_ip);
    akdmenu_menu_append(&wifi_sta.menu, &wifi_sta.disconnect);
    akdmenu_menu_append(&wifi_sta.menu, &wifi_sta.enable);
    akdmenu_menu_append(&wifi_sta.menu, &wifi_sta.wps_connect);
    do {
        akdmenu_menu_append(&wifi_sta.connect_with.menu, &wifi_sta.connect_with.e1);
        akdmenu_menu_append(&wifi_sta.connect_with.menu, &wifi_sta.connect_with.e2);
        akdmenu_menu_append(&wifi_sta.connect_with.menu, &wifi_sta.connect_with.e3);
        akdmenu_menu_append(&wifi_sta.connect_with.menu, &wifi_sta.connect_with.e4);
        akdmenu_menu_rewind(&wifi_sta.connect_with.menu);
    } while (0);
    akdmenu_menu_append(&wifi_sta.menu, &wifi_sta.connect_with.menu);
    akdmenu_menu_rewind(&wifi_sta.menu);
}

static akdmenu_menu_t root = akdmenu_decl_menu("Opcje WiFi", true);

akdmenu_menu_t *wifimgr_menu_main_wifi_create(void) {
    wire_up_ap();
    akdmenu_menu_append(&root, &wifi_ap.menu);
    wire_up_sta();
    akdmenu_menu_append(&root, &wifi_sta.menu);
    akdmenu_menu_rewind(&root);

    akdmenu_item_set_visible(&wifi_sta.menu, false);

    return &root;
}

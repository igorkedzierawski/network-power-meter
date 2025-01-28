#ifndef _WIFIMGR_MENU_INTERNAL_H_
#define _WIFIMGR_MENU_INTERNAL_H_

#include "akdmenu.h"

akdmenu_menu_t *wifimgr_menu_main_wifi_create(void);

#endif

/*
ROOT:
    Sieciowy Miernik Mocy
    Opcje WiFi:
        Tryb Punktu Dostępowego [ON/OFF]:
            [ON]:
                Wyświetl SSID
                Wyświetl IP Bramy
                Wyłącz
            [OFF]:
                Włącz
        Tryb Stacji:[ON/OFF]
            [ON]:
                Wyświetl IP Urządzenia
                Rozłącz
            [OFF]:
                Połącz przez WPS
                //TODO Połącz do zapamiętanego kanału (w nvs):
                    1 (SSID_1)
                    2 (SSID_2)
                    ...
    Opcje samplera [ON/OFF]:
        [ON]:
            Wyłącz sampler
            Podgląd pomiarów (możliwość przestawiania z zapisem pozycji do nvs):
                Vrms
                Vpp
                I1rms
                I1pp
                I2rms
                I2pp
                P1rms
                P2rms
                wsp. odksz. 1
                wsp. odksz. 2
        [OFF]:
            Włącz sampler
            Opcje kanałów:
                Toggle kanał V
                Toggle kanał I1
                Toggle kanał I2
*/


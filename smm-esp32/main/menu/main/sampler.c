#include "main.h"

#include "akdmenu.h"

static akdmenu_entry_t emptyinfo = akdmenu_decl_entry("<Empty ;c>", true, NULL);

static akdmenu_menu_t root = akdmenu_decl_menu("Opcje Samplera", true);

akdmenu_menu_t *menu_main_sampler_create(void) {
    akdmenu_menu_append(&root, &emptyinfo);
    akdmenu_menu_rewind(&root);
    return &root;
}

/*
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

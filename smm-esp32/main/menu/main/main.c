#include "main.h"
#include "../../wifimgr/menu.h"

#include "akdmenu.h"

akdmenu_menu_t root = akdmenu_decl_root_menu();

akdmenu_menu_t *menu_main_create(void) {
    akdmenu_menu_append(&root, wifimgr_menu_main_wifi_create());
    // akdmenu_menu_append(&root, akdmenu_any_as_item(menu_main_wifi_create()));
    akdmenu_menu_append(&root, menu_main_sampler_create());
    akdmenu_menu_rewind(&root);
    return &root;
}

#include "menu.h"
#include "main/main.h"

#include <stdlib.h>
#include <stdio.h>

#include "akdmenu.h"
#include "config.h"

void default_handler(akdmenu_cmd_t cmd) {
    printf("Akcja! [%d]\n", cmd);
}

static akdmenu_item_t *line_dbuf[2 * SMM_CONFIG_MENU_ROWS];
static akdmenu_host_t host;

void menu_init(akdmenu_redraw_handler_v redraw_handler) {
    akdmenu_host_init(&host, SMM_CONFIG_MENU_ROWS, SMM_CONFIG_MENU_COLUMNS, line_dbuf, menu_main_create(), redraw_handler);
}

inline akdmenu_host_t *menu_get_host() {
    return &host;
}

void menu_submit_click(void *_, void *click_type_as_void) {
    menu_click_type_e click_type = (menu_click_type_e)(uint32_t)click_type_as_void;
    akdmenu_host_submit_cmd(&host, click_type);
}

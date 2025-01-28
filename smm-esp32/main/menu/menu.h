#ifndef _MENU_H_
#define _MENU_H_

#include "akdmenu.h"

typedef enum {
    MENU_CLICK_TYPE_UP_SINGLE = AKDMENU_CMD_UP,
    MENU_CLICK_TYPE_UP_DOUBLE = AKDMENU_CMD_RIGHT,
    MENU_CLICK_TYPE_UP_LONG = AKDMENU_CMD_RIGHT,
    MENU_CLICK_TYPE_DOWN_SINGLE = AKDMENU_CMD_DOWN,
    MENU_CLICK_TYPE_DOWN_DOUBLE = AKDMENU_CMD_LEFT,
    MENU_CLICK_TYPE_DOWN_LONG = AKDMENU_CMD_LEFT,
} menu_click_type_e;

void menu_init(akdmenu_redraw_handler_v redraw_handler);

void menu_submit_click(void *button_handle, void *click_type_as_void);

akdmenu_host_t *menu_get_host();

#endif
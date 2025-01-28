#include "textmenu.h"

#include <string.h>
#include <stdio.h>

#include "akdmenu.h"
#include "menu/menu.h"

#define TEXTMENU_LINE_COUNT 5

static struct {
    akdmenu_menu_t menu;
    akdmenu_entry_t lines[TEXTMENU_LINE_COUNT];
} mmenu = {
    .menu = akdmenu_decl_menu(NULL, false),
    .lines[0] = akdmenu_decl_entry(NULL, false, NULL),
};

void textmenu_init(void) {
    for (size_t i = 1; i < TEXTMENU_LINE_COUNT; i++) {
        memcpy(&mmenu.lines[i], &mmenu.lines[0], sizeof(mmenu.lines[0]));
    }
    for (size_t i = 0; i < TEXTMENU_LINE_COUNT; i++) {
        akdmenu_menu_append(&mmenu.menu, &mmenu.lines[i]);
    }
    akdmenu_menu_rewind(&mmenu.menu);
}

void textmenu_print(akdmenu_menu_t *parent, const char **lines, size_t n) {
    mmenu.menu.meta.parent = parent;
    n = n > TEXTMENU_LINE_COUNT ? TEXTMENU_LINE_COUNT : n;
    for (size_t i = n; i < TEXTMENU_LINE_COUNT; i++) {
        akdmenu_item_set_visible(&mmenu.lines[i], false);
    }
    for (size_t i = 0; i < n; i++) {
        mmenu.lines[i].meta.text = lines[i];
        akdmenu_item_set_visible(&mmenu.lines[i], true);
    }
    akdmenu_menu_rewind(&mmenu.menu);
    akdmenu_item_set_visible(&mmenu.menu, true);
    menu_get_host()->current_menu = &mmenu.menu;
    akdmenu_host_submit_cmd(menu_get_host(), AKDMENU_CMD_REDRAW);
}

void textmenu_print_wraplines(akdmenu_menu_t *parent, const char *long_line, size_t len) {
    len = len <= 15 ? 1 : 1 + ((len - 1) / 15);
    const char *lines[] = {
        long_line, long_line + 15, long_line + 30, long_line + 45, long_line + 60};
    textmenu_print(parent, lines, len);
}

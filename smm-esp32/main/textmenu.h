#ifndef _TEXT_MENU_H_
#define _TEXT_MENU_H_

#include "akdmenu.h"

void textmenu_init(void);

void textmenu_print(akdmenu_menu_t *parent, const char **lines, size_t n);

void textmenu_print_wraplines(akdmenu_menu_t *parent, const char *long_line, size_t len);

#endif
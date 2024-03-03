/*
 * menu.h
 * Menus for 4x20 character LCD

 */
#ifndef MENU_H
#define MENU_H

extern bool menu_settings();
extern bool menu_brew();
extern bool menu_main();
extern bool menu_sleep();


typedef enum { MENU_MAIN=0, MENU_SETTING=1, MENU_MODIFY=2, MENU_ERROR=3, MENU_BREW=4, MENU_SLEEP=5 } menu_list_t;

typedef struct setting
{
  char *name;
  char *unit;
  double *val;
  double delta;
  int decimals;
} setting_t;

extern const char *menus[];
extern const setting_t settings_list[];
extern const int num_settings;

#endif // MENU_H
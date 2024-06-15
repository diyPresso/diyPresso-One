/*
 * menu.h
 * Menus for 4x20 character LCD

 */
#ifndef MENU_H
#define MENU_H

extern int menu_settings();
extern bool menu_brew();
extern bool menu_main();
extern bool menu_sleep();
extern bool menu_wifi(char *msg);
extern bool menu_saved();
extern bool menu_error(const char *msg);
extern bool menu_commissioning();
extern bool menu_state();
extern int menu_preset();
extern int menu_preset_settings();
void apply_preset(int preset);

typedef enum { MENU_MAIN=0, MENU_SETTING=1, MENU_MODIFY=2, MENU_ERROR=3, MENU_BREW=4, MENU_SLEEP=5, MENU_CONFIRM=6, MENU_WIFI=7, MENU_SAVED=8, MENU_STATE=9, MENU_COMMISSIONING=10, PRESET_MENU=11 } menu_list_t;

typedef struct setting
{
  char *name;
  char *unit;
  double *val;
  double delta;
  int decimals;
} setting_t;

extern int presetIndex;
extern const char *menus[];
extern const setting_t settings_list[];
extern const int num_settings;

#endif // MENU_H
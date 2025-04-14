/*
 * menu.h
 * Menus for 4x20 character LCD

 */
#ifndef MENU_H
#define MENU_H

#define ANIMATION_REFRESH_RATE_MS 100 // in msec, the base rate for animation updates
#define SLEEP_SPINNER_REFRESH_RATE_MS 500 // in msec, the base rate for sleep spinner updates

extern int menu_settings(bool button_pressed);
extern bool menu_brew();
extern bool menu_main();
extern bool menu_sleep();
extern bool menu_wifi(char *msg);
extern bool menu_saved();
extern bool menu_error(const char *msg);
extern bool menu_commissioning();
extern bool menu_state();

typedef enum
{
  MENU_MAIN = 0,
  MENU_SETTING = 1,
  MENU_MODIFY = 2,
  MENU_ERROR = 3,
  MENU_BREW = 4,
  MENU_SLEEP = 5,
  MENU_CONFIRM = 6,
  MENU_WIFI = 7,
  MENU_SAVED = 8,
  MENU_STATE = 9,
  MENU_COMMISSIONING = 10
} menu_list_t;

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
/*
 * menu.h
 * Menus for 4x20 character LCD

 */
#ifndef MENU_H
#define MENU_H

#define ANIMATION_REFRESH_RATE_MS 100 // in msec, the base rate for animation updates
#define SLEEP_SPINNER_REFRESH_RATE_MS 500 // in msec, the base rate for sleep spinner updates

extern bool menu_main();
extern bool menu_warning_almost_empty();
extern bool menu_sleep();
extern bool menu_wifi(char *msg);
extern bool menu_saved();
extern bool menu_error(const char *msg);
extern bool menu_commissioning();
extern bool menu_state();
extern void update_display(bool button_pressed);
extern void menu_ready_reset();

typedef enum
{
  MENU_MAIN_ONE = 0,
  MENU_MAIN_TWO = 1,
  MENU_ERROR = 2,
  MENU_SLEEP = 3,
  MENU_WIFI = 4,
  MENU_SAVED = 5,
  MENU_STATE = 6,
  MENU_COMMISSIONING = 7,
  MENU_WARNING_ALMOST_EMPTY = 8
} menu_list_t;

extern const char *menus[];

#endif // MENU_H

char reservoir_level_indicator();

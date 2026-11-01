/*
  Settings menu: a two level menu on the 4x20 display, operated with the rotary encoder.

  - Turn to move the cursor, press to open a submenu, edit a value or run an action.
  - Changes are made in a working copy of the settings. When leaving the menu with changes,
    "Save changes?" is asked (default NO). Only on YES the settings are saved and applied.
  - Actions (tare, reset shot counters, commissioning, reset to defaults) ask for
    confirmation (default NO) and are saved right away.
  - "Start cleaning" asks for confirmation, then opens a temporary manual pump mode.
    Unsaved changes are discarded.
  - The last item of a submenu ("Set brew defs", ...) sets the values of that submenu to
    their defaults after confirmation (default NO). This is a change in the working copy,
    saved with "Save changes?". The tare and the shot counters are kept.
*/
#ifndef DP_SETTINGS_MENU_H
#define DP_SETTINGS_MENU_H

#include "dp_settings.h"

class SettingsMenu
{
public:
  typedef enum { RESULT_NONE, RESULT_SAVED, RESULT_EXIT } result_t;

  void open();             // open the menu at the root, start editing a copy of the settings
  void close();            // leave the menu, discard unsaved changes
  bool is_open() { return _state != STATE_CLOSED; }
  result_t update(bool button_pressed); // handle input and draw the menu, call on each display update

private:
  typedef enum { STATE_CLOSED, STATE_LIST, STATE_EDIT, STATE_CONFIRM, STATE_SAVE, STATE_STATUS } state_t;

  state_t _state = STATE_CLOSED;
  DpSettings _edit;        // working copy of the settings
  static const int MENU_MAX_DEPTH = 2; // max number of submenu levels below the root
  const struct menu *_menu = nullptr; // current menu
  int _cursor = 0;         // index of the selected visible item
  int _top = 0;            // first visible item on the display
  struct { const struct menu *menu; int cursor, top; } _parents[MENU_MAX_DEPTH]; // menus to return to with Back
  int _depth = 0;
  const struct menu_item *_item = nullptr; // item being edited or confirmed
  bool _choice = false;    // YES/NO choice of a confirmation screen
  long _prev_pos = 0;      // previous encoder position

  void select(const struct menu_item *item);
  void run_action(int action);
  void draw_list();
  void draw_edit();
  void draw_choice(const char *title, const char *question, const char *value = "");
};

extern SettingsMenu settingsMenu;

#endif // DP_SETTINGS_MENU_H

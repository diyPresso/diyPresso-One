/*
  Settings menu, see dp_settings_menu.h
  We are non-blocking and we assume to be called ~ 2x to 4x per second
 */
#include "dp.h"
#include "dp_settings_menu.h"
#include "dp_menu.h"
#include "dp_display.h"
#include "dp_hardware.h"
#include "dp_boiler.h"
#include "dp_reservoir.h"

SettingsMenu settingsMenu;

typedef enum { ITEM_SUBMENU, ITEM_VALUE, ITEM_SELECT, ITEM_ACTION, ITEM_TEXT, ITEM_STATUS, ITEM_BACK, ITEM_EXIT } item_type_t;

#define ITEM_MODEL_TWO 1 // flag: only shown on the Model Two (steam group)

typedef struct menu_item
{
  item_type_t type;
  const char *label;  // max 12 characters for a value, 19 for other items
  int id;             // setting id, action or text id
  const char *unit;   // unit (ITEM_VALUE) or "\0" separated list of options (ITEM_SELECT)
  double step;        // increment per encoder step (ITEM_VALUE), 0 = read only. Decimals: see dp_settings_list.h
  int flags;
  const struct menu *submenu; // ITEM_SUBMENU
  int setting;          // ITEM_ACTION: setting id to show as value, -1 = none
  const char *question; // ITEM_ACTION: question of the confirmation screen, max 2 lines of 20 characters separated by "\n"
} menu_item_t;

typedef struct menu
{
  const char *title;
  const menu_item_t *items;
  int count;
} menu_t;

// Actions, all ask for confirmation first (default NO)
enum { ACTION_TARE, ACTION_ZERO_COUNTER, ACTION_COMMISSIONING, ACTION_DEFAULTS, ACTION_MENU_DEFAULTS };

// Read-only texts
enum { TEXT_VERSION, TEXT_HARDWARE };

#define SUBMENU(label, menu, flags)               { ITEM_SUBMENU, label, 0, "", 0, flags, &menu }
#define VALUE(label, setting, unit, step)         { ITEM_VALUE, label, SETTING_##setting, unit, step, 0 }
#define SELECT(label, setting, options)           { ITEM_SELECT, label, SETTING_##setting, options, 1, 0 }
#define ACTION(label, action)                     { ITEM_ACTION, label, action, "", 0, 0, nullptr, -1, "Are you sure?" }
#define VALUE_ACTION(label, setting, action, question) { ITEM_ACTION, label, action, "", 0, 0, nullptr, SETTING_##setting, question }
#define MENU_DEFAULTS(label, question)            { ITEM_ACTION, label, ACTION_MENU_DEFAULTS, "", 0, 0, nullptr, -1, question }
#define TEXT(label, text)                         { ITEM_TEXT, label, text, "", 0, 0 }
#define STATUS(label)                             { ITEM_STATUS, label, 0, "", 0, 0 }
#define BACK()                                    { ITEM_BACK, " < Back", 0, "", 0, 0 }
#define EXIT()                                    { ITEM_EXIT, " < Exit", 0, "", 0, 0 }

#define OFF_ON "OFF\0ON\0"

#define MENU(title, items) { title, items, sizeof(items) / sizeof(menu_item_t) }

static const menu_item_t brew_items[] = {
    VALUE("Temperature", temperature, "\337C", 0.5),
    VALUE("Pre-infusion", preInfusionTime, "s", 0.1),
    VALUE("Infusion", infusionTime, "s", 0.1),
    VALUE("Extraction", extractionTime, "s", 0.5),
    VALUE_ACTION("Shot counter", shotCounter, ACTION_ZERO_COUNTER, "Reset counter?"),
    MENU_DEFAULTS("Set brew defs", "Reset brew to\ndefault values?"),
    BACK(),
};
static const menu_t brew_menu = MENU("BREW", brew_items);

static const menu_item_t steam_items[] = {
    VALUE("Temperature", steamTemperature, "\337C", 0.5),
    VALUE("Pump power", steamPumpPower, "%", 10),
    MENU_DEFAULTS("Set steam defs", "Reset steam to\ndefault values?"),
    BACK(),
};
static const menu_t steam_menu = MENU("STEAM", steam_items);

static const menu_item_t reservoir_items[] = {
    ACTION("Tare (full)", ACTION_TARE),
    VALUE("Weight trim", trimWeight, "%", 0.05),
    MENU_DEFAULTS("Set reservoir defs", "Reset reservoir to\ndefault values?"),
    BACK(),
};
static const menu_t reservoir_menu = MENU("RESERVOIR", reservoir_items);

static const menu_item_t boiler_pid_items[] = {
    VALUE("P-Gain", P, "%/\337C", 0.2),
    VALUE("I-Gain", I, "%/\337Cs", 0.01),
    VALUE("D-Gain", D, "%s/\337C", 1),
    VALUE("DFF factor", dffFactorPct, "%", 0.5),
    VALUE("DFF decay", dffDecay, "", 0.005),
    VALUE("Feedforward", ffReady, "%", 0.2),
    VALUE("Windup min", windupMin, "%", 1),
    VALUE("Windup max", windupMax, "%", 1),
    VALUE("Integr. band", integrationBand, "\337C", 0.5),
    SELECT("Cond. integr", conditionalIntegration, OFF_ON),
    VALUE("I-down fact.", iDownFactor, "", 0.5),
    SELECT("Brew I-hold", brewIntegralHold, OFF_ON),
    VALUE("Hold timeout", brewHoldTimeoutSec, "s", 10),
    VALUE("Dip window", brewHoldDipWindowSec, "s", 10),
    MENU_DEFAULTS("Set boiler PID defs", "Reset boiler PID to\ndefault values?"),
    BACK(),
};
static const menu_t boiler_pid_menu = MENU("BOILER PID", boiler_pid_items);

static const menu_item_t steam_pid_items[] = {
    VALUE("P-Gain", steamP, "%/\337C", 0.2),
    VALUE("I-Gain", steamI, "%/\337Cs", 0.01),
    VALUE("D-Gain", steamD, "%s/\337C", 1),
    VALUE("Feedforward", steamFF, "%", 0.5),
    VALUE("Pump P-Gain", steamPumpP, "%/\337C", 0.2),
    VALUE("Pump I-Gain", steamPumpI, "%/\337Cs", 0.01),
    VALUE("Pump D-Gain", steamPumpD, "%s/\337C", 1),
    VALUE("Windup min", steamWindupMin, "%", 1),
    VALUE("Windup max", steamWindupMax, "%", 1),
    VALUE("Integr. band", steamIntegrationBand, "\337C", 0.5),
    VALUE("Pump I band", steamPumpIntegrationBand, "\337C", 0.5),
    SELECT("Cond. integr", steamConditionalIntegration, OFF_ON),
    VALUE("I-down fact.", steamIDownFactor, "", 0.5),
    MENU_DEFAULTS("Set steam PID defs", "Reset steam PID to\ndefault values?"),
    BACK(),
};
static const menu_t steam_pid_menu = MENU("STEAM PID", steam_pid_items);

static const menu_item_t system_items[] = {
    STATUS("Status"),
    SELECT("WiFi", wifiMode, "OFF\0ON\0CONFIG-AP\0"),
    SELECT("Knob dir", encoderReverse, "NORMAL\0REVERSE\0"),
    ACTION("Commissioning", ACTION_COMMISSIONING),
    ACTION("Reset defaults", ACTION_DEFAULTS),
    TEXT("Firmware", TEXT_VERSION),
    TEXT("Hardware", TEXT_HARDWARE),
    BACK(),
};
static const menu_t system_menu = MENU("SYSTEM", system_items);

static const menu_item_t root_items[] = {
    SUBMENU("Brew", brew_menu, 0),
    SUBMENU("Steam", steam_menu, ITEM_MODEL_TWO),
    SUBMENU("Reservoir", reservoir_menu, 0),
    SUBMENU("Boiler PID", boiler_pid_menu, 0),
    SUBMENU("Steam PID", steam_pid_menu, ITEM_MODEL_TWO),
    SUBMENU("System", system_menu, 0),
    EXIT(),
};

static const menu_t root_menu = MENU("SETTINGS", root_items);

#define LIST_ROWS 3 // number of items shown below the title

static bool is_visible(const menu_item_t *item)
{
  return !(item->flags & ITEM_MODEL_TWO) || hardware.has_steam_group();
}

static int visible_count(const menu_t *menu)
{
  int n = 0;
  for (int i = 0; i < menu->count; i++)
    if (is_visible(&menu->items[i])) n++;
  return n;
}

static const menu_item_t *visible_item(const menu_t *menu, int index)
{
  for (int i = 0; i < menu->count; i++)
  {
    const menu_item_t *item = &menu->items[i];
    if (is_visible(item) && index-- == 0) return item;
  }
  return nullptr;
}

/// @brief Get a string from a list of "\0" separated strings, the last item has zero length
static const char *option_text(const char *options, int index)
{
  const char *ptr = options;
  while (index-- > 0 && strlen(ptr))
    ptr += strlen(ptr) + 1;
  return ptr;
}

static const char *text_value(int id)
{
  switch (id)
  {
  case TEXT_VERSION: return SOFTWARE_VERSION;
  case TEXT_HARDWARE: return hardware.has_steam_group() ? "Two r" HARDWARE_REVISION : "One r" HARDWARE_REVISION;
  default: return "";
  }
}

/// @brief Write a string into a display line, clipped at the end of the line
static void put(char *line, int col, const char *s)
{
  while (*s && col < 20)
    line[col++] = *s++;
}

static void put_right(char *line, const char *s)
{
  put(line, max(0, 20 - (int)strlen(s)), s);
}

static void show(char *screen)
{
  char *no_args[1] = {(char *)""};
  screen[80] = 0;
  display.show(screen, no_args);
}

static void clear(char *screen)
{
  memset(screen, ' ', 80);
  screen[80] = 0;
}

/// @brief Format the value of a setting item, with unit if it fits in max_len
static void format_value(char *dest, DpSettings &values, const menu_item_t *item, int max_len)
{
  dest[0] = 0;
  switch (item->type)
  {
  case ITEM_VALUE:
    format_float(dest, values.get((setting_id_t)item->id), DpSettings::decimals((setting_id_t)item->id), 0);
    if ((int)(strlen(dest) + strlen(item->unit)) <= max_len)
      strcat(dest, item->unit);
    break;
  case ITEM_SELECT:
    strcpy(dest, option_text(item->unit, (int)values.get((setting_id_t)item->id)));
    break;
  case ITEM_TEXT:
    strcpy(dest, text_value(item->id));
    break;
  case ITEM_SUBMENU:
    strcpy(dest, "\176"); // right arrow
    break;
  case ITEM_ACTION:
    if (item->setting >= 0)
      format_float(dest, values.get((setting_id_t)item->setting), DpSettings::decimals((setting_id_t)item->setting), 0);
    break;
  default:
    break;
  }
}

void SettingsMenu::open()
{
  _edit = settings;
  _menu = &root_menu;
  _cursor = _top = 0;
  _depth = 0;
  _prev_pos = display.encoder_value();
  _state = STATE_LIST;
}

void SettingsMenu::close()
{
  _state = STATE_CLOSED; // the working copy is not used anymore: unsaved changes are discarded
}

SettingsMenu::result_t SettingsMenu::update(bool button_pressed)
{
  long pos = display.encoder_value();
  int delta = pos - _prev_pos;
  _prev_pos = pos;
  result_t result = RESULT_NONE;

  switch (_state)
  {
  case STATE_LIST:
    _cursor = constrain(_cursor + delta, 0, visible_count(_menu) - 1);
    if (button_pressed)
      select(visible_item(_menu, _cursor));
    break;

  case STATE_EDIT:
    if (delta)
    {
      setting_id_t id = (setting_id_t)_item->id;
      if (_item->type == ITEM_SELECT)
        _edit.set(id, _edit.get(id) + delta);
      else
      {
        double scale = pow(10.0, DpSettings::decimals(id)); // round to the shown decimals, avoids 2.9999 after 0.1 steps
        _edit.set(id, round((_edit.get(id) + delta * _item->step) * scale) / scale);
      }
    }
    if (button_pressed)
      _state = STATE_LIST;
    break;

  case STATE_CONFIRM:
    if (delta)
      _choice = delta > 0;
    if (button_pressed)
    {
      _state = STATE_LIST;
      if (_choice)
        run_action(_item->id);
    }
    break;

  case STATE_SAVE:
    if (delta)
      _choice = delta > 0;
    if (button_pressed)
    {
      if (_choice)
      {
        settings = _edit;
        settings.save();
        boilerController.clear_error();
        reservoir.clear_error();
        settings.apply();
        result = RESULT_SAVED;
      }
      else
        result = RESULT_EXIT;
      _state = STATE_CLOSED;
    }
    break;

  case STATE_STATUS:
    if (button_pressed)
      _state = STATE_LIST;
    break;

  default:
    break;
  }

  if (_state == STATE_CLOSED && result == RESULT_NONE)
    result = RESULT_EXIT;

  switch (_state)
  {
  case STATE_LIST: draw_list(); break;
  case STATE_EDIT: draw_edit(); break;
  case STATE_CONFIRM:
  {
    char value[24];
    format_value(value, _edit, _item, 19 - strlen(_item->label));
    draw_choice(_item->id == ACTION_MENU_DEFAULTS ? _menu->title : _item->label, _item->question, value);
    break;
  }
  case STATE_SAVE: draw_choice("SETTINGS", "Save changes?"); break;
  case STATE_STATUS: menu_state(); break;
  default: break;
  }
  return result;
}

void SettingsMenu::select(const menu_item_t *item)
{
  if (!item)
    return;
  switch (item->type)
  {
  case ITEM_SUBMENU:
    if (_depth >= MENU_MAX_DEPTH)
      break;
    _parents[_depth++] = { _menu, _cursor, _top };
    _menu = item->submenu;
    _cursor = _top = 0;
    break;
  case ITEM_VALUE:
    if (item->step == 0) // read only
      break;
    // fall through
  case ITEM_SELECT:
    _item = item;
    _state = STATE_EDIT;
    break;
  case ITEM_ACTION:
    _item = item;
    _choice = false;
    _state = STATE_CONFIRM;
    break;
  case ITEM_STATUS:
    _state = STATE_STATUS;
    break;
  case ITEM_BACK:
    if (_depth > 0)
    {
      _depth--;
      _menu = _parents[_depth].menu;
      _cursor = _parents[_depth].cursor;
      _top = _parents[_depth].top;
      if (!_edit.same_values(settings)) // unsaved changes: select Exit, if this menu has one
        for (int i = 0; i < visible_count(_menu); i++)
          if (visible_item(_menu, i)->type == ITEM_EXIT)
            _cursor = i;
    }
    break;
  case ITEM_EXIT:
    if (_edit.same_values(settings))
      _state = STATE_CLOSED;
    else
    {
      _choice = false;
      _state = STATE_SAVE;
    }
    break;
  default:
    break;
  }
}

/// @brief Run a confirmed action. Actions change the stored settings directly and save them,
/// the working copy is updated so the unsaved changes are kept.
/// Except ACTION_MENU_DEFAULTS: it only sets the values of the current menu to their defaults
/// in the working copy, they are saved with "Save changes?" like edited values.
void SettingsMenu::run_action(int action)
{
  switch (action)
  {
  case ACTION_TARE:
    reservoir.tare();
    settings.tareWeight(reservoir.get_tare());
    settings.save();
    _edit.tareWeight(settings.tareWeight());
    break;
  case ACTION_ZERO_COUNTER:
    settings.zeroShotCounter();
    settings.save();
    _edit.shotCounter(0);
    break;
  case ACTION_COMMISSIONING:
    settings.commissioningDone(0);
    settings.save();
    _state = STATE_CLOSED; // the machine starts commissioning, unsaved changes are discarded
    break;
  case ACTION_DEFAULTS:
    settings.reset_to_defaults();
    settings.save();
    settings.apply();
    _edit = settings;
    break;
  case ACTION_MENU_DEFAULTS:
    for (int i = 0; i < _menu->count; i++)
    {
      const menu_item_t *item = &_menu->items[i];
      if (item->type == ITEM_VALUE || item->type == ITEM_SELECT) // not the actions: tare and shot counter are kept
        _edit.set((setting_id_t)item->id, DpSettings::default_value((setting_id_t)item->id));
    }
    break;
  }
}

void SettingsMenu::draw_list()
{
  char screen[81], buf[24];
  int count = visible_count(_menu);

  if (_cursor < _top) _top = _cursor;
  if (_cursor >= _top + LIST_ROWS) _top = _cursor - LIST_ROWS + 1;

  clear(screen);
  put(screen, 0, _menu->title);
  sprintf(buf, "%d/%d", _cursor + 1, count);
  put_right(screen, buf);

  for (int row = 0; row < LIST_ROWS && _top + row < count; row++)
  {
    const menu_item_t *item = visible_item(_menu, _top + row);
    char *line = screen + 20 * (row + 1);
    if (_top + row == _cursor)
      line[0] = '>';
    put(line, 1, item->label);
    format_value(buf, _edit, item, 19 - 1 - strlen(item->label));
    put_right(line, buf);
  }
  show(screen);
}

void SettingsMenu::draw_edit()
{
  char screen[81], buf[24];
  clear(screen);
  put(screen, 0, "MODIFY");
  put_right(screen, _menu->title);
  put(screen + 20, 0, _item->label);
  format_value(buf, _edit, _item, 0);
  put(screen + 40, 2, buf);
  if (_item->type == ITEM_VALUE)
    put(screen + 40, 3 + strlen(buf), _item->unit);
  put(screen + 60, 0, "[TURN]  [PRESS] = OK");
  show(screen);
}

void SettingsMenu::draw_choice(const char *title, const char *question, const char *value)
{
  char screen[81];
  clear(screen);
  put(screen, 0, title);
  put_right(screen, value);
  const char *line2 = strchr(question, '\n');
  if (line2)
  {
    for (int col = 0; question + col < line2 && col < 20; col++)
      screen[20 + col] = question[col];
    put(screen + 40, 0, line2 + 1);
  }
  else
    put(screen + 20, 0, question);
  put(screen + 60, 3, _choice ? " NO   [YES]" : "[NO]   YES ");
  show(screen);
}

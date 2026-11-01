/*
  menu functions
  We are non-blocking and we assume to be called ~ 2x to 4x per second
 */
#include "dp.h"
#include "dp_menu.h"
#include "dp_display.h"
#include "dp_brew.h"
#include "dp_brew_switch.h"
#include "dp_reservoir.h"
#include "dp_boiler.h"
#include "dp_heater.h"
#include "dp_pump.h"
#include "dp_settings.h"
#include "dp_settings_menu.h"
#include "dp_commission.h"
#include "dp_machine.h"
#include "dp_serial.h"
#include "dp_steam_process.h"
#include "dp_steam_thermoblock.h"
#include <Timer.h>

const char *errors_list[] = {
  // 01234567890123456789
    " OVER TEMPERATURE   ",
    " UNDER TEMPERATURE  ",
    " TEMPERATURE SENSOR ",
    "   HEATER TIMEOUT   ",
    "   WEIGHT SENSOR    "};

const int num_errors = sizeof(errors_list) / sizeof(char *);

#define CELSIUS_CHAR 0xDF
#define CELSIUS_STR "\337"

// const char spinner_chars[] = "/-\|";
// Custom characters 0..7. Character 0 is written as \10: code 0 ends a string, the display shows
// custom characters 0..7 also at codes 8..15.
const char spinner_chars[] = "\10\1\2\3\4\5\6\7";
const int num_spinner_chars = sizeof(spinner_chars) - 1; // without the string terminator

const char *menus[] = {
    // MAIN_ONE=0
    // 01234567890123456789
    "Boiler #####/#####\337C" // [0:actual] / [1:set_temp]
    "Power    ### % ## # "    // [2:percentage] [3:ON_OFF] [4:PUMP]
    "############# #####s"    // [5:state] [6:time]
    "Weight  ##### gram #",   // [7:Weight] [8:level]

    // MAIN_TWO=1
    // 01234567890123456789
    "Boiler #####/#####\337C" // [0:actual] / [1:set_temp]
    "Steam #### ###/###\337C" // [2:steam_state] [3:steam_temp]/[4:steam_set_temp]
    "############# #####s"    // [5:state] [6:time]
    "Weight  ##### gram #",   // [7:Weight] [8:level]

    // ERROR=2
    // 01234567890123456789
    "****** ERROR *******"
    "*##################*"
    "* TURN MACHINE OFF *"
    "********************",

    // SLEEP=3
    // 01234567890123456789
    "     ##########     "
    "   I AM SLEEPING!   "
    "  PRESS BUTTON TO   "
    "     WAKE ME...     ",

    // WIFI=4
    // 01234567890123456789
    " WIFI CONNECTING... "
    " ################## "
    " ################## "
    "  [PRESS] to cancel ",
    // SAVED=5
    // 01234567890123456789
    "                    "
    "   SETTINGS SAVED   "
    "                    "
    "                    ",
    // STATE=6
    // 01234567890123456789
    "####### ############"
    "B: ################ "
    "E: ################ "
    "R: ################ ",

    // COMMISSIONING=7
    // 01234567890123456789
    "___COMMISSIONING___ "
    " ################## "
    " ################## "
    " Weight ##### gram  ",

    // WARNING_ALMOST_EMPTY=8
  // 01234567890123456789
    "       Warning!     "
    "     Almost empty   "
    " Push to start brew "
    "Weight  ##### gram #", // [0:Weight] [1:level]

    // CLEANING=9
    // 01234567890123456789
    "CLEANING ##### shots" // [0:shots since cleaning]
    "####################" // [1:status]
    "Lever UP=ON DOWN=OFF"
    " Press knob to exit ",

    // CLEANING_REMINDER=10
    // 01234567890123456789
    "  CLEANING ADVISED  "
    " ##### shots since  " // [0:shots since cleaning]
    "   last cleaning    "
    "  Push to continue  ",
};
const int num_menus = sizeof(menus) / sizeof(char *);


// Main menu
bool menu_main()
{
  static unsigned int animation_counter = 0;
  char bufs[10][32];
  char *arg[10];
  char pump_spinner[2], heater_spinner[2], level_spinner[2];

  static unsigned long last_count_increment_t = 0;
  unsigned long delta_t = millis() - last_count_increment_t;

  if (delta_t > ANIMATION_REFRESH_RATE_MS) //increment animation_counter for animations every X msec
  {
    last_count_increment_t = millis();
    animation_counter++;
  }

  // Animations
  if (pumpDevice.is_on())
  {
    pump_spinner[0] = spinner_chars[animation_counter % 8];
    pump_spinner[1] = 0;
  }
  else
    pump_spinner[0] = 0;

  if (heaterDevice.is_on())
  {
    heater_spinner[0] = spinner_chars[7];
    heater_spinner[1] = 0;
  }
  else
    heater_spinner[0] = 0;

  if (reservoir.is_empty() or reservoir.is_almost_empty()) 
  {
    level_spinner[0] = spinner_chars[((animation_counter % 8) > 4) ? 7 : 0]; // flash empty with a period of 8
  }
  else
  {
    level_spinner[0] = reservoir_level_indicator();
  }
  level_spinner[1] = 0;

  for (int i = 0; i < 10; i++)
    arg[i] = bufs[i];

  // [0:actual] / [1:set_temp]
  format_float(arg[0], boilerController.act_temp(), 1, 5);
  format_float(arg[1], boilerController.set_temp(), 1);

  if (hardware.has_steam_group())
  {
    // Steam [2:steam_state] [3:steam_temp]/[4:steam_set_temp]
    arg[2] = (char *)steamProcess.get_short_state();
    format_float(arg[3], steamThermoblock.act_temp(), 1, 5);
    format_float(arg[4], steamThermoblock.set_temp(), 1);
  }
  else
  {
    // Power [2:percentage] [3:ON_OFF] [4:PUMP]
    format_float(arg[2], heaterDevice.power(), 0, 3);
    strcpy(arg[3], heaterDevice.is_on() ? "ON" : "");
    arg[4] = pump_spinner;
  }

  // [5:state] [6:time]
  arg[5] = (char *)brewProcess.get_state_name();
  format_float(arg[6], brewProcess.brew_time(), 1, 5);

  // [7:Weight] [8:level]
  if (brewProcess.is_busy())
    format_float(arg[7], brewProcess.weight(), 0, 5);
  else if (brewProcess.is_finished())
    format_float(arg[7], brewProcess.end_weight(), 0, 5);
  else
    format_float(arg[7], reservoir.weight(), 0, 5);

  arg[8] = level_spinner;

  display.show(menus[hardware.has_steam_group() ? MENU_MAIN_TWO : MENU_MAIN_ONE], arg);
  return false;
}

// Reservoir almost empty warning menu
bool menu_warning_almost_empty()
{
  char bufs[10][32];
  char *arg[10];

  for (int i = 0; i < 10; i++)
    arg[i] = bufs[i];

  // [0:Weight]
  format_float(bufs[0], reservoir.weight(), 0, 5);

  // [1:level]
  bufs[1][0] = reservoir_level_indicator();
  bufs[1][1] = 0;
 
  display.show(menus[MENU_WARNING_ALMOST_EMPTY], arg);
  return false;
}

bool menu_cleaning()
{
  char shots[12];
  char *args[] = { shots, (char *)machineController.cleaning_status(), (char *)"" };
  sprintf(shots, "%5d", settings.shotCounterSinceCleaning());
  display.show(menus[MENU_CLEANING], args);
  return false;
}

bool menu_cleaning_reminder()
{
  char shots[12];
  char *args[] = { shots, (char *)"" };
  sprintf(shots, "%5d", settings.shotCounterSinceCleaning());
  display.show(menus[MENU_CLEANING_REMINDER], args);
  return false;
}

/// @brief true if the cleaning reminder is on and the shots since cleaning reached the limit
static bool cleaning_due()
{
  return settings.cleaningReminder() && settings.shotCounterSinceCleaning() >= settings.cleaningReminderShots();
}

bool menu_commissioning()
{
  struct { const char *state; const char *name; const char *info; } substates[] = {
    { "init",    "Fill reservoir",    "And press button" },
    { "fill",    "Filling boiler",    "Wait..." },
    { "purge",   "Purge",             "Put brew lever UP" },
    { "check",   "Press button when", "Water pours out" },
    { "confirm", "Done!",             "Put lever DOWN" },
    { "done",    "Done!",             "" },
  };
  char buf[32];
  char *args[3];
  char weight[10];

  format_float(weight, reservoir.weight(), 0, 5);
  args[2] = weight;

  const char *state = commissioningProcess.get_state_name();
  const char *name = "?";
  const char *info = "?";

  for (auto &s : substates) {
    if (strcmp(state, s.state) == 0) {
      name = s.name;
      info = s.info;
      break;
    }
  }

  args[0] = (char *)name;
  args[1] = (char *)info;
  if (strcmp(state, "fill") == 0)
  {
    sprintf(buf, "Wait %d...", (int)INITIAL_PUMP_TIME - (int)commissioningProcess.state_time());
    args[1] = buf;
  }
  display.show(menus[MENU_COMMISSIONING], args);
  return false;
}

bool menu_sleep()
{
  char *sleep_spinner[] = 
      {
          "          ",
          "Z         ",
          "Zz        ",
          "Zzz       ",
          "Zzzz      ",
          "Zzzzz     ",
          "Zzzzzz    ",
          " Zzzzzzz  ",
          "  Zzzzzzz ",
          "   Zzzzzzz",
          "    Zzzzzz",
          "     Zzzzz",
          "      Zzzz",
          "       Zzz",
          "        Zz",
          "         Z",
          "          ",
      };
  static unsigned int animation_counter = 0;
  static unsigned long last_count_increment_t = 0;

  unsigned long delta_t = millis() - last_count_increment_t;
  if (delta_t > SLEEP_SPINNER_REFRESH_RATE_MS) //increment animation_counter for animations every X msec
  {
    last_count_increment_t = millis();
    animation_counter += 1;
  }
  if (animation_counter >= sizeof(sleep_spinner) / sizeof(const char *))
    animation_counter = 0;
  display.show(menus[MENU_SLEEP], &sleep_spinner[animation_counter]);
  return false;
}

bool menu_error(const char *msg)
{

  static char *args[10];
  args[0] = (char *)brewProcess.get_state_name();
  args[1] = (char *)brewProcess.get_error_text();
  args[2] = (char *)boilerController.get_state_name();
  args[3] = (char *)boilerController.get_error_text();
  args[4] = (char *)reservoir.get_error_text();

  display.show(menus[MENU_STATE], args);
  return false;

  /*static unsigned int animation_counter=0;
  static char *arg[10];
  arg[0] = (char*)msg;
  display.show(menus[MENU_ERROR], arg);
  return false; */
}

bool menu_wifi(char *msg = "")
{
  const char *wifi_spinner[] =
      {
          ".     ",
          " .    ",
          "  .   ",
          "   .  ",
          "    . ",
          "     .",
      };
  static unsigned int spinner = 0;
  static char *args[10];
  spinner += 1;
  if (spinner >= sizeof(wifi_spinner) / sizeof(const char *))
    spinner = 0;
  args[0] = (char *)wifi_spinner[spinner];
  args[1] = msg;
  display.show(menus[MENU_WIFI], args);
  return false;
}

bool menu_saved()
{
  static char *args[10];
  display.show(menus[MENU_SAVED], args);
  return false;
}

bool menu_state()
{
  static char *args[10];
  args[0] = (char *)brewProcess.get_state_name();
  args[1] = (char *)brewProcess.get_error_text();
  args[2] = (char *)boilerController.get_state_name();
  args[3] = (char *)boilerController.get_error_text();
  args[4] = (char *)reservoir.get_error_text();

  display.show(menus[MENU_STATE], args);
  return false;
}

char reservoir_level_indicator()
{
  int level_index = (num_spinner_chars * reservoir.level()) / 100.0;
  return spinner_chars[min(max(0, level_index), num_spinner_chars - 1)];
}

typedef enum { MAIN, SETTINGS, SAVED, WARNING, CLEANING_REMINDER } ready_menu_t;
static ready_menu_t ready_menu = MAIN;
#define CLEANING_REMINDER_REPEAT 5 // shots after a dismissed cleaning reminder before it is shown again
static int cleaning_reminder_dismissed = -1; // shots since cleaning when the reminder was dismissed, -1 = not dismissed

static bool cleaning_reminder_pending()
{
  int shots = settings.shotCounterSinceCleaning();
  return cleaning_reminder_dismissed < 0 || shots < cleaning_reminder_dismissed // not dismissed since the last cleaning
         || shots >= cleaning_reminder_dismissed + CLEANING_REMINDER_REPEAT;
}

void menu_ready_reset()
{
  settingsMenu.close(); // discard unsaved changes
  ready_menu = MAIN;
}

bool menu_ready(bool button_pressed)
{
  static Timer saved_timer = Timer(MILLIS);

  if (brewProcess.is_warning_almost_empty() && ready_menu != WARNING)
  {
    settingsMenu.close();
    ready_menu = WARNING;
  }

  switch (ready_menu)
  {
  case WARNING:
    menu_warning_almost_empty();
    if (!brewProcess.is_warning_almost_empty())
      ready_menu = MAIN;
    break;

  case CLEANING_REMINDER:
    menu_cleaning_reminder();
    if (button_pressed)
      cleaning_reminder_dismissed = settings.shotCounterSinceCleaning();
    if (button_pressed || brewProcess.is_busy() || !cleaning_due())
      ready_menu = MAIN;
    break;

  case MAIN:
    if (cleaning_due() && cleaning_reminder_pending()
        && !brewProcess.is_busy() && !brewProcess.is_finished() && !button_pressed)
    {
      ready_menu = CLEANING_REMINDER;
      menu_cleaning_reminder();
      break;
    }
    menu_main();
    if (button_pressed)
    {
      settingsMenu.open();
      ready_menu = SETTINGS;
    }
    break;

  case SETTINGS:
    if (brewProcess.is_busy()) {
      settingsMenu.close(); // discard unsaved changes
      ready_menu = MAIN;
      break;
    }
    switch (settingsMenu.update(button_pressed))
    {
    case SettingsMenu::RESULT_SAVED:
      ready_menu = SAVED;
      break;
    case SettingsMenu::RESULT_EXIT:
      ready_menu = MAIN;
      break;
    default:
      break;
    }
    break;

  case SAVED:
    menu_saved();
    display.button_pressed();
    if (saved_timer.state() != status_t::RUNNING) {
      saved_timer.start();
    } else if (saved_timer.read() > 1000) {
      saved_timer.stop();
      ready_menu = MAIN;
    }
    break;

  default:
    ready_menu = MAIN;
  }
  return false;
}

// Display update: pick the right menu based on machine state
void update_display(bool button_pressed)
{
  const char *machine_state = machineController.get_state_name();

  if (strcmp(machine_state, "commissioning") == 0)
    menu_commissioning();
  else if (machineController.is_cleaning())
    menu_cleaning();
  else if (strcmp(machine_state, "sleep") == 0)
    menu_sleep();
  else if (strcmp(machine_state, "error") == 0)
    menu_error("ERROR");
  else
    menu_ready(button_pressed);
}

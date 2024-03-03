/*
  display
 */
#include "dp_menu.h"
#include "dp_display.h"
#include "dp_brew.h"
#include "dp_brew_switch.h"
#include "dp_reservoir.h"
#include "dp_boiler.h"
#include "dp_heater.h"
#include "dp_pump.h"
#include "dp_settings.h"

double settings_vals[10];

// "text", "unit", pointer, increment, decimals
const setting_t settings_list[] =
{
  {"Temperature", "\337C", &settings_vals[0], 0.5, 2 },
  {"Pre-infusion time", "sec", &settings_vals[1], 0.1, 2 },
  {"Infusion time", "sec", &settings_vals[2], 0.1, 2 },
  {"Extraction time", "sec", &settings_vals[3], 0.5, 2 },
  {"Extraction weight", "gram", &settings_vals[4], 0.5, 2 },
  {"P-Gain", "%/\337C", &settings_vals[5], 0.1, 2 },
  {"I-Gain", "%/\337C/s",  &settings_vals[6],0.02, 2 },
  {"D-Gain", "%s",  &settings_vals[7], 0.02, 2 },
  {"FF-Value", "%",  &settings_vals[8], 0.02, 2 },
  {"Shot counter", "shots",  &settings_vals[9], 0.00, 0 },
  {"       [SAVE]", "",  &settings_vals[10], -1.0, 0 }
};

const int num_settings = sizeof(settings_list) / sizeof(setting_t);


const char *errors_list[] = {
  //0123456789012345678
  " OVER TERMPERATURE  ",
  " UNDER TERMPERATURE ",
  " TEMPERATURE SENSOR ",
  "   HEATER TIMEOUT   ",
  "   WEIGHT SENSOR    "
};

const int num_errors = sizeof(errors_list) / sizeof(char*);

#define CELCIUS_CHAR 0xDF
#define CELCIUS_STR "\337"

//const char spinner_chars[] = "/-\|";
const char spinner_chars[] = "\0\1\2\3\4\5\6\7";

const char *menus[] = {
// MAIN=0
// 01234567890123456789
  "Boiler #####/#####\337C"
  "Power    ### % ##   "
  "Time   ##### sec #  "
  "Weight ##### gram   ",

// SETTING=1
// 01234567890123456789
  "SETTING      [##/##]"
  "################### "
  "  ########## #######"
  "             [PRESS]",

// MODIFY=2
// 01234567890123456789
  "MODIFY       [##/##]"
  "################### "
  "  ########## #######"
  "              [TURN]",

// ERROR=3
// 01234567890123456789
  "****** ERROR *******"
  "*##################*"
  "* TURN MACHINE OFF *"
  "********************",

// BREW=4
// 01234567890123456789
  "STATE ############  "
  "STEP-T ######## sec "
  "BREW-T ######## sec "
  "                    ",

};

const int num_menus = sizeof(menus) / sizeof(char*);


bool menu_brew()
{
  char bufs[10][32];
  char *arg[10];

  arg[0] = bufs[0];
  arg[1] = bufs[1];
  arg[2] = bufs[2];

  format_float(arg[0], brewProcess.state, 0);
  arg[0] = brew_state_names[brewProcess.state];
  format_float(arg[1], brewProcess.step_time(), 1);
  format_float(arg[2], brewProcess.brew_time(), 1);

  display.show(menus[MENU_BREW], arg);
  return false;
}

// Main menu
bool menu_main()
{
  static int count=0;
  char bufs[10][32];
  char *arg[10];
  char spinner[2], heater[2];

  if ( pumpDevice.is_on() )
  {
    spinner[0] = spinner_chars[count % 8];
    spinner[1] = 0;
    count += 1;
  }
  else
    spinner[0] = 0;
  if ( heaterDevice.is_on() )
  {
    heater[0] = spinner_chars[7];
    heater[1] = 0;
  }
  else
    heater[0] = 0;

  for(int i=0; i<10; i++)
    arg[i] = bufs[i];

  format_float(arg[0], boilerController.act_temp(), 1, 5);
  format_float(arg[1], boilerController.set_temp(), 1);
  format_float(arg[2], heaterDevice.power(), 0, 3);
  strcpy(arg[3], heaterDevice.is_on() ? "ON" : "");
  //arg[3] = heater;
  format_float(arg[4], brewProcess.brew_time(), 1, 5);
  arg[5] = spinner;
  format_float(arg[6], reservoir.weight(), 0, 5);
  display.show(menus[MENU_MAIN], arg);
  return false;
}

// add a value to a setting
double add_value(int n, double delta)
{
  double result;
  switch ( n )
  {
    case 0: return settings.temperature(settings.temperature() + delta);
    case 1: return settings.preInfusionTime(settings.preInfusionTime() + delta);
    case 2: return settings.infusionTime(settings.infusionTime() + delta);
    case 3: return settings.extractionTime(settings.extractionTime() + delta);
    case 4: return settings.extractionWeight(settings.extractionWeight() + delta);
    case 5: return settings.P(settings.P() + delta);
    case 6: return settings.I(settings.I() + delta);
    case 7: return settings.D(settings.D() + delta);
    case 8: return settings.FF(settings.FF() + delta);
    case 9: return settings.shotCounter(); // Shot counter;
    case 10: return 0; // SAVE
  }
}

// Settings menu (blocking)
bool menu_settings()
{
  static char buf[32], bufs[10][32];
  static char *arg[10];
  static bool modify = false, prev_modify = false;
  static int idx = 0;
  static bool button_pressed;
  static long pos=0, prev_pos=0, loop=0;
  static double set_val=0;

  for(int i=0; i<10; i++)
    arg[i] = bufs[i];
  format_float(arg[1], num_settings, 0);

//  while ( loop++ < 1000 && brewSwitch.down())
  {
    setting_t set = settings_list[idx];
    pos = display.encoder_value();
    button_pressed = display.button_pressed();
    if ( modify )
    {
      set_val = add_value(idx, set.delta * (pos - prev_pos));
    }
    else
    {
      idx +=  pos - prev_pos;
      idx %= num_settings;
      if ( idx < 0) idx += num_settings;
      set_val = add_value(idx, 0.0);
    }
    if ( button_pressed )
       modify = !modify;
    if ( modify && set.delta == -1.0)
      return true;

    if (set.delta == -1.0) // Save menu: no value to display
      *arg[3] = 0;
    else
      format_float(arg[3], set_val, set.decimals, 10);  // Show the value

    format_float(arg[0], idx+1, 0, 2);
    arg[2] = set.name;
    arg[4] = set.unit;
    display.show(menus[modify ? MENU_MODIFY : MENU_SETTING], arg);
    prev_pos = pos;
    prev_modify = modify;
  }
  return false;
}
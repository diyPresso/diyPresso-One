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

double settings_vals[32];

// the increment setting has some special values:
#define READ_ONLY 0         // only display value, cannot modify
#define EXECUTE_FUNCTION -1 // Execute a function, the 'decimals' field contains a function ID
#define SELECT_ITEM -2      // Select item from a list, 'unit' has an `ASCII-ZERO` separated list

// List of function IDs
#define FUNCTION_SAVE 1
#define FUNCTION_TARE 2
#define FUNCTION_ZERO 3


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
  {"Shot counter", "shots",  &settings_vals[9], READ_ONLY, 0 },
  {"WIFI Mode", "OFF\0ON\0CONFIG-AP\0", &settings_vals[10], SELECT_ITEM, 1},
  {"Weight trim", "%",  &settings_vals[11], 0.05, 2 },
  {"   <Tare Weight>", "", &settings_vals[31], EXECUTE_FUNCTION, FUNCTION_TARE },
  {"   <Zero Counter>", "", &settings_vals[31], EXECUTE_FUNCTION, FUNCTION_ZERO },
  {"       <SAVE>", "",  &settings_vals[31], EXECUTE_FUNCTION, FUNCTION_SAVE }
};

const int num_settings = sizeof(settings_list) / sizeof(setting_t);


const char *errors_list[] = {
  //0123456789012345678
  " OVER TEMPERATURE   ",
  " UNDER TEMPERATURE  ",
  " TEMPERATURE SENSOR ",
  "   HEATER TIMEOUT   ",
  "   WEIGHT SENSOR    "
};

const int num_errors = sizeof(errors_list) / sizeof(char*);

#define CELSIUS_CHAR 0xDF
#define CELSIUS_STR "\337"

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
  "SETTINGS     [##/##]"
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
// SLEEP=5
// 01234567890123456789
  "    ###########     "
  "   I AM SLEEPING!   "
  " LONG PRESS BUTTON  "
  "   TO WAKE ME...    ",

// CONFIRM=6
// 01234567890123456789
  "####################"
  "    ARE YOU SURE?   "
  "                    "
  "       ###    [TURN]",

// WIFI=7
// 01234567890123456789
  " WIFI CONNECTING... "
  " ################## "
  " ################## "
  "                    ",
// SAVED=8
// 01234567890123456789
  "                    "
  "   SETTINGS SAVED   "
  "                    "
  "                    "

};
const int num_menus = sizeof(menus) / sizeof(char*);

const char *get_string_item(const char *items, int index);
int get_item_count(const char *items);
double add_value(int n, double delta);


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
  format_float(arg[4], brewProcess.brew_time(), 1, 5);
  arg[5] = spinner;
  format_float(arg[6], reservoir.weight(), 0, 5);
  display.show(menus[MENU_MAIN], arg);
  return false;
}


// Settings menu (blocking)
bool menu_settings()
{
  static char buf[32], bufs[10][32];
  static char *arg[10];
  static bool modify = false;
  static int idx = 0;
  static bool button_pressed;
  static long pos=0, prev_pos=0, loop=0;
  static double set_val=0;

  for(int i=0; i<10; i++)
    arg[i] = bufs[i];
  format_float(arg[1], num_settings, 0);

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
  {
    if ( set.delta == EXECUTE_FUNCTION )
    {
      switch ( set.decimals )
      {
        case FUNCTION_SAVE: return true; break;
        case FUNCTION_TARE: reservoir.tare(); settings.tareWeight( reservoir.get_tare() ); return true; break;
        case FUNCTION_ZERO: settings.zeroShotCounter(); return true; break;
      }
    }
    else
    {
      if (set.delta != READ_ONLY)
        modify = !modify;
    }
  }

  if ( modify && set.delta == EXECUTE_FUNCTION && set.decimals == FUNCTION_SAVE )
    return true;

  arg[2] = set.name;
  arg[4] = set.unit;

  switch ( (int)set.delta )
  {
    case EXECUTE_FUNCTION: *arg[3] = 0; break;    // A function to execute: no value to display
    case SELECT_ITEM: strcpy(arg[3], get_string_item(set.unit, set_val)); arg[4] = ""; break;
    default: format_float(arg[3], set_val, set.decimals, 10); break;  // Show the value
  }

  format_float(arg[0], idx+1, 0, 2); // the item number
  display.show(menus[modify ? MENU_MODIFY : MENU_SETTING], arg);
  prev_pos = pos;

  return false;
}

// add a value to a setting
double add_value(int n, double delta)
{
  double result;
  if ( delta != 0 ) Serial.print("delta:"); Serial.print(delta);

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
    case 9: return settings.shotCounter();
    case 10: return settings.wifiMode(settings.wifiMode() - (delta / 2.0) );
    case 11: return settings.trimWeight( settings.trimWeight() + delta);
    default: return 0;
  }
}


bool menu_sleep()
{
  char *sleep_spinner[] =
  {
    "         ",
    "Z        ",
    "Zz       ",
    "Zzz      ",
    "Zzzz     ",
    "Zzzzz    ",
    "Zzzzzz   ",
    " Zzzzzzz ",
    "  Zzzzzzz",
    "   Zzzzzz",
    "    Zzzzz",
    "     Zzzz",
    "      Zzz",
    "       Zz",
    "        Z",
    "         ",
  };
  static unsigned int spinner=0;
  static unsigned int counter=0;
  counter += 1;
  if ( counter == 5 ){ counter=0; spinner +=1; }
  if ( spinner >= sizeof(sleep_spinner) / sizeof(const char *)) spinner = 0;
  display.show(menus[MENU_SLEEP], &sleep_spinner[spinner]);
  return false;
}

bool menu_wifi(char *msg="")
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
  static unsigned int spinner=0;
  static char *args[10];
  spinner += 1;
  if ( spinner >= sizeof(wifi_spinner) / sizeof(const char *)) spinner = 0;
  args[0] = (char*)wifi_spinner[spinner];
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


/// @brief Get a string from a list of strings
/// @param a pointer to list of ASCIZ terminated strings, where the last item has zero length
/// @param index the index in the list (0 ..[n-items]) 
/// @return pointer to ASCIZ string from list. If index is larger than items, return zero length item
const char *get_string_item(const char *items, int index)
{
  const char *ptr = items;
  while ( index && strlen(ptr) )
  {
    ptr += strlen(ptr)+1;
    index--;
  }
  return ptr;
}

int get_item_count(const char *items)
{
  const char *ptr = items;
  int count = 0;
  while ( strlen(ptr) )
  {
    ptr += strlen(ptr)+1;
    count++;
  }
  return count;
}


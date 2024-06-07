/*
  menu functions
  We are non-blocking and we assume to be called ~ 2x to 4x per second
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
#define FUNCTION_DEFAULTS 4
#define FUNCTION_EXIT 5


// "text", "unit", pointer, increment, decimals
const setting_t settings_list[] =
{
  {"Temperature", "\337C", &settings_vals[0], 0.5, 2 },
  {"Pre-infusion time", "sec", &settings_vals[1], 0.1, 2 },
  {"Infusion time", "sec", &settings_vals[2], 0.1, 2 },
  {"Extraction time", "sec", &settings_vals[3], 0.5, 2 },
  {"Extraction weight", "gram", &settings_vals[4], 0.5, 2 },
  {"P-Gain", "%/\337C", &settings_vals[5], 0.2, 1 },
  {"I-Gain", "%/\337C/s",  &settings_vals[6],0.01, 2 },
  {"D-Gain", "%s",  &settings_vals[7], 0.2, 1 },
  {"FF-heat Value", "%",  &settings_vals[8], 0.2, 1 },
  {"FF-ready Value", "%",  &settings_vals[9], 0.2, 1 },
  {"FF-brew Value", "%",  &settings_vals[10], 0.2, 1 },
  {"Shot counter", "shots",  &settings_vals[11], READ_ONLY, 0 },
  {"WIFI Mode", "OFF\0ON\0CONFIG-AP\0", &settings_vals[12], SELECT_ITEM, 1},
  {"Weight trim", "%",  &settings_vals[13], 0.05, 2 },
  {"Commissioning done", "NO\0YES\0", &settings_vals[14], SELECT_ITEM, 1},
  {"   <Tare Weight>", "FULL", &settings_vals[31], EXECUTE_FUNCTION, FUNCTION_TARE },
  {"   <Zero Counter>", "", &settings_vals[31], EXECUTE_FUNCTION, FUNCTION_ZERO },
  {"<Reset to defaults>", "", &settings_vals[31], EXECUTE_FUNCTION, FUNCTION_DEFAULTS },
  {"       <EXIT>", "",  &settings_vals[31], EXECUTE_FUNCTION, FUNCTION_EXIT },
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
  "Boiler #####/#####\337C" // [0:actual] / [1:set_temp]
  "Power    ### % ## # "    // [2:percentage] [3:ON_OFF] [4:PUMP]
  "############# #####s"    // [5:state] [6:time]
  "Weight ##### gram # ",   // [7:Weight] [8:level]

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
  "                    ",
// STATE=9
// 01234567890123456789
  "####### ############"
  "B: ################ "
  "E: ################ "
  "R: ################ ",

// STATE=10
// 01234567890123456789
  "   COMMISSIONING    "
  " ################## "
  " ################## "
  " Weight ##### gram  ",
// PRESET=11
// 01234567890123456789
  "Select Preset:      "
  "#################   "
  "##################  "
  "    [TURN AND PRESS]",

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

  arg[0] = (char*)brewProcess.get_state_name();
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
  char pump_spinner[2], heater_spinner[2], level_spinner[2];

  count += 1;
  if ( pumpDevice.is_on() )
  {
    pump_spinner[0] = spinner_chars[count % 8];
    pump_spinner[1] = 0;
  }
  else
    pump_spinner[0] = 0;

  if ( heaterDevice.is_on() )
  {
    heater_spinner[0] = spinner_chars[7];
    heater_spinner[1] = 0;
  }
  else
    heater_spinner[0] = 0;

  if ( reservoir.is_empty() )
  {
    level_spinner[0] = spinner_chars[ (count % 2) ? 7 : 0]; // flash empty
  }
  else
  {
    int level = (8*reservoir.level())/100.0;
    if (level < 0) level = 0;
    if (level > 7) level = 7;
    level_spinner[0] = spinner_chars[ level ];
    level_spinner[1] = 0;
  }

  for(int i=0; i<10; i++)
    arg[i] = bufs[i];

  // [0:actual] / [1:set_temp]
  format_float(arg[0], boilerController.act_temp(), 1, 5);
  format_float(arg[1], boilerController.set_temp(), 1);

  // [2:percentage] [3:ON_OFF] [4:PUMP]
  format_float(arg[2], heaterDevice.power(), 0, 3);
  strcpy(arg[3], heaterDevice.is_on() ? "ON" : "");
  arg[4] = pump_spinner;

  // [5:state] [6:time]
  arg[5] = (char*)brewProcess.get_state_name();
  format_float(arg[6], brewProcess.brew_time(), 1, 5);

  // [7:Weight] [8:level]
  if ( brewProcess.is_busy() )
    format_float(arg[7], brewProcess.weight(), 0, 5);
  else if ( brewProcess.is_finished() )
    format_float(arg[7], brewProcess.end_weight(), 0, 5);
  else
    format_float(arg[7], reservoir.weight(), 0, 5);

  arg[8] = level_spinner;

  display.show(menus[MENU_MAIN], arg);
  return false;
}


// Settings menu (blocking)
/* Returns 0 if the settings menu should continue running, 1 if a save action was performed and the menu should be exited, 
 * and 2 if an exit action was performed and the menu should be exited.
*/
int menu_settings()
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
        case FUNCTION_EXIT: return 2;
        case FUNCTION_SAVE: settings.save(); break;
        case FUNCTION_TARE: reservoir.tare(); settings.tareWeight( reservoir.get_tare() ); settings.save(); break;
        case FUNCTION_ZERO: settings.zeroShotCounter(); break;
        case FUNCTION_DEFAULTS: settings.defaults(); break;
      }
      return 1;
    }
    else
    {
      if (set.delta != READ_ONLY)
        modify = !modify;
    }
  }

  if ( modify && set.delta == EXECUTE_FUNCTION && set.decimals == FUNCTION_SAVE )
    return 1;

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

  return 0;
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
    case 8: return settings.ff_heat(settings.ff_heat() + delta);
    case 9: return settings.ff_ready(settings.ff_ready() + delta);
    case 10: return settings.ff_brew(settings.ff_brew() + delta);
    case 11: return settings.shotCounter();
    case 12: return settings.wifiMode(settings.wifiMode() - (delta / 2.0) );
    case 13: return settings.trimWeight( settings.trimWeight() + delta);
    case 14: return settings.commissioningDone(settings.commissioningDone() + (delta / 2.0) );

    default: return 0;
  }
}




bool menu_commissioning()
{
  static char *substate_names[] = {"Fill reservoir", "Filling boiler", "Purge", "Done!", "?"};
  static char *substate_info[] = {"And press button", "Wait...", "Put lever UP", "Put lever DOWN", "?"};
  char *args[3];
  char weight[10];
  int substate=4;

  format_float(weight, brewProcess.weight(), 0, 5);
  args[2] = weight;

  if ( brewProcess.is_init() ) substate = 0;
  if ( brewProcess.is_fill() ) substate = 1;
  if ( brewProcess.is_purge() ) substate = 2;
  if ( brewProcess.is_done() ) substate = 3;
  
  args[0] = (char*)substate_names[substate];
  args[1] = (char*)substate_info[substate];
  display.show(menus[MENU_COMMISSIONING], args);
  if ( display.button_pressed() )
    return true;
  return false;
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


bool menu_error(const char *msg)
{

  static char *args[10];
  args[0] = (char*)brewProcess.get_state_name();
  args[1] = (char*)brewProcess.get_error_text();
  args[2] = (char*)boilerController.get_state_name();
  args[3] = (char*)boilerController.get_error_text();
  args[4] = (char*)reservoir.get_error_text();

  display.show(menus[MENU_STATE], args);
  return false;

  /*static unsigned int counter=0;
  static char *arg[10];
  arg[0] = (char*)msg;
  display.show(menus[MENU_ERROR], arg);
  return false; */
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

void apply_preset(int preset) {
  switch (preset) {
    case 0: // Lungo
      settings.preInfusionTime(1);
      settings.infusionTime(3);
      settings.extractionTime(30);
      settings.extractionWeight(50);
      break;
    case 1: // Espresso
      settings.preInfusionTime(1);
      settings.infusionTime(3);
      settings.extractionTime(23);
      settings.extractionWeight(36);
      break;
    /*
    case 2: //preset 3
      settings.preInfusionTime(1);
      settings.infusionTime(3);
      settings.extractionTime(23);
      settings.extractionWeight(36);
      break;
    case 3: //Cancel
      break;*/
    default:
      break;
  }
}

bool menu_preset() {
  static int preset_index = 0;
  static char *arg[4];
  static char bufs[4][32];
  const char *presets[] = {"1. Lungo", "2. Espresso"};
  const int num_presets = sizeof(presets) / sizeof(presets[0]);

  for (int i = 0; i < 4; i++)
    arg[i] = bufs[i];

  int encoder_value = display.encoder_value();
  preset_index = encoder_value % num_presets;
  if (preset_index < 0) preset_index += num_presets;

  for (int i = 0; i < num_presets; i++) {
    if (i == preset_index) {
      snprintf(arg[i], 32, "> %s", presets[i]);
    } else {
      snprintf(arg[i], 32, "  %s", presets[i]);
    }
  }
  display.show(menus[PRESET_MENU], arg);

  if (display.button_pressed()) {
    apply_preset(preset_index);
    return true;
  };

  return false;
}
/* 
// more presets?
bool menu_preset() {
  static int preset_index = 0;
  static char *arg[4];
  static char bufs[4][32];
  const char *presets[] = {"1. Lungo", "2. Espresso", "3. Cappuccino", "4. Latte"};
  const int num_presets = sizeof(presets) / sizeof(presets[0]);

  for (int i = 0; i < 4; i++)
    arg[i] = bufs[i];

  int encoder_value = display.encoder_value();
  preset_index = encoder_value % num_presets;
  if (preset_index < 0) preset_index += num_presets;

  for (int i = 0; i < num_presets; i++) {
    if (i == preset_index) {
      snprintf(arg[i % 4], 32, "> %s", presets[i]);
    } else {
      snprintf(arg[i % 4], 32, "  %s", presets[i]);
    }
  }
  display.show(menus[PRESET_MENU], arg);

  if (display.button_pressed()) {
    apply_preset(preset_index + 1);
    return true;
  }

  return false;
}
*/






bool menu_state()
{
  static char *args[10];
  args[0] = (char*)brewProcess.get_state_name();
  args[1] = (char*)brewProcess.get_error_text();
  args[2] = (char*)boilerController.get_state_name();
  args[3] = (char*)boilerController.get_error_text();
  args[4] = (char*)reservoir.get_error_text();

  display.show(menus[MENU_STATE], args);
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


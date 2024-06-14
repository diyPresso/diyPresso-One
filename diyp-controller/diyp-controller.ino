/*
    diyPresso main controller
    (c) 2024 - diyPresso

    Used Libraries:
    * Timer v1.2.1 - stefan Staub
    * EasyWiFi-for-Mkr1010 v1.4.0 - JAY fOX  - https://github.com/javos65/EasyWifi-for-MKR1010/
    * wdt_samd21 v1.1.0 -Guglielmo Braguglia - https://github.com/gpb01/wdt_samd21

    This software uses a singleton design pattern for all modules. A .cpp file with an instance of a single object is created for each
    function. Such a module may contain instances to low-level devices and use other modules and objects

    Nomenclature:
     - 'device'      Hardware input/output (e.g. "GPIO", "LCD", "Thermistor", etc, state representation of external device) has `on()`, `off()` functions etc.
     - 'controller'  Control a device (e.g. "Heater", has little to no state) has a (non blocking) `control()` function
     - 'process'     Control a process (e.g. "brewProcess") has a (non blocking) `control()` function. Typically a state machine

    Global Objects:

    * settings - load and save settings to flash
    * menu - The menu system: logo(), main(), settings(), error()
    * screen - The 4x20 character display: init(), show(), logo()
      * lcd

    * encoder - Rotary encoder has start(), position(), pressed_count()

    * brewProcess - The brewing process: start(), stop()

    * boilerController - The boiler with heater and temp. sensor: on(), off(), setpoint(), actual(), power(), errors()
      * thermistor -- Adafruit_MAX31865 PT1000 sensor
      * heaterControl -- PWM Control of the heater output

    * reservoir - The water reservoir with weight scale
      * weight(), tarre(), level(), empty()
      * hx711

*/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#include "dp.h"
#include "dp_hardware.h"
#include "dp_led.h"
#include "dp_settings.h"
#include "dp_encoder.h"
#include "dp_boiler.h"
#include "dp_reservoir.h"
#include "dp_display.h"
#include "dp_menu.h"
#include "dp_brew.h"
#include "dp_brew_switch.h"
#include "dp_heater.h"
#include "dp_pump.h"

#include "dp_wifi.h"
#include "dp_mqtt.h"
int presetIndex = 0;

/**
 * @brief setup code
 * initialize all objects and state
 */
void setup()
{
  int result = 0;
  Serial.begin(115200);
  delay(1000);
  Serial.println(__DATE__ " " __TIME__);
  statusLed.color(ColorLed::WHITE);

  encoder.start();
  display.init();
  display.logo(__DATE__, __TIME__);

  if ((result = settings.load()) < 0)
  {
    Serial.println("Failed to load settings, result=");
    Serial.println(result);
    Serial.print("Save default settings, result=");
    Serial.println(settings.save());
  }
  else
    Serial.println("Load settings OK, result=");
  Serial.println(result);

  Serial.println(encoder.button_count());
  if (encoder.button_count() > 3)
  {
    Serial.println("button pressed 4x at startup: perform factory reset of settings");
    settings.defaults();
    Serial.println(settings.save());
  }
  presetIndex = settings.defaultPreset();

  apply_settings();

  display.custom_chars(custom_chars_spinner);

  Serial.println("INIT DONE");
  heaterDevice.pwm_period(2.0); // [sec]
  boilerController.off();

  if ( settings.wifiMode() != WIFI_MODE_OFF)
  {
    if (settings.wifiMode() == WIFI_MODE_AP)
    {
      wifi_erase();
      settings.wifiMode(WIFI_MODE_ON);
      settings.save();
    }
    menu_wifi("starting");
    wifi_setup();
    wifi_loop();
    delay(1000);
  }
  mqttDevice.init();
}

void apply_settings()
{
  Serial.print("temperature=");
  Serial.println(settings.temperature());
  boilerController.set_temp(settings.temperature());
  Serial.print("defaultPreset=");
  Serial.println(settings.defaultPreset());
  Serial.print("P=");
  Serial.println(settings.P());
  Serial.print("I=");
  Serial.println(settings.I());
  Serial.print("D=");
  Serial.println(settings.D());
  Serial.print("ff_heat=");
  Serial.println(settings.ff_heat());
  Serial.print("ff_ready=");
  Serial.println(settings.ff_ready());
  Serial.print("ff_brew=");
  Serial.println(settings.ff_brew());
  boilerController.set_pid(settings.P(), settings.I(), settings.D());
  boilerController.set_ff_heat(settings.ff_heat());
  boilerController.set_ff_ready(settings.ff_ready());
  boilerController.set_ff_brew(settings.ff_brew());

  Serial.print("tareWeight=");
  Serial.println(settings.tareWeight());
  Serial.print("trimWeight=");
  Serial.println(settings.trimWeight());
  reservoir.set_trim(settings.trimWeight());
  reservoir.set_tare(settings.tareWeight());

  Serial.print("presetIndex=");
  Serial.println(presetIndex);

  Serial.print("preInfusionTime=");
  Serial.println(settings.preInfusionTime(presetIndex));
  Serial.print("infuseTime=");
  Serial.println(settings.infusionTime(presetIndex));
  Serial.print("extractTime=");
  Serial.println(settings.extractionTime(presetIndex));
  Serial.print("extractionWeight=");
  Serial.println(settings.extractionWeight(presetIndex));
  brewProcess.preInfuseTime = settings.preInfusionTime(presetIndex);
  brewProcess.infuseTime = settings.infusionTime(presetIndex);
  brewProcess.extractTime = settings.extractionTime(presetIndex);
}


// Output the state to serial port
void print_state()
{
  static unsigned long prev_time = millis();
  if (time_since(prev_time) > 500)
  {
    Serial.print("setpoint:");
    Serial.print(boilerController.set_temp());
    Serial.print(", power:");
    Serial.print(heaterDevice.power());
    Serial.print(", average:");
    Serial.print(heaterDevice.average());
    Serial.print(", act_temp:");
    Serial.print(boilerController.act_temp());
    Serial.print(", boiler-state:");
    Serial.print(boilerController.get_state_name());
    Serial.print(", boiler-error:");
    Serial.print(boilerController.get_error_text());
    Serial.print(", brew-state:");
    Serial.print(brewProcess.get_state_name());
    Serial.print(", weight:");
    Serial.print(brewProcess.weight());
    Serial.print(", end_weight:");
    Serial.print(brewProcess.end_weight());
    Serial.print(", reservoir_level:");
    Serial.print(reservoir.level());

    Serial.println();
    prev_time = millis();
  }
}


// Send the state to MQTT
void send_state()
{
  static unsigned long prev_time = millis();
  if (time_since(prev_time) > 5000)
  {
    mqttDevice.write("t_set", boilerController.set_temp());
    mqttDevice.write("t_act", boilerController.act_temp());
    mqttDevice.write("h_pwr", heaterDevice.power());
    mqttDevice.write("h_avg", heaterDevice.average() );
    mqttDevice.write("r_lvl", reservoir.level() );
    mqttDevice.write("r_wgt", reservoir.weight() );
    mqttDevice.write("w_cur", brewProcess.weight() );
    mqttDevice.write("w_end", brewProcess.end_weight() );
    mqttDevice.write("shots", (long)settings.shotCounter() );

    mqttDevice.write("boil", (char*)boilerController.get_state_name() );
    if ( boilerController.is_error() )
      mqttDevice.write("boil_err", (char*)boilerController.get_error_text() );

    mqttDevice.write("brew", (char*)brewProcess.get_state_name() );
    if ( brewProcess.is_error() )
      mqttDevice.write("brew_err", (char*)brewProcess.get_error_text() );

    if ( reservoir.is_error() )
      mqttDevice.write("res_err", (char*)reservoir.get_error_text() );

    mqttDevice.write("msec", (long)millis() );
    mqttDevice.send();

    prev_time = millis();
  }
}


typedef enum { COMMISSIONING, MAIN, SETTINGS, SLEEP, SAVED, ERROR, INFO, PRESET, PRESET_SETTINGS } menus_t;

/**
 * @brief main process loop
 */
void loop()
{
  static unsigned long timer = 0;
  static unsigned long counter = 0;
  static menus_t menu = COMMISSIONING;

/// BEGIN Test code to simulate heater
#ifdef SIMULATE
  timer += 1;
  if (timer < 150)
    boilerController.set_temp(settings.temperature());
  else
    boilerController.set_temp(20.0);
  if (timer > 300)
    timer = 0;
#endif
  /// END Test code to simulate heater

  heaterDevice.control();
  boilerController.control();
  brewProcess.run();
  int menuSettings;
  int presetMenu;
  int presetMenuSettings;

  send_state();
  mqttDevice.run();

  if (false)
    print_state();

  if ( brewProcess.is_error() )
    menu = ERROR; // error menu

  switch (menu)
  {
  case COMMISSIONING:
    if ( settings.commissioningDone() )
      menu = MAIN;
    else
      menu_commissioning();
    break;
  case MAIN: // main menu
    if ( !settings.commissioningDone() )
      menu = COMMISSIONING;

    counter = 0;
    menu_main();
    if(!brewProcess.is_busy()) {
      if (display.button_long_pressed()) {
        menu = SETTINGS;
      }
      if(!brewSwitch.up()) {
        if (display.button_pressed()) 
          menu = PRESET;
      }
    }
    if( display.encoder_changed() )
      menu = INFO;
    break;
  case SETTINGS: // settings menu
    if ( !settings.commissioningDone() )
      menu = COMMISSIONING;

    if (brewProcess.is_busy())
      menu = MAIN; // When brewing: Always show main menu

    menuSettings = menu_settings();
    if (menuSettings == 1)
    {
      Serial.println("Done!");
      boilerController.clear_error();
      reservoir.clear_error();
      apply_settings();
      display.button_pressed(); // prevent entering settings again
      menu = SAVED;
    } else if (menuSettings == 2)
    {
      Serial.println("Cancel!");
      display.button_pressed();
      display.encoder_changed();
      menu = MAIN;
    }
    break;
  case SLEEP: // sleep menu
    menu_sleep();

    if (!brewProcess.is_awake())
    {
      menu = MAIN;
      display.button_pressed(); // consume button pressed event, prevent jump to settings menu
      display.encoder_changed();
    }
    break;
  case SAVED: // saved menu
    menu_saved();
    display.button_pressed();
    display.encoder_changed();
    if (counter++ > 5)
      menu = MAIN;
    break;
  case ERROR: // error menu
    menu_error("ERROR");
    boilerController.off();
    pumpDevice.off();
    if (display.button_pressed())
    {
      boilerController.clear_error();
      reservoir.clear_error();
      brewProcess.clear_error();
      menu = MAIN;
    }
    break;
  case INFO: // state info menu
    menu_state();
    if(!brewProcess.is_busy()) {
      if (display.button_long_pressed()) {
        menu = SETTINGS;
      }
      if(!brewSwitch.up()) {
        if (display.button_pressed()) 
          menu = PRESET;
      }
    }
    if( display.encoder_changed() )
      menu = MAIN;
    break;
  case PRESET:
    if ( !settings.commissioningDone() )
      menu = COMMISSIONING;

    if (brewProcess.is_busy())
      menu = MAIN; 
    menuSettings = menu_preset();
    if( menuSettings == 1)
    {
      display.button_pressed();
      display.encoder_changed();
      apply_settings();
      menu = SAVED;
    }
    if(menuSettings == 2) 
    {

      menu = PRESET_SETTINGS;
    }
    break;
  case PRESET_SETTINGS:
    if (brewProcess.is_busy())
      menu = MAIN; // When brewing: Always show main menu
    presetMenuSettings = menu_preset_settings();
    if (presetMenuSettings == 1)
    {
      apply_settings();
      menu = SAVED;
    }
    if(presetMenuSettings==2)
    {
      display.button_pressed();
      display.encoder_changed();
      menu = MAIN;
    }
    break;
  default:
    menu = MAIN;
  }

  // sleep deactivation and menu selection (note: sleep can be activated automatically)
  if (display.button_long_pressed())
  {
    if (!brewProcess.is_awake())
      brewProcess.wakeup();
  }
  if (!brewProcess.is_awake())
    menu = SLEEP;
}


#ifdef TEST_CODE
void test_heater_loop()
{
  static double delay_time = 10000.0;
  static double power = 0.0;
  // brewProcess.run();
  power += delay_time / 100000.0;
  if (power > 150.0)
    power = -50.0;
  heaterDevice.power(power);
  heaterDevice.on();
  heaterDevice.off();

  statusLed.color(heaterDevice.is_on() ? ColorLed::RED : ColorLed::BLUE);

  delayMicroseconds(delay_time);
  print_state();
}
#endif

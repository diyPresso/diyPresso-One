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

#include "dp_hardware.h"
#include "dp_led.h"
#include "dp_settings.h"
#include "dp_encoder.h"
#include "dp_boiler.h"
#include "dp_reservoir.h"
#include "dp_display.h"
#include "dp_menu.h"
#include "dp_brew.h"
#include "dp_heater.h"
#include "dp_pump.h"

#include "dp_wifi.h"

static bool wifi_enabled = false;

/**
 * @brief setup code
 * initialize all objects and state
 */
void setup()
{
  int result=0;
  Serial.begin(115200);
  delay(1000);
  Serial.println(__DATE__ " " __TIME__);
  statusLed.color(ColorLed::WHITE);

  encoder.start();
  display.init();
  display.logo(__DATE__, __TIME__);

  if ( (result=settings.load()) < 0)
  {
    Serial.println("Failed to load settings, result="); Serial.println(result);
    Serial.print("Save default settings, result=");
    Serial.println( settings.save() );
  }
  else
    Serial.println("Load settings OK, result="); Serial.println(result);

  Serial.println(encoder.button_count());
  if ( encoder.button_count() > 3 )
  {
    Serial.println("button pressed 4x at startup: perform factory reset of settings");
    settings.defaults();
    Serial.println( settings.save() );
  }

  apply_settings();

  display.custom_chars(custom_chars_spinner);

  Serial.println("INIT DONE");
  heaterDevice.pwm_period(2.0); // [sec]
  boilerController.off();

  if ( wifi_enabled )
  {
    if ( settings.wifiMode() == WIFI_MODE_AP)
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

}

void apply_settings()
{
  Serial.print("temperature="); Serial.println( settings.temperature() );
  boilerController.set_temp( settings.temperature() );

  Serial.print("P="); Serial.println( settings.P() );
  Serial.print("I="); Serial.println( settings.I() );
  Serial.print("D="); Serial.println( settings.D() );
  boilerController.set_pid( settings.P(), settings.I(), settings.D() );

  Serial.print("tareWeight="); Serial.println( settings.tareWeight() );
  Serial.print("trimWeight="); Serial.println( settings.trimWeight() );
  reservoir.set_trim( settings.trimWeight() );
  reservoir.set_tare( settings.tareWeight() );

  Serial.print("preInfusionTime="); Serial.println( settings.preInfusionTime() );
  Serial.print("infuseTime="); Serial.println( settings.infusionTime() );
  Serial.print("extractTime="); Serial.println( settings.extractionTime() );
  Serial.print("extractionWeight="); Serial.println( settings.extractionWeight() );
  brewProcess.preInfuseTime=settings.preInfusionTime();
  brewProcess.infuseTime=settings.infusionTime();
  brewProcess.extractTime=settings.extractionTime();
  wifi_enabled = settings.wifiMode() != WIFI_MODE_OFF;
}

// Output the state to serial port
void print_state()
{
  static unsigned long prev_time = millis();
  if ( time_since(prev_time) > 500)
  {
    Serial.print("setpoint:"); Serial.print(boilerController.set_temp());
    Serial.print(", power:");  Serial.print(heaterDevice.power());
    Serial.print(", average:"); Serial.print(heaterDevice.average());
    Serial.print(", act_temp:"); Serial.print(boilerController.act_temp());
    Serial.print(", boiler-state:"); Serial.print(boilerController.get_state_name());
    Serial.print(", boiler-error:"); Serial.print(boilerController.get_error_text());
    Serial.print(", brew-state:"); Serial.print(brewProcess.get_state_name());
    Serial.print(", weight:"); Serial.print(brewProcess.weight());
    Serial.print(", end_weight:"); Serial.print(brewProcess.end_weight());
    Serial.println();
    prev_time = millis();
  }

}

// Test boiler loop
void loop()
{
  static unsigned long timer=0;
  static unsigned long counter=0;
  static unsigned menu=0;

/// BEGIN Test code to simulate heater
#ifdef SIMULATE
  timer += 1;
  if ( timer < 150 )
      boilerController.set_temp( settings.temperature() );
  else
      boilerController.set_temp( 20.0 );
  if ( timer > 300 )
    timer = 0;
#endif
/// END Test code to simulate heater

  heaterDevice.control();
  boilerController.control();
  brewProcess.run();
  print_state();

  switch ( menu )
  {
    case 0: // main menu
      counter = 0;
      menu_main();
      if ( display.encoder_changed() )
        menu = 1;
      break;
    case 1:
      if ( menu_settings() )
      {
        Serial.println("Done!");
        boilerController.clear_error();
        apply_settings();
        settings.save();
        menu = 3;
      }
      break;
    case 2: // sleep menu
      menu_sleep();
      if ( !brewProcess.is_awake() )
        menu = 0;
      break;
    case 3: // saved menu
      menu_saved();
      display.button_pressed();
      if ( counter++ > 10 )
        menu = 0;
      break;
    case 4: // error menu
      menu_error("ERROR");
      boilerController.off();
      pumpDevice.off();
      break;
    default:
      menu = 0;
  }

  // sleep (de)activation and menu selection (note: sleep can be activated automatically)
   if (display.button_long_pressed())
      {
        if ( brewProcess.is_awake() )
          brewProcess.sleep();
        else
          brewProcess.wakeup();
      }
  if ( !brewProcess.is_awake() )
    menu = 2;

}

#ifdef TEST_CODE
void test_heater_loop()
{
  static double delay_time = 10000.0;
  static double power=0.0;
  // brewProcess.run();
  power += delay_time/100000.0;
  if ( power > 150.0) power = -50.0;
  heaterDevice.power(power);
  heaterDevice.on();
  heaterDevice.off();

  statusLed.color( heaterDevice.is_on() ? ColorLed::RED : ColorLed::BLUE);

  delayMicroseconds(delay_time);
  print_state();
}
#endif




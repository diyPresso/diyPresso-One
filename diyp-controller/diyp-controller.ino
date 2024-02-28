/*
    diyPresso main controller
    (c) 2024 - diyPresso

    Used Libraries:
    Timer v1.2.1 - stefan Staub

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

  display.init();
  display.logo();

  if ( (result=settings.load()) < 0)
  {
    Serial.println("Failed to load settings, result="); Serial.println(result);
    Serial.print("Save default settings, result=");
    Serial.println( settings.save() );
  }
  else
    Serial.println("Load settings OK, result="); Serial.println(result);

  Serial.println( settings.temperature() );
  Serial.println( settings.P() );
  Serial.println( settings.I() );
  Serial.println( settings.D() );

  boilerController.set_pid( settings.P(), settings.I(), settings.D() );
  boilerController.set_temp( settings.temperature() );

  display.custom_chars(custom_chars_spinner);
  //plotMainMenu();   // LCD main menu after 3 seconds
  encoder.start();
  Serial.println("INIT DONE");
  // display_settings();
  heaterDevice.pwm_period(0.5); // [sec]
  boilerController.maintain();
  boilerController.set_temp( settings.temperature() );
}


// Output the state to serial port
void print_state()
{
  static unsigned long prev_time = millis();
  if ( time_since(prev_time) > 500)
  {
    Serial.print("setpoint:");
    Serial.print(boilerController.set_temp());
    Serial.print(", power:");
    Serial.print(heaterDevice.power());
    Serial.print(", average:");
    Serial.print(heaterDevice.average());
    Serial.print(", act_temp:");
    Serial.print(boilerController.act_temp());
    Serial.print(", error:");
    Serial.println(boiler_error_text[boilerController.error()]);
    prev_time = millis();
  }
}


// Test boiler loop
void loop()
{
  static unsigned long timer=0;
  static unsigned menu=0;

  timer += 1;
  if ( timer < 150 )
      boilerController.set_temp( settings.temperature() );
  else
      boilerController.set_temp( 20.0 );
  if ( timer > 300 )
    timer = 0;
  heaterDevice.control();
  boilerController.control();
  brewProcess.run();
  //print_state();

  switch ( menu )
  {
    case 0:
      menu_main();
      if ( display.encoder_changed() )
        menu +=1;
      break;
    case 1:
      display_settings(); // blocking!
      display.encoder_changed();
      Serial.println("Done!");
      brewProcess.preInfuseTime=settings.preInfusionTime();
      brewProcess.infuseTime=settings.infusionTime();
      brewProcess.extractTime=settings.extractionTime();
      settings.save();
      menu = 0;
      break;
    default:
      menu = 0;
  }

}



void test_heater_loop()
{
  static double delay_time = 10000.0;
  static double power=0.0;
  // brewProcess.run();
  power += delay_time/100000.0;
  if ( power > 150.0) power = -50.0;
  heaterDevice.power(power);
  //heaterDevice.on();
  //heaterDevice.off();

  statusLed.color( heaterDevice.is_on() ? ColorLed::RED : ColorLed::BLUE);

  delayMicroseconds(delay_time);
  print_state();
}





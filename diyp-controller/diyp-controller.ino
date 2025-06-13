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
      * thermistor -- Adafruit MAX31865 PT1000 sensor, using MAX31865_NonBlocking libary for non-blocking continues read-out
      * heaterControl -- PWM Control of the heater output

    * reservoir - The water reservoir with weight scale
      * weight(), tarre(), level(), empty()
      * hx711

*/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Timer.h>

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

#include "dp_serial.h"
#include "dp_wifi.h"
#include "dp_mqtt.h"


/**
 * @brief setup code
 * initialize all objects and state
 */
void setup()
{
  int result = 0;
  
  delay(1000);
  dpSerial.send(__DATE__ " " __TIME__);
  statusLed.color(ColorLed::WHITE);

  encoder.start();
  display.init();
  display.logo(__DATE__, __TIME__);

  if ((result = settings.load()) < 0)
  {
    dpSerial.send("Failed to load settings, result=");
    dpSerial.send(result);
    Serial.print("Save default settings, result=");
    dpSerial.send(settings.save());
  }
  else
    dpSerial.send("Load settings OK, result=");
  dpSerial.send(result); 

  dpSerial.send(encoder.button_count());
  if (encoder.button_count() > 3)
  {
    dpSerial.send("button pressed 4x at startup: perform factory reset of settings");
    settings.defaults();
    dpSerial.send(settings.save());
  }

  dpSerial.send_settings();

  display.custom_chars(custom_chars_spinner);

  boilerController.init(); // moved this out of the constructor, because the arduino just bricked if called earlier. Not sure why though...

  settings.apply();

  dpSerial.send("INIT DONE");
  
  heaterDevice.pwm_period(1.0); // [sec]
  boilerController.off();

  if (settings.wifiMode() != WIFI_MODE_OFF)
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

    dpSerial.send("");
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
    mqttDevice.write("h_avg", heaterDevice.average());
    mqttDevice.write("r_lvl", reservoir.level());
    mqttDevice.write("r_wgt", reservoir.weight());
    mqttDevice.write("w_cur", brewProcess.weight());
    mqttDevice.write("w_end", brewProcess.end_weight());
    mqttDevice.write("shots", (long)settings.shotCounter());

    mqttDevice.write("boil", (char *)boilerController.get_state_name());
    if (boilerController.is_error())
      mqttDevice.write("boil_err", (char *)boilerController.get_error_text());

    mqttDevice.write("brew", (char *)brewProcess.get_state_name());
    if (brewProcess.is_error())
      mqttDevice.write("brew_err", (char *)brewProcess.get_error_text());

    if (reservoir.is_error())
      mqttDevice.write("res_err", (char *)reservoir.get_error_text());

    mqttDevice.write("msec", (long)millis());
    mqttDevice.send();

    prev_time = millis();
  }
}

typedef enum
{
  COMMISSIONING,
  MAIN,
  SETTINGS,
  SLEEP,
  SAVED,
  ERROR,
  INFO,
  WARNING_ALMOST_EMPTY
} menus_t;



// #define LOOP_COUNT_TEST
// #define LOOP_TIMERS // To monitor the performance of the main loop

/**
 * @brief main process loop
 */
void loop()
{
  #ifdef LOOP_COUNT_TEST
    static unsigned long loopCounter = 0;
    static unsigned long lastTime = millis();
    unsigned long now = millis();

    loopCounter++;
    if (now - lastTime > 1000)
    {
      dpSerial.send("Loop counter: " + String(loopCounter) + " time elapsed: " + String(now - lastTime) + "ms");
      loopCounter = 0;
      lastTime = now;
    }
  #endif
  
  #ifdef LOOP_TIMERS
    unsigned long tstart = millis();

    unsigned long t1, t2, t3, t4;
  #endif

  static Timer menu_saved_timer = Timer(MILLIS);
  static menus_t menu = COMMISSIONING;

  bool button_pressed = display.button_pressed();



/// BEGIN Test code to simulate heater
#ifdef SIMULATE
  static unsigned long timer = 0;
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

  brewProcess.run((button_pressed ? BrewProcess::MSG_BUTTON : BrewProcess::MSG_NONE));
  int menuSettings;

  dpSerial.receive(); // check for incoming serial commands

  send_state();
  mqttDevice.run();

  #ifdef LOOP_TIMERS
    t1 = millis();
  #endif
  

  if (false) // TODO: turn ON.
    print_state();


  if (brewProcess.is_error())
    menu = ERROR; // error menu

  switch (menu)
  {
  case COMMISSIONING:
    if (settings.commissioningDone())
      menu = MAIN;
    else
      menu_commissioning();
    break;
  case MAIN: // main menu
    if (!settings.commissioningDone()) {
      menu = COMMISSIONING;
      break;
    } else if (brewProcess.is_warning_almost_empty()) {
      menu = WARNING_ALMOST_EMPTY;
    }

#ifdef LOOP_TIMERS
    t2 = millis();
#endif
    menu_main();
#ifdef LOOP_TIMERS
    t3 = millis();
#endif

    if (button_pressed) {
      menu = SETTINGS;
    } else if (display.encoder_changed()) {
      menu = INFO;
    }
    break;
  case SETTINGS: // settings menu
    if (!settings.commissioningDone())
      menu = COMMISSIONING;

    if (brewProcess.is_busy())
      menu = MAIN; // When brewing: Always show main menu

    menuSettings = menu_settings(button_pressed);
    if (menuSettings == 1)
    {
      boilerController.clear_error();
      reservoir.clear_error();
      settings.apply();
      display.button_pressed(); // prevent entering settings again
      menu = SAVED;
    }
    else if (menuSettings == 2)
    {
      dpSerial.send("Cancel!");
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

    if (menu_saved_timer.state() != status_t::RUNNING) {
      menu_saved_timer.start(); // start timer if not running
    } else if (menu_saved_timer.read() > 1000) {
      menu_saved_timer.stop(); // stop timer when time is up (1s) and go back to main menu
      menu = MAIN;
    }
    break;
  case ERROR: // error menu
    menu_error("ERROR");
    boilerController.off();
    pumpDevice.off();
    if (button_pressed)
    {
      boilerController.clear_error();
      reservoir.clear_error();
      brewProcess.clear_error();
      menu = MAIN;
    }
    break;
  case INFO: // state info menu
    menu_state();
    if (button_pressed)
      menu = SETTINGS;
    if (display.encoder_changed())
      menu = MAIN;
    break;
  case WARNING_ALMOST_EMPTY: // warning menu

    if (brewProcess.is_warning_almost_empty())
      menu_warning_almost_empty();
    else
      menu = MAIN; // go back to main menu if not almost empty anymore
    break;
  default:
    menu = MAIN;
  }

#ifdef LOOP_TIMERS
  t4 = millis();
#endif


  // sleep (de)activation and menu selection (note: sleep can be activated automatically)
  if (display.button_long_pressed())
  {
    if (brewProcess.is_awake())
      brewProcess.sleep();
    else
      brewProcess.wakeup();
  }
  if (!brewProcess.is_awake())
    menu = SLEEP;

  #ifdef LOOP_TIMERS
    unsigned long tend = millis();

    dpSerial.send("loop: " + String(tend - tstart) + "ms, t0: " + String(t1 - tstart) + "ms, t1: " + String(t2 - t1) + "ms, t2: " + String(t3 - t2) + "ms, t3: " + String(t4 - t3) + "ms, t4: " + String(tend - t4) + "ms");
  #endif
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

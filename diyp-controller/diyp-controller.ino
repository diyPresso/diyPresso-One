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
#include "dp_machine.h"

#include "dp_serial.h"
#include "dp_wifi.h"
#include "dp_mqtt.h"
#include "dp_commission.h"

#include "dp_steam_process.h"
#include "dp_steam_thermoblock.h"
#include "dp_steam_switch.h"
#include "dp_steam_heater.h"
#include "dp_steam_pump.h"

/**
 * @brief setup code
 * initialize all objects and state
 */
void setup()
{
  int result = 0;

  delay(1000);
  hardware.detect_model();
  dpSerial.send(__DATE__ " " __TIME__);
  dpSerial.send("diyPresso " + String(hardware.model_name()) + " starting up...");
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
  if(hardware.has_steam_group()) 
  { 
    steamThermoblock.init();
    steamPump.init();
    steamProcess.init();
  }


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

void loop_count()
{
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
}

void simulate_heater()
{
  static unsigned long timer = 0;
  timer += 1;
  if (timer < 150)
    boilerController.set_temp(settings.temperature());
  else
    boilerController.set_temp(20.0);
  if (timer > 300)
    timer = 0;
}

/**
 * @brief main process loop
 * Structured as: 1) Read inputs  2) Run FSMs  3) Update outputs
 */
void loop()
{
  #ifdef LOOP_COUNT
    loop_count();
  #endif

// --- 1. READ INPUTS --- //
  bool button_pressed = display.button_pressed();
  bool long_pressed = display.button_long_pressed();
  if (hardware.has_steam_group()) steamSwitch.read();  
  // if (steamSwitch.pressed()) dpSerial.send("Steam button pressed, steam process state: " + String(steamProcess.get_state_name())); 
  // if (steamSwitch.long_pressed()) dpSerial.send("Steam button long pressed");
  
  dpSerial.receive();

  boilerController.read_sensor(); // read boiler temperature sensor
  if (hardware.has_steam_group()) steamThermoblock.read_sensor(); // read steam thermoblock temperature sensor
  
  reservoir.read();               // read load cell

  #ifdef SIMULATE
    simulate_heater();
  #endif

// --- 2. RUN FSMs --- //
  machineController.run(long_pressed ? MachineController::MSG_LONG_PRESS
                      : button_pressed ? MachineController::MSG_BUTTON
                      : steamSwitch.long_pressed() ? MachineController::MSG_STEAM_LONG_PRESS
                      : steamSwitch.pressed() ? MachineController::MSG_STEAM_BUTTON
                      : MachineController::MSG_NONE);
  
  boilerController.run();
  if (hardware.has_steam_group()) steamThermoblock.run();

// --- 3. UPDATE OUTPUTS --- //
  heaterDevice.control(); // drive heater PWM
  if (hardware.has_steam_group()) steamHeater.control();
  if (hardware.has_steam_group()) steamPump.control();
  update_display(button_pressed);
  dpSerial.print_state();
  mqttDevice.send_state();
  mqttDevice.run();
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

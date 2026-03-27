/*
  boiler.cpp
  diyPresso Boiler control
  (c) 2024 DiyEspresso - PBRI - CC-BY-NC
 */
#include "dp.h"
#include "dp_hardware.h"
#include "dp_boiler.h"
#include "dp_heater.h"
#include "dp_settings.h"
#include "dp_brew.h"
//#include <Adafruit_MAX31865.h>

#ifdef WATCHDOG_ENABLED
#include <wdt_samd21.h>
#endif

BoilerStateMachine boilerController = BoilerStateMachine();

void BoilerStateMachine::state_off()
{
  ON_ENTRY() {}
  if (_on)
    next(&BoilerStateMachine::state_heating);
}

void BoilerStateMachine::state_heating()
{
  ON_ENTRY()
  {
    _pid.setFeedForward(_ffHeat, false);
  }
  if (!_on)
    next(&BoilerStateMachine::state_off);
  if (_brew)
    next(&BoilerStateMachine::state_brew);
  if (abs(_set_temp - _act_temp) < TEMP_WINDOW)
    next(&BoilerStateMachine::state_ready);
  ON_TIMEOUT_SEC(TIMEOUT_HEATING)
  goto_error(BOILER_ERROR_TIMEOUT_HEATING);
  ON_EXIT()
  {
    _pid.setFeedForward(0, false);
  }
}

void BoilerStateMachine::state_ready()
{
  ON_ENTRY()
  {
    _pid.setFeedForward(_ffReady, false);
  }
  if (!_on)
    next(&BoilerStateMachine::state_off);
  if (_brew)
    next(&BoilerStateMachine::state_brew);
  if (abs(_set_temp - _act_temp) > TEMP_WINDOW)
    next(&BoilerStateMachine::state_heating);
  ON_TIMEOUT_SEC(TIMEOUT_READY)
  goto_error(BOILER_ERROR_READY_TIMEOUT);
}

void BoilerStateMachine::state_brew()
{
  if (!_on)
    next(&BoilerStateMachine::state_off);
  if (!_brew)
    next(&BoilerStateMachine::state_heating);
  ON_ENTRY()
  {
    _pid.setFeedForward(_ffReady, true);
  }

  // if ( (_set_temp - _act_temp ) > TEMP_WINDOW) goto_error(BOILER_ERROR_UNDER_TEMP);
  ON_TIMEOUT_SEC(TIMEOUT_BREW)
  goto_error(BOILER_ERROR_TIMEOUT_BREW);
  ON_EXIT()
  {
    _pid.setFeedForward(0, false);
    _brew = false;
  }
}

void BoilerStateMachine::state_error()
{
  ON_ENTRY() {}
  off();
  _power = 0;
  _set_temp = 0;
  if (_error == BOILER_ERROR_NONE)
    next(&BoilerStateMachine::state_off);
}

void BoilerStateMachine::goto_error(boiler_error_t error)
{
  _error = error;
  next(&BoilerStateMachine::state_error);
}

void BoilerStateMachine::init()
{
  _pid.begin(&_act_temp, &_power, &_set_temp, settings.P(), settings.I(), settings.D(), settings.ffReady(), false, 1000, &reservoir); // get defaults from setting and set PID sample time to 1s (same as HeaterDevice)
  _pid.setOutputLimits(0, 100);
  _pid.setWindUpLimits(WINDUP_LIMIT_MIN, WINDUP_LIMIT_MAX); // set bounds for the integral term to prevent integral wind-up
  _pid.start();

  begin();  // start the thermistor.
  delay(500); // wait for the thermistor to start up. TODO: wait in loop?
  _error = BOILER_ERROR_NONE;
  _rtd_error = 0;
  _on = true;
  _last_control_time = millis();
#ifdef WATCHDOG_ENABLED
  wdt_init(WDT_CONFIG_PER_16K);
#endif
}

void BoilerStateMachine::begin()
{
  thermistor.begin(MAX31865::RTD_2WIRE, MAX31865::FILTER_50HZ, MAX31865:: CONV_MODE_CONTINUOUS); // set to 2WIRE, default filter and continuous conversion mode.
}



void BoilerStateMachine::read_sensor(void)
{
  double raw_temp = thermistor.getTemperature(RNOMINAL, RREF);

#ifdef SIMULATE
  raw_temp = heaterDevice.average(); // hack for testing, read average power as actual temperature
#endif

  // Apply Exponential Moving Average filter to filter out noise and smooth the temperature readings.
  if (!_temp_initialized) {
    _act_temp = raw_temp;
    _temp_initialized = true;
  } else {
    _act_temp = (TEMP_FILTER_ALPHA * raw_temp) + ((1.0 - TEMP_FILTER_ALPHA) * _act_temp);
  }

  _rtd_error = thermistor.getFault();
  if (_rtd_error)
  {
    thermistor.clearFault();
    goto_error(BOILER_ERROR_RTD);
  }
  else
  {
    if (_act_temp > TEMP_LIMIT_HIGH)
      goto_error(BOILER_ERROR_OVER_TEMP);
    if (_act_temp < TEMP_LIMIT_LOW)
      goto_error(BOILER_ERROR_RTD);
  }
}

void BoilerStateMachine::run(void)
{
  if (_on && _last_control_time + TIMEOUT_CONTROL_MSEC < millis())
    goto_error(BOILER_ERROR_CONTROL_TIMEOUT);
  _last_control_time = millis();

  StateMachine::run();

  _pid.compute();

  if (_power_control_mode == POWER_CONTROL_STATIC) {
    _power = _power_static;
  }

  if (_act_temp > (TEMP_LIMIT_HIGH + 2.0))
    _power = 0;

  heaterDevice.power(_on ? _power : 0.0);
#ifdef WATCHDOG_ENABLED
  wdt_reset();
#endif

}

const char *BoilerStateMachine::get_error_text()
{
  switch (_error)
  {
  case BOILER_ERROR_NONE:
    return "OK";
  case BOILER_ERROR_OVER_TEMP:
    return "OVER_TEMP";
  case BOILER_ERROR_UNDER_TEMP:
    return "UNDER_TEMP";
  case BOILER_ERROR_RTD:
    return "RTD_ERROR";
  case BOILER_ERROR_SSR_TIMEOUT:
    return "SSR_TIMEOUT";
  case BOILER_ERROR_TIMEOUT_BREW:
    return "BREW_TIMEOUT";
  case BOILER_ERROR_CONTROL_TIMEOUT:
    return "CONTROL_TIMEOUT";
  case BOILER_ERROR_READY_TIMEOUT:
    return "READY_TIMEOUT";
  case BOILER_ERROR_TIMEOUT_HEATING:
    return "TIMEOUT_HEATING";
  default:
    return "UNKNOWN";
  }
}

bool BoilerStateMachine::set_power_control_mode_str(String mode)
{
  mode.trim();
  mode.toUpperCase();
  
  if (mode == "PID") {
    _power_control_mode = POWER_CONTROL_PID;
    return true;
  } else if (mode == "STATIC") {
    _power_control_mode = POWER_CONTROL_STATIC;
    return true;
  }
  return false; // Invalid mode
}

String BoilerStateMachine::get_power_control_mode_str() const
{
  switch (_power_control_mode) {
    case POWER_CONTROL_PID:
      return "PID";
    case POWER_CONTROL_STATIC:
      return "STATIC";
    default:
      return "UNKNOWN";
  }
}
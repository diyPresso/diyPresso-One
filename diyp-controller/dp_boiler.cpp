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

//#include <Adafruit_MAX31865.h>

#ifdef WATCHDOG_ENABLED
#include <wdt_samd21.h>
#endif

BoilerStateMachine boilerController = BoilerStateMachine();

void BoilerStateMachine::state_off()
{
  ;
  if (_on)
    NEXT(state_heating);
}

void BoilerStateMachine::state_heating()
{
  ON_ENTRY()
  {
    _pid.setFeedForward(_ff_heat);
  }
  if (!_on)
    NEXT(state_off);
  if (_brew)
    NEXT(state_brew);
  if (abs(_set_temp - _act_temp) < TEMP_WINDOW)
    NEXT(state_ready);
  ON_TIMEOUT_SEC(TIMEOUT_HEATING)
  goto_error(BOILER_ERROR_TIMEOUT_HEATING);
  ON_EXIT()
  {
    _pid.setFeedForward(0);
  }
}

void BoilerStateMachine::state_ready()
{
  ON_ENTRY()
  {
    _pid.setFeedForward(_ff_ready);
  }
  if (!_on)
    NEXT(state_off);
  if (_brew)
    NEXT(state_brew);
  if (abs(_set_temp - _act_temp) > TEMP_WINDOW)
    NEXT(state_heating);
  ON_TIMEOUT_SEC(TIMEOUT_READY)
  goto_error(BOILER_ERROR_READY_TIMEOUT);
}

void BoilerStateMachine::state_brew()
{
  if (!_on)
    NEXT(state_off);
  if (!_brew)
    NEXT(state_heating);
  ON_ENTRY()
  {
    _pid.setFeedForward(_ff_brew);
  }

  // if ( (_set_temp - _act_temp ) > TEMP_WINDOW) goto_error(BOILER_ERROR_UNDER_TEMP);
  ON_TIMEOUT_SEC(TIMEOUT_BREW)
  goto_error(BOILER_ERROR_TIMEOUT_BREW);
  ON_EXIT()
  {
    _pid.setFeedForward(0);
    _brew = false;
  }
}

void BoilerStateMachine::state_error()
{
  off();
  _power = 0;
  _set_temp = 0;
  if (_error == BOILER_ERROR_NONE)
    NEXT(state_off);
}

void BoilerStateMachine::goto_error(boiler_error_t error)
{
  _error = error;
  NEXT(state_error);
}

void BoilerStateMachine::init()
{
  _pid.begin(&_act_temp, &_power, &_set_temp, settings.P(), settings.I(), settings.D(), settings.ff_ready(), 1000); // get defaults from setting and set PID sample time to 1s (same as HeaterDevice)
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



void BoilerStateMachine::control(void)
{

  //unsigned long start_time = millis();
  //_act_temp = thermistor.temperature(RNOMINAL, RREF);
  _act_temp = thermistor.getTemperature(RNOMINAL, RREF);

#ifdef SIMULATE
  _act_temp = heaterDevice.average(); // hack for testing, read average power as actual temperature
#endif

  //_rtd_error = thermistor.readFault();
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

  if (_on && _last_control_time + TIMEOUT_CONTROL_MSEC < millis())
    goto_error(BOILER_ERROR_CONTROL_TIMEOUT);
  _last_control_time = millis();

  run();

  _pid.compute();

  // char buffer[10];
  // Serial.print("Diff: ");
  // snprintf(buffer, sizeof(buffer), "%6.1f", _power2 - _power);
  // Serial.print(buffer);
  // Serial.print("PID1: ");
  // snprintf(buffer, sizeof(buffer), "%6.1f", _power);
  // Serial.print(buffer);
  // Serial.print(" PID2: ");
  // snprintf(buffer, sizeof(buffer), "%6.1f", _power2);
  // Serial.print(buffer);
  // Serial.print(" Temp: ");
  // Serial.print(_act_temp);
  // Serial.print("/");
  // Serial.print(_set_temp);
  // Serial.print(" P: ");
  // Serial.print(_pid.P());
  // Serial.print("/");
  // Serial.print(_pid2.P());
  // Serial.print(" I: ");
  // Serial.print(_pid.I());
  // Serial.print("/");
  // Serial.print(_pid2.I());
  // Serial.print(" D: ");
  // Serial.print(_pid.D());
  // Serial.print("/");
  // Serial.println(_pid2.D());

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

const char *BoilerStateMachine::get_state_name()
{
  RETURN_STATE_NAME(off);
  RETURN_STATE_NAME(heating);
  RETURN_STATE_NAME(ready);
  RETURN_STATE_NAME(brew);
  RETURN_STATE_NAME(error);
  RETURN_NONE_STATE_NAME()
  RETURN_UNKNOWN_STATE_NAME();
}
/*
 * dp_steam_thermoblock.cpp
 * Steam Thermoblock temperature controller (Model Two only)
 * (c) 2026 diyPresso
 */
#include "dp.h"
#include "dp_hardware.h"
#include "dp_steam_thermoblock.h"
#include "dp_steam_heater.h"
#include "dp_settings.h"

SteamThermoblock steamThermoblock = SteamThermoblock();

void SteamThermoblock::init()
{
  steamHeater.init();

  _pid.begin(&_act_temp, &_power, &_set_temp, 0.0, 0.0, 0.0, 3.0, false, 1000, nullptr); // overridden by settings.apply()
  _pid.setOutputLimits(0, 100);
  _pid.setWindUpLimits(-5, 5); //TODO: adjust windup limits
  _pid.setSerialLabel("DP_STEAM_STATE");
  _pid.start();

  _thermistor.begin(MAX31865::RTD_2WIRE, MAX31865::FILTER_50HZ, MAX31865::CONV_MODE_CONTINUOUS);
  delay(500);

  _error = STEAM_THERMO_ERROR_NONE;
  _rtd_error = 0;
  _on = false;
  _last_control_time = millis();
}

void SteamThermoblock::read_sensor()
{
  double raw_temp = _thermistor.getTemperature(RNOMINAL, RREF);

  if (!_temp_initialized) {
    _act_temp = raw_temp;
    _temp_initialized = true;
  } else {
    _act_temp = (TEMP_FILTER_ALPHA * raw_temp) + ((1.0 - TEMP_FILTER_ALPHA) * _act_temp);
  }

  _rtd_error = _thermistor.getFault();
  if (_rtd_error)
  {
    _thermistor.clearFault();
    goto_error(STEAM_THERMO_ERROR_RTD);
  }
  else
  {
    if (_act_temp > STEAM_TEMP_LIMIT_HIGH)
      goto_error(STEAM_THERMO_ERROR_OVER_TEMP);
    if (_act_temp < STEAM_TEMP_LIMIT_LOW)
      goto_error(STEAM_THERMO_ERROR_RTD);
  }
}

void SteamThermoblock::run()
{
  if (_on && _last_control_time + STEAM_TIMEOUT_CONTROL_MSEC < millis())
    goto_error(STEAM_THERMO_ERROR_CONTROL_TIMEOUT);
  _last_control_time = millis();

  StateMachine::run();

  _pid.compute();

  if (_act_temp > (STEAM_TEMP_LIMIT_HIGH + 2.0))
    _power = 0;

  double output = _on ? _power : 0.0;
  steamHeater.power(output);
}

void SteamThermoblock::state_off()
{
  ON_ENTRY() {}
  if (_on)
    next(&SteamThermoblock::state_heating);
}

void SteamThermoblock::state_heating()
{
  ON_ENTRY() {}
  if (!_on)
    next(&SteamThermoblock::state_off);
  if (_set_temp - _act_temp < STEAM_TEMP_WINDOW)
    next(&SteamThermoblock::state_ready);
  ON_TIMEOUT_SEC(STEAM_TIMEOUT_HEATING)
    goto_error(STEAM_THERMO_ERROR_TIMEOUT_HEATING);
}

void SteamThermoblock::state_ready()
{
  ON_ENTRY() {}
  if (!_on)
    next(&SteamThermoblock::state_off);
  if (abs(_set_temp - _act_temp) > STEAM_TEMP_WINDOW)
    next(&SteamThermoblock::state_heating);
  ON_TIMEOUT_SEC(STEAM_TIMEOUT_READY)
    goto_error(STEAM_THERMO_ERROR_READY_TIMEOUT);
}

void SteamThermoblock::state_error()
{
  ON_ENTRY() {}
  off();
  _power = 0;
  _set_temp = 0;
  if (_error == STEAM_THERMO_ERROR_NONE)
    next(&SteamThermoblock::state_off);
}

void SteamThermoblock::goto_error(steam_thermo_error_t error)
{
  _error = error;
  next(&SteamThermoblock::state_error);
}

const char *SteamThermoblock::get_error_text()
{
  switch (_error)
  {
  case STEAM_THERMO_ERROR_NONE:          return "OK";
  case STEAM_THERMO_ERROR_RTD:           return "RTD_ERROR";
  case STEAM_THERMO_ERROR_OVER_TEMP:     return "OVER_TEMP";
  case STEAM_THERMO_ERROR_TIMEOUT_HEATING: return "HEAT_TIMEOUT";
  case STEAM_THERMO_ERROR_READY_TIMEOUT: return "READY_TIMEOUT";
  case STEAM_THERMO_ERROR_CONTROL_TIMEOUT: return "CTRL_TIMEOUT";
  default:                               return "UNKNOWN";
  }
}

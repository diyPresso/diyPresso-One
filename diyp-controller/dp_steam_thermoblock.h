/*
 * dp_steam_thermoblock.h
 * Steam Thermoblock temperature controller (Model Two only)
 * (c) 2026 diyPresso
 */
#ifndef STEAM_THERMOBLOCK_H
#define STEAM_THERMOBLOCK_H

#include "dp_hardware.h"
#include "dp_fsm.h"
#include "dp_pid.h"
#include <Arduino.h>
#include <MAX31865_NonBlocking.h>

// Steam temperatures in [degC]
#define STEAM_TEMP_WINDOW 3.0
#define STEAM_TEMP_LIMIT_HIGH 180.0
#define STEAM_TEMP_LIMIT_LOW 1.0
#define STEAM_TEMP_DEFAULT 165.0

#define STEAM_WINDUP_LIMIT_MIN -5.0  // windup limits in %
#define STEAM_WINDUP_LIMIT_MAX 60.0  //
#define STEAM_INTEGRATION_BAND 0.0   // only integrate when error is within this band [degC], when conditional integration is enabled (0 = no band)

// Steam timeouts in [sec]
#define STEAM_TIMEOUT_HEATING (60 * 5)
#define STEAM_TIMEOUT_READY (60 * 30)
#define STEAM_TIMEOUT_CONTROL_MSEC (1000 * 10)

typedef enum
{
  STEAM_THERMO_ERROR_NONE,
  STEAM_THERMO_ERROR_RTD,
  STEAM_THERMO_ERROR_OVER_TEMP,
  STEAM_THERMO_ERROR_TIMEOUT_HEATING,
  STEAM_THERMO_ERROR_READY_TIMEOUT,
  STEAM_THERMO_ERROR_CONTROL_TIMEOUT,
} steam_thermo_error_t;

class SteamThermoblock : public StateMachine<SteamThermoblock>
{
public:
  SteamThermoblock() : StateMachine(&SteamThermoblock::state_off) {};

  void init();
  void read_sensor();
  void run();

  void on() { _on = true; }
  void off() { _on = false; _power = 0; }
  bool is_on() { return _on; }
  bool is_ready() { return in_state(&SteamThermoblock::state_ready); }
  bool is_error() { return in_state(&SteamThermoblock::state_error); }

  double set_temp() { return _set_temp; }
  double set_temp(double temp) { return _set_temp = min((double)STEAM_TEMP_LIMIT_HIGH, max(temp, 0.0)); }
  double act_temp() { return _act_temp; }
  double act_power() { return _power; }

  void clear_error() { _error = STEAM_THERMO_ERROR_NONE; }
  int error() { return _error; }
  const char *get_error_text();

  void set_pid(double p, double i, double d) { _p = p; _i = i; _d = d; }                     // gains while the steam pump is off
  void set_pump_pid(double p, double i, double d) { _pump_p = p; _pump_i = i; _pump_d = d; } // gains while the steam pump is on
  void set_windup_limits(double min, double max) { _pid.setWindUpLimits(min, max); }
  void set_conditional_integration(bool enabled, double bandDegC) { _pid.setConditionalIntegration(enabled, bandDegC); }
  bool get_serial_output() const { return _pid.getSerialOutput(); }
  void set_serial_output(const bool& enabled) { _pid.setSerialOutput(enabled); }

  // Auto-tune methods
  void startAutoTune() { _pid.startAutoTune(); }
  void cancelAutoTune() { _pid.cancelAutoTune(); }
  bool isAutoTuning() const { return _pid.isAutoTuning(); }
  autotune_state_t getAutoTuneState() const { return _pid.getAutoTuneState(); }
  AutoTuneResults getAutoTuneResults() const { return _pid.getAutoTuneResults(); }

private:
  DpPID _pid;
  MAX31865 _thermistor = MAX31865(PIN_STEAM_THERM_CS);

  double _act_temp = 0, _set_temp = STEAM_TEMP_DEFAULT, _power = 0;
  bool _on = false;
  steam_thermo_error_t _error = STEAM_THERMO_ERROR_NONE;
  int _rtd_error = 0;
  unsigned long _last_control_time = 0;
  double _p = 0, _i = 0, _d = 0, _pump_p = 0, _pump_i = 0, _pump_d = 0;
  bool _pump_was_on = false;

  static constexpr double TEMP_FILTER_ALPHA = 0.15;
  bool _temp_initialized = false;

  void goto_error(steam_thermo_error_t err);

  void state_off();
  void state_heating();
  void state_ready();
  void state_error();
};

extern SteamThermoblock steamThermoblock;

#endif // STEAM_THERMOBLOCK_H

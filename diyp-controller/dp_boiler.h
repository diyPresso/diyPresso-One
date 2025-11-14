/* BOILER.h
 * Boiler
 * (c) 2025 diyPresso
 */
#ifndef BOILER_H
#define BOILER_H

#include "dp_hardware.h"

#define _DP_FSM_TYPE BoilerStateMachine // used for the state machine macro NEXT()
#include "dp_fsm.h"
#include "dp_pid.h"
#include "dp_heater.h"
#include "dp_reservoir.h"
#include <Arduino.h>

#include <MAX31865_NonBlocking.h> 


// Temperatures in [degC]
#define TEMP_WINDOW 10.0      // in temperature range
#define TEMP_LIMIT_HIGH 108.0 // > is TOO HGH
#define TEMP_LIMIT_LOW 1.0    // < is TOO LOW
#define TEMP_MIN_BREW 10.0    // do not brew under this temp

#define WINDUP_LIMIT_MIN -7.0 // windup limits in %
#define WINDUP_LIMIT_MAX 7.0  // 

// Times in [msec]
#define TIMEOUT_HEATING (600)    // maximum heater on time: 10 minutes
#define TIMEOUT_BREW (60 * 3)    // maximum brew on time: 3 minutes
#define TIMEOUT_READY (60 * 120) // maximum time in state ready: 2 hour

#define TIMEOUT_CONTROL_MSEC (1000 * 10)    // Max time between control updates [milliseconds]
#define TIMEOUT_HEATER_SSR_MSEC (1000 * 60) // maximum time the SSR is allowed to be ON [milliseconds]

// Various boiler errors
typedef enum
{
  BOILER_ERROR_NONE,
  BOILER_ERROR_RTD,
  BOILER_ERROR_TIMEOUT_BREW,
  BOILER_ERROR_TIMEOUT_HEATING,
  BOILER_ERROR_READY_TIMEOUT,
  BOILER_ERROR_SSR_TIMEOUT,
  BOILER_ERROR_OVER_TEMP,
  BOILER_ERROR_UNDER_TEMP,
  BOILER_ERROR_CONTROL_TIMEOUT,
  BOILER_ERROR_UNKNOWN,
} boiler_error_t;

// Power control modes
typedef enum
{
  POWER_CONTROL_PID,    // PID controller (default)
  POWER_CONTROL_STATIC, // Static power output
} power_control_mode_t;

class BoilerStateMachine : public StateMachine<BoilerStateMachine>
{
public:
  BoilerStateMachine() : StateMachine(&BoilerStateMachine::state_off) {}; // moved intit() out of the constructor, because the arduino just bricked if called earlier. Not sure why though...
  int error() { return _error; }
  void clear_error() { _error = BOILER_ERROR_NONE; }
  double set_temp() { return _set_temp; }
  double set_temp(double temp) { return _set_temp = min(TEMP_LIMIT_HIGH, max(temp, 0.0)); }
  double act_temp() { return _act_temp; }
  double act_power() { return _power; }
  double set_ff_heat(double ff) { return _ff_heat = min(100.0, max(ff, 0.0)); }
  double get_ff_heat(void) { return _ff_heat; }
  double set_ff_ready(double ff) { return _ff_ready = min(100.0, max(ff, 0.0)); }
  double get_ff_ready(void) { return _ff_ready; }
  double set_ff_brew(double ff) { return _ff_brew = min(100.0, max(ff, 0.0)); }
  double get_ff_brew(void) { return _ff_brew; }
  void set_pid(double p, double i, double d) { _pid.setCoefficients(p, i, d); }
  bool get_serial_output() const { return _pid.getSerialOutput(); }
  void set_serial_output(const bool& enabled) { _pid.setSerialOutput(enabled); }

  void set_power_control_mode(power_control_mode_t mode) { _power_control_mode = mode; }
  bool set_power_control_mode_str(String mode);
  power_control_mode_t get_power_control_mode() const { return _power_control_mode; }
  String get_power_control_mode_str() const;
  void reset_power_control_mode() { _power_control_mode = POWER_CONTROL_PID; }

  void set_power_static(double power) { _power_static = min(100.0, max(power, 0.0)); };
  double get_power_static() const { return _power_static; };

  // Auto-tune methods
  void startAutoTune() { return _pid.startAutoTune(); }
  void cancelAutoTune() { _pid.cancelAutoTune(); }
  autotune_state_t getAutoTuneState() const { return _pid.getAutoTuneState(); }
  AutoTuneResults getAutoTuneResults() const { return _pid.getAutoTuneResults(); }
  
  void on() { _on = true; }
  void off()
  {
    _on = false;
    heaterDevice.power(0);
  }
  void start_brew() { _brew = true; }
  void stop_brew() { _brew = false; }
  bool is_on() { return _on; }
  bool is_ready() { return _cur_state == &BoilerStateMachine::state_ready; }
  bool is_error() { return _cur_state == &BoilerStateMachine::state_error; }
  const char *get_error_text();
  const char *get_state_name();
  void control();
  void begin();
  void init(); 


private:
  DpPID _pid;
  double _act_temp = 0, _set_temp = 0, _ff_heat = 0, _ff_ready = 0, _ff_brew = 0, _power = 0;
  bool _on = false, _brew = false;
  power_control_mode_t _power_control_mode = POWER_CONTROL_PID;
  double _power_static = 0.0;
  unsigned long _last_control_time = 0;
  boiler_error_t _error = BOILER_ERROR_NONE;
  int _rtd_error = 0;   // current RTD errors
  void state_off();     // SSR is forced OFF
  void state_heating(); // Temperature control, but not yet on target temperature
  void state_ready();   // temperature control, within range of target temperature
  void state_brew();    // temperature control in brewing mode with feed-forward active
  void state_error();   // heater is forced OFF, error code is set, set state to OFF to clear error
  void goto_error(boiler_error_t err);
  MAX31865 thermistor = MAX31865(PIN_THERM_CS);
};

extern BoilerStateMachine boilerController;

#endif // BOILER_H 
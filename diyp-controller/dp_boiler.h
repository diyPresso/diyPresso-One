// BOILER.h
#ifndef BOILER_H
#define BOILER_H

#define _DP_FSM_TYPE BoilerStateMachine // used for the state machine macro NEXT()
#include "dp_fsm.h"
#include "ArduPID.h"

// Temperatures in [degC]
#define TEMP_WINDOW 2.0 // in temperature range
#define TEMP_LIMIT_HIGH 104.0 // > is TOO HGH
#define TEMP_LIMIT_LOW 1.0 // < is TOO LOW
#define TEMP_MIN_BREW 10.0 // do not brew under this temp

// Times in [msec]
#define TIMEOUT_HEATING (1000*600) // maximum heater on time: 10 minutes
#define TIMEOUT_BREW (1000*60) // maximum brew on time: 1 minutes
#define TIMEOUT_READY (1000*600) // maximum time in state ready: 10 minutes
#define TIMEOUT_HEATER_SSR (1000 * 60) // maximum time the SSR is allowed to be ON
#define TIMEOUT_CONTROL (1000 * 10) // Max time between control updates

// Various boiler errors
typedef enum {
  BOILER_ERROR_NONE, BOILER_ERROR_RTD, BOILER_ERROR_TIMEOUT_BREW, BOILER_ERROR_TIMEOUT_HEATING, BOILER_ERROR_READY_TIMEOUT,
  BOILER_ERROR_SSR_TIMEOUT, BOILER_ERROR_OVER_TEMP, BOILER_ERROR_UNDER_TEMP, BOILER_ERROR_CONTROL_TIMEOUT, BOILER_ERROR_UNKNOWN,
} boiler_error_t;


class BoilerStateMachine : public StateMachine<BoilerStateMachine>
{
  public:
    BoilerStateMachine() : StateMachine( &BoilerStateMachine::state_off ) { init(); };
    int error() { return _error; }
    void clear_error() { _error = BOILER_ERROR_NONE; }
    double set_temp() { return _set_temp; }
    double set_temp(int temp) { return _set_temp = min(TEMP_LIMIT_HIGH, max(temp, 0.0)); }
    double act_temp() { return _act_temp; }
    double act_power() { return _power; } // power in [%]
    double set_ff(double ff) { return _ff = min(100.0, max(ff, 0.0)); } // feedforward in [%]
    double get_ff(void) { return _ff; } // feedforward in [%]
    void set_pid(double p, double i, double d) { _pid.setCoefficients(p, i, d); }
    void on() { _on = true; }
    void off() { _on = false; }
    void start_brew() { _brew = true; }
    void stop_brew() { _brew = false; }
    bool is_on() { return _on; }
    bool is_ready() { return _cur_state == &BoilerStateMachine::state_ready; }
    const char *get_error_text();
    const char *get_state_name();
    void control();
  private:
    ArduPID _pid;
    double _act_temp=0, _set_temp=0, _ff=0, _power=0;
    bool _on = false, _brew = false;
    unsigned long _last_control_time=0;
    boiler_error_t _error = BOILER_ERROR_NONE;
    int _rtd_error = 0;  // current RTD errors
    void state_off();     // SSR is forced OFF
    void state_heating(); // Temperature control, but not yet on target temperature
    void state_ready();   // temperature control, within range of target temperature
    void state_brew();    // temperature control in brewing mode with feed-forward active
    void state_error();   // heater is forced OFF, error code is set, set state to OFF to clear error
    void init();
    void goto_error(boiler_error_t err);
};

extern BoilerStateMachine boilerController;

#endif // BOILER_H
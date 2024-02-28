// BOILER.h
#ifndef BOILER_H
#define BOILER_H
#include "ArduPID.h"

#define TEMP_WINDOW 1 // in temperature range
#define TEMP_LIMIT_HIGH 104 // TOO HGH
#define TEMP_LIMIT_LOW 2 // LOO LOW
#define TEMP_MIN_BREW 70 // do not brew under this temp

#define TIMEOUT_HEATING (1000*600) // maximum heater on time: 10 minutes
#define TIMEOUT_BREW (1000*60) // maximum brew on time: 1 minutes
#define TIMEOUT_READY (1000*600) // maximum time in state ready: 10 minutes
#define TIMEOUT_HEATER_SSR (1000 * 60) // maximum time the SSR is allowed to be ON
#define TIMEOUT_CONTROL (1000 * 1) // Max time between control updates


// Boiler state machine states:
//  INIT: Initial state (once at startup)
//  OFF: SSR is forced OFF
//  HEATING: temperature control, but not yet on target temperature
//  READY: temperature control, within range of target temperature
//  BREW: temperature control with feed-forward active
//  ERROR: heater is forced OFF, error code is set, set state OFF to clear error
typedef enum {BOILER_INIT, BOILER_OFF, BOILER_HEATING, BOILER_READY, BOILER_BREW, BOILER_ERROR} boiler_state_t;


// Various boiler errors
typedef enum {
  BOILER_ERROR_NONE, BOILER_ERROR_RTD, BOILER_ERROR_TIMEOUT_BREW, BOILER_ERROR_TIMEOUT_HEATING,
  BOILER_ERROR_SSR_TIMEOUT, BOILER_ERROR_OVER_TEMP, BOILER_ERROR_UNDER_TEMP, BOILER_ERROR_CONTROL_TIMEOUT 
} boiler_error_t;

extern const char *boiler_error_text[];


class BoilerController
{
    private:
      ArduPID pid;
      double _act_temp=0, _set_temp=0, _ff=0, _power=0;
      boiler_state_t _state = BOILER_OFF, _prev_state = BOILER_INIT;
      boiler_error_t _error = BOILER_ERROR_NONE;
      int _rtd_error = 0;  // current RTD errors
      unsigned long _last_time=0, _current_time=0, _state_timer=0; // state duration timers
      boiler_error_t error(boiler_error_t err) {_error=err; state(BOILER_ERROR); return _error; }
      boiler_state_t state(boiler_state_t state) { return _state = state; }
      void update(void);

    public:
        BoilerController();
        void control(void); // Control function, needs to be called > 1x/sec
        boiler_state_t on() { return state(BOILER_HEATING); } // start heating
        boiler_state_t off() { return state(BOILER_OFF); } // switch boiler off
        boiler_state_t maintain() { return state(BOILER_HEATING); } // maintain mode (without feed-forward)
        boiler_state_t brew() { return state(BOILER_BREW); } // brew mode, with feed-forward
        boiler_error_t error() { return _error; }  // return error code (0=OK)
        boiler_state_t state() { return _state; }  // current state
        double set_temp(double temp) { return _set_temp = min(TEMP_LIMIT_HIGH, max(temp, TEMP_LIMIT_LOW)); }
        double set_temp(void) { return _set_temp; }
        double act_temp() { return _act_temp; }
        double act_power() { _power; } // power in [%]
        void feedforward(double ff) { _ff = min(100.0, max(ff, 0.0)); } // feedforward in [%] 
        double feedforward(void) { return _ff; } // feedforward in [%] 
        void set_pid(double p, double i, double d) { pid.setCoefficients(p, i, d); }
};

extern BoilerController boilerController;

#endif // BOILER_H
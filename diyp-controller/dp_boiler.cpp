/*
 diyPresso Boiler control
 */
#include <Arduino.h>
#include <Adafruit_MAX31865.h>
#include "ArduPID.h"
#include "dp_hardware.h"
#include "dp_boiler.h"
#include "dp_heater.h"
#include "dp_time.h"
#include "dp_fsm.h"

#define NEXT(state) next(&BoilerStateMachine::state)
#define ON_ENTRY() if ( on_entry() )
#define ON_EXIT() if ( on_exit() )
#define ON_TIMEOUT(t) if ( on_timeout(t) )
#define ON_MESSAGE(m) if ( on_message(m) )

class BoilerStateMachine : public StateMachine<BoilerStateMachine>
{
  public:
    BoilerStateMachine() : StateMachine( &BoilerStateMachine::state_off ) { };
    int error() { return _error; }
    int set_temp() { return _set_temp; }
    int set_temp(int temp) { _set_temp = temp; }
    void control();
  private:
    double _act_temp=0, _set_temp=0, _ff=0, _power=0;
    boiler_state_t _state = BOILER_OFF, _prev_state = BOILER_INIT;
    boiler_error_t _error = BOILER_ERROR_NONE;
    int _rtd_error = 0;  // current RTD errors
    unsigned long _last_time=0, _current_time=0, _state_timer=0; // state duration timers
    void state_off();
    void state_heating();
    void state_ready();
    void state_brew();
    void state_error();
    void goto_error(boiler_error_t err);
};

void BoilerStateMachine::state_off()
{
  _set_temp = 0;
  ON_MESSAGE(2) NEXT(state_heating);
}

void BoilerStateMachine::state_heating()
{
  NEXT(state_ready);
  if (abs(_set_temp - _act_temp) < TEMP_WINDOW)
    NEXT(state_ready); 
}

void BoilerStateMachine::state_ready()
{
  NEXT(state_brew);
}

void BoilerStateMachine::state_brew()
{
  ON_TIMEOUT(TIMEOUT_BREW) goto_error(BOILER_ERROR_TIMEOUT_BREW);
}

void BoilerStateMachine::state_error()
{
  ON_MESSAGE(1) NEXT(state_off);
}

void BoilerStateMachine::goto_error(boiler_error_t error)
{
  _error = error;
  NEXT(state_error);
}

// globals
const char *boiler_error_text[] = {
  "NONE", "ERROR_RTD", "TIMEOUT_BREW", "TIMEOUT_HEATING",
  "SSR_TIMEOUT", "OVER_TEMP", "UNDER_TEMP", "CONTROL_TIMEOUT" 
};

Adafruit_MAX31865 thermistor = Adafruit_MAX31865(PIN_THERM_CS, PIN_THERM_MOSI, PIN_THERM_MISO, PIN_THEM_SCLK);
BoilerController boilerController = BoilerController();


BoilerController::BoilerController()
{
  double p=10, i=0.2, d=0.2; // default controller values
  pid.begin(&_act_temp, &_power, &_set_temp, p, i, d);
  pid.setOutputLimits(0, 100);
  pid.setBias(0);
  pid.setWindUpLimits( -100.0, 20.0); // bounds for the integral term to prevent integral wind-up
  pid.start();
  thermistor.begin(MAX31865_2WIRE);  // set to 2WIRE or 4WIRE as necessary  
  _error = BOILER_ERROR_NONE;
  _rtd_error = 0;
  control(); // at least one control loop to init the state
}



// read the thermistor and errors
void BoilerController::update(void)
{
  _act_temp = thermistor.temperature(RNOMINAL, RREF);
  uint8_t fault = thermistor.readFault();
  if ( fault ) {
     error(BOILER_ERROR_RTD);
    _rtd_error = fault;
    thermistor.clearFault();
  }
  _act_temp = heaterDevice.average();
  heaterDevice.power(_power);
}

/*
 * boiler state machine and control loop
*/
void BoilerController::control(void)
{
  _current_time = millis();
  if ( !_last_time )
    _last_time = _current_time;

  update();
  pid.compute();
 
  if ( _prev_state != _state )
    _state_timer = _current_time;

  switch (_state)
  {
    case BOILER_OFF:
      _power = 0;
      _error = BOILER_ERROR_NONE;
      break;

    case BOILER_HEATING:
      if (abs(_set_temp - _act_temp) < TEMP_WINDOW)
        state(BOILER_READY); 
      pid.setBias(0);
      if ( time_diff(_current_time, _state_timer) > TIMEOUT_HEATING )
        error(BOILER_ERROR_TIMEOUT_HEATING);
      break;
    
    case BOILER_READY:
      pid.setBias(0);
      if (abs(_set_temp - _act_temp) > TEMP_WINDOW)
        state(BOILER_HEATING); 
      if ( time_diff(_current_time, _state_timer) > TIMEOUT_READY )
        state(BOILER_OFF);
      break;

    case BOILER_BREW:
      pid.setBias(_ff);
      if (_act_temp < TEMP_MIN_BREW )
        state(BOILER_OFF); 
      if ( time_diff(_current_time, _state_timer) > TIMEOUT_BREW )
        error(BOILER_ERROR_TIMEOUT_BREW);
      break;

    case BOILER_ERROR:
      _power = 0;
      _set_temp = 1;
      break;

    default:
      _state = BOILER_OFF;
  } 

  _prev_state = _state;

  // Check for errors
  if ( _act_temp > TEMP_LIMIT_HIGH)
    error(BOILER_ERROR_OVER_TEMP);
  
  if ( _rtd_error )
    error(BOILER_ERROR_RTD);
  
  /* if ( time_diff(_current_time, _last_time) > 1000 )
    error(BOILER_ERROR_CONTROL_TIMEOUT);
  */
}

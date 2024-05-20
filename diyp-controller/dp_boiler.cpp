/*
 diyPresso Boiler control
 */
#include "dp.h"
#include "dp_hardware.h"
#include "dp_boiler.h"
#include "dp_heater.h"

#include <Adafruit_MAX31865.h>
#include "ArduPID.h"

#ifdef WATCHDOG_ENABLED
#include <wdt_samd21.h>
#endif

Adafruit_MAX31865 thermistor = Adafruit_MAX31865(PIN_THERM_CS, PIN_THERM_MOSI, PIN_THERM_MISO, PIN_THEM_SCLK);

BoilerStateMachine boilerController = BoilerStateMachine();


void BoilerStateMachine::state_off()
{;
  if ( _on ) NEXT(state_heating);
}

void BoilerStateMachine::state_heating()
{
  ON_ENTRY()
  {
     _pid.setBias(_ff_heat);
  }
  if ( ! _on ) NEXT(state_off);
  if ( _brew ) NEXT(state_brew);
  if ( abs(_set_temp - _act_temp) < TEMP_WINDOW) NEXT(state_ready);
  ON_TIMEOUT(TIMEOUT_HEATING) goto_error(BOILER_ERROR_TIMEOUT_HEATING);
  ON_EXIT()
  {
     _pid.setBias(0);
  }

}

void BoilerStateMachine::state_ready()
{
  ON_ENTRY()
  {
      _pid.setBias(_ff_ready);
  }
  if ( ! _on ) NEXT(state_off);
  if ( _brew ) NEXT(state_brew);
  if ( abs(_set_temp - _act_temp) > TEMP_WINDOW) NEXT(state_heating);
  ON_TIMEOUT(TIMEOUT_READY) goto_error(BOILER_ERROR_READY_TIMEOUT);
}

void BoilerStateMachine::state_brew()
{
  if  ( ! _on ) NEXT(state_off);
  if  ( ! _brew ) NEXT(state_heating);
  _pid.setBias(_ff_brew);
  // if ( (_set_temp - _act_temp ) > TEMP_WINDOW) goto_error(BOILER_ERROR_UNDER_TEMP);
  ON_TIMEOUT(TIMEOUT_BREW) goto_error(BOILER_ERROR_TIMEOUT_BREW);
  ON_EXIT() { _pid.setBias(0); _brew = false; }
}

void BoilerStateMachine::state_error()
{
  off();
  _power = 0;
  _set_temp = 0;
  if ( _error == BOILER_ERROR_NONE ) NEXT(state_off);
}

void BoilerStateMachine::goto_error(boiler_error_t error)
{
  _error = error;
  NEXT(state_error);
}


void BoilerStateMachine::init()
{
  double p=10, i=0.2, d=0.2; // default controller values
  _pid.begin(&_act_temp, &_power, &_set_temp, p, i, d);
  _pid.setOutputLimits(0, 100);
  _pid.setBias(3);
  _pid.setWindUpLimits( -100.0, 5.0); // bounds for the integral term to prevent integral wind-up
  _pid.start();
  thermistor.begin(MAX31865_2WIRE);  // set to 2WIRE or 4WIRE as necessary
  _error = BOILER_ERROR_NONE;
  _rtd_error = 0;
  _last_control_time = millis();
#ifdef WATCHDOG_ENABLED
  wdt_init ( WDT_CONFIG_PER_16K );
#endif
}


// boiler control loop: read RTD, check errors, run state machine, set outputs
void BoilerStateMachine::control(void)
{
  _act_temp = thermistor.temperature(RNOMINAL, RREF);

#ifdef SIMULATE
  _act_temp = heaterDevice.average(); // hack for testing, read average power as actual temperature
#endif

  _rtd_error = thermistor.readFault();
  if ( _rtd_error ) {
    thermistor.clearFault();
    goto_error(BOILER_ERROR_RTD);
  }
  else
  {
    if ( _act_temp > TEMP_LIMIT_HIGH )
      goto_error(BOILER_ERROR_OVER_TEMP);
    if ( _act_temp < TEMP_LIMIT_LOW )
      goto_error(BOILER_ERROR_RTD);
  }

  if ( _last_control_time + TIMEOUT_CONTROL < millis() )
    goto_error(BOILER_ERROR_CONTROL_TIMEOUT);
  _last_control_time = millis();

  run();
  _pid.compute();
  heaterDevice.power( _on ? _power : 0.0 );
#ifdef WATCHDOG_ENABLED
  wdt_reset();
#endif

}


const char *BoilerStateMachine::get_error_text()
{
    switch ( _error )
    {
      case BOILER_ERROR_NONE: return "OK";
      case BOILER_ERROR_OVER_TEMP: return "OVER_TEMP";
      case BOILER_ERROR_UNDER_TEMP: return "UNDER_TEMP";
      case BOILER_ERROR_RTD: return "RTD_ERROR";
      case BOILER_ERROR_SSR_TIMEOUT: return "SSR_TIMEOUT";
      case BOILER_ERROR_TIMEOUT_BREW: return "BREW_TIMEOUT";
      case BOILER_ERROR_CONTROL_TIMEOUT: return "CONTROL_TIMEOUT";
      case BOILER_ERROR_READY_TIMEOUT: return "READY_TIMEOUT";
      case BOILER_ERROR_TIMEOUT_HEATING: return "TIMEOUT_HEATING";
      default: return "UNKNOWN";
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
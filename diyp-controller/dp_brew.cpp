/*
 diyEspresso Brew Process control
 Implemented as a Finite State Machine
 */
#include "dp.h"
#include "dp_boiler.h"
#include "dp_reservoir.h"
#include "dp_pump.h"
#include "dp_brew_switch.h"
#include "dp_display.h"
#include "dp_led.h"
#include "dp_settings.h"
#include "dp_brew.h"


BrewProcess brewProcess = BrewProcess();

// Note: brew-switch DOWN = closed (circulating via OverPressure valve), UP=open (brewing)
void BrewProcess::common_transitions()
{
  if ( brewSwitch.down() )
    if ( _initialized )
      NEXT( state_idle);
  if ( reservoir.is_empty() ) NEXT(state_empty);
  ON_MESSAGE(SLEEP) NEXT(state_sleep);
}

// initial state at startup, brew switch needs to be moved from UP (open) to DOWN (closed) before init
void BrewProcess::state_init()
{
  static bool was_up = false;
  statusLed.color(ColorLed::BLACK);
  boilerController.off();
  if ( brewSwitch.up() ) was_up = true;
  if ( brewSwitch.down() && was_up )
    NEXT( state_fill );
  if ( reservoir.is_empty() ) NEXT(state_empty);
}

// pump for 10 seconds and check that reservoir level has dropped
void BrewProcess::state_fill()
{
  ON_ENTRY()
  {
    _start_weight = reservoir.weight();
    pumpDevice.on();
  }
  statusLed.color( blink() ? ColorLed::YELLOW : ColorLed::BLACK);
  ON_TIMEOUT(1000*INITIAL_PUMP_TIME) 
  {
     if ( (reservoir.weight() - _start_weight) > -INITIAL_WEIGHT_DROP )
      goto_error(BREW_ERROR_FILL);
    else
      NEXT(state_circulate);
  }
  common_transitions();
}

// keep pumping and check that reservoir level is kept constant
void BrewProcess::state_circulate()
{
  ON_ENTRY() _start_weight = reservoir.weight();
  ON_TIMEOUT(1000*MAINTAIN_PUMP_TIME)
  {
    if (abs( _start_weight - reservoir.weight()) > MAINTAIN_WEIGHT_VARIATION )
      goto_error(BREW_ERROR_CIRCULATION);
    else
    {
      NEXT(state_idle);
      _initialized = true;
    }
  }
  common_transitions();
}

void BrewProcess::state_sleep()
{
  statusLed.color(ColorLed::BLACK);
  boilerController.off();
  pumpDevice.off();
  ON_MESSAGE(WAKEUP) NEXT(state_idle);
}

void BrewProcess::state_empty()
{
  statusLed.color(ColorLed::CYAN);
  ON_ENTRY()
  {
    boilerController.off();
    pumpDevice.off();
  }
  if ( !reservoir.is_empty() && brewSwitch.down() ) NEXT(state_idle);
}

void BrewProcess::state_idle()
{
  ON_ENTRY()
  {
    if ( !is_prev_state(STATE(state_finished)))
      _end_weight = weight();
    _brewTimer.stop();
    statusLed.color(ColorLed::GREEN);
    pumpDevice.off();
    boilerController.stop_brew();
    boilerController.on();
    boilerController.set_temp(settings.temperature());
  }
  if ( brewSwitch.up() ) NEXT(state_pre_infuse);
  common_transitions();
  ON_TIMEOUT(1000*AUTOSLEEP_TIMEOUT) NEXT(state_sleep);
  if ( !_initialized ) NEXT(state_init);
}

void BrewProcess::state_pre_infuse()
{
  ON_ENTRY()
  {
    _start_weight = reservoir.weight();
    _brewTimer.start();
    statusLed.color(ColorLed::BLUE);
    pumpDevice.on();
    boilerController.start_brew();
    settings.incShotCounter();
  }
  ON_TIMEOUT(1000*preInfuseTime) NEXT(state_infuse);
  common_transitions();
}

void BrewProcess::state_infuse()
{
  ON_ENTRY()
  {
    boilerController.stop_brew();
    statusLed.color(ColorLed::YELLOW);
    pumpDevice.off();
  }
  ON_TIMEOUT(1000*infuseTime) NEXT(state_extract);
  common_transitions();
}

void BrewProcess::state_extract()
{
  ON_ENTRY()
  {
    _start_weight = reservoir.weight();
    statusLed.color(ColorLed::PURPLE);
    pumpDevice.on();
    boilerController.start_brew();
  }
  //if ( boiler.act_temp() < BREW_MIN_TEMP) NEXT(idle);
  ON_TIMEOUT(1000*extractTime) NEXT(state_finished);
  common_transitions();
}


void BrewProcess::state_finished()
{
  ON_ENTRY()
  {
    _end_weight = weight();
    statusLed.color(ColorLed::CYAN);
    pumpDevice.off();
    boilerController.stop_brew();
    _brewTimer.stop();
  }
  if ( display.button_pressed() ) NEXT(state_extract);
  ON_TIMEOUT(1000*finishedTime) goto_error(BREW_ERROR_TIMEOUT);
  common_transitions();
}


void BrewProcess::state_error()
{
  ON_ENTRY()
  {
    statusLed.color(ColorLed::RED);
    pumpDevice.off();
    _brewTimer.stop();
    boilerController.off();
  }
  common_transitions();
  ON_MESSAGE(RESET) NEXT(state_init);
}

void BrewProcess::goto_error(brew_error_t error)
{
  _error = error;
  NEXT(state_error);
}

const char *BrewProcess::get_state_name()
{
    RETURN_STATE_NAME(init);
    RETURN_STATE_NAME(fill);
    RETURN_STATE_NAME(circulate);
    RETURN_STATE_NAME(sleep);
    RETURN_STATE_NAME(empty);
    RETURN_STATE_NAME(idle);
    RETURN_STATE_NAME(pre_infuse);
    RETURN_STATE_NAME(infuse);
    RETURN_STATE_NAME(extract);
    RETURN_STATE_NAME(finished);
    RETURN_STATE_NAME(error);
    RETURN_UNKNOWN_STATE_NAME();
}

const char *BrewProcess::get_error_text()
{
  switch(_error)
  {
    case BREW_ERROR_NONE: return "OK";
    case BREW_ERROR_FILL: return "NO_FILL";
    case BREW_ERROR_CIRCULATION: return "NO_CIRCULATION";
    case BREW_ERROR_TIMEOUT: return "TIMEOUT";
    default: return "UNKNOWN";
  }
}
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
    if ( settings.commissioningDone() )
      NEXT( state_idle);
  if ( reservoir.is_empty() ) NEXT(state_empty);
  ON_MESSAGE(SLEEP) NEXT(state_sleep);
}

// initial state at startup, 
// If we are not commissioned, ask user to fill reservoir and tare the measurement
// Next state is fill
// brew switch needs to be moved from UP (open) to DOWN (closed) before init

void BrewProcess::state_init()
{
  statusLed.color(ColorLed::BLACK);
  boilerController.off();
  if ( settings.commissioningDone() )
    NEXT(state_idle);
  if ( display.button_pressed() )
  {
    reservoir.tare();
    settings.tareWeight( reservoir.get_tare() );
    settings.save();
    NEXT( state_fill );
  }
}

// pump 60 seconds and check that reservoir level has dropped
void BrewProcess::state_fill()
{
  static int counter=0;
  counter += 1;
  ON_ENTRY()
  {
    _start_weight = reservoir.weight();
    pumpDevice.on();
  }
  statusLed.color( blink() ? ColorLed::YELLOW : ColorLed::BLACK);
  if ( counter >= 30 )
  {
    if( abs(_start_weight - reservoir.weight()) < FILL_WEIGHT_VARIATION  )
    {
       NEXT(state_purge);
      counter = 0;
    }
    _start_weight = reservoir.weight();

  }
  ON_TIMEOUT(1000*INITIAL_PUMP_TIME) goto_error(BREW_ERROR_FILL);
}

// Open gate and see that water flows out
void BrewProcess::state_purge()
{
  if (abs( _start_weight - reservoir.weight()) > PURGE_WEIGHT_DROP )
  {
    pumpDevice.off();
    NEXT(state_done);
  }
  ON_TIMEOUT(1000*PURGE_TIMEOUT) goto_error(BREW_ERROR_PURGE);
  common_transitions();
}

// Done commissioning
void BrewProcess::state_done()
{
  if ( brewSwitch.down() ) 
  {
    settings.commissioningDone(1);
    settings.save();
    NEXT(state_idle);
  }
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
  if ( !settings.commissioningDone() ) NEXT(state_init);
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
    if (!is_prev_state(STATE(state_finished))) {
      _start_weight = reservoir.weight();
    }
    statusLed.color(ColorLed::PURPLE);
    pumpDevice.on();
    boilerController.start_brew();
    settings.incShotCounter();

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
  if ( display.button_pressed() ) {
    _brewTimer.start();
    NEXT(state_extract);
  }
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
    RETURN_STATE_NAME(purge);
    RETURN_STATE_NAME(sleep);
    RETURN_STATE_NAME(empty);
    RETURN_STATE_NAME(idle);
    RETURN_STATE_NAME(done);
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
    case BREW_ERROR_PURGE: return "NO_PURGE";
    case BREW_ERROR_TIMEOUT: return "TIMEOUT";
    default: return "UNKNOWN";
  }
}
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

void BrewProcess::common_transitions()
{
  if ( brewSwitch.down() ) NEXT(state_idle);
  ON_MESSAGE(SLEEP) NEXT(state_sleep);
}

void BrewProcess::state_init()
{
  statusLed.color(ColorLed::BLACK);
  NEXT(state_idle);
}

void BrewProcess::state_sleep()
{
  statusLed.color(ColorLed::BLACK);
  boilerController.off();
  ON_MESSAGE(WAKEUP) NEXT(state_idle);
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
    boilerController.on();
    boilerController.set_temp(settings.temperature());
  }
  if ( brewSwitch.up() ) NEXT(state_pre_infuse);
  common_transitions();
  ON_TIMEOUT(1000*AUTOSLEEP_TIMEOUT) NEXT(state_sleep);
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
  }
  if ( display.button_pressed() ) NEXT(state_extract);
  ON_TIMEOUT(1000*finishedTime) NEXT(state_error);
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
}


const char *BrewProcess::get_state_name()
{
    RETURN_STATE_NAME(init);
    RETURN_STATE_NAME(sleep);
    RETURN_STATE_NAME(idle);
    RETURN_STATE_NAME(pre_infuse);
    RETURN_STATE_NAME(infuse);
    RETURN_STATE_NAME(extract);
    RETURN_STATE_NAME(finished);
    RETURN_STATE_NAME(error);
    RETURN_UNKNOWN_STATE_NAME();
}
/*
 diyEspresso Brew Process control
 Implemented as a Finite State Machine
 Uses state machine library: YASM v1.2.0 - Bricofoy - https://github.com/bricofoy/yasm/
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

void BrewProcess::state_init()
{
  statusLed.color(ColorLed::BLACK);
  NEXT(state_idle);
}

void BrewProcess::state_sleep()
{
  statusLed.color(ColorLed::BLACK);
  ON_MESSAGE(WAKEUP) NEXT(state_idle);
}

void BrewProcess::state_idle()
{
  ON_ENTRY()
  {
    _brewTimer.stop();
    statusLed.color(ColorLed::GREEN);
    pumpDevice.off();
    boilerController.on();
  }
  if ( brewSwitch.up() ) NEXT(state_pre_infuse);
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
  if ( brewSwitch.down() ) NEXT(state_idle);
  ON_TIMEOUT(1000*preInfuseTime) NEXT(state_infuse);
}

void BrewProcess::state_infuse()
{
  ON_ENTRY()
  {
    boilerController.stop_brew();
    statusLed.color(ColorLed::YELLOW);
    pumpDevice.off();
    boilerController.on();
  }
  if ( brewSwitch.down() ) NEXT(state_idle);
  ON_TIMEOUT(1000*infuseTime) NEXT(state_extract);
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
  if ( brewSwitch.down() ) NEXT(state_idle);
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
  if ( brewSwitch.down() ) NEXT(state_idle);
  ON_TIMEOUT(1000*finishedTime) NEXT(state_error);
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
  if ( brewSwitch.down() ) NEXT(state_idle);
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
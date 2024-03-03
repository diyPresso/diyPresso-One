/*
 diyEspresso Brew Process control
 Implemented as a Finite State Machine
 Uses state machine library: YASM v1.2.0 - Bricofoy - https://github.com/bricofoy/yasm/
 */
#include "dp_brew.h"
#include "dp_boiler.h"
#include "dp_pump.h"
#include "dp_brew_switch.h"
#include "dp_display.h"
#include "dp_led.h"
#include "dp_settings.h"


BrewProcess brewProcess = BrewProcess();

// Some handy macros to write a state machine
#define STATE(s) do { _prev_state = state; state = s; } while(0) // Set state variable 
#define AFTER(t, s) do { if ( (_time+(unsigned long)(1000*t)) < millis() ) set_state(&BrewProcess::s); } while(0) // set next state after time
#define NEXT(s) set_state(&BrewProcess::s) // Set next state fct
#define ON_ENTRY() if ( state != _prev_state ) // Execute once on entry of state

char *brew_state_names[] = { "INIT", "IDLE", "PRE_INFUSE", "INFUSE", "EXTRACT", "FINISHED", "ERROR"};

void BrewProcess::init()
{
  STATE(BREW_INIT);
  statusLed.color(ColorLed::BLACK);
  NEXT(idle);
}

void BrewProcess::sleep_state()
{
  STATE(BREW_SLEEP);
  statusLed.color(ColorLed::BLACK);
}


void BrewProcess::idle()
{
  STATE(BREW_IDLE);
  _start = millis();
  _brewTimer.stop();
  statusLed.color(ColorLed::GREEN);
  pumpDevice.off();
  boilerController.on();
  if ( brewSwitch.up() ) NEXT(pre_infuse);
}


void BrewProcess::pre_infuse()
{
  STATE(BREW_PRE_INFUSE);
  ON_ENTRY()
  {
    _brewTimer.start();
    statusLed.color(ColorLed::BLUE);
    pumpDevice.on();
    boilerController.start_brew();
    settings.incShotCounter();
  }
  if ( brewSwitch.down() ) NEXT(idle);
  AFTER(preInfuseTime, infuse);
}


void BrewProcess::infuse()
{
  STATE(BREW_INFUSE);
  boilerController.stop_brew();
  statusLed.color(ColorLed::YELLOW);
  pumpDevice.off();
  boilerController.on();
  if ( brewSwitch.down() ) NEXT(idle);
  AFTER(infuseTime, extract);
}


void BrewProcess::extract()
{
  STATE(BREW_EXTRACT);
  statusLed.color(ColorLed::PURPLE);
  pumpDevice.on();
  boilerController.start_brew();
  //if ( boiler.act_temp() < BREW_MIN_TEMP) NEXT(idle);
  AFTER(extractTime, finished);
  if ( brewSwitch.down() ) NEXT(idle);
}


void BrewProcess::finished()
{
  STATE(BREW_FINISHED);
  statusLed.color(ColorLed::CYAN);
  pumpDevice.off();
  boilerController.stop_brew();
  if ( display.button_pressed() ) NEXT(extract);
  if ( brewSwitch.down() ) NEXT(idle);
  AFTER(finishedTime, error);
}


void BrewProcess::error()
{
  STATE(BREW_ERROR);
  statusLed.color(ColorLed::RED);
  pumpDevice.off();
  _brewTimer.stop();
  boilerController.off();
  if ( brewSwitch.down() ) NEXT(idle);
}

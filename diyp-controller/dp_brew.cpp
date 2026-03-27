/*
 diyEspresso Brew Process control
 Implemented as a Finite State Machine
 (c) 2024 diyPresso
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

// Check common state transitions
// Note: brew-switch DOWN = closed (circulating via OverPressure valve), UP=open (brewing)
void BrewProcess::common_transitions()
{
  if (brewSwitch.down())
    next(&BrewProcess::state_idle);
  if (reservoir.is_empty())
    next(&BrewProcess::state_empty);
}

void BrewProcess::state_empty()
{
  statusLed.color(ColorLed::CYAN);
  ON_ENTRY()
  {
    boilerController.off();
    pumpDevice.off();
  }
  if (!reservoir.is_empty() && brewSwitch.down())
    next(&BrewProcess::state_idle);
}

void BrewProcess::state_idle()
{
  ON_ENTRY()
  {
    if (!is_prev_state(&BrewProcess::state_finished))
      _end_weight = weight();
    _brewTimer.stop();
    statusLed.color(ColorLed::GREEN);
    pumpDevice.off();
    boilerController.stop_brew();
    boilerController.on();
    boilerController.set_temp(settings.temperature());
  }
  if (brewSwitch.up())
    if (reservoir.is_almost_empty())
      next(&BrewProcess::state_warning_pre_brew);
    else
      next(&BrewProcess::state_pre_infuse);
  common_transitions();
}

// Warning state before brewing when reservoir is almost empty
void BrewProcess::state_warning_pre_brew()
{
  ON_ENTRY()
  {
    statusLed.color(ColorLed::RED);
  }

  if (brewSwitch.down())
  {
    next(&BrewProcess::state_idle);
  }

  ON_MESSAGE(MSG_BUTTON)
  {
    next(&BrewProcess::state_pre_infuse);
  }
  common_transitions();
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
  ON_TIMEOUT_SEC(preInfuseTime)
  next(&BrewProcess::state_infuse);
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
  ON_TIMEOUT_SEC(infuseTime)
  next(&BrewProcess::state_extract);
  common_transitions();
}

void BrewProcess::state_extract()
{
  ON_ENTRY()
  {
    if (!is_prev_state(&BrewProcess::state_finished))
    {
      _start_weight = reservoir.weight();
    }
    statusLed.color(ColorLed::PURPLE);
    pumpDevice.on();
    boilerController.start_brew();
    settings.incShotCounter();
  }
  // if ( boiler.act_temp() < BREW_MIN_TEMP) NEXT(idle); // extra check?
  ON_TIMEOUT_SEC(extractTime)
  next(&BrewProcess::state_finished);
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
  ON_MESSAGE(MSG_BUTTON)
  {
    _brewTimer.start();
    next(&BrewProcess::state_extract);
  }
  ON_TIMEOUT_SEC(finishedTime)
  goto_error(BREW_ERROR_TIMEOUT);
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
  ON_MESSAGE(RESET) {
    _error = BREW_ERROR_NONE;
    next(&BrewProcess::state_idle);
  }
}

void BrewProcess::goto_error(brew_error_t error)
{
  _error = error;
  next(&BrewProcess::state_error);
}

const char *BrewProcess::get_error_text()
{
  switch (_error)
  {
  case BREW_ERROR_NONE:
    return "OK";
  case BREW_ERROR_TIMEOUT:
    return "TIMEOUT";
  default:
    return "UNKNOWN";
  }
}
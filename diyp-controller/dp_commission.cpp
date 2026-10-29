/*
 * dp_commission.cpp
 * diyPresso Commissioning Process
 * First-time setup: tare weight, fill boiler, purge air, verify water flow
 * (c) 2024 diyPresso - CC-BY-NC
 */
#include "dp.h"
#include "dp_commission.h"
#include "dp_boiler.h"
#include "dp_reservoir.h"
#include "dp_pump.h"
#include "dp_brew_switch.h"
#include "dp_led.h"
#include "dp_settings.h"

CommissioningProcess commissioningProcess = CommissioningProcess();

// Initial state at startup
// If already commissioned, jump straight to completed
// Otherwise wait for user to fill reservoir and press button to tare + start filling
void CommissioningProcess::state_init()
{
  ON_ENTRY() {}
  statusLed.color(ColorLed::BLACK);
  boilerController.off();
  if (settings.commissioningDone())
    next(&CommissioningProcess::state_done);
  else
  {
    ON_MESSAGE(MSG_BUTTON)
    {
      reservoir.tare();
      settings.tareWeight(reservoir.get_tare());
      settings.save();
      next(&CommissioningProcess::state_fill);
    }
  }
}

// Fill the boiler by pumping for INITIAL_PUMP_TIME seconds
void CommissioningProcess::state_fill()
{
  ON_ENTRY()
  {
    _start_weight = reservoir.weight();
    pumpDevice.on();
  }
  statusLed.color(blink() ? ColorLed::YELLOW : ColorLed::BLACK);
  ON_TIMEOUT_SEC(INITIAL_PUMP_TIME)
  {
    next(&CommissioningProcess::state_purge);
  }
}

// Let the user open the brew lever (UP) to observe water flowing out
void CommissioningProcess::state_purge()
{
  ON_ENTRY() {}
  if (brewSwitch.up())
    next(&CommissioningProcess::state_check);
  ON_TIMEOUT_SEC(PURGE_TIMEOUT)
  goto_error(COMMISSION_ERROR_PURGE);
}

// User confirms water is flowing; verify minimum weight drop
void CommissioningProcess::state_check()
{
  ON_ENTRY() {}
  ON_MESSAGE(MSG_BUTTON)
  {
    if (abs(_start_weight - reservoir.weight()) > PURGE_WEIGHT_DROP_MINIMUM)
    {
      pumpDevice.off();
      next(&CommissioningProcess::state_confirm);
    }
    else
    {
      goto_error(COMMISSION_ERROR_NO_WATER);
    }
  }
  ON_TIMEOUT_SEC(PURGE_TIMEOUT)
  goto_error(COMMISSION_ERROR_PURGE);
}

// Wait for user to put brew lever DOWN to confirm
void CommissioningProcess::state_confirm()
{
  ON_ENTRY() {}
  if (brewSwitch.down())
  {
    settings.commissioningDone(1);
    settings.save();
    next(&CommissioningProcess::state_done);
  }
  ON_TIMEOUT_SEC(PURGE_TIMEOUT)
  goto_error(COMMISSION_ERROR_PURGE);
}

// Terminal state: commissioning is finished
void CommissioningProcess::state_done()
{
  ON_ENTRY() {}
}

void CommissioningProcess::state_error()
{
  ON_ENTRY()
  {
    statusLed.color(ColorLed::RED);
    pumpDevice.off();
    boilerController.off();
  }
  ON_MESSAGE(RESET)
  {
    _error = COMMISSION_ERROR_NONE;
    next(&CommissioningProcess::state_init);
  }
}

void CommissioningProcess::goto_error(commission_error_t error)
{
  _error = error;
  next(&CommissioningProcess::state_error);
}

const char *CommissioningProcess::get_error_text()
{
  switch (_error)
  {
  case COMMISSION_ERROR_NONE:
    return "OK";
  case COMMISSION_ERROR_FILL:
    return "NO_FILL";
  case COMMISSION_ERROR_PURGE:
    return "NO_PURGE";
  case COMMISSION_ERROR_NO_WATER:
    return "NO_WATER";
  default:
    return "UNKNOWN";
  }
}

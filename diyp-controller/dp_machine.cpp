/*
 * dp_machine.cpp
 * diyPresso Machine Controller
 * Top-level orchestrator FSM: coordinates commissioning, brewing, sleep, and error states
 * (c) 2026 diyPresso - Joop van Heekeren - CC-BY-NC
 */
#include "dp.h"
#include "dp_machine.h"
#include "dp_hardware.h"
#include "dp_commission.h"
#include "dp_brew.h"
#include "dp_brew_switch.h"
#include "dp_boiler.h"
#include "dp_reservoir.h"
#include "dp_pump.h"
#include "dp_settings.h"
#include "dp_menu.h"
#include "dp_steam_process.h"
#include "dp_steam_thermoblock.h"
#include "dp_steam_pump.h"


MachineController machineController = MachineController();

static const unsigned long CLEANING_PUMP_LIMIT_MS = 60000UL;

bool MachineController::start_cleaning()
{
  if (!in_state(&MachineController::state_ready) || brewProcess.is_busy() || !brewSwitch.down()
      || boilerController.is_error() || reservoir.is_error())
    return false;
  _cleaning_requested = true;
  return true;
}

const char *MachineController::cleaning_status()
{
  if (_cleaning_exit) return "Lower lever to exit";
  if (reservoir.is_almost_empty()) return "Refill reservoir";
  if (_cleaning_pumping) return "Pump running";
  if (!_cleaning_armed) return "Lower brew lever";
  return "Raise lever to pump";
}

// Commissioning: run the commissioning process
void MachineController::state_commissioning()
{
  ON_ENTRY() { commissioningProcess.reset(); }

  boilerController.off();
  steamThermoblock.off();
  steamPump.off();

  commissioningProcess.run(_message == MSG_BUTTON ? CommissioningProcess::MSG_BUTTON : CommissioningProcess::MSG_NONE);

  if (commissioningProcess.is_done())
    next(&MachineController::state_ready);
  if (commissioningProcess.is_error())
    next(&MachineController::state_error);
}

// Ready: machine is commissioned, run the brew process
void MachineController::state_ready()
{
  ON_ENTRY() {
    boilerController.on();
    menu_ready_reset();
    if(hardware.has_steam_group()) {
      steamProcess.init();
    }
  }

  if (_cleaning_requested)
  {
    _cleaning_requested = false;
    // Recheck: the lever or a fault may have changed since menu confirmation.
    if (brewSwitch.down() && !brewProcess.is_busy() && !boilerController.is_error()
        && !reservoir.is_error() && settings.commissioningDone())
    {
      next(&MachineController::state_cleaning);
      return;
    }
  }

  brewProcess.run(_message == MSG_BUTTON ? BrewProcess::MSG_BUTTON : BrewProcess::MSG_NONE);

  int steamMsg = SteamProcess::MSG_NONE;
  if (_message == MSG_STEAM_LONG_PRESS) steamMsg = SteamProcess::MSG_LONG_PRESS;
  else if (_message == MSG_STEAM_BUTTON) steamMsg = SteamProcess::MSG_BUTTON;
  steamProcess.run(steamMsg);

  // Reset auto-sleep timer on user activity
  if (_message == MSG_BUTTON || _message == MSG_STEAM_BUTTON || _message == MSG_STEAM_LONG_PRESS
      || brewProcess.is_busy() || steamProcess.is_busy())
    reset_timeout();

  if (brewProcess.is_error() || boilerController.is_error())
    next(&MachineController::state_error);

  if (!settings.commissioningDone())
    next(&MachineController::state_commissioning);

  ON_MESSAGE(MSG_LONG_PRESS)
    next(&MachineController::state_sleep);

  ON_TIMEOUT_SEC(AUTOSLEEP_TIMEOUT)
    next(&MachineController::state_sleep);
}

// Manual cleaning: the brew lever drives the pump without running a shot recipe.
void MachineController::state_cleaning()
{
  ON_ENTRY()
  {
    menu_ready_reset();
    _cleaning_armed = false; // require lever down before the first run
    _cleaning_exit = false;
    _cleaning_pumping = false;
    pumpDevice.off();
    boilerController.stop_brew();
  }

  // Steam is unavailable while cleaning; normal boiler temperature control remains active.
  steamThermoblock.off();
  steamPump.off();

  if (_message == MSG_BUTTON || _message == MSG_LONG_PRESS)
    _cleaning_exit = true;

  if (brewSwitch.down())
  {
    _cleaning_armed = true;
    _cleaning_pumping = false;
    pumpDevice.off();
    if (_cleaning_exit)
      next(&MachineController::state_ready);
  }
  else if (!_cleaning_exit && _cleaning_armed && !reservoir.is_almost_empty()
           && !reservoir.is_error() && !boilerController.is_error())
  {
    if (!_cleaning_pumping)
    {
      _cleaning_pump_started = millis();
      _cleaning_pumping = true;
    }
    if (millis() - _cleaning_pump_started >= CLEANING_PUMP_LIMIT_MS)
    {
      _cleaning_armed = false; // lower the lever before another run
      _cleaning_pumping = false;
      pumpDevice.off();
    }
    else
      pumpDevice.on();
  }
  else
  {
    pumpDevice.off();
    _cleaning_pumping = false;
    _cleaning_armed = false;
  }

  if (reservoir.is_almost_empty())
  {
    pumpDevice.off();
    _cleaning_pumping = false;
    _cleaning_armed = false;
  }

  if (_message != MSG_NONE || _cleaning_pumping)
    reset_timeout();
  if (brewSwitch.down())
  {
    ON_TIMEOUT_SEC(AUTOSLEEP_TIMEOUT)
      next(&MachineController::state_sleep);
  }

  if (boilerController.is_error() || reservoir.is_error())
    next(&MachineController::state_error);

  ON_EXIT()
  {
    pumpDevice.off();
    _cleaning_pumping = false;
    _cleaning_armed = false;
    _cleaning_exit = false;
  }
}

// Sleep: boiler off, pump off
void MachineController::state_sleep()
{
  ON_ENTRY() {}

  boilerController.off();
  pumpDevice.off();
  steamThermoblock.off();
  steamPump.off();

  if (_message == MSG_BUTTON || _message == MSG_LONG_PRESS)
    next(&MachineController::state_ready);
}

// Error: everything off, wait for button to clear
void MachineController::state_error()
{
  ON_ENTRY() {}

  boilerController.off();
  pumpDevice.off();
  steamThermoblock.off();
  steamPump.off();

  ON_MESSAGE(MSG_BUTTON)
  {
    boilerController.clear_error();
    reservoir.clear_error();
    brewProcess.clear_error();
    commissioningProcess.clear_error();
    next(&MachineController::state_ready);
  }
}

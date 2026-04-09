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
#include "dp_boiler.h"
#include "dp_reservoir.h"
#include "dp_pump.h"
#include "dp_settings.h"
#include "dp_menu.h"
#include "dp_steam_process.h"
#include "dp_steam_thermoblock.h"
#include "dp_steam_pump.h"


MachineController machineController = MachineController();

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

  brewProcess.run(_message == MSG_BUTTON ? BrewProcess::MSG_BUTTON : BrewProcess::MSG_NONE);

  int steamMsg = SteamProcess::MSG_NONE;
  if (_message == MSG_STEAM_LONG_PRESS) steamMsg = SteamProcess::MSG_LONG_PRESS;
  else if (_message == MSG_STEAM_BUTTON) steamMsg = SteamProcess::MSG_BUTTON;
  steamProcess.run(steamMsg);

  // Reset auto-sleep timer on user activity
  if (_message == MSG_BUTTON || brewProcess.is_busy())
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

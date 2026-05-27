/*
 * dp_steam_process.cpp
 * Steam Process workflow FSM (Model Two only)
 * (c) 2026 diyPresso
 */
#include "dp.h"
#include "dp_steam_process.h"
#include "dp_steam_thermoblock.h"
#include "dp_steam_pump.h"
#include "dp_steam_switch.h"
#include "dp_reservoir.h"

SteamProcess steamProcess = SteamProcess();

// off: default state, for machines without steam group
void SteamProcess::state_off()
{
  ON_ENTRY()
  {
    steamThermoblock.off();
    steamPump.off();
  }
}

// idle: heating off, pump off
void SteamProcess::state_idle()
{
  ON_ENTRY()
  {
    steamThermoblock.off();
    steamPump.off();
  }
  ON_MESSAGE(MSG_BUTTON)
    next(&SteamProcess::state_heating);
}

// heating: thermoblock heating up
void SteamProcess::state_heating()
{
  ON_ENTRY()
  {
    steamThermoblock.on();
    steamPump.off();
  }
  if (reservoir.is_empty())
    next(&SteamProcess::state_empty);
  if (steamThermoblock.is_ready())
    next(&SteamProcess::state_ready);
  ON_MESSAGE(MSG_LONG_PRESS)
    next(&SteamProcess::state_idle);
  ON_MESSAGE(MSG_BUTTON)
    next(&SteamProcess::state_steaming);
  ON_TIMEOUT_SEC(STEAM_TIMEOUT_HEATING_SEC)
    if (!steamThermoblock.isAutoTuning()) // don't timeout during PID autotune
      goto_error(STEAM_PROCESS_ERROR_HEATING_TIMEOUT);
}

// ready: at temperature, waiting for user
void SteamProcess::state_ready()
{
  ON_ENTRY() {}
  if (reservoir.is_empty())
    next(&SteamProcess::state_empty);
  ON_MESSAGE(MSG_LONG_PRESS)
    next(&SteamProcess::state_idle);
  ON_MESSAGE(MSG_BUTTON)
    next(&SteamProcess::state_steaming);
  ON_TIMEOUT_SEC(STEAM_TIMEOUT_READY_SEC) {
    if (!steamThermoblock.isAutoTuning()) // don't timeout during PID autotune
      next(&SteamProcess::state_idle);
  }
}

// steaming: pump on, steam flowing
void SteamProcess::state_steaming()
{
  ON_ENTRY()
  {
    steamThermoblock.on();
    steamPump.on();
  }
  if (reservoir.is_empty())
    next(&SteamProcess::state_empty);
  ON_MESSAGE(MSG_LONG_PRESS)
    next(&SteamProcess::state_idle);
  ON_MESSAGE(MSG_BUTTON)
    next(&SteamProcess::state_heating);
  ON_TIMEOUT_SEC(STEAM_TIMEOUT_STEAMING_SEC)
    next(&SteamProcess::state_heating);
}

// empty: reservoir low, stop everything
void SteamProcess::state_empty()
{
  ON_ENTRY()
  {
    steamThermoblock.off();
    steamPump.off();
    _error = STEAM_PROCESS_ERROR_EMPTY;
  }
  if (!reservoir.is_empty())
    next(&SteamProcess::state_idle);
}

void SteamProcess::state_error()
{
  ON_ENTRY()
  {
    steamThermoblock.off();
    steamPump.off();
  }
  ON_MESSAGE(RESET) {
    _error = STEAM_PROCESS_ERROR_NONE;
    next(&SteamProcess::state_idle);
  }
}

void SteamProcess::goto_error(steam_process_error_t error)
{
  _error = error;
  next(&SteamProcess::state_error);
}

const char *SteamProcess::get_error_text()
{
  switch (_error)
  {
  case STEAM_PROCESS_ERROR_NONE:            return "OK";
  case STEAM_PROCESS_ERROR_HEATING_TIMEOUT: return "HEAT_TIMEOUT";
  case STEAM_PROCESS_ERROR_EMPTY:           return "EMPTY";
  default:                                  return "UNKNOWN";
  }
}

const char *SteamProcess::get_short_state()
{
  if (in_state(&SteamProcess::state_off))      return "OFF";
  if (in_state(&SteamProcess::state_idle))     return "IDLE";
  if (in_state(&SteamProcess::state_heating))  return "HEAT";
  if (in_state(&SteamProcess::state_ready))    return "RDY";
  if (in_state(&SteamProcess::state_steaming)) return "STM";
  if (in_state(&SteamProcess::state_empty))    return "EMPT";
  if (in_state(&SteamProcess::state_error))    return "ERR";
  return "?";
}

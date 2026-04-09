/*
 * dp_steam_process.h
 * Steam Process workflow FSM (Model Two only)
 * (c) 2026 diyPresso
 */
#ifndef STEAM_PROCESS_H
#define STEAM_PROCESS_H

#include "dp_fsm.h"
#include <Arduino.h>

// Steam process timeouts in [sec]
#define STEAM_TIMEOUT_HEATING_SEC   (60 * 2)
#define STEAM_TIMEOUT_READY_SEC     (60 * 5)
#define STEAM_TIMEOUT_STEAMING_SEC  (60 * 5)

typedef enum
{
  STEAM_PROCESS_ERROR_NONE,
  STEAM_PROCESS_ERROR_HEATING_TIMEOUT,
  STEAM_PROCESS_ERROR_EMPTY,
} steam_process_error_t;

class SteamProcess : public StateMachine<SteamProcess>
{
public:
  SteamProcess() : StateMachine(&SteamProcess::state_off) {};
  typedef enum { MSG_NONE = 0, MSG_BUTTON = 1, MSG_LONG_PRESS = 2, RESET = 10 };

  void init() { next(&SteamProcess::state_idle); }
  void off() { next(&SteamProcess::state_off); }
  void clear_error() { run(RESET); }
  bool is_off() { return in_state(&SteamProcess::state_off); }
  bool is_error() { return in_state(&SteamProcess::state_error); }
  bool is_busy() { return in_state(&SteamProcess::state_steaming); }
  bool is_idle() { return in_state(&SteamProcess::state_idle); }
  const char *get_error_text();
  const char *get_short_state();

protected:
  steam_process_error_t _error = STEAM_PROCESS_ERROR_NONE;
  void goto_error(steam_process_error_t err);
  void state_off();
  void state_idle();
  void state_heating();
  void state_ready();
  void state_steaming();
  void state_empty();
  void state_error();
};

extern SteamProcess steamProcess;

#endif // STEAM_PROCESS_H

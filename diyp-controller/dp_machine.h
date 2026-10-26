/*
 * dp_machine.h
 * diyPresso Machine Controller
 * Top-level orchestrator FSM: coordinates commissioning, brewing, sleep, and error states
 * (c) 2026 diyPresso - Joop van Heekeren - CC-BY-NC
 */
#ifndef MACHINE_H
#define MACHINE_H

#include <Arduino.h>
#include "dp_fsm.h"

class MachineController : public StateMachine<MachineController>
{
public:
  enum { MSG_NONE = 0, MSG_BUTTON = 1, MSG_LONG_PRESS = 2, MSG_STEAM_BUTTON = 3, MSG_STEAM_LONG_PRESS = 4 };

  MachineController() : StateMachine(&MachineController::state_commissioning) {};

  bool start_cleaning();
  bool is_cleaning() { return _cleaning_requested || in_state(&MachineController::state_cleaning); }
  const char *cleaning_status();

protected:
  bool _cleaning_requested = false;
  bool _cleaning_armed = false;
  bool _cleaning_exit = false;
  bool _cleaning_pumping = false;
  unsigned long _cleaning_pump_started = 0;

  void state_commissioning();
  void state_ready();
  void state_cleaning();
  void state_sleep();
  void state_error();
};

extern MachineController machineController;

#endif // MACHINE_H

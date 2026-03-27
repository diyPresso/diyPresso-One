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
  typedef enum { MSG_NONE = 0, MSG_BUTTON = 1, MSG_LONG_PRESS = 2 };

  MachineController() : StateMachine(&MachineController::state_commissioning) {};

protected:
  void state_commissioning();
  void state_ready();
  void state_sleep();
  void state_error();
};

extern MachineController machineController;

#endif // MACHINE_H

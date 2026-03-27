/*
 * dp_commission.h
 * diyPresso Commissioning Process
 * First-time setup: tare weight, fill boiler, purge air, verify water flow
 * (c) 2026 diyPresso - Joop van Heekeren -  CC-BY-NC
 */
#ifndef COMMISSION_H
#define COMMISSION_H

#include <Arduino.h>
#include "dp_fsm.h"

typedef enum
{
  COMMISSION_ERROR_NONE,
  COMMISSION_ERROR_PURGE,
  COMMISSION_ERROR_FILL,
  COMMISSION_ERROR_NO_WATER,
} commission_error_t;

class CommissioningProcess : public StateMachine<CommissioningProcess>
{
private:
  commission_error_t _error = COMMISSION_ERROR_NONE;
  double _start_weight = 0.0;

public:
  CommissioningProcess() : StateMachine(&CommissioningProcess::state_init) {};

  typedef enum { MSG_NONE = 0, MSG_BUTTON = 1, RESET = 10 };

  void reset() { _error = COMMISSION_ERROR_NONE; next(&CommissioningProcess::state_init); }
  void clear_error() { run(RESET); }
  bool is_done() { return in_state(&CommissioningProcess::state_done); }
  bool is_error() { return in_state(&CommissioningProcess::state_error); }
  const char *get_error_text();

protected:
  void goto_error(commission_error_t err);
  void state_init();
  void state_fill();
  void state_purge();
  void state_check();
  void state_confirm();
  void state_done();
  void state_error();
};

extern CommissioningProcess commissioningProcess;

#endif // COMMISSION_H

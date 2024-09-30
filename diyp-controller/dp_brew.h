#ifndef BREW_H
#define BREW_H

#define BREW_MIN_TEMP 93
#include <Arduino.h>
#include <Timer.h>
#include "dp_time.h"
#include "dp_reservoir.h"

#define _DP_FSM_TYPE BrewProcess // used for the state machine macro NEXT()
#include "dp_fsm.h"

// Various errors
typedef enum
{
  BREW_ERROR_NONE,
  BREW_ERROR_PURGE,
  BREW_ERROR_FILL,
  BREW_ERROR_TIMEOUT,
  BREW_ERROR_NO_WATER,
} brew_error_t;

class BrewProcess : public StateMachine<BrewProcess>
{
private:
  typedef enum BrewProcessMessages
  {
    START = 1,
    STOP = 2,
    SLEEP = 3,
    WAKEUP = 4,
    RESET = 10
  };
  brew_error_t _error;

public:
  double preInfuseTime = 3, infuseTime = 4, extractTime = 10, finishedTime = 60;
  BrewProcess() : StateMachine(STATE(state_init)) {};
  void start() { run(START); }
  void stop() { run(STOP); }
  void sleep() { run(SLEEP); }
  void wakeup() { run(WAKEUP); }
  void clear_error() { run(RESET); };
  bool is_awake() { return !IN_STATE(sleep); }
  bool is_error() { return IN_STATE(error); }
  bool is_finished() { return IN_STATE(finished); }
  bool is_init() { return IN_STATE(init); }
  bool is_fill() { return IN_STATE(fill); }
  bool is_check() { return IN_STATE(check); }
  bool is_done() { return IN_STATE(done); }
  bool is_purge() { return IN_STATE(purge); }
  bool is_busy() { return IN_STATE(pre_infuse) || IN_STATE(infuse) || IN_STATE(extract); }
  double brew_time() { return _brewTimer.read() / 1000.0; }
  double weight() { return _start_weight - reservoir.weight(); }
  double end_weight() { return _end_weight; }
  virtual const char *get_state_name();
  const char *get_error_text();
  typedef enum
  {
    MSG_NONE = 0,
    MSG_BUTTON = 1
  };

protected:
  double _start_weight = 0.0, _end_weight = 0.0;
  Timer _brewTimer = Timer();
  void state_sleep();
  void state_init();
  void state_fill();
  void state_purge();
  void state_check();
  void state_done();
  void state_idle();
  void state_empty();
  void state_error();
  void state_pre_infuse();
  void state_infuse();
  void state_extract();
  void state_finished();
  void common_transitions();
  void goto_error(brew_error_t err);
};

extern BrewProcess brewProcess;

#endif // BREW_H

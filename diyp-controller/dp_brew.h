#ifndef BREW_H
#define BREW_H

#define BREW_MIN_TEMP 93
#include <Arduino.h>
#include <Timer.h>
#include "dp_time.h"
#include "dp_reservoir.h"
#include "dp_fsm.h"

// Various errors
typedef enum
{
  BREW_ERROR_NONE,
  BREW_ERROR_TIMEOUT,
} brew_error_t;

class BrewProcess : public StateMachine<BrewProcess>
{
private:
  typedef enum BrewProcessMessages
  {
    START = 1,
    STOP = 2,
    RESET = 10
  };
  brew_error_t _error;

public:
  double preInfuseTime = 3, infuseTime = 4, extractTime = 10, finishedTime = 60;
  BrewProcess() : StateMachine(&BrewProcess::state_idle) {};
  void start() { run(START); }
  void stop() { run(STOP); }
  void clear_error() { run(RESET); };
  bool is_error() { return in_state(&BrewProcess::state_error); }
  bool is_finished() { return in_state(&BrewProcess::state_finished); }
  bool is_busy() { return in_state(&BrewProcess::state_pre_infuse) || in_state(&BrewProcess::state_infuse) || in_state(&BrewProcess::state_extract); }
  bool is_warning_almost_empty() { return in_state(&BrewProcess::state_warning_pre_brew); }
  double brew_time() { return _brewTimer.read() / 1000.0; }
  double weight() { return _start_weight - reservoir.weight(); }
  double end_weight() { return _end_weight; }
  const char *get_error_text();
  typedef enum
  {
    MSG_NONE = 0,
    MSG_BUTTON = 1
  };

protected:
  double _start_weight = 0.0, _end_weight = 0.0;
  Timer _brewTimer = Timer();
  void common_transitions();
  void goto_error(brew_error_t err);
  void state_idle();
  void state_empty();
  void state_error();
  void state_warning_pre_brew();
  void state_pre_infuse();
  void state_infuse();
  void state_extract();
  void state_finished();
};

extern BrewProcess brewProcess;

#endif // BREW_H

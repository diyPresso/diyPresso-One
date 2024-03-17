#ifndef BREW_H
#define BREW_H

#define BREW_MIN_TEMP 93
#include <Arduino.h>
#include <Timer.h>
#include "dp_time.h"
#include "dp_reservoir.h"

#define _DP_FSM_TYPE BrewProcess // used for the state machine macro NEXT()
#include "dp_fsm.h"


class BrewProcess : public StateMachine<BrewProcess>
{
  private:
    typedef enum BrewProcessMessages { START=1, STOP=2, SLEEP=3, WAKEUP=4 };
  public:
    double preInfuseTime=3, infuseTime=4, extractTime=10, finishedTime=5;
    BrewProcess() : StateMachine( STATE(state_init) ) { };
    void start() { run(START); }
    void stop() { run(STOP); }
    void sleep() { run(SLEEP); }
    void wakeup() { run(WAKEUP); }
    bool is_awake() { return ! in_state( STATE(state_sleep) ); }
    double brew_time() { return _brewTimer.read() / 1000.0; }
    double step_time() { return _state_time/1000.0; }
    double weight() { return _start_weight - reservoir.weight(); }
    double end_weight() { return _end_weight; }
    virtual const char *get_state_name();

  protected:
    double _start_weight = 0.0, _end_weight=0.0;
    Timer _brewTimer = Timer();
    void state_sleep();
    void state_init();
    void state_idle();
    void state_error();
    void state_pre_infuse();
    void state_infuse();
    void state_extract();
    void state_finished();
    void common_transitions();

};

extern BrewProcess brewProcess;


#endif // BREW_H

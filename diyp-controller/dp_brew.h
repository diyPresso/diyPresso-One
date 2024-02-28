#ifndef BREW_H
#define BREW_H

#define BREW_MIN_TEMP 93
#include <Arduino.h>
#include <Timer.h>
#include "dp_time.h"

typedef enum {
  BREW_INIT=0, BREW_IDLE=1, BREW_PRE_INFUSE=2, BREW_INFUSE=3, BREW_EXTRACT=4, BREW_FINISHED=5, BREW_ERROR=6 
} brew_state_t;

extern char *brew_state_names[];

class BrewProcess
{
    private:
        typedef void (BrewProcess::*state_function_ptr)();
        state_function_ptr _state_function = &BrewProcess::init;
        brew_state_t _prev_state=BREW_INIT;
        unsigned long _time=0, _start=0;
        Timer _brewTimer = Timer(); 
        void set_state(state_function_ptr function_ptr) {_state_function=function_ptr; _time=millis(); }

        void init();
        void idle();
        void error();
        void pre_infuse();
        void infuse();
        void extract();
        void finished();

    public:
        brew_state_t state=BREW_INIT;
        double preInfuseTime=3, infuseTime=4, extractTime=10, finishedTime=5;
        BrewProcess() { set_state(&BrewProcess::init); };
        void run(void) {if (_state_function != NULL) (this->*_state_function)();} 
        void start() { set_state(&BrewProcess::pre_infuse); }
        void stop() { set_state(&BrewProcess::idle); }
        double brew_time() { return _brewTimer.read() / 1000.0; }
        double step_time() { return time_since(_time) / 1000.0; }
};

extern BrewProcess brewProcess;


#endif // BREW_H

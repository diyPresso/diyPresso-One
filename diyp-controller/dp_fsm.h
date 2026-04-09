/*
    state machine class template
    (c) 2024 DiyEspresso - PBRI - CC-BY-NC

    Implements a FSM with class functions. Only allow valid state transitions by responding to messages to state machine.
    The state function can respond to entry and exit conditions, and request a new state to be set.

    You can create a state-machine from this template class in this way:

    class MyStateMachine : public StateMachine<MyStateMachine>
    {
        private:
            void state_1();
            void state_2();
            void state_3();
        public:
            MyStateMachine() : StateMachine(&MyStateMachine::state_1) {}
    };

    // a prototype state handler function
    MyStateMachine::state_1()
    {
        ON_ENTRY() { ON_ENTRY_CODE ... }  // Executed once, if we enter this state. Also sets state name via __func__.
        ON_TIMEOUT_SEC(10) next(&MyStateMachine::state_3); // Executed when we are longer in this state than timeout [seconds]
        ON_MESSAGE(MSG_A) next(&MyStateMachine::state_2); // Executed when we receive a message
        ON_EXIT() { ON_EXIT_CODE ... } // use as last statement in function, executed once if we leave this state
    }

    To execute the state machine:

    if ( stateMachine.run(msg) )
    {
        error("Unhandled message [msg] in state [state]");
    }

    Note: Every state function MUST use ON_ENTRY() to ensure the state name is set correctly.

*/

#ifndef _DP_FSM_H
#define _DP_FSM_H

#include <string.h>

template<typename T>
class StateMachine
{
    public:
        typedef void (T::*state_function_ptr)();

    protected:
        state_function_ptr _cur_state, _next_state, _prev_state;
        const char* _state_name = "<none>";
        int _message = 0;
        unsigned long _state_time = 0;
        void next(state_function_ptr state) { _next_state = state; };
        bool on_entry(const char* func_name) {
            // Strip "state_" prefix (6 chars) if present
            if (strncmp(func_name, "state_", 6) == 0) func_name += 6;
            _state_name = func_name;
            return _cur_state != _prev_state;
        }
        bool on_exit() { return _cur_state != _next_state; }
        bool on_timeout( unsigned long duration ) { return (_state_time+duration) < millis(); }
        bool on_message(int msg) { if ( msg == _message) { _message = 0; return true; } return false; }
        bool no_message() { return _message == 0; }
        bool is_prev_state(state_function_ptr state) { return _prev_state == state; }
        bool is_next_state(state_function_ptr state) { return _next_state == state; }
        bool is_in_state(state_function_ptr state) { return _cur_state == state; }
        void reset_timeout() { _state_time = millis(); }
        void state_none() { }

    public:
        StateMachine(state_function_ptr initial_state)
            : _cur_state(initial_state), _next_state(initial_state),
              _prev_state(&StateMachine::state_none) {}

        bool in_state(state_function_ptr state) { return _cur_state == state; }
        bool run() { return run(0); }
        bool run(int msg)
        {
            _message = msg;
            (((T*)this)->*_cur_state)();
            _prev_state = _cur_state;
            if ( _next_state != _cur_state )
            {
                _cur_state = _next_state;
                _next_state = _cur_state;
                _state_time = millis();
            }
            return !no_message();
        }
        double state_time() { unsigned long t = millis()-_state_time; if (t > 0) return t/1000.0; else return (0xFFFFFFFF-t)/1000.0; }

        const char *get_state_name() { return _state_name; }
};

// Convenience macros
// ON_ENTRY() automatically sets the state name from __func__ (stripping "state_" prefix)
#define ON_ENTRY() if ( on_entry(__func__) )
#define ON_EXIT() if ( on_exit() )
#define ON_TIMEOUT(t) if ( on_timeout(t) )
#define ON_TIMEOUT_SEC(t) if ( on_timeout((1000*t)) )
#define ON_MESSAGE(m) if ( on_message(m) )

#endif // _DP_FSM_H
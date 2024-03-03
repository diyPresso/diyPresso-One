/*
    state machine class template
    (c) 2024 DiyEspresso - PBRI - CC-BY-NC

    Implements a FSM with class functions. Only allow valid state transitions by responding to messages to state machine.
    The state function can respond to entry and exit conditions, and request a new state to be set.

    You can create a state-machine from this template class in this way:

    class MyStateMachine : public StateMachine<MyStateMachine>
    {
        private:
            void state1();
            void state2();
            void state3();
    };


    // a prototype state handler function
    MyStateMachine::state_function()
    {
        if ( on_entry() ) ON_ENTRY_CODE ...  // Executed once, if we enter this state
        if ( on_timeout(10.0) ) next(error_state_function); // Executed when we are longer in this state than timeout [seconds]
        if ( on_message(MSG_A) ) next(state_function1); // Executed when we receive a message
        else if ( on_message(MSG_B) ) next(state_function2); // Note: only 1 message per handler execution is received
        if ( on_exit() ) ON_EXIT_CODE ... // use as last statement in function, executed once if we leave this state
    }

    To execute the state machine:

    if ( stateMachine.run(msg) )
    {
        error("Unhandled message [msg] in state [state]");
    }

*/


template<typename T>
class StateMachine
{
    public:
        typedef void (T::*state_function_ptr)();

    protected:
        state_function_ptr _cur_state, _next_state, _prev_state;
        int _message = 0;
        unsigned long _state_time = 0;
        void next(state_function_ptr state) { _next_state = state; };
        bool on_entry() { return _cur_state != _prev_state; }
        bool on_exit() { return _cur_state != _next_state; }
        bool on_timeout( unsigned long duration ) { return (_state_time+duration) < millis(); }
        bool on_message(int msg) { if ( msg == _message) { _message = 0; return true; } return false; }
        bool no_message() { return _message == 0; }
        void state_none() { }

    public:
        StateMachine(state_function_ptr initial_state)
        {
            _cur_state = initial_state;
            _next_state = &StateMachine::state_none;
            _prev_state = &StateMachine::state_none;
        }
        bool in_state(state_function_ptr s) { return _cur_state == s; }
        bool run() { run(0); }
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
};

// Some convenient macros (note: set _DP_FSM_TYPE to the Class name of your state machine before including this header to use them)
#define NEXT(state) next(&_DP_FSM_TYPE::state)
#define ON_ENTRY() if ( on_entry() )
#define ON_EXIT() if ( on_exit() )
#define ON_TIMEOUT(t) if ( on_timeout(t) )
#define ON_MESSAGE(m) if ( on_message(m) )
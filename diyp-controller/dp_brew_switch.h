#ifndef BREW_HANDLE_H
#define BREW_HANDLE_H
#include <Arduino.h>
#include "dp_hardware.h"

class BrewSwitch
{
    public:
        BrewSwitch() { pinMode(PIN_BREW_SWITCH, INPUT_PULLUP); }
        bool up(void) { return digitalRead(PIN_BREW_SWITCH); } 
        bool down() { return !up(); }
};

extern BrewSwitch brewSwitch;

#endif // BREW_HANDLE_H

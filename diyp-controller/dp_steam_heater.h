/* Steam Thermoblock Heater with software PWM
 (c) 2026 - CC-BY-NC - diyPresso
*/

#ifndef STEAM_HEATER_H
#define STEAM_HEATER_H
#include <Arduino.h>
#include "dp_hardware.h"

class SteamHeater
{
    private:
        double _power=0.0, _average=0.0; // [0..100%]
        unsigned long _pwm_period = 1000000, _time=0, _period=0; // microsec, default PWM = 1 sec
        bool _on = false;
    public:
        void init(void) { pinMode(PIN_SSR_STEAM_HEATER, OUTPUT); off(); }
        void control(void);
        void on(void) { control(); }
        void off(void) { _power = 0.0; control(); }
        void power(double p) { _power = min(100, max(p, 0)); control(); }
        double power() { return _power; }
        double average() { return _average; }
        bool is_on(void) { return _on; }
};

extern SteamHeater steamHeater;

#endif // STEAM_HEATER_H

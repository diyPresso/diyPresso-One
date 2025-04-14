/* Heater Device with software PWM (Pulse Width Modulation)
 (c) 2025 - CC-BY-NC - diyPresso
*/

#ifndef HEATER_H
#define HEATER_H
#include <Arduino.h>
#include "dp_hardware.h"
#include "dp_led.h"

class HeaterDevice
{
    private:
        double _power=0.0, _average=0.0; // [0..100%]
        unsigned long _pwm_period = 1000000, _time=0, _period=0; // microsec, default PWM = 1 sec]
        bool _on = false;
    public:
        HeaterDevice() { pinMode(PIN_SSR_HEATER, OUTPUT); off();  }
        void control(void); // control PWM output
        void pwm_period(double t) { _pwm_period =  min(1E7, max(1E5, t*1E6)); control(); } // set pwm period in [sec] between 0.1 and 10.0 sec
        void on(void) { _power = 100.0; control();  } // sets power to 100%, not really an on switch
        void off(void) { _power = 0.0; control();  } // sets power to 0%, not really an off switch
        void power(double p) { _power = min(100, max(p, 0)); control(); }
        double power() { return _power; }
        double average() { return _average; }
        bool is_on(void) { return _on; }
        double pwm_period() { return _pwm_period / 1E6; } // actual PWM period in [sec]
};

extern HeaterDevice heaterDevice;

#endif // header

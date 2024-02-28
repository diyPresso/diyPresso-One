/* Heater Device with software PWM 
 (c) 2024 - CC-BY-NC - diyEspresso - PBRI
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
        void pwm_period(double t) { _pwm_period =  min(1E7, max(1000, t*1E6)); control(); } // set pwm period in [sec] between 0.001 and 10.0 sec
        void on(void) { _power = 100.0; control();  } 
        void off(void) { _power = 0.0; control();  }
        void power(double p) { _power = min(100, max(p, 0)); control(); }
        double power() { return _power; }
        double average() { return _average; }
        bool is_on(void) { return _on; }
        double pwm_period() { return _pwm_period / 1E6; } // actual PWM period in [sec]
};

extern HeaterDevice heaterDevice;

#endif // header

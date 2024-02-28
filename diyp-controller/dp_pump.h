#ifndef PUMP_H
#define PUMP_H

#include <Arduino.h>
#include "dp_hardware.h"

class PumpDevice
{
    private:       
      bool _on=false;
    public:
      PumpDevice() { pinMode(PIN_SSR_PUMP, OUTPUT); off(); }
      void on(void) { _on=true; digitalWrite(PIN_SSR_PUMP, HIGH); } 
      void off() { _on = false; digitalWrite(PIN_SSR_PUMP, LOW); }
      bool is_on(void) { return _on; } 
};

extern PumpDevice pumpDevice;

#endif // PUMP_H

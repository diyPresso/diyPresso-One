/* Heater control with PWM (Pulse Width Modulation)
 (c) 2025 - CC-BY-NC - diyPresso
*/
#include "dp_heater.h"
#include "dp_time.h"

#define LPF_FACTOR 0.01   // Low pass filter coefficient. Smaller is lower bandwidth

HeaterDevice heaterDevice = HeaterDevice();

/* PWM control,  
Note: we need to be called at least 1x per PWM period to work correctly, and not faster than 1000 Hz (due to timer resolution) 
TODO: Remove float calculations from loop
*/

void HeaterDevice::control(void)
{
  unsigned long on_period = (_power/100.0) * _pwm_period;
  unsigned long delta = usec_since(_time);

  _period += delta;  
  if ( _period >= _pwm_period )
    _period -= _pwm_period;
  if ( _period < on_period )
  {
    digitalWrite(PIN_SSR_HEATER, HIGH); 
    _on=true;
  }
  else
  {
    digitalWrite(PIN_SSR_HEATER, LOW); 
    _on=false;
  }
  _average = LPF_FACTOR * _power + (1.0-LPF_FACTOR) * _average; //TODO: need delta in here
  if ( delta ) // update time if not zero
    _time = micros();
}

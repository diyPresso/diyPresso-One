/* Steam Thermoblock Heater with software PWM
 (c) 2026 - CC-BY-NC - diyPresso
*/
#include "dp_steam_heater.h"
#include "dp_time.h"

#define LPF_FACTOR 0.01

SteamHeater steamHeater = SteamHeater();

void SteamHeater::control(void)
{
  unsigned long on_period = (_power/100.0) * _pwm_period;
  unsigned long delta = usec_since(_time);

  _period += delta;
  if ( _period >= _pwm_period )
    _period -= _pwm_period;
  if ( _period < on_period )
  {
    digitalWrite(PIN_SSR_STEAM_HEATER, HIGH);
    _on = true;
  }
  else
  {
    digitalWrite(PIN_SSR_STEAM_HEATER, LOW);
    _on = false;
  }
  _average = LPF_FACTOR * _power + (1.0-LPF_FACTOR) * _average;
  if ( delta )
    _time = micros();
}

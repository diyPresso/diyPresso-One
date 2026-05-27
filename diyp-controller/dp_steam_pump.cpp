/*
 * dp_steam_pump.cpp
 * Steam Pump device with software PWM (Model Two only)
 * (c) 2026 diyPresso
 */
#include "dp_steam_pump.h"
#include "dp_time.h"

SteamPump steamPump = SteamPump();

void SteamPump::control(void)
{
  if (!_on) {
    digitalWrite(PIN_SSR_STEAM_PUMP, LOW);
    _time = micros();
    _period = 0;
    return;
  }

  unsigned long on_period = (_power / 100.0) * _pwm_period;
  unsigned long delta = usec_since(_time);

  _period += delta;
  if (_period >= _pwm_period)
    _period -= _pwm_period;
  if (_period < on_period)
  {
    digitalWrite(PIN_SSR_STEAM_PUMP, HIGH);
  }
  else
  {
    digitalWrite(PIN_SSR_STEAM_PUMP, LOW);
  }
  if (delta)
    _time = micros();
}

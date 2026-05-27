/*
 * dp_steam_pump.h
 * Steam Pump device with software PWM (Model Two only)
 * (c) 2026 diyPresso
 */
#ifndef STEAM_PUMP_H
#define STEAM_PUMP_H

#include <Arduino.h>
#include "dp_hardware.h"

class SteamPump
{
private:
  double _power = 100.0; // [0..100%]
  unsigned long _pwm_period = 400000, _time = 0, _period = 0; // microsec, PWM = 400ms
  bool _on = false;

public:
  void init(void) { pinMode(PIN_SSR_STEAM_PUMP, OUTPUT); off(); }
  void control(void);
  void on() { _on = true; control(); }
  void off() { _on = false; control(); }
  void power(double p) { _power = min(100.0, max(p, 0.0)); control(); }
  double power() { return _power; }
  bool is_on() { return _on; }
};

extern SteamPump steamPump;

#endif // STEAM_PUMP_H

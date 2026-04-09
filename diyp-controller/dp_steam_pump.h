/*
 * dp_steam_pump.h
 * Steam Pump device (Model Two only)
 * (c) 2026 diyPresso
 */
#ifndef STEAM_PUMP_H
#define STEAM_PUMP_H

#include <Arduino.h>
#include "dp_hardware.h"

class SteamPump
{
private:
  bool _on = false;

public:
  SteamPump() { pinMode(PIN_SSR_STEAM_PUMP, OUTPUT); off(); }
  void on() { _on = true; digitalWrite(PIN_SSR_STEAM_PUMP, HIGH); }
  void off() { _on = false; digitalWrite(PIN_SSR_STEAM_PUMP, LOW); }
  bool is_on() { return _on; }
};

extern SteamPump steamPump;

#endif // STEAM_PUMP_H

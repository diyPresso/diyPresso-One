/*
 * dp_steam_switch.h
 * Steam Switch input — momentary push button (Model Two only)
 * (c) 2026 diyPresso
 */
#ifndef STEAM_SWITCH_H
#define STEAM_SWITCH_H

#include <Arduino.h>
#include "dp_hardware.h"

class SteamSwitch
{
public:
  SteamSwitch() { pinMode(PIN_STEAM_SWITCH, INPUT_PULLUP); }
  void read();
  bool pressed() { return _pressed; }
  bool long_pressed() { return _long_pressed; }

private:
  bool _prev_state = HIGH;
  unsigned long _press_time = 0;
  bool _pressed = false;
  bool _long_pressed = false;
  bool _held = false;
  bool _long_fired = false;
  static constexpr unsigned long DEBOUNCE_MS = 50;
  static constexpr unsigned long LONG_PRESS_MS = 1000;
};

extern SteamSwitch steamSwitch;

#endif // STEAM_SWITCH_H

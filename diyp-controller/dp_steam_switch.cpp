/*
 * dp_steam_switch.cpp
 * Steam Switch input — momentary push button (Model Two only)
 * (c) 2026 diyPresso
 */
#include "dp_steam_switch.h"

SteamSwitch steamSwitch;

void SteamSwitch::read()
{
  _pressed = false;
  _long_pressed = false;

  bool state = digitalRead(PIN_STEAM_SWITCH);

  // While held, check for long press threshold
  if (_held && !_long_fired && (millis() - _press_time) >= LONG_PRESS_MS)
  {
    _long_pressed = true;
    _long_fired = true;
  }

  if (state == _prev_state)
    return;
  if ((millis() - _press_time) < DEBOUNCE_MS)
    return;

  _prev_state = state;

  if (state == LOW) // button pressed down
  {
    _press_time = millis();
    _held = true;
    _long_fired = false;
  }
  else // button released
  {
    _held = false;
    if (!_long_fired) // only short press if long press didn't fire
      _pressed = true;
  }
}

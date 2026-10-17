/*
 * dp_brew_integral_hold.cpp
 * Brew integral hold FSM, see dp_brew_integral_hold.h
 * (c) 2026 diyPresso
 */
#include "dp_brew_integral_hold.h"

void BrewIntegralHold::update(bool brew, double actTemp, double setTemp)
{
  _brew = brew;
  _act_temp = actTemp;
  _set_temp = setTemp;
  run();
}

void BrewIntegralHold::reset()
{
  _cur_state = _next_state = &BrewIntegralHold::state_idle;
  _state_name = "idle";
  reset_timeout();
}

bool BrewIntegralHold::is_holding()
{
  return in_state(&BrewIntegralHold::state_hold_brew) ||
         in_state(&BrewIntegralHold::state_hold_wait_dip) ||
         in_state(&BrewIntegralHold::state_hold_recover);
}

void BrewIntegralHold::state_idle()
{
  ON_ENTRY() {}
  if (_brew)
    next(&BrewIntegralHold::state_armed_brew);
}

void BrewIntegralHold::state_armed_brew()
{
  ON_ENTRY() {}
  if (_act_temp < _set_temp)
    next(&BrewIntegralHold::state_hold_brew);
  else if (!_brew)
  {
    brew_ended();
    next(&BrewIntegralHold::state_armed_window);
  }
}

void BrewIntegralHold::state_armed_window()
{
  ON_ENTRY() {}
  if (_brew)
    next(&BrewIntegralHold::state_armed_brew);
  else if (_act_temp < _set_temp)
    next(&BrewIntegralHold::state_hold_wait_dip);
  else ON_TIMEOUT(_dip_window_ms) // time in this state = time since the brew ended
    next(&BrewIntegralHold::state_idle);
}

void BrewIntegralHold::state_hold_brew()
{
  ON_ENTRY() {}
  if (!_brew)
  {
    brew_ended();
    next(&BrewIntegralHold::state_hold_wait_dip);
  }
}

void BrewIntegralHold::state_hold_wait_dip()
{
  ON_ENTRY() {}
  if (_brew)
    next(&BrewIntegralHold::state_hold_brew);
  else if (timed_out())
    next(&BrewIntegralHold::state_idle);
  else if (_act_temp >= _set_temp && dip_window_passed()) // small dip, or the dip ended during the brew. Not earlier: the dip arrives late (sensor dead time)
    next(&BrewIntegralHold::state_idle);
  else if (_act_temp < _set_temp - BREW_HOLD_DIP_BAND)
    next(&BrewIntegralHold::state_hold_recover);
}

void BrewIntegralHold::state_hold_recover()
{
  ON_ENTRY() {}
  if (_brew)
    next(&BrewIntegralHold::state_hold_brew);
  else if (timed_out() || _act_temp >= _set_temp)
    next(&BrewIntegralHold::state_idle);
}

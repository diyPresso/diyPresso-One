/*
 * dp_brew_integral_hold.h
 * Brew integral hold FSM: freezes the boiler PID integral term during the post-brew temperature dip
 * (c) 2026 diyPresso
 *
 * The pre-brew integral term is the steady state value. Letting it build up during the post-brew dip
 * causes overshoot, as it can only unwind by overshooting. Between brew start and the dip, the integral
 * may still correct a pre-brew excess, so the hold starts at the dip, not at brew start.
 *
 * - A brew arms the hold. The hold activates the first time T < setpoint while armed.
 * - Arming ends dipWindowSec after the brew ended. No dip within the window: no hold.
 * - Release: after the brew ended, T rises back to setpoint (hold_recover when T has been below
 *   setpoint - BREW_HOLD_DIP_BAND; hold_wait_dip for a small dip or a dip that ended during the brew,
 *   but only after dipWindowSec since the brew ended, as the dip arrives late at the sensor),
 *   or timeoutSec after the brew ended (safety timeout).
 * - A new brew re-arms, or keeps an active hold and restarts the release logic.
 *
 * States:
 *   idle          -> brew: armed_brew
 *   armed_brew    -> T < SP: hold_brew, brew ended: armed_window
 *   armed_window  -> brew: armed_brew, T < SP: hold_wait_dip, dip window: idle
 *   hold_brew     -> brew ended: hold_wait_dip
 *   hold_wait_dip -> brew: hold_brew, T >= SP after dip window: idle, T < SP - band: hold_recover, timeout: idle
 *   hold_recover  -> brew: hold_brew, T >= SP: idle, timeout: idle
 */
#ifndef BREW_INTEGRAL_HOLD_H
#define BREW_INTEGRAL_HOLD_H

#include <Arduino.h>
#include "dp_fsm.h"

#define BREW_HOLD_TIMEOUT_SEC 180   // release the hold this long after the brew ended [sec]
#define BREW_HOLD_DIP_WINDOW_SEC 60 // the hold can only activate until this long after the brew ended [sec]
#define BREW_HOLD_DIP_BAND 0.2      // temperature must drop this far below setpoint to count as the post-brew dip [degC]

class BrewIntegralHold : public StateMachine<BrewIntegralHold>
{
public:
  BrewIntegralHold() : StateMachine(&BrewIntegralHold::state_idle) {};

  void update(bool brew, double actTemp, double setTemp); // call once per control cycle
  void reset(); // cancel the hold and arming immediately
  bool is_holding();

  void set_timeout_sec(unsigned long sec) { _timeout_ms = sec * 1000UL; }
  void set_dip_window_sec(unsigned long sec) { _dip_window_ms = sec * 1000UL; }

private:
  bool _brew = false;
  double _act_temp = 0, _set_temp = 0;
  unsigned long _brew_end_time = 0;
  unsigned long _timeout_ms = BREW_HOLD_TIMEOUT_SEC * 1000UL;
  unsigned long _dip_window_ms = BREW_HOLD_DIP_WINDOW_SEC * 1000UL;

  void brew_ended() { _brew_end_time = millis(); }
  bool timed_out() { return (millis() - _brew_end_time) > _timeout_ms; }
  bool dip_window_passed() { return (millis() - _brew_end_time) > _dip_window_ms; }

  void state_idle();
  void state_armed_brew();
  void state_armed_window();
  void state_hold_brew();
  void state_hold_wait_dip();
  void state_hold_recover();
};

#endif // BREW_INTEGRAL_HOLD_H

# diyPresso State Machines

## Machine Controller + Sub-FSMs

The MachineController is the top-level orchestrator. It dispatches to the CommissioningProcess
and BrewProcess sub-FSMs in the `commissioning` and `ready` states respectively.

```mermaid
stateDiagram-v2
    [*] --> commissioning

    state commissioning {
        [*] --> c_init
        c_init --> c_done : commissioningDone
        c_init --> c_fill : button (tare + pump)
        c_fill --> c_purge : timeout (pump done)
        c_purge --> c_check : lever up
        c_purge --> c_error : timeout
        c_check --> c_confirm : button (weight OK)
        c_check --> c_error : button (no water)
        c_check --> c_error : timeout
        c_confirm --> c_done : lever down
        c_confirm --> c_error : timeout
        c_error --> c_init : RESET
    }

    commissioning --> ready : commissioning done
    commissioning --> error : commissioning error

    state ready {
        [*] --> idle

        idle --> warning : lever up + almost empty
        idle --> pre_infuse : lever up
        idle --> empty : reservoir empty

        warning --> idle : lever down
        warning --> pre_infuse : button (override)
        warning --> empty : reservoir empty

        empty --> idle : refilled + lever down

        pre_infuse --> infuse : timeout (pre‑infuse time)
        pre_infuse --> idle : lever down
        pre_infuse --> empty : reservoir empty

        infuse --> extract : timeout (infuse time)
        infuse --> idle : lever down
        infuse --> empty : reservoir empty

        extract --> finished : timeout (extract time)
        extract --> idle : lever down
        extract --> empty : reservoir empty

        finished --> idle : lever down
        finished --> extract : button (extend)
        finished --> b_error : timeout
        finished --> empty : reservoir empty

        b_error --> idle : lever down / RESET
    }

    ready --> sleep : long press / auto‑timeout
    ready --> error : brew or boiler error
    ready --> commissioning : !commissioningDone

    sleep --> ready : button / long press

    error --> ready : button (clear all errors)
```

## Boiler Controller

The BoilerStateMachine runs independently as a peer controller (PID + safety).

```mermaid
stateDiagram-v2
    [*] --> off

    off --> heating : on()

    heating --> off : off()
    heating --> ready : temp within window
    heating --> brew : start_brew()
    heating --> error : timeout

    ready --> off : off()
    ready --> heating : temp outside window
    ready --> brew : start_brew()
    ready --> error : timeout

    brew --> off : off()
    brew --> heating : stop_brew()
    brew --> error : timeout

    error --> off : error cleared
```

## Brew Integral Hold

The BrewIntegralHold FSM is owned by the BoilerStateMachine and runs every boiler cycle. It freezes the
boiler PID integral term during the post-brew temperature dip, so the integral does not build up and
cause overshoot. A brew only arms the hold; the hold activates at the dip (T < setpoint), so before the
dip the integral can still correct a pre-brew excess.

It is reset to `idle` on boiler off, error, setpoint change, or when `brewIntegralHold=0`.
"Timeout" is `brewHoldTimeoutSec` after the brew ended; "dip window" is `brewHoldDipWindowSec` after the
brew ended; "band" is `BREW_HOLD_DIP_BAND` (0.2 °C). The state is shown as `brewIHold` in the PID serial
output and as `brewIntegralHoldState` in `GET info`.

```mermaid
stateDiagram-v2
    [*] --> idle

    idle --> armed_brew : brew

    armed_brew --> hold_brew : T < SP
    armed_brew --> armed_window : brew ended

    armed_window --> armed_brew : brew
    armed_window --> hold_wait_dip : T < SP
    armed_window --> idle : dip window (no dip)

    hold_brew --> hold_wait_dip : brew ended

    hold_wait_dip --> hold_brew : brew
    hold_wait_dip --> idle : T >= SP after dip window (small dip)
    hold_wait_dip --> hold_recover : T < SP - band
    hold_wait_dip --> idle : timeout

    hold_recover --> hold_brew : brew
    hold_recover --> idle : T >= SP
    hold_recover --> idle : timeout
```

Integral hold is active in `hold_brew`, `hold_wait_dip` and `hold_recover`. The dip arrives at the
sensor 35–50 s after brew start, so in `hold_wait_dip` T ≥ SP only releases once the dip window has
passed since the brew ended.

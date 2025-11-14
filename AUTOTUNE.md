# PID Auto-Tune Feature

## Overview
The auto-tune feature uses the Relay (Ziegler-Nichols) method to automatically determine optimal PID parameters for the boiler temperature control system.

## How It Works

### Relay Method
1. **Relay Control**: Oscillates heater power between 0 and `relayAmplitude`
2. **Peak Detection**: Tracks temperature peaks and valleys with noise band filtering (±0.15°C)
3. **Period Calculation**: Measures oscillation period (Pu) from peak-to-peak timing
4. **Gain Calculation**: Calculates ultimate gain (Ku) using formula: `Ku = (4 × d) / (π × a)`
   - `d` = relay amplitude (typically 20%)
   - `a` = oscillation amplitude (half of peak-to-valley difference)

### Ziegler-Nichols Tuning Rules (Classic PID)
- **Kp** = 0.6 × Ku
- **Ki** = 1.2 × Ku / Pu  
- **Kd** = 0.075 × Ku × Pu

## Usage via Serial Commands

### Start Auto-Tune
```
PUT boilerPidAutotune start           # Uses default relay amplitude (20%)
```

**Requirements:**
- Boiler must be ON and in HEATING or READY state
- Temperature must be stable (within 5°C of setpoint)

### Monitor Progress
```
GET boilerPidAutotune
```

**Response states:**
- `IDLE` - Not running
- `RUNNING` - Collecting oscillation data
- `SUCCESS` - Tuning completed successfully
- `FAILED_TIMEOUT` - Timed out waiting for oscillations
- `FAILED_NO_OSCILLATION` - Insufficient oscillation amplitude

When SUCCESS, also returns:
```
state=SUCCESS
Kp=12.345
Ki=0.678
Kd=90.123
Ku=20.575
Pu=45.230
GET boilerPidAutotune OK
```

### Cancel Auto-Tune
```
PUT boilerPidAutotune cancel
```
Immediately stops auto-tune and returns to normal PID control.

### Apply Results
```
PUT boilerPidAutotune apply
```
Applies the calculated PID parameters to the active controller. Only works if auto-tune completed successfully.

## Parameters

### Relay Amplitude
- **Value**: 20% (defined by `AUTOTUNE_RELAY_OUTPUT`)
- **Effect**: 
  - Controls the magnitude of power oscillations
  - Fixed value ensures consistent and predictable auto-tune behavior
  - Can be changed by modifying the constant in `dp_pid.h` if needed

### Setpoint Band
- **Value**: ±0.5°C (defined by `AUTOTUNE_SETPOINT_BAND`)
- **Purpose**: Hysteresis band around setpoint for relay switching (prevents chattering)

### Peak Detection Noise Band
- **Value**: ±0.15°C (defined by `AUTOTUNE_PEAK_NOISE_BAND`)
- **Purpose**: Filters out measurement noise to avoid false peak detection during oscillations

### Timeout
- **Value**: 600 seconds (10 minutes) (defined by `AUTOTUNE_TIMEOUT_MS`)
- **Purpose**: Prevents infinite loops if oscillations don't occur

### Minimum Oscillations
- **Value**: 3 complete cycles (defined by `AUTOTUNE_MIN_CYCLES`)
- **Purpose**: Ensures stable period measurement

## Implementation Details

### Files Modified
- `dp_pid.h` / `dp_pid.cpp`: Core auto-tune logic
- `dp_boiler.h`: Wrapper methods for boiler controller
- `dp_serial.h` / `dp_serial.cpp`: Serial command interface

### Key Classes/Methods
- `AutoTuneResults` struct: Holds Kp, Ki, Kd, Ku, Pu, isValid
- `DpPID::startAutoTune()`: Initiates auto-tune with default relay amplitude
- `DpPID::processAutoTune()`: Runs during each control cycle
- `DpPID::getAutoTuneResults()`: Returns calculated parameters
- `DpPID::cancelAutoTune()`: Stops auto-tune

### State Machine
```
IDLE → RUNNING → SUCCESS
            ↓
    FAILED_TIMEOUT
            ↓
    FAILED_NO_OSCILLATION
```

## Best Practices

1. **Start with stable temperature**: Let the boiler reach setpoint with existing PID before starting
2. **Wait for completion**: Typical auto-tune takes 2-4 minutes
3. **Review results**: Check that Kp, Ki, Kd values are reasonable before applying
4. **Fine-tune if needed**: Ziegler-Nichols provides a good starting point but may need manual adjustment

## Troubleshooting

### Auto-Tune Won't Start
- Check boiler is ON (`boilerController.is_on()`)
- Check boiler is in valid state (HEATING or READY)
- Check temperature is within 5°C of setpoint

### Auto-Tune Times Out
- Relay amplitude may be too small (adjust `AUTOTUNE_RELAY_OUTPUT` in `dp_pid.h` if needed)
- System may have excessive lag
- Check heater is responding correctly

### Results Seem Wrong
- Oscillations may not be stable
- Try running auto-tune again
- Check for external disturbances (brew cycle, etc.)
- Consider adjusting `AUTOTUNE_RELAY_OUTPUT` in `dp_pid.h` if oscillations are consistently too large or too small

## Example Session

```
# Start auto-tune
PUT boilerPidAutotune start
> PUT boilerPidAutotune OK, Auto-tune started

# Monitor progress (repeat periodically)
GET boilerPidAutotune
> state=RUNNING
> GET boilerPidAutotune OK

# Wait for completion...
GET boilerPidAutotune
> state=SUCCESS
> Kp=12.500
> Ki=0.650
> Kd=85.000
> Ku=20.833
> Pu=46.154
> GET boilerPidAutotune OK

# Apply the results
PUT boilerPidAutotune apply
> PUT boilerPidAutotune OK, Applied Kp=12.500 Ki=0.650 Kd=85.000
```

## Technical Notes

- Auto-tune runs in the same control loop as normal PID (`DpPID::compute()`)
- When auto-tune is active, normal PID calculation is bypassed
- Peak detection uses rising/falling edge tracking with noise band hysteresis
- Period is averaged over all detected oscillations for accuracy
- Results are stored until next auto-tune or controller reset

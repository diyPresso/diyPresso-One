/*
 * diyPresso global header file
 */
#ifndef DP_H
#define DP_H

#include <Arduino.h>

// #define SIMULATE // Define this to compile as SIMULATED device (no hardware)
#define WATCHDOG_ENABLED // if not defined: Watchdog is disabled! ENABLE FOR PRODUCTION!!!!

#define AUTOSLEEP_TIMEOUT  (60*60.0) // [sec] When longer than this time in idle, goto sleep
#define INITIAL_PUMP_TIME (10.0) // time to pump at startup [sec]
#define INITIAL_WEIGHT_DROP (10.0) // [grams] min required drop in reservoir at initial pump
#define MAINTAIN_PUMP_TIME (5.0)    // time to pump while maintaining reservoir level [sec]
#define MAINTAIN_WEIGHT_VARIATION (3.0) // [grams] max reservoir variation during pump/maintain state

#define SOFTWARE_VERSION "1.4.0"

#endif // LED_H
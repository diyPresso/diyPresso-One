/*
 * diyPresso global header file
 */
#ifndef DP_H
#define DP_H

#include <Arduino.h>

// #define SIMULATE // Define this to compile as SIMULATED device (no hardware)
#define WATCHDOG_ENABLED // if not defined: Watchdog is disabled! ENABLE FOR PRODUCTION!!!!

#define AUTOSLEEP_TIMEOUT  (60*60.0) // [sec] When longer than this time in idle, goto sleep
#define INITIAL_PUMP_TIME (60.0) // time to pump at startup [sec]
#define FILL_WEIGHT_VARIATION (3.0) // Weight variation after filling
#define PURGE_TIMEOUT (60.0)
#define PURGE_WEIGHT_DROP (50.0) // [grams] max reservoir variation during pump/maintain state

#define SOFTWARE_VERSION "1.6.0"

#endif // LED_H
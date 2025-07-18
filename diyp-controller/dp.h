/*
 * diyPresso global header file
 */
#ifndef DP_H
#define DP_H

#include <Arduino.h>

// #define SIMULATE // Define this to compile as SIMULATED device (no hardware)
#define WATCHDOG_ENABLED // if not defined: Watchdog is disabled! ENABLE FOR PRODUCTION!!!!

#define AUTOSLEEP_TIMEOUT (60 * 60.0)   // [sec] When longer than this time in idle, goto sleep
#define INITIAL_PUMP_TIME (30.0)        // time to pump at startup [sec]
#define FILL_WEIGHT_DROP_MINIMUM (10.0) // Mimimum Weight drop after filling boiler [grams]
#define PURGE_TIMEOUT (30.0)
#define PURGE_WEIGHT_DROP_MINIMUM (50.0) // Minimum weight drop after purging [gram]

#define SOFTWARE_VERSION "1.7.1"
#define BUILD_DATE __DATE__ " " __TIME__

#endif // DP_H
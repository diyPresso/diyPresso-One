/*
 * diyPresso global header file
 */
#ifndef DP_H
#define DP_H

#include <Arduino.h>

// #define SIMULATE // Define this to compile as SIMULATED device (no hardware)
#define WATCHDOG_ENABLED // if not defined: Watchdog is disabled! ENABLE FOR PRODUCTION!!!!
#define  AUTOSLEEP_TIMEOUT  (60*60.0) // When longer than this time in idle, goto sleep



#define SOFTWARE_VERSION "1.3.0"

#endif // LED_H
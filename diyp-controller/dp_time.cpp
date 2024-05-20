/*
 Time functions
 */
#include <Arduino.h>

const ulong UL_MAX = 4294967295;

// return difference between two timestamps, where `ts1` is larger than `ts2`, handles wraparound
unsigned long time_diff(unsigned long ts1, unsigned long ts2)
{
    if (ts1 < ts2) // Handle overflow
		return (UL_MAX - ts2) + ts1;
	else
		return ts1 - ts2;
}

// return time since timestamp in msec 
unsigned long time_since(unsigned long ts)
{
    return time_diff(millis(), ts);
}

// return time since timestamp in microseconds 
unsigned long usec_since(unsigned long ts)
{
    return time_diff(micros(), ts);
}

// return true/false, period approx 1 sec
bool blink ()
{
    return (millis() >> 9) & 1;
}

#ifndef TIME_H
#define TIME_H
#include <Arduino.h>

unsigned long time_diff(unsigned long ts1, unsigned long ts2);
unsigned long time_since(unsigned long ts);
unsigned long usec_since(unsigned long usec_ts);

#endif


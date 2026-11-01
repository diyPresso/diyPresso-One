/*
  Shot counter log: stores the shot counters in their own flash area, separate from the settings.

  Two flash rows of 16 byte records. Each save appends a record to an erased (0xFF) slot, a row is
  only erased when the log moves to it, so a row erase is done once every 16 saves and the row with
  the newest record is never erased. A record that is cut off by a power loss has an invalid crc and
  is skipped, the previous record is used. A shot does not rewrite the settings, so a power loss
  while saving the counters cannot corrupt the settings.
  The firmware upload fills the log with zeros (no valid records).
*/
#ifndef DP_COUNTER_LOG_H
#define DP_COUNTER_LOG_H

class CounterLog
{
public:
  bool load(int &shotCounter, int &shotCounterSinceCleaning);
  bool save(int shotCounter, int shotCounterSinceCleaning);
};

extern CounterLog counterLog;

#endif // DP_COUNTER_LOG_H

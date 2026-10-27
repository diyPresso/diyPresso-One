/*
  Shot counter log, see dp_counter_log.h
*/

#include "dp_counter_log.h"
#include <FlashStorage.h>
#include "dp_settings.h"

#define LOG_ROW_SIZE 256 // SAMD21 flash row, the erase unit
#define LOG_ROWS 2

typedef struct {
  uint32_t sequence; // increments with every record, the newest valid record is used
  int32_t shotCounter;
  int32_t shotCounterSinceCleaning;
  uint32_t crc; // crc of the fields above
} record_t;

#define LOG_RECORDS_PER_ROW (LOG_ROW_SIZE / sizeof(record_t))
#define LOG_RECORDS (LOG_RECORDS_PER_ROW * LOG_ROWS)

__attribute__((__aligned__(LOG_ROW_SIZE)))
static const uint8_t log_data[LOG_ROW_SIZE * LOG_ROWS] = { };
static FlashClass log_flash(log_data, sizeof(log_data));
static const record_t erased_record = { 0xFFFFFFFF, -1, -1, 0xFFFFFFFF };

CounterLog counterLog;

static const record_t *slot(int index) { return (const record_t *)log_data + index; }

static uint32_t record_crc(const record_t *rec)
{
  return DpSettings::crc32((const unsigned char *)rec, sizeof(record_t) - 4);
}

/// @brief find the newest valid record
/// @return index of the record, -1 if there is no valid record
static int newest(record_t *newest_rec)
{
  int index = -1;
  for (int i = 0; i < (int)LOG_RECORDS; i++)
  {
    record_t rec;
    log_flash.read(slot(i), &rec, sizeof(rec)); // not read directly: the compiler would use the zeros of the const array
    if (memcmp(&rec, &erased_record, sizeof(rec)) == 0 || rec.crc != record_crc(&rec))
      continue;
    if (index < 0 || rec.sequence > newest_rec->sequence)
    {
      *newest_rec = rec;
      index = i;
    }
  }
  return index;
}

/// @brief get the counters of the newest valid record
/// @return true if a valid record was found, the counters are unchanged if not
bool CounterLog::load(int &shotCounter, int &shotCounterSinceCleaning)
{
  record_t rec;
  if (newest(&rec) < 0)
    return false;
  shotCounter = rec.shotCounter;
  shotCounterSinceCleaning = rec.shotCounterSinceCleaning;
  return true;
}

/// @brief append the counters to the log, only if they differ from the newest record
/// @return true if a record was written
bool CounterLog::save(int shotCounter, int shotCounterSinceCleaning)
{
  record_t last;
  int index = newest(&last);
  if (index >= 0 && last.shotCounter == shotCounter && last.shotCounterSinceCleaning == shotCounterSinceCleaning)
    return false;

  record_t rec;
  rec.sequence = index >= 0 ? last.sequence + 1 : 1;
  rec.shotCounter = shotCounter;
  rec.shotCounterSinceCleaning = shotCounterSinceCleaning;
  rec.crc = record_crc(&rec);

  // Next slot after the newest record. Move to the start of the other row when the row is full,
  // or when the slot is not erased (a cut off record, or the zeros after a firmware upload).
  // That row is erased first: it never holds the newest record.
  int next = (index + 1) % LOG_RECORDS;
  record_t next_rec;
  log_flash.read(slot(next), &next_rec, sizeof(next_rec));
  if (memcmp(&next_rec, &erased_record, sizeof(next_rec)) != 0 && next % LOG_RECORDS_PER_ROW != 0)
    next = (next / LOG_RECORDS_PER_ROW + 1) % LOG_ROWS * LOG_RECORDS_PER_ROW;
  if (next % LOG_RECORDS_PER_ROW == 0)
    log_flash.erase(slot(next), LOG_ROW_SIZE);

  log_flash.write(slot(next), &rec, sizeof(rec));
  return true;
}

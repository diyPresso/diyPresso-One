/* 
  reservoir.h
  measure weight and level of reservoir
*/
#ifndef RESERVOIR_H
#define RESERVOIR_H

#define RESERVOIR_ALMOST_EMPTY_WARNING_LEVEL 12.0 // empty level threshold [%], triggers a warning to refill upon brew start. Can be overwritten by press. - 12% = 180 grams
#define RESERVOIR_EMPTY_LEVEL 3.34 // empty level threshold [%] - 3.34% = ~50 grams
#define RESERVOIR_CAPACITY 1500.0 // capacity of reservoir in [grams]

typedef enum {
  RESERVOIR_ERROR_NONE, RESERVOIR_ERROR_SENSOR, RESERVOIR_ERROR_NO_READINGS,
  RESERVOIR_ERROR_OUT_OF_RANGE, RESERVOIR_ERROR_NEGATIVE
} reservoir_error_t;

class Reservoir
{
    private:
      double _level = 0.0;  // level [0..100%]
      double _tare = 0.0;   // tare weight (when empty) [gr]
      double _weight = 0.0; // current gross weight [gr]
      double _offset = 240000.0; // zero level offset [adc_units]
      double _scale = 427.4;     // scale [adc_units/gram]
      double _trim = 0.0;        // scale trim to match calibrated weight [%]
      int _readings = 0;         // number of readings without measurement
      reservoir_error_t _error = RESERVOIR_ERROR_NONE;
      void read();  // update the internal state, based on weight measurement
    public:
      Reservoir();
      double level() { return max(0, min(100.0 * ( weight() / RESERVOIR_CAPACITY), 100.0)); } // level [in %]
      double weight() { read(); return _weight - _tare; } // net weight
      double get_tare() { return _tare; }
      void set_tare(double t) { _tare = t; clear_error(); }
      void set_trim(double t) { _trim = t; }
      void tare() { read(); _tare = _weight - RESERVOIR_CAPACITY; clear_error(); } // note: tare when reservoir is full
      bool is_empty() { return level() < RESERVOIR_EMPTY_LEVEL; } // return true if under empty limit
      bool is_almost_empty() { return level() < RESERVOIR_ALMOST_EMPTY_WARNING_LEVEL; } // return true if under warning limit
      bool is_error() { return _error != RESERVOIR_ERROR_NONE; }
      reservoir_error_t error() { return _error; }
      const char *get_error_text();
      void clear_error() { _error = RESERVOIR_ERROR_NONE; }
};

extern Reservoir reservoir;

#endif // RESERVOIR_H
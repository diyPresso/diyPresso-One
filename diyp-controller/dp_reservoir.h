/* 
  reservoir.h
  measure weight and level of reservoir
*/
#ifndef RESERVOIR_H
#define RESERVOIR_H

#define RESERVOIR_EMPTY_LEVEL 10.0 // empty level threshold [%]
#define RESERVOIR_CAPACITY 1000.0 // capacity of reservoir in [grams]

typedef enum {
  RESERVOIR_ERROR_NONE 
} reservoir_error_t;

class Reservoir
{
    private:
      double _level = 0;  // level [0..100%]
      double _tare = 0;  // tare weight [gr]
      double _weight = 0; // weight [gr]
      double _offset = 240000; // zero level offset [adc_units]
      double _scale = 427.4;   // scale [adc_units/gram]
      double _trim = 0.0;      // scale trim to match calibrated weight [%]
      reservoir_error_t _error = RESERVOIR_ERROR_NONE;
      void read();  // update the internal state, based on weight measurement
    public:
      Reservoir();
      double level() { return max(0, min(100.0 * ( weight() / RESERVOIR_CAPACITY), 100.0)); } // level [in %]
      double weight() { read(); return _weight - _tare; }
      double get_tare() { return _tare; }
      void set_tare(double t) { _tare = t; }
      void set_trim(double t) { _trim = t; }
      void tare() { read(); _tare = _weight; }
      bool is_empty() { return level() < RESERVOIR_EMPTY_LEVEL; } // return true if under empty limit
      reservoir_error_t error() { return _error; }
};

extern Reservoir reservoir;

#endif // RESERVOIR_H
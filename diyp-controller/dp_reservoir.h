/* 
  reservoir.h
  measure weight and level of reservoir
*/
#ifndef RESERVOIR_H
#define RESERVOIR_H

#define RESERVOIR_EMPTY_LEVEL 5.0 // empty level threshold [%]
#define RESERVOIR_CAPACITY 500.0 // capacity of reservoir in [grams]

typedef enum {
  RESERVOIR_ERROR_NONE 
} reservoir_error_t;

class Reservoir
{
    private:
      double _level = 0;  // level [0..100%]
      double _tarre = 0;  // tarre weight [gr]
      double _weight = 0; // weight [gr]
      double _offset = 240000; // zero level offset [adc_units]
      double _scale = 427.4;   // scale [adc_units/gram]
      reservoir_error_t _error = RESERVOIR_ERROR_NONE;
      void read();  // update the internal state, based on weight measurement
    public:
      Reservoir();
      double level() { read(); return 100.0 * (_weight / RESERVOIR_CAPACITY); } // level [in %]
      double weight() { read(); return _weight - _tarre; }
      void tarre() { read(); _tarre = _weight; }
      void tarre(double t) { _tarre = t; }
      bool empty() { return level() < RESERVOIR_EMPTY_LEVEL; } // return true if under empty limit
      reservoir_error_t error() { return _error; }
};

extern Reservoir reservoir;

#endif // RESERVOIR_H
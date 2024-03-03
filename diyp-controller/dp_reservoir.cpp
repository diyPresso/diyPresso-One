/*
 diyEspresso reservoir control
 */
#include "dp_reservoir.h"
#include "dp_hardware.h"
#include <Arduino.h>
#include "HX711.h"


// globals
HX711 scale;

Reservoir reservoir;

Reservoir::Reservoir()
{
  scale.begin(PIN_HX711_DAT, PIN_HX711_CLK);
  read(); // at least one reading at start to init the state
}

/// @brief Read weight sensor and store the scaled weight value
void Reservoir::read()
{
  _weight = scale.read_average(3);
  _weight = (_weight - _offset) / ( (1.0+_trim/100.0) * _scale);
}


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
  read(); // at least one control loop to init the state
}

void Reservoir::read()
{
  _weight = scale.read_average(3);
  _weight = (_weight - _offset) / _scale;
  if ( _weight <10 ) // clamp weight at low end
    _weight = 1;
}


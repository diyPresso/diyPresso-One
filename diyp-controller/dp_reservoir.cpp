/*
 diyEspresso reservoir control
 */
#include "dp.h"
#include "dp_hardware.h"
#include "dp_reservoir.h"
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
  scale.wait_ready_timeout(1);
  if ( scale.is_ready() )
  {
    _weight = scale.read();
    _weight = (_weight - _offset) / ( (1.0+_trim/100.0) * _scale);
  }
}


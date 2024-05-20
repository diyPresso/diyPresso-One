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
  _readings += 1;
  if (_readings > 10)
    _error = RESERVOIR_ERROR_NO_READINGS;
  if ( scale.is_ready() )
  {
    _readings = 0;
    _weight = scale.read();
    Serial.println(_weight);
    _weight = (_weight - _offset) / ( (1.0+_trim/100.0) * _scale);
    double w = weight();
    if ( w > RESERVOIR_CAPACITY + 100.0 || w < -100 )
      _error = RESERVOIR_ERROR_OUT_OF_RANGE;
  }
}

const char *Reservoir::get_error_text()
{
    switch( _error )
    {
      case RESERVOIR_ERROR_NONE: return "OK"; break;
      case RESERVOIR_ERROR_OUT_OF_RANGE: return "OUT_OF_RANGE"; break;
      case RESERVOIR_ERROR_NO_READINGS: return "NO_READINGS"; break;
      case RESERVOIR_ERROR_SENSOR: return "SENSOR_ERROR"; break;
      default: return "UNKNOWN"; break;
    }
}

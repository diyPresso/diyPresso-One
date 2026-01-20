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
  double new_gross_weight;

  // scale.wait_ready_timeout(1); //removed to make non-blocking, don't think this is needed
  _readings += 1;
  
  if (_readings > 50)
    _error = RESERVOIR_ERROR_NO_READINGS;
  
  if ( scale.is_ready() )
  {
  
    // read the weight from the sensor and calculate the gross weight
    new_gross_weight = (scale.read() - _offset) / ( _scale);

    // Serial.print("nr of readings: ");
    // Serial.println(_readings);
     _readings = 0;

    // simple deglitcher: only accept new reading if within _glitch_limit grams of previous reading, or on 3 consecutive glitches, or on first reading (-1)
    if  (abs(new_gross_weight - _weight_gross) > _glitch_limit && _deglitched < 3 && _deglitched > -1 )
    {
      _deglitched += 1;  
      Serial.print("!!!! Deglitched reservoir reading. New: "); // TODO: disable debug
      Serial.print(new_gross_weight);
      Serial.print(" old: ");
      Serial.print(_weight_gross);
      Serial.print(" diff: ");
      Serial.print(abs(new_gross_weight - _weight_gross));
      Serial.print(" deglitched count: ");
      Serial.println(_deglitched);
    }
    else
    {
      _deglitched = 0;
      _weight_gross = new_gross_weight;
      // Serial.println("Not deglitched");
    }

    _weight_net = (_weight_gross / (1.0 + _trim / 100.0)) - _tare;
    if ( _weight_net > RESERVOIR_CAPACITY + 100.0 ) _error = RESERVOIR_ERROR_OUT_OF_RANGE;
    if ( _weight_net < -100.0 ) _error = RESERVOIR_ERROR_NEGATIVE;
  } 
  

}

const char *Reservoir::get_error_text()
{
    switch( _error )
    {
      case RESERVOIR_ERROR_NONE: return "OK"; break;
      case RESERVOIR_ERROR_OUT_OF_RANGE: return "OUT_OF_RANGE"; break;
      case RESERVOIR_ERROR_NEGATIVE: return "NEGATIVE_READING"; break;
      case RESERVOIR_ERROR_NO_READINGS: return "NO_READINGS"; break;
      case RESERVOIR_ERROR_SENSOR: return "SENSOR_ERROR"; break;
      default: return "UNKNOWN"; break;
    }
}

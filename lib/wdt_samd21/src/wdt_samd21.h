/*

   wdt_samd21: a very simple watch dog timer module for ATSAM D21

   (c) 2022 Guglielmo Braguglia

   Based on the work of MartinL:
   (c) 2018 MartinL (arduino forum: https://forum.arduino.cc/u/MartinL)

   For more details: https://forum.arduino.cc/t/wdt-watchdog-timer-code/353610/11
                   : https://github.com/arduino/ArduinoModule-CMSIS-Atmel/blob/master/CMSIS-Atmel/CMSIS/Device/ATMEL/samd21/include/component/wdt.h

   Valid "timeout" constant values are:
      WDT_CONFIG_PER_8        8 clock cycles
      WDT_CONFIG_PER_16      16 clock cycles
      WDT_CONFIG_PER_32      32 clock cycles
      WDT_CONFIG_PER_64      64 clock cycles
      WDT_CONFIG_PER_128    128 clock cycles
      WDT_CONFIG_PER_256    256 clock cycles
      WDT_CONFIG_PER_512    512 clock cycles
      WDT_CONFIG_PER_1K    1024 clock cycles
      WDT_CONFIG_PER_2K    2048 clock cycles
      WDT_CONFIG_PER_4K    4096 clock cycles
      WDT_CONFIG_PER_8K    8192 clock cycles
      WDT_CONFIG_PER_16K  16384 clock cycles

   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   This module is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This module is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this module.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef WDT_SAMD21
#define WDT_SAMD21

#include <Arduino.h>

#if !defined (__SAMD21G18A__)
#error "This library is only for SAMD21G18A, compilation aborted."
#endif

void wdt_init ( unsigned long wdt_config_per =  WDT_CONFIG_PER_2K );

void wdt_reset ( void );

void wdt_disable ( void );

void wdt_reEnable ( void );

#endif

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

#include <wdt_samd21.h>

// -------------------- wdt_init -----------------------

void wdt_init ( unsigned long wdt_config_per ) {
   //
   // Set up the generic clock (GCLK2) used to clock the watchdog timer at 1.024kHz
   REG_GCLK_GENDIV = GCLK_GENDIV_DIV ( 4 ) |         // Divide the 32.768kHz clock source by divisor 32, where 2^(4 + 1): 32.768kHz/32=1.024kHz
                     GCLK_GENDIV_ID ( 2 );           // Select Generic Clock (GCLK) 2
   while ( GCLK->STATUS.bit.SYNCBUSY );              // Wait for synchronization

   REG_GCLK_GENCTRL = GCLK_GENCTRL_DIVSEL |          // Set to divide by 2^(GCLK_GENDIV_DIV(4) + 1)
                      GCLK_GENCTRL_IDC |             // Set the duty cycle to 50/50 HIGH/LOW
                      GCLK_GENCTRL_GENEN |           // Enable GCLK2
                      GCLK_GENCTRL_SRC_OSCULP32K |   // Set the clock source to the ultra low power oscillator (OSCULP32K)
                      GCLK_GENCTRL_ID ( 2 );         // Select GCLK2
   while ( GCLK->STATUS.bit.SYNCBUSY );              // Wait for synchronization
   //
   // Feed GCLK2 to WDT (Watchdog Timer)
   REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |           // Enable GCLK2 to the WDT
                      GCLK_CLKCTRL_GEN_GCLK2 |       // Select GCLK2
                      GCLK_CLKCTRL_ID_WDT;           // Feed the GCLK2 to the WDT
   while ( GCLK->STATUS.bit.SYNCBUSY );              // Wait for synchronization
   //
   // Set and Enable the WDT
   REG_WDT_CONFIG = wdt_config_per;                  // Set the WDT reset timeout
   while ( WDT->STATUS.bit.SYNCBUSY );               // Wait for synchronization
   REG_WDT_CTRL = WDT_CTRL_ENABLE;                   // Enable the WDT in normal mode
   while ( WDT->STATUS.bit.SYNCBUSY );               // Wait for synchronization
}

// ------------------- wdt_reset -----------------------

void wdt_reset ( void ) {
   //
   // Must be called periodically to reset the WDT and to avoid the restart
   while ( WDT->STATUS.bit.SYNCBUSY );               // Check if the WDT registers are synchronized
   REG_WDT_CLEAR = WDT_CLEAR_CLEAR_KEY;              // Clear the watchdog timer
   while ( WDT->STATUS.bit.SYNCBUSY );               // Wait for synchronization
}

// ------------------ wdt_disable ----------------------

void wdt_disable ( void ) {
   //
   // Disable the WDT
   while ( WDT->STATUS.bit.SYNCBUSY );               // Check if the WDT registers are synchronized
   REG_WDT_CTRL = WDT_CTRL_RESETVALUE;               // Disable the WDT in normal mode
   while ( WDT->STATUS.bit.SYNCBUSY );               // Wait for synchronization
}

// ------------------ wdt_reEnable ----------------------

void wdt_reEnable ( void ) {
   //
   // Enable the WDT
   while ( WDT->STATUS.bit.SYNCBUSY );               // Check if the WDT registers are synchronized
   REG_WDT_CTRL = WDT_CTRL_ENABLE;                   // Enable the WDT in normal mode
   while ( WDT->STATUS.bit.SYNCBUSY );               // Wait for synchronization
}

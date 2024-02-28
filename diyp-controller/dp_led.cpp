/*
  RGB Color LED
  For MKR1010: use Wifi RGB led, for other boards we may use a NeoPixel 
  or RGB led directly connected to GPIO Pins
  NOTE: On mkr1010 board Wifi RGB led cannot be used before init() or loop() (e.g. in constructor of classes): this hangs the board.
 */
#include "dp_led.h"
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>

ColorLed statusLed = ColorLed();


ColorLed::ColorLed(void)
{
  return;
}


void ColorLed::color(const bool c[])
{
  WiFiDrv::pinMode(25, OUTPUT); // R
  WiFiDrv::pinMode(26, OUTPUT); // G
  WiFiDrv::pinMode(27, OUTPUT); // B
  WiFiDrv::digitalWrite(25, (c[0] ? HIGH : LOW) );
  WiFiDrv::digitalWrite(26, (c[1] ? HIGH : LOW) );
  WiFiDrv::digitalWrite(27, (c[2] ? HIGH : LOW) );
}

/*
 diyEspresso Wifi interface
 */
#include "dp.h"
#include <Arduino.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include "EasyWiFi.h"

/*********** Global Settings  **********/
EasyWiFi MyEasyWiFi;
char MyAPName[]= {"diyPresso-One"};

void wifi_setup()
{
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
  }
  WiFi.setHostname("diyPresso-One");
  MyEasyWiFi.apname(MyAPName);
  MyEasyWiFi.seed(5);
}

void printWiFiStatus()
{
    Serial.print("\nStatus: SSID: "); Serial.print(WiFi.SSID());
    IPAddress ip = WiFi.localIP(); Serial.print(" - IPAddress: "); Serial.print(ip);
    long rssi = WiFi.RSSI(); Serial.print("- Rssi: "); Serial.print(rssi); Serial.println("dBm");
}

void wifi_loop()
{
  if (WiFi.status()==WL_CONNECTED)
  {
    printWiFiStatus();
  }
  else
  {
    Serial.println("* Not Connected, starting EasyWiFi");
    MyEasyWiFi.start();
  }
} // end Main loop

void wifi_erase()
{
   MyEasyWiFi.erase();
}



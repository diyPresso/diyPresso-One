/*
 diyEspresso Wifi interface
 */
#include "dp.h"
#include <Arduino.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include "EasyWiFi.h"
#include "dp_encoder.h"

/*********** Global Settings  **********/
EasyWiFi MyEasyWiFi;
char MyAPName[]= {"diyPresso-One"};
static int cancel_button_count = 0; // encoder button count when wifi_loop() started

// Called by EasyWiFi while connecting or waiting for AP input: any button press since wifi_loop() started cancels
bool wifi_cancel_requested()
{
  return encoder.button_count() != cancel_button_count;
}

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

bool wifi_loop(bool config_ap)
{
  cancel_button_count = encoder.button_count();
  if (WiFi.status()==WL_CONNECTED && !config_ap)
  {
    printWiFiStatus();
    return true;
  }
  Serial.println(config_ap ? "* Config AP requested, starting EasyWiFi" : "* Not Connected, starting EasyWiFi");
  return MyEasyWiFi.start(config_ap);
} // end Main loop



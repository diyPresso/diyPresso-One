 /*
 * EasyWiFi
 * Created by John V. - 2020 V 1.4.1
 *
 * Released into the public domain on github: https://github.com/javos65/EasyWifi-for-MKR1010
 *
 * CUSTOMIZED FOR DIYPRESSO - this is not the upstream library (unmaintained since 2020), do not replace it.
 * Changes compared to upstream:
 *  - Progress is shown on the machine display via menu_wifi() (implemented in dp_menu.cpp)
 *  - The watchdog is reset while waiting
 *  - Credentials are stored unencrypted as a fixed size struct
 *  - start() returns whether a connection was made, and can go straight to the AP (forceAP)
 *  - Waiting for AP input stops after APTIMEOUT_MS, or when wifi_cancel_requested() (dp_wifi.cpp) returns true
 *  - Credentials entered on the AP page are only saved after they connected successfully
 */
#ifndef EASYWIFI_H
#define EASYWIFI_H

#include "Arduino.h"
#include <WiFiNINA.h>
#include <WiFiUdp.h>

// Define AP Wifi-Client parameters
#define MAXSSID 10                         // MAX number of SSID's listed after search
#define SSIDBUFFERSIZE 32                  // SSID name BUFFER size
#define APCHANNEL  5                       // AP wifi channel
#define SECRET_SSID "UnKnownWireless"		   // Backup SSID - not required
#define SECRET_PASS "NoPassword"	         // Backup Pass - not required
#define CREDENTIALFILE "/fs/credfile"
#define APNAME "EasyWiFi_AP"
#define MAXCONNECT 4                       // Max number of wifi logon connects before opening AP
#define ESCAPECONNECT 15                   // Max number of Total wifi logon retries-connects before escaping/stopping the Wifi start
#define APTIMEOUT_MS (5UL * 60UL * 1000UL) // Close the AP and stop the Wifi start when no credentials are entered within this time

// Define UDP settings for DNS
#define UDP_PACKET_SIZE 1024          // UDP packet size time out, preventign too large packet reads
#define DNSHEADER_SIZE 12             // DNS Header
#define DNSANSWER_SIZE 16             // DNS Answer = standard set with Packet Compression
#define DNSMAXREQUESTS 32             // trigger first DNS requests, to redirect to own web-page
#define UDPPORT  53                   // local port to listen for UDP packets

// Define RGB values for NINALed
#define RED 16,0,0
#define ORANGE 5,3,0
#define GREEN 0,8,0
#define BLUE 0,0,20
#define PURPLE 6,0,10
#define CYAN 0,6,10
#define BLACK 0,0,0

class EasyWiFi
{
  public:
    EasyWiFi();
    bool start(bool forceAP = false);
    byte erase();
    byte apname(char * name);
    void seed(int value);
    void led(boolean value);
    void useAP(boolean value);
    void NINAled(char r, char g, char b);

  private:
    void SimpleDecypher(char * textin, char * textout);
    void SimpleCypher(char * textin, char * textout);
    byte Check_Credentials();
    byte Erase_Credentials();
    byte Write_Credentials(char * buf1,int size1,char * buf2,int size2);
    byte Read_Credentials(char * buf1,char * buf2);
    void APWiFiClientCheck();
    void APDNSScan();
    void listNetworks();
    void APSetup();
    void APStop();
    bool isConnected(int status);
    void printWiFiStatus();

};

#endif

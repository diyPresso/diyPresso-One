#ifndef WIFI_H
#define WIFI_H
#include "dp.h"

/*
 * Wifi setup, done once at the end of setup() (diyp-controller.ino), based on the "WIFI mode" setting:
 *
 *  OFF        Wifi is not started.
 *  ON         Connect with the credentials stored in flash (by EasyWiFi). When that fails, the configuration
 *             access point (AP) "diyPresso-One" is opened, see below.
 *  CONFIG-AP  Selected in the settings menu to enter new credentials. On the next boot the mode is first saved
 *             as OFF, then the AP is opened directly. Only when a connection is made the mode is saved as ON.
 *             A setup that is cancelled, times out or is interrupted by a power cycle leaves Wifi OFF.
 *
 * Configuration AP: the user connects a phone to the "diyPresso-One" network and enters SSID and password
 * on the web page. These are tried and only stored in flash when they connect, so the previous credentials
 * are kept until then. Wrong credentials re-open the AP (EasyWiFi gives up after ESCAPECONNECT attempts).
 *
 * The machine does not run (no display menus, no brewing) while this is busy. So both connecting and the AP
 * can always be left: pressing the button cancels (see wifi_cancel_requested()), and the AP closes after
 * APTIMEOUT_MS. The machine then continues to boot without Wifi.
 *
 * EasyWiFi.cpp/.h is a customized copy of the EasyWiFi library, see EasyWiFi.h.
 */

void wifi_setup();
bool wifi_loop(bool config_ap = false); // returns true when connected
bool wifi_cancel_requested();

#endif // WIFI_H

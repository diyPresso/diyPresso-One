/*
  List of all persistent settings, one line per setting:

    X(type, name, key, default, min, max, decimals)

  type      int or double
  name      accessor name: settings.name() and settings.name(value), and the setting id SETTING_name
  key       key in the serial interface (GET/PUT settings). Do not change: the update client
            writes the settings back by key after a firmware upgrade.
  default   default value, set by defaults()
  min, max  the setter clamps the value to this range
  decimals  decimals shown in the menu, edits are rounded to this. The serial output uses
            at least 2 decimals.

  The order is free: the emulated EEPROM is in flash and is erased by every firmware upload,
  after which the defaults are loaded and the update client writes the user's settings back.

  Adding a setting: add a line here, use it in DpSettings::apply() and,
  if needed, add it to the menu in dp_settings_menu.cpp.
*/
#ifndef DP_SETTINGS_LIST_H
#define DP_SETTINGS_LIST_H
//  type,   name,                        key,                           default, min,    max, decimals
#define DP_SETTINGS_LIST(X) \
  X(double, temperature,                 "temperature",                 98.0,   0.0,     104.0,    1) \
  X(double, preInfusionTime,             "preInfusionTime",             3.0,    0.0,     60.0,     1) \
  X(double, infusionTime,                "infusionTime",                1.0,    0.0,     60.0,     1) \
  X(double, extractionTime,              "extractionTime",              25.0,   0.0,     60.0,     1) \
  X(double, extractionWeight,            "extractionWeight",            0.0,    1.0,     500.0,    1) \
  X(double, P,                           "p",                           6.0,    0.0,     10.0,     1) \
  X(double, I,                           "i",                           0.05,   0.0,     20.0,     2) \
  X(double, D,                           "d",                           150.0,  0.0,     999.0,    0) \
  X(double, dffFactorPct,                "dffFactorPct",                70.0,   0.0,     150.0,    1) \
  X(double, ffReady,                     "ffReady",                     5.5,    0.0,     100.0,    1) \
  X(double, tareWeight,                  "tareWeight",                  0.0,    -2000.0, 2000.0,   2) \
  X(double, trimWeight,                  "trimWeight",                  0.0,    -90.0,   50.0,     2) \
  X(int,    commissioningDone,           "commissioningDone",           0,      0,       1,        0) \
  X(int,    shotCounter,                 "shotCounter",                 0,      0,       INT32_MAX, 0) \
  X(int,    wifiMode,                    "wifiMode",                    WIFI_MODE_OFF, 0, 2,       0) \
  X(double, steamTemperature,            "steamTemperature",            165.0,  0.0,     170.0,    1) \
  X(double, steamP,                      "steamP",                      3.5,    0.0,     50.0,     1) \
  X(double, steamI,                      "steamI",                      0.05,   0.0,     10.0,     2) \
  X(double, steamD,                      "steamD",                      50.0,   0.0,     999.0,    0) \
  X(double, steamPumpPower,              "steamPumpPower",              100.0,  10.0,    100.0,    0) \
  X(double, windupMin,                   "windupMin",                   -15.0,  -100.0,  0.0,      0) \
  X(double, windupMax,                   "windupMax",                   8.0,    0.0,     100.0,    0) \
  X(double, integrationBand,             "integrationBand",             4.0,    0.0,     20.0,     1) \
  X(int,    conditionalIntegration,      "conditionalIntegration",      1,      0,       1,        0) \
  X(double, steamWindupMin,              "steamWindupMin",              -5.0,   -100.0,  0.0,      0) \
  X(double, steamWindupMax,              "steamWindupMax",              100.0,  0.0,     100.0,    0) \
  X(double, steamIntegrationBand,        "steamIntegrationBand",        5.0,    0.0,     30.0,     1) \
  X(int,    steamConditionalIntegration, "steamConditionalIntegration", 1,      0,       1,        0) \
  X(int,    brewIntegralHold,            "brewIntegralHold",            1,      0,       1,        0) \
  X(int,    brewHoldTimeoutSec,          "brewHoldTimeoutSec",          180,    10,      600,      0) \
  X(int,    brewHoldDipWindowSec,        "brewHoldDipWindowSec",        60,     10,      180,      0) \
  X(double, dffDecay,                    "dffDecay",                    0.97,   0.90,    1.0,      3) \
  X(double, steamPumpP,                  "steamPumpP",                  5.0,    0.0,     50.0,     1) \
  X(double, steamPumpI,                  "steamPumpI",                  0.1,    0.0,     10.0,     2) \
  X(double, steamPumpD,                  "steamPumpD",                  50.0,   0.0,     999.0,    0) \
  X(double, steamPumpIntegrationBand,    "steamPumpIntegrationBand",    0.0,    0.0,     30.0,     1) \
  X(double, iDownFactor,                 "iDownFactor",                 1.0,    1.0,     5.0,      1) \
  X(double, steamIDownFactor,            "steamIDownFactor",            3.0,    1.0,     5.0,      1) \
  X(int,    encoderReverse,              "encoderReverse",              0,      0,       1,        0)

/*
  Old key names, still accepted by PUT settings. After a firmware upgrade the update client
  writes back the keys of the firmware it upgraded from, so these must stay.

    A(alias, name)    alias: old key, name: the setting it sets
*/
#define DP_SETTINGS_ALIASES(A) \
  A("P",           P) \
  A("I",           I) \
  A("D",           D) \
  A("ff_ready",    ffReady) \
  A("infuseTime",  infusionTime) \
  A("extractTime", extractionTime)

/*
  Keys of removed settings: accepted by PUT settings and ignored, for the same reason.

    R(key)
*/
#define DP_SETTINGS_REMOVED(R) \
  R("ffBrew") \
  R("ff_brew") \
  R("ffHeat") \
  R("ff_heat")

#endif // DP_SETTINGS_LIST_H

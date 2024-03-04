/*
  dp_settings class
  Loads and Saves the persistent settings.
  if no changes are made to the settings, nothing is saved
  If no valid data is present, default values are saved
  the setters check the range of the values to save, to prevent incorrect data
*/

#ifndef DiyeSettings_h
#define DiyeSettings_h

#include "Arduino.h"

typedef enum wifi_modes { WIFI_MODE_OFF, WIFI_MODE_ON, WIFI_MODE_AP };

class DiyeSettings
{
    private:
        typedef struct __attribute__ ((packed)) settings_struct { // a packed struct has no alignment of fields
            unsigned long int crc; // crc of all the the fields after the crc
            unsigned long int version; // settings struct version
            double temperature;
            double preInfusionTime;
            double infusionTime;
            double extractionTime;
            double extractionWeight;
            double p, i, d, ff;
            double tareWeight;
            double trimWeight;
            int shotCounter;
            int wifiMode;
        } settings_t;
        settings_t settings;
        void read(settings_t *s);
        void update_crc(void);
        bool crc_is_valid(settings_t *s);
        unsigned long crc32(const unsigned char *s, size_t n);
    public:
        DiyeSettings();
        void defaults();
        int load();
        int save();
        double temperature() { return settings.temperature; }
        double temperature(double t) { return settings.temperature = min(110.0, max(t, 0.0)); }
        double preInfusionTime() { return settings.preInfusionTime; }
        double preInfusionTime(double t) { return settings.preInfusionTime = min(60.0, max(t, 0.0)); }
        double infusionTime() { return settings.infusionTime; }
        double infusionTime(double t) { return settings.infusionTime = min(60.0, max(t, 0.0)); }
        double extractionTime() { return settings.extractionTime; }
        double extractionTime(double t) { return settings.extractionTime = min(60.0, max(t, 0.0)); }
        double extractionWeight() { return settings.extractionWeight; }
        double extractionWeight(double w) { return settings.extractionWeight = min(500.0, max(w, 1.0)); }
        double P() { return settings.p; }
        double P(double p) { return settings.p = min(10.0, max(p, 0.0)); }
        double I() { return settings.i; }
        double I(double i) { return settings.i = min(10.0, max(i, 0.0)); }
        double D() { return settings.d; }
        double D(double d) { return settings.d = min(10.0, max(d, 0.0)); }
        double FF() { return settings.ff; }
        double FF(double ff) { return settings.ff = min(100.0, max(ff, 0.0)); }
        double tareWeight() { return settings.tareWeight; }
        double tareWeight(double t) { return settings.tareWeight = min(1000.0, max(t, 0.0)); }
        double trimWeight() { return settings.trimWeight; }
        double trimWeight(double t) { return settings.trimWeight = min(10.0, max(t, -10.0)); }
        int wifiMode() { return settings.wifiMode; }
        int wifiMode(int state) { return settings.wifiMode = min(2, max(state, 0)); }
        int shotCounter() { return settings.shotCounter; }
        int incShotCounter() { return settings.shotCounter += 1; }
        void zeroShotCounter() { settings.shotCounter = 0; }
};

extern DiyeSettings settings;


#endif // DiyeSettings_h
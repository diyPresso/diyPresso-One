/*
  dp_settings class
  Loads and Saves the persistent settings.
  if no changes are made to the settings, nothing is saved
  If no valid data is present, default values are saved
  the setters check the range of the values to save, to prevent incorrect data
*/

#ifndef DpSettings_h
#define DpSettings_h

#include "Arduino.h"
#include "dp_serial.h"

typedef enum wifi_modes { WIFI_MODE_OFF, WIFI_MODE_ON, WIFI_MODE_AP };

class DpSettings
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
            double p, i, d, dffFactorPct, ffHeat, ffReady;
            double tareWeight;
            double trimWeight;
            int commissioningDone;
            int shotCounter;
            int wifiMode;
            double steamTemperature;
            double steamP, steamI, steamD;
            double steamPumpPower;
            double windupMin, windupMax, integrationBand;
            int conditionalIntegration;
            double steamWindupMin, steamWindupMax, steamIntegrationBand;
            int steamConditionalIntegration;
            int brewIntegralHold;
            int brewHoldTimeoutSec;
            int brewHoldDipWindowSec;
            double dffDecay;
            double steamPumpP, steamPumpI, steamPumpD;
            double steamPumpIntegrationBand;
            double iDownFactor, steamIDownFactor;
            int encoderReverse;
        } settings_t;
        settings_t settings;
        void read(settings_t *s);
        void update_crc(void);
        bool crc_is_valid(settings_t *s);
        unsigned long crc32(const unsigned char *s, size_t n);
    public:
        DpSettings();
        void defaults();
        int load();
        int save();
        void apply();
        String serialize();
        int deserialize(String input);
        double temperature() { return settings.temperature; }
        double temperature(double t) { return settings.temperature = min(104.0, max(t, 0.0)); }
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
        double I(double i) { return settings.i = min(20.0, max(i, 0.00)); }
        double D() { return settings.d; }
        double D(double d) { return settings.d = min(999.0, max(d, 0.0)); }
        double ffHeat() { return settings.ffHeat; }
        double ffHeat(double ff) { return settings.ffHeat = min(100.0, max(ff, 0.0)); }
        double ffReady() { return settings.ffReady; }
        double ffReady(double ff) { return settings.ffReady = min(100.0, max(ff, 0.0)); }
        double dffFactorPct() { return settings.dffFactorPct; }
        double dffFactorPct(double dff) { return settings.dffFactorPct = min(150.0, max(dff, 0.0)); }

        double tareWeight() { return settings.tareWeight; }
        double tareWeight(double t) { return settings.tareWeight = min(2000.0, max(t, -2000.0)); }
        double trimWeight() { return settings.trimWeight; }
        double trimWeight(double t) { return settings.trimWeight = min(50.0, max(t, -90.0)); }
        int wifiMode() { return settings.wifiMode; }
        int wifiMode(int state) { return settings.wifiMode = min(2, max(state, 0)); }
        int shotCounter() { return settings.shotCounter; }
        int shotCounter(int count) { return settings.shotCounter = min(INT32_MAX, max(count, 0)); }
        int commissioningDone() { return settings.commissioningDone; }
        int commissioningDone(int state) { return settings.commissioningDone = min(1, max(state, 0)); }
        int incShotCounter() { return settings.shotCounter += 1; }
        void zeroShotCounter() { settings.shotCounter = 0; }

        double steamTemperature() { return settings.steamTemperature; }
        double steamTemperature(double t) { return settings.steamTemperature = min(170.0, max(t, 0.0)); }
        double steamP() { return settings.steamP; }
        double steamP(double p) { return settings.steamP = min(50.0, max(p, 0.0)); }
        double steamI() { return settings.steamI; }
        double steamI(double i) { return settings.steamI = min(10.0, max(i, 0.0)); }
        double steamD() { return settings.steamD; }
        double steamD(double d) { return settings.steamD = min(999.0, max(d, 0.0)); }
        double steamPumpP() { return settings.steamPumpP; }
        double steamPumpP(double p) { return settings.steamPumpP = min(50.0, max(p, 0.0)); }
        double steamPumpI() { return settings.steamPumpI; }
        double steamPumpI(double i) { return settings.steamPumpI = min(10.0, max(i, 0.0)); }
        double steamPumpD() { return settings.steamPumpD; }
        double steamPumpD(double d) { return settings.steamPumpD = min(999.0, max(d, 0.0)); }
        double steamPumpPower() { return settings.steamPumpPower; }
        double steamPumpPower(double p) { return settings.steamPumpPower = min(100.0, max(p, 10.0)); }

        // PID integral term limits and conditional integration (serial interface only, not in the menu)
        double windupMin() { return settings.windupMin; }
        double windupMin(double w) { return settings.windupMin = min(0.0, max(w, -100.0)); }
        double windupMax() { return settings.windupMax; }
        double windupMax(double w) { return settings.windupMax = min(100.0, max(w, 0.0)); }
        double integrationBand() { return settings.integrationBand; }
        double integrationBand(double b) { return settings.integrationBand = min(20.0, max(b, 0.0)); }
        int conditionalIntegration() { return settings.conditionalIntegration; }
        int conditionalIntegration(int state) { return settings.conditionalIntegration = min(1, max(state, 0)); }
        double steamWindupMin() { return settings.steamWindupMin; }
        double steamWindupMin(double w) { return settings.steamWindupMin = min(0.0, max(w, -100.0)); }
        double steamWindupMax() { return settings.steamWindupMax; }
        double steamWindupMax(double w) { return settings.steamWindupMax = min(100.0, max(w, 0.0)); }
        double steamIntegrationBand() { return settings.steamIntegrationBand; }
        double steamIntegrationBand(double b) { return settings.steamIntegrationBand = min(30.0, max(b, 0.0)); }
        double steamPumpIntegrationBand() { return settings.steamPumpIntegrationBand; }
        double steamPumpIntegrationBand(double b) { return settings.steamPumpIntegrationBand = min(30.0, max(b, 0.0)); }
        double iDownFactor() { return settings.iDownFactor; }
        double iDownFactor(double f) { return settings.iDownFactor = min(5.0, max(f, 1.0)); }
        double steamIDownFactor() { return settings.steamIDownFactor; }
        double steamIDownFactor(double f) { return settings.steamIDownFactor = min(5.0, max(f, 1.0)); }
        int steamConditionalIntegration() { return settings.steamConditionalIntegration; }
        int steamConditionalIntegration(int state) { return settings.steamConditionalIntegration = min(1, max(state, 0)); }
        int brewIntegralHold() { return settings.brewIntegralHold; }
        int brewIntegralHold(int state) { return settings.brewIntegralHold = min(1, max(state, 0)); }
        int brewHoldTimeoutSec() { return settings.brewHoldTimeoutSec; }
        int brewHoldTimeoutSec(int t) { return settings.brewHoldTimeoutSec = min(600, max(t, 10)); }
        int brewHoldDipWindowSec() { return settings.brewHoldDipWindowSec; }
        int brewHoldDipWindowSec(int t) { return settings.brewHoldDipWindowSec = min(180, max(t, 10)); }
        double dffDecay() { return settings.dffDecay; }
        double dffDecay(double d) { return settings.dffDecay = min(1.0, max(d, 0.90)); }
        int encoderReverse() { return settings.encoderReverse; }
        int encoderReverse(int state) { return settings.encoderReverse = min(1, max(state, 0)); }
};

extern DpSettings settings;


#endif // DpSettings_h
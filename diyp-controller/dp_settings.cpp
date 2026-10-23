/*
  loading and saving of a settings struct.
  We check if the stored settings struct is valid, and the version corresponds to the expected version.
  If there is a mismatch in version or the CRC is invalid we load default values.
  (c) 2024 - diyEspresso - PBRI - CC-BY-NC
*/

#include "dp_settings.h"
#include <FlashAsEEPROM.h>
#include "dp_boiler.h"
#include "dp_steam_thermoblock.h"
#include "dp_steam_pump.h"
#include "dp_reservoir.h"
#include "dp_brew.h"
#include "dp_encoder.h"


DpSettings settings = DpSettings();

DpSettings::DpSettings()
{
    defaults();
}


/// @brief  calculate crc32 of a buffer
/// @param s pointer to start of buffer 
/// @param n length of buffer in bytes
/// @return  32bit crc
unsigned long DpSettings::crc32(const unsigned char *s, size_t n)
{
    uint32_t crc = 0xFFFFFFFF;

	for(size_t i=0;i<n;i++)
    {
	    unsigned char ch = s[i];
		for(size_t j=0;j<8;j++)
        {
			uint32_t b = (ch^crc) & 1;
		    crc >>= 1;
		    if(b) crc = crc^0xEDB88320;
		    ch >>= 1;
		}
	}
    return ~crc;
}


/// @brief  Calculate the new CRC value of the settings struct
void DpSettings::update_crc(void)
{
    unsigned char *s = (unsigned char*) &settings;
    settings.crc = crc32( s + 4, sizeof(settings_t) - 4);
}


/// @brief return true if CRC of settings struct is valid
/// @param s pointer to settings struct
/// @return true if CRC is valid
bool DpSettings::crc_is_valid(settings_t *s)
{
    return crc32( ((unsigned char*)s) + 4, sizeof(settings_t) - 4 ) == s->crc;
}


/// @brief set all values to default in settings stuct
void DpSettings::defaults()
{
    settings.version = 4;  // bumped: added PID windup and conditional integration settings
    settings.temperature = 98.0;
    settings.preInfusionTime = 3;
    settings.infusionTime = 1;
    settings.extractionTime = 25;
    settings.p = 6.0;
    settings.i = 0.05;
    settings.d = 150.0;
    settings.ffHeat = 5.5;
    settings.ffReady = 5.5;
    settings.dffFactorPct = 70.0;  // 70%
    settings.tareWeight = 0.0;
    settings.trimWeight = 0.0;
    settings.wifiMode = 0; // off=0
    settings.shotCounter = 0;
    settings.commissioningDone = 0; // default is 0 (not done)
    settings.steamTemperature = 165.0;
    settings.steamP = 3.5;
    settings.steamI = 0.05;
    settings.steamD = 50.0;
    settings.steamPumpP = 5.0;
    settings.steamPumpI = 0.1;
    settings.steamPumpD = 50.0;
    settings.steamPumpPower = 100.0;
    settings.windupMin = WINDUP_LIMIT_MIN;
    settings.windupMax = WINDUP_LIMIT_MAX;
    settings.integrationBand = INTEGRATION_BAND;
    settings.conditionalIntegration = 1;
    settings.steamWindupMin = STEAM_WINDUP_LIMIT_MIN;
    settings.steamWindupMax = STEAM_WINDUP_LIMIT_MAX;
    settings.steamIntegrationBand = STEAM_INTEGRATION_BAND;
    settings.steamPumpIntegrationBand = STEAM_PUMP_INTEGRATION_BAND;
    settings.iDownFactor = 1.0;
    settings.steamIDownFactor = 3.0;
    settings.steamConditionalIntegration = 1;
    settings.brewIntegralHold = 1;
    settings.brewHoldTimeoutSec = BREW_HOLD_TIMEOUT_SEC;
    settings.brewHoldDipWindowSec = BREW_HOLD_DIP_WINDOW_SEC;
    settings.dffDecay = DFF_ENERGY_STOCK_PILE_DECAY;
    settings.encoderReverse = 0;
    update_crc();
}


/// @brief read settings struct from EEPROM to memory
/// @param s  pointer to settings struct in memory
void DpSettings::read(settings_t *s)
{
    unsigned char *p = (unsigned char*)s;
    for (int i=0; i<sizeof(settings_t); i++)
    {
        *p++ = EEPROM.read(i);
        //Serial.print(i); Serial.print("="); Serial.println(EEPROM.read(i));
    }
}


/*
 * load()
 * return value:
 *  0 = OK, settings loaded
 * -1 = No valid EEPROM values
 * -2 = CRC incorrect
 * -3 = Settings struct version incorrect
 */
int DpSettings::load()
{
    settings_t set;
    if ( !EEPROM.isValid() )
    {
        defaults();
        return -1;
    }
    read( &set);
    if ( !crc_is_valid(&set) )
    {
        defaults();
        return -2;
    }
    if ( set.version != settings.version )
    {
        defaults();
        return -3;
    }
    read( &settings );
    return 0;
}


/*
 * save()
 * return value:
 * 0 = No change
 * 1 = Changed values saved
 */
int DpSettings::save()
{
    settings_t old_settings;
    update_crc();
    read( &old_settings );

    unsigned char *s = (unsigned char*) &settings;
    unsigned char *d = (unsigned char*) &old_settings;
    bool changed = false;
    for (int i=0; i<sizeof(settings_t); i++, s+=1, d+=1 )
    {
        if ( *s != *d )
        {
            EEPROM.write(i, *s);
            changed = true;
        }
    }
    if ( changed )
    {
        EEPROM.commit();
    }
    return changed ? 1 : 0;
}


void DpSettings::apply()
{
  boilerController.set_temp(temperature());

  boilerController.set_pid(P(), I(), D());
  boilerController.set_ffHeat(ffHeat());
  boilerController.set_ffReady(ffReady());
  boilerController.set_dffFactorPct(dffFactorPct());
  boilerController.set_windup_limits(windupMin(), windupMax());
  boilerController.set_conditional_integration(conditionalIntegration(), integrationBand());
  boilerController.set_brew_integral_hold(brewIntegralHold(), brewHoldTimeoutSec(), brewHoldDipWindowSec());
  boilerController.set_dffDecay(dffDecay());
  boilerController.set_iDownFactor(iDownFactor());

  reservoir.set_trim(trimWeight());
  reservoir.set_tare(tareWeight());
  brewProcess.preInfuseTime = preInfusionTime();
  brewProcess.infuseTime = infusionTime();
  brewProcess.extractTime = extractionTime();

  steamThermoblock.set_temp(steamTemperature());
  steamThermoblock.set_pid(steamP(), steamI(), steamD());
  steamThermoblock.set_pump_pid(steamPumpP(), steamPumpI(), steamPumpD());
  steamThermoblock.set_iDownFactor(steamIDownFactor());
  steamThermoblock.set_windup_limits(steamWindupMin(), steamWindupMax());
  steamThermoblock.set_conditional_integration(steamConditionalIntegration(), steamIntegrationBand(), steamPumpIntegrationBand());
  steamPump.power(steamPumpPower());

  encoder.set_reverse(encoderReverse());
}

String DpSettings::serialize() {
    String result = "";
    result += "version=" + String(settings.version) + "\n";
    result += "crc=" + String(settings.crc) + "\n";
    result += "temperature=" + String(settings.temperature) + "\n";
    result += "preInfusionTime=" + String(settings.preInfusionTime) + "\n";
    result += "infusionTime=" + String(settings.infusionTime) + "\n";
    result += "extractionTime=" + String(settings.extractionTime) + "\n";
    result += "extractionWeight=" + String(settings.extractionWeight) + "\n";
    result += "p=" + String(settings.p) + "\n";
    result += "i=" + String(settings.i) + "\n";
    result += "d=" + String(settings.d) + "\n";
    result += "dffFactorPct=" + String(settings.dffFactorPct) + "\n";
    result += "ffHeat=" + String(settings.ffHeat) + "\n";
    result += "ffReady=" + String(settings.ffReady) + "\n";
    result += "tareWeight=" + String(settings.tareWeight) + "\n";
    result += "trimWeight=" + String(settings.trimWeight) + "\n";
    result += "commissioningDone=" + String(settings.commissioningDone) + "\n";
    result += "shotCounter=" + String(settings.shotCounter) + "\n";
    result += "wifiMode=" + String(settings.wifiMode) + "\n";
    result += "steamTemperature=" + String(settings.steamTemperature) + "\n";
    result += "steamP=" + String(settings.steamP) + "\n";
    result += "steamI=" + String(settings.steamI) + "\n";
    result += "steamD=" + String(settings.steamD) + "\n";
    result += "steamPumpPower=" + String(settings.steamPumpPower) + "\n";
    result += "steamPumpP=" + String(settings.steamPumpP) + "\n";
    result += "steamPumpI=" + String(settings.steamPumpI) + "\n";
    result += "steamPumpD=" + String(settings.steamPumpD) + "\n";
    result += "windupMin=" + String(settings.windupMin) + "\n";
    result += "windupMax=" + String(settings.windupMax) + "\n";
    result += "integrationBand=" + String(settings.integrationBand) + "\n";
    result += "conditionalIntegration=" + String(settings.conditionalIntegration) + "\n";
    result += "steamWindupMin=" + String(settings.steamWindupMin) + "\n";
    result += "steamWindupMax=" + String(settings.steamWindupMax) + "\n";
    result += "steamIntegrationBand=" + String(settings.steamIntegrationBand) + "\n";
    result += "steamPumpIntegrationBand=" + String(settings.steamPumpIntegrationBand) + "\n";
    result += "iDownFactor=" + String(settings.iDownFactor) + "\n";
    result += "steamIDownFactor=" + String(settings.steamIDownFactor) + "\n";
    result += "steamConditionalIntegration=" + String(settings.steamConditionalIntegration) + "\n";
    result += "brewIntegralHold=" + String(settings.brewIntegralHold) + "\n";
    result += "brewHoldTimeoutSec=" + String(settings.brewHoldTimeoutSec) + "\n";
    result += "brewHoldDipWindowSec=" + String(settings.brewHoldDipWindowSec) + "\n";
    result += "dffDecay=" + String(settings.dffDecay, 3) + "\n";
    result += "encoderReverse=" + String(settings.encoderReverse) + "\n";
    return result;
}


/* receives a string, parses it and updates the settings. For example:
temperature=98.50,P=7.00,I=0.30,D=80.00,dff_factor=90.00,ff_ready=10.00,ff_brew=80.00,tareWeight=0.00,trimWeight=0.00,preInfusionTime=3.00,infuseTime=1.00,extractTime=25.00,extractionWeight=0.00,commissioningDone=1,shotCounter=5,wifiMode=0

can also be a subset of these values.

return value:
  0 = OK
 -1 = Invalid input string
 -2 = Unknown key

 Note: does not save the settings to EEPROM, call save() after changing settings. This is done on purpose to avoid unnecessary EEPROM writes.
*/
int DpSettings::deserialize(String serialized_settings) {
    int error = 0;

    // split the input string into key value pairs
    int pos = 0;
    while (pos < serialized_settings.length()) {
        int equalPos = serialized_settings.indexOf('=', pos);
        if (equalPos == -1) {
            error = -1;
            break;
        }
        String key = serialized_settings.substring(pos, equalPos);
        pos = equalPos + 1;

        int commaPos = serialized_settings.indexOf(',', pos);
        String value;
        if (commaPos == -1) {
            value = serialized_settings.substring(pos);
            pos = serialized_settings.length();
        } else {
            value = serialized_settings.substring(pos, commaPos);
            pos = commaPos + 1;
        }

        Serial.println("key: " + key + ", value: " + value);

        // Process the key-value pairs
        if (key == "temperature") {
            temperature(value.toDouble());
        } else if (key == "p" || key == "P") {
            P(value.toDouble());
        } else if (key == "i" || key == "I") {
            I(value.toDouble());
        } else if (key == "d" || key == "D") {
            D(value.toDouble());
        } else if (key == "dffFactorPct") {
            dffFactorPct(value.toDouble());
        } else if (key == "ffHeat" || key == "ff_heat") {
            ffHeat(value.toDouble());
        } else if (key == "ffReady" || key == "ff_ready") {
            ffReady(value.toDouble());
        } else if (key == "ffBrew" || key == "ff_brew") {
            // Deprecated: ignored
            Serial.println("WARNING: ff_brew / ffBrew is deprecated and will be ignored.");
        } else if (key == "tareWeight") {
            tareWeight(value.toDouble());
        } else if (key == "trimWeight") {
            trimWeight(value.toDouble());
        } else if (key == "preInfusionTime") {
            preInfusionTime(value.toDouble());
        } else if (key == "infusionTime" || key == "infuseTime") { // infuseTime is a deprecated alias for infusionTime
            infusionTime(value.toDouble());
        } else if (key == "extractionTime" || key == "extractTime") { // extractTime is a deprecated alias for extractionTime
            extractionTime(value.toDouble());
        } else if (key == "extractionWeight") {
            extractionWeight(value.toDouble());
        } else if (key == "commissioningDone") {
            commissioningDone(value.toInt());
        } else if (key == "shotCounter") {
             shotCounter(value.toInt());
        } else if (key == "wifiMode") {
            wifiMode(value.toInt());
        } else if (key == "steamTemperature") {
            steamTemperature(value.toDouble());
        } else if (key == "steamP") {
            steamP(value.toDouble());
        } else if (key == "steamI") {
            steamI(value.toDouble());
        } else if (key == "steamD") {
            steamD(value.toDouble());
        } else if (key == "steamPumpPower") {
            steamPumpPower(value.toDouble());
        } else if (key == "steamPumpP") {
            steamPumpP(value.toDouble());
        } else if (key == "steamPumpI") {
            steamPumpI(value.toDouble());
        } else if (key == "steamPumpD") {
            steamPumpD(value.toDouble());
        } else if (key == "windupMin") {
            windupMin(value.toDouble());
        } else if (key == "windupMax") {
            windupMax(value.toDouble());
        } else if (key == "integrationBand") {
            integrationBand(value.toDouble());
        } else if (key == "conditionalIntegration") {
            conditionalIntegration(value.toInt());
        } else if (key == "steamWindupMin") {
            steamWindupMin(value.toDouble());
        } else if (key == "steamWindupMax") {
            steamWindupMax(value.toDouble());
        } else if (key == "steamIntegrationBand") {
            steamIntegrationBand(value.toDouble());
        } else if (key == "steamPumpIntegrationBand") {
            steamPumpIntegrationBand(value.toDouble());
        } else if (key == "iDownFactor") {
            iDownFactor(value.toDouble());
        } else if (key == "steamIDownFactor") {
            steamIDownFactor(value.toDouble());
        } else if (key == "steamConditionalIntegration") {
            steamConditionalIntegration(value.toInt());
        } else if (key == "brewIntegralHold") {
            brewIntegralHold(value.toInt());
        } else if (key == "brewHoldTimeoutSec") {
            brewHoldTimeoutSec(value.toInt());
        } else if (key == "brewHoldDipWindowSec") {
            brewHoldDipWindowSec(value.toInt());
        } else if (key == "dffDecay") {
            dffDecay(value.toDouble());
        } else if (key == "encoderReverse") {
            encoderReverse(value.toInt());
        } else {
            Serial.println("Unknown key: " + key);
            error = -2; //unknown key
        }
    }

    if (error < 0) {
        load(); // discard update and restore settings from EEPROM on error
    }

    return error;
}
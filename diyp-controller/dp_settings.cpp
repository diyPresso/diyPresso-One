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

template<typename T> static T to_type(double v) { return (T)v; }
template<> int to_type<int>(double v) { return (int)lround(v); }

static String to_string(double v, int decimals) { return String(v, max(2, decimals)); } // at least 2 decimals in the serial output
static String to_string(int v, int decimals) { return String(v); }

// Old key names and keys of removed settings, accepted by deserialize(), see dp_settings_list.h
static const struct { const char *alias; setting_id_t id; } key_aliases[] = {
#define DP_SETTING_ALIAS(alias, name) { alias, SETTING_##name },
    DP_SETTINGS_ALIASES(DP_SETTING_ALIAS)
#undef DP_SETTING_ALIAS
};

static const char *removed_keys[] = {
#define DP_SETTING_REMOVED(key) key,
    DP_SETTINGS_REMOVED(DP_SETTING_REMOVED)
#undef DP_SETTING_REMOVED
};

// Setting keys in the serial interface, indexed by setting id
static const char *setting_keys[] = {
#define DP_SETTING_KEY(type, name, key, def, lo, hi, dec) key,
    DP_SETTINGS_LIST(DP_SETTING_KEY)
#undef DP_SETTING_KEY
};

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
#define DP_SETTING_DEFAULT(type, name, key, def, lo, hi, dec) settings.name = def;
    DP_SETTINGS_LIST(DP_SETTING_DEFAULT)
#undef DP_SETTING_DEFAULT
    update_crc();
}


/// @brief set all values to default, except the commissioning state and the reservoir calibration (tare and trim)
void DpSettings::reset_to_defaults()
{
    int commissioning_done = commissioningDone();
    double tare_weight = tareWeight();
    double trim_weight = trimWeight();
    defaults();
    commissioningDone(commissioning_done);
    tareWeight(tare_weight);
    trimWeight(trim_weight);
    update_crc();
}


/// @brief get a setting by id
double DpSettings::get(setting_id_t id)
{
    switch (id)
    {
#define DP_SETTING_GET(type, name, key, def, lo, hi, dec) case SETTING_##name: return name();
    DP_SETTINGS_LIST(DP_SETTING_GET)
#undef DP_SETTING_GET
    default: return 0;
    }
}


/// @brief set a setting by id, clamped to its range. Integer settings are rounded.
/// @return the new value
double DpSettings::set(setting_id_t id, double value)
{
    switch (id)
    {
#define DP_SETTING_SET(type, name, key, def, lo, hi, dec) case SETTING_##name: return name(to_type<type>(value));
    DP_SETTINGS_LIST(DP_SETTING_SET)
#undef DP_SETTING_SET
    default: return 0;
    }
}


/// @brief number of decimals of a setting, as shown in the menu
int DpSettings::decimals(setting_id_t id)
{
    switch (id)
    {
#define DP_SETTING_DECIMALS(type, name, key, def, lo, hi, dec) case SETTING_##name: return dec;
    DP_SETTINGS_LIST(DP_SETTING_DECIMALS)
#undef DP_SETTING_DECIMALS
    default: return 0;
    }
}


/// @brief true if all setting values are equal to the values of the other settings object
bool DpSettings::same_values(const DpSettings &other)
{
    // skip the crc, compare the version and all values
    return memcmp(((const unsigned char*)&settings) + 4, ((const unsigned char*)&other.settings) + 4, sizeof(settings_t) - 4) == 0;
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
#define DP_SETTING_SERIALIZE(type, name, key, def, lo, hi, dec) result += key "=" + to_string(settings.name, dec) + "\n";
    DP_SETTINGS_LIST(DP_SETTING_SERIALIZE)
#undef DP_SETTING_SERIALIZE
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
        int id = -1;
        for (int n = 0; n < SETTING_COUNT; n++)
            if (key == setting_keys[n]) id = n;
        for (auto &a : key_aliases)
            if (key == a.alias) id = a.id;
        bool removed = false;
        for (auto r : removed_keys)
            if (key == r) removed = true;

        if (id >= 0) {
            set((setting_id_t)id, value.toDouble());
        } else if (removed) {
            Serial.println("WARNING: " + key + " is deprecated and will be ignored.");
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
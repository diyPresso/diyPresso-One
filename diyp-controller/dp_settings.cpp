/*
  loading and saving of a settings struct.
  We check if the stored settings struct is valid, and the version corresponds to the expected version.
  If there is a mismatch in version or the CRC is invalid we load default values.
  (c) 2024 - diyEspresso - PBRI - CC-BY-NC
*/

#include "dp_settings.h"
#include <FlashAsEEPROM.h>

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
    settings.version = 1;  // Update this if new fields are added to the settings structure to prevent incorrect reads
    settings.temperature = 98.0;
    settings.preInfusionTime = 3;
    settings.infusionTime = 1;
    settings.extractionTime = 25;
    settings.p = 7.0;
    settings.i = 0.3;
    settings.d = 80.0;
    settings.ff_heat = 3.0;
    settings.ff_ready = 10.0;
    settings.ff_brew = 80.0;
    settings.tareWeight = 0.0;
    settings.trimWeight = 0.0;
    settings.wifiMode = 0; // off=0
    settings.shotCounter = 0;
    settings.commissioningDone = 0; // default is 0 (not done)
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
  boilerController.set_ff_heat(ff_heat());
  boilerController.set_ff_ready(ff_ready());
  boilerController.set_ff_brew(ff_brew());

  reservoir.set_trim(trimWeight());
  reservoir.set_tare(tareWeight());

  brewProcess.preInfuseTime = preInfusionTime();
  brewProcess.infuseTime = infusionTime();
  brewProcess.extractTime = extractionTime();

  



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
    result += "ff_heat=" + String(settings.ff_heat) + "\n";
    result += "ff_ready=" + String(settings.ff_ready) + "\n";
    result += "ff_brew=" + String(settings.ff_brew) + "\n";
    result += "tareWeight=" + String(settings.tareWeight) + "\n";
    result += "trimWeight=" + String(settings.trimWeight) + "\n";
    result += "commissioningDone=" + String(settings.commissioningDone) + "\n";
    result += "shotCounter=" + String(settings.shotCounter) + "\n";
    result += "wifiMode=" + String(settings.wifiMode) + "\n";    
    return result;
}


/* receives a string, parses it and updates the settings. For example:
temperature=98.50,P=7.00,I=0.30,D=80.00,ff_heat=3.00,ff_ready=10.00,ff_brew=80.00,tareWeight=0.00,trimWeight=0.00,preInfusionTime=3.00,infuseTime=1.00,extractTime=25.00,extractionWeight=0.00,commissioningDone=1,shotCounter=5,wifiMode=0

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
        } else if (key == "ff_heat") {
            ff_heat(value.toDouble());
        } else if (key == "ff_ready") {
            ff_ready(value.toDouble());
        } else if (key == "ff_brew") {
            ff_brew(value.toDouble());
        } else if (key == "tareWeight") {
            tareWeight(value.toDouble());
        } else if (key == "trimWeight") {
            trimWeight(value.toDouble());
        } else if (key == "preInfusionTime") {
            preInfusionTime(value.toDouble());
        } else if (key == "infusionTime" || key == "infuseTime") { // infuseTime is an depricated alias for infusionTime
            infusionTime(value.toDouble());
        } else if (key == "extractionTime" || key == "extractTime") { // extractTime is an depricated alias for extractionTime
            extractionTime(value.toDouble());
        } else if (key == "extractionWeight") {
            extractionWeight(value.toDouble());
        } else if (key == "commissioningDone") {
            commissioningDone(value.toInt());
        } else if (key == "shotCounter") {
             shotCounter(value.toInt());
        } else if (key == "wifiMode") {
            wifiMode(value.toInt());
        } else {
            Serial.println("Unknown key: " + key);
            error = -2; //unknown key
        }
    }

    if (error < 0) {
        load(); // discart updarte and restore settings from EEPROM on error
    }

    return error;
}

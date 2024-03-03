/*
  loading and saving of a settings struct.
  We check if the stored settings struct is valid, and the version corresponds to the expected version.
  If there is a mismatch in version or the CRC is invalid we load default values.
  (c) 2024 - diyEspresso - PBRI - CC-BY-NC
*/

#include "dp_settings.h"
#include <FlashAsEEPROM.h>

DiyeSettings settings = DiyeSettings();

DiyeSettings::DiyeSettings()
{
    defaults();
}


unsigned long DiyeSettings::crc32(const unsigned char *s, size_t n)
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


void DiyeSettings::update_crc(void)
{
    unsigned char *s = (unsigned char*) &settings;
    settings.crc = crc32( s + 4, sizeof(settings_t) - 4);
}


bool DiyeSettings::crc_is_valid(settings_t *s)
{
    return crc32( ((unsigned char*)s) + 4, sizeof(settings_t) - 4 ) == s->crc;
}


void DiyeSettings::defaults()
{
    settings.version = 1;  // Update this if new fields are added to the settings structure to prevent incorrect reads
    settings.temperature = 98.0;
    settings.preInfusionTime = 3;
    settings.infusionTime = 3;
    settings.extractionTime = 25;
    settings.p = 30;
    settings.i = 0.8;
    settings.d = 0.8;
    settings.ff = 10.0;
    settings.tareWeight = 0.0;
    settings.trimWeight = 0.0;
    settings.wifiState = 0;
    settings.shotCounter = 0;
    update_crc();
}


void DiyeSettings::read(settings_t *s)
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
int DiyeSettings::load()
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
int DiyeSettings::save()
{
    settings_t old_settings;
    update_crc();
    read( &old_settings );

    /*Serial.print("new CRC:"); Serial.println(settings.crc);
    Serial.print("old temp:"); Serial.println(old_settings.temperature);
    Serial.print("new temp:"); Serial.println(settings.temperature); */

    unsigned char *s = (unsigned char*) &settings;
    unsigned char *d = (unsigned char*) &old_settings;
    bool changed = false;
    for (int i=0; i<sizeof(settings_t); i++, s+=1, d+=1 )
    {
        if ( *s != *d )
        {
            EEPROM.write(i, *s);
            changed = true;
            //Serial.print("changed:"); Serial.print(i); Serial.print("="); Serial.println(*s);
        }
    }
    if ( changed )
    {
        EEPROM.commit();
    }
    return changed ? 1 : 0;
}

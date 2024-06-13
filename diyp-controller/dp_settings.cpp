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
    
    // Default Preset Values
    settings.defaultPreset = 0;  // default is 0 (Lungo)
    settings.presets[0] = {1, 3, 30, 50};  // Lungo - pre-infuse, infuse, extraction time, extraction weight.
    settings.presets[1] = {1, 3, 23, 36};  // Espresso - pre-infuse, infuse, extraction time, extraction weight.
    settings.presets[2] = {1, 3, 60, 100}; // Double Lungo - pre-infuse, infuse, extraction time, extraction weight.
    settings.presets[3] = {1, 3, 46, 72};  // Double Espresso - pre-infuse, infuse, extraction time, extraction weight.

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

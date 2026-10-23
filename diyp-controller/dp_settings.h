/*
  dp_settings class
  Loads and Saves the persistent settings.
  if no changes are made to the settings, nothing is saved
  If no valid data is present, default values are saved
  the setters check the range of the values to save, to prevent incorrect data

  All settings are defined in dp_settings_list.h. From that list the struct fields, the
  clamped getters/setters, defaults(), serialize() and deserialize() are generated.
*/

#ifndef DpSettings_h
#define DpSettings_h

#include "Arduino.h"
#include "dp_serial.h"
#include "dp_settings_list.h"

typedef enum wifi_modes { WIFI_MODE_OFF, WIFI_MODE_ON, WIFI_MODE_AP } wifi_mode_t;

// Setting ids: SETTING_temperature, SETTING_P, ...
typedef enum
{
#define DP_SETTING_ID(type, name, key, def, lo, hi, dec) SETTING_##name,
    DP_SETTINGS_LIST(DP_SETTING_ID)
#undef DP_SETTING_ID
    SETTING_COUNT
} setting_id_t;

class DpSettings
{
    private:
        typedef struct __attribute__ ((packed)) settings_struct { // a packed struct has no alignment of fields
            unsigned long int crc; // crc of all the the fields after the crc
            unsigned long int version; // settings struct version
#define DP_SETTING_FIELD(type, name, key, def, lo, hi, dec) type name;
            DP_SETTINGS_LIST(DP_SETTING_FIELD)
#undef DP_SETTING_FIELD
        } settings_t;
        settings_t settings;
        void read(settings_t *s);
        void update_crc(void);
        bool crc_is_valid(settings_t *s);
        unsigned long crc32(const unsigned char *s, size_t n);
    public:
        DpSettings();
        void defaults();
        void reset_to_defaults();
        int load();
        int save();
        void apply();
        String serialize();
        int deserialize(String input);
        double get(setting_id_t id);
        double set(setting_id_t id, double value);
        static int decimals(setting_id_t id);
        bool same_values(const DpSettings &other);

        // Getter and clamped setter for each setting, e.g. temperature() and temperature(98.0)
#define DP_SETTING_ACCESSORS(type, name, key, def, lo, hi, dec) \
        type name() { return settings.name; } \
        type name(type v) { return settings.name = min((type)(hi), max(v, (type)(lo))); }
        DP_SETTINGS_LIST(DP_SETTING_ACCESSORS)
#undef DP_SETTING_ACCESSORS

        int incShotCounter() { return settings.shotCounter += 1; }
        void zeroShotCounter() { settings.shotCounter = 0; }
};

extern DpSettings settings;


#endif // DpSettings_h

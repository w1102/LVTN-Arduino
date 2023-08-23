#include "eepromrw.h"

void
eepromInit ()
{
    EEPROM.begin (EEPROM_SIZE);
}

void
eepromClean ()
{
    for (int i = 0; i < EEPROM_SIZE; i++)
    {
        EEPROM.write (i, 0);
    }
    EEPROM.commit ();
}

void
eepromReadWifiCredentials (String &ssid, String &password)
{
    ssid = EEPROM.readString (EEPROM_WIFI_SSID_ADDR);
    password = EEPROM.readString (EEPROM_WIFI_SPK_ADDR);
}

void
eepromSaveWifiCredentials (String ssid, String password)
{
    eepromClean ();

    EEPROM.writeString (EEPROM_WIFI_SSID_ADDR, ssid);
    EEPROM.writeString (EEPROM_WIFI_SPK_ADDR, password);

    EEPROM.commit ();
}

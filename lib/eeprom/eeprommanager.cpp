#include "eeprommanager.h"

namespace eeprom
{
    void
    init ()
    {
        EEPROM.begin (constants::eeprom::totalSize);
    }

    void
    clean ()
    {
        for (uint16_t i = 0; i < constants::eeprom::totalSize; i++)
        {
            EEPROM.write (i, 0);
        }
        EEPROM.commit ();
    }

    void
    readWifiCredentials (String &ssid, String &password)
    {
        ssid = EEPROM.readString (constants::eeprom::wifiSsidAddr);
        password = EEPROM.readString (constants::eeprom::wifiPskAddr);
    }

    void
    saveWifiCredentials (String ssid, String password)
    {
        if (
            ssid.length () > constants::eeprom::wifiCredentialsLen
            || password.length () > constants::eeprom::wifiCredentialsLen)
        {
            return;
        }

        clean ();

        EEPROM.writeString (constants::eeprom::wifiSsidAddr, ssid);
        EEPROM.writeString (constants::eeprom::wifiPskAddr, password);

        EEPROM.commit ();
    }

}

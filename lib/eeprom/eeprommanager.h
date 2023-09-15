#ifndef EEPROM_MANAGER_H_
#define EEPROM_MANAGER_H_

#include "constants.h"
#include <Arduino.h>
#include <EEPROM.h>

namespace eeprom
{
    void init ();
    void clean();
    void readWifiCredentials(String& ssid, String& password);
    void saveWifiCredentials(String ssid, String password);
}

#endif // EEPROM_MANAGER_H_

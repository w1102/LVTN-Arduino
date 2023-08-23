#ifndef EEPROMRW_H
#define EEPROMRW_H

#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_SIZE 64

#define EEPROM_WIFI_SSID_ADDR 0
#define EEPROM_WIFI_SPK_ADDR 32

void eepromInit();
void eepromClean();
void eepromReadWifiCredentials(String& ssid, String& password);
void eepromSaveWifiCredentials(String ssid, String password);


#endif // EEPROMRW_H
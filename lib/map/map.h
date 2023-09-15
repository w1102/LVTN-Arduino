#ifndef MAP_H_
#define MAP_H_

#include "ArduinoJson.h"
#include "constants.h"
#include "cppQueue.h"
#include "types.h"
#include <Arduino.h>

class Map
{
  public:
    Map ();

    int getImportLineCount ();
    int getExportLineCount ();
    bool lineCountIsInMainBranch(int lineCount);

    static bool parseMapMsg (String &mapMsg);
    static bool parseAgvInfo (AGVInfo &agvInfo, String &agvInfoMsg);
    static void parseAgvInfoToString(AGVInfo& agvInfo, String& dst);

  private:
    static DynamicJsonDocument mapJsonObj;
    static DynamicJsonDocument agvObj;
};

class StorageMap
{
  public:
    StorageMap ();
    bool parseMapMsg (String mapStr);
    bool parseLineCountMsg (int &lineCount, String &lineCountMsg);

    MapSize mapSize ();

    int getImportLineCount ();
    int getExportLineCount ();
    int getHomeLinecount ();

    String lineCount2mapPosStr (int lineCount);
};

#endif // MAP_H_

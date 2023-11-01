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

    int  getHomeLineCnt () const;
    int  getImportLineCount () const;
    int  getExportLineCount () const;
    bool isInMainBranch (int);

    static bool parseMapMsg (const String& mapMsg);
    static bool parseAgvInfo (const String& agvInfoMsg, AGVInfo& agvInfo);
    static void parseAgvInfoToString (const AGVInfo& agvInfo, String& agvInfoStr);

  private:
    static DynamicJsonDocument m_mapDoc;
    static DynamicJsonDocument m_agvDoc;
};

#endif // MAP_H_

#ifndef MAP_HPP_
#define MAP_HPP_

#include <Arduino.h>
#include "types.h"

class StorageMap
{
  public:
    StorageMap();
    bool parseMapMsg(String mapStr);
 
    MapSize mapSize();

    int getImportLineCount();
    int getExportLineCount();
    int getHomeLinecount();

    int parseLineCountMsg(String PosStr);
    String lineCount2mapPosStr(int lineCount);

};

#endif // MAP_HPP_
#include "map.h"

DynamicJsonDocument Map::mapJsonObj (6200);
DynamicJsonDocument Map::agvObj (1024);

Map::Map () {};

bool
Map::parseMapMsg (String &mapMsg)
{
    DeserializationError error = deserializeJson (mapJsonObj, mapMsg);

    if (error)
    {
        Serial.print (F ("Map deserializeJson() failed"));
        return false;
    }

    return true;
}

bool
Map::parseAgvInfo (AGVInfo &agvInfo, String &agvInfoMsg)
{
    DeserializationError error = deserializeJson (agvObj, agvInfoMsg);

    if (error)
    {
        Serial.print (F ("AGVInfo deserializeJson() failed"));
        return false;
    }

    agvInfo.id = new String (agvObj[constants::map::jsonKey::id].as<String> ());

    agvInfo.leaveHomeActs = new String (agvObj[constants::map::jsonKey::leaveHomeActs].as<String> ());
    agvInfo.forwardDirHomingActs = new String (agvObj[constants::map::jsonKey::forwardDirHomingActs].as<String> ());
    agvInfo.rewardDirHomingActs = new String (agvObj[constants::map::jsonKey::rewardDirHomingActs].as<String> ());

    agvInfo.direction = (agvObj[constants::map::jsonKey::direction].as<int> () == 1)
                            ? forward
                            : reward;

    agvInfo.currentLineCount = agvObj[constants::map::jsonKey::currentLineCount];

    agvInfo.isHome = agvObj[constants::map::jsonKey::isHome];
    agvInfo.isMainBranch = agvObj[constants::map::jsonKey::isMainBranch];
    agvInfo.isItemPicked = agvObj[constants::map::jsonKey::isItemPicked];

    return true;
}

void
Map::parseAgvInfoToString (AGVInfo &agvInfo, String &dst)
{
    agvObj.clear ();

    agvObj[constants::map::jsonKey::id] = *agvInfo.id;
    agvObj[constants::map::jsonKey::leaveHomeActs] = *agvInfo.leaveHomeActs;
    agvObj[constants::map::jsonKey::forwardDirHomingActs] = *agvInfo.forwardDirHomingActs;
    agvObj[constants::map::jsonKey::rewardDirHomingActs] = *agvInfo.rewardDirHomingActs;
    agvObj[constants::map::jsonKey::direction] = agvInfo.direction == forward ? 1 : 0;
    agvObj[constants::map::jsonKey::currentLineCount] = agvInfo.currentLineCount;
    agvObj[constants::map::jsonKey::isHome] = agvInfo.isHome;
    agvObj[constants::map::jsonKey::isItemPicked] = agvInfo.isItemPicked;
    agvObj[constants::map::jsonKey::isMainBranch] = agvInfo.isMainBranch;

    serializeJsonPretty (agvObj, dst);
}

int
Map::getImportLineCount ()
{
    return 0;
}

int
Map::getExportLineCount ()
{
    return 7;
}

bool
Map::lineCountIsInMainBranch (int lineCount)
{
    return lineCount == getImportLineCount () || lineCount == getExportLineCount ();
}

DynamicJsonDocument mapMsgObj (6200);

StorageMap::StorageMap ()
{
}

bool
StorageMap::parseMapMsg (String mapStr)
{
    DeserializationError error = deserializeJson (mapMsgObj, mapStr);

    if (error)
    {
        Serial.print (F ("deserializeJson() failed: "));
        Serial.println (error.c_str ());
        return false;
    }

    return true;
}

MapSize
StorageMap::mapSize ()
{
    MapSize size;
    size.width = mapMsgObj["size"]["width"];
    size.height = mapMsgObj["size"]["height"];

    return size;
}

bool
StorageMap::parseLineCountMsg (int &lineCount, String &lineCountMsg)
{
    DynamicJsonDocument lineCountMsgObj (256);
    DeserializationError error = deserializeJson (lineCountMsgObj, lineCountMsg);

    if (error)
    {
        lineCountMsgObj.clear ();
        return false;
    }

    int posX = lineCountMsgObj["x"];
    int posY = lineCountMsgObj["y"];

    JsonArray nodes = mapMsgObj["nodes"];
    for (JsonObject node : nodes)
    {
        JsonObject point = node["point"];
        int nodeX = point["x"];
        int nodeY = point["y"];

        if (
            posX == nodeX && posY == nodeY)
        {
            JsonObject data = node["data"];
            lineCount = data["lineCount"];
            break;
        }
    }

    lineCountMsgObj.clear ();
    return true;
}

int
StorageMap::getImportLineCount ()
{
    return 0; // for testing

    int lineCount = 0;

    JsonArray nodes = mapMsgObj["nodes"];
    for (JsonObject node : nodes)
    {
        String role = node["role"];
        if (role == "importPallet")
        {
            JsonObject data = node["data"];
            lineCount = data["lineCount"];
            break;
        }
    }

    return lineCount;
}

int
StorageMap::getExportLineCount ()
{
    return 9;

    int lineCount = 0;

    JsonArray nodes = mapMsgObj["nodes"];
    for (JsonObject node : nodes)
    {
        String role = node["role"];
        if (role == "exportPallet")
        {
            JsonObject data = node["data"];
            lineCount = data["lineCount"];
            break;
        }
    }

    return lineCount;
}

int
StorageMap::getHomeLinecount ()
{
    return 1;
}

String
StorageMap::lineCount2mapPosStr (int lineCount)
{
    String mapPosStr = "";

    JsonArray nodes = mapMsgObj["nodes"];
    for (JsonObject node : nodes)
    {
        JsonObject data = node["data"];
        int _lineCount = data["lineCount"];

        if (
            _lineCount == lineCount)
        {
            JsonObject pos = node["point"];
            serializeJson (pos, mapPosStr);
            break;
        }
    }

    return mapPosStr;
}

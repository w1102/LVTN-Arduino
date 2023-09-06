#include "map.h"
#include <ArduinoJson.h>

DynamicJsonDocument mapMsgObj(6200);

StorageMap::StorageMap()
{
}

bool StorageMap::parseMapMsg(String mapStr)
{
    DeserializationError error = deserializeJson(mapMsgObj, mapStr);

    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return false;
    }

    return true;
}

MapSize StorageMap::mapSize()
{
    MapSize size;
    size.width = mapMsgObj["size"]["width"];
    size.height = mapMsgObj["size"]["height"];

    return size;
}

bool StorageMap::parseLineCountMsg(int &lineCount, String& lineCountMsg)
{
    DynamicJsonDocument lineCountMsgObj(256);
    DeserializationError error = deserializeJson(lineCountMsgObj, lineCountMsg);

    if (error)
    {
        lineCountMsgObj.clear();
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
            posX == nodeX &&
            posY == nodeY)
        {
            JsonObject data = node["data"];
            lineCount = data["lineCount"];
            break;
        }
    }

    lineCountMsgObj.clear();
    return true;
}

int StorageMap::getImportLineCount()
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

int StorageMap::getExportLineCount()
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

int StorageMap::getHomeLinecount()
{
    return 1;
}

String StorageMap::lineCount2mapPosStr(int lineCount)
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
            serializeJson(pos, mapPosStr);
            break;
        }
    }

    return mapPosStr;
}

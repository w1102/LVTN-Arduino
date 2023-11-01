#include "map.h"

DynamicJsonDocument Map::m_mapDoc (6200);
DynamicJsonDocument Map::m_agvDoc (1024);

Map::Map () {};

bool Map::parseMapMsg (const String& mapMsg)
{
    DeserializationError error = deserializeJson (m_mapDoc, mapMsg);

    if (error)
    {
        Serial.print (F ("Map deserializeJson() failed"));
        return false;
    }

    return true;
}

bool Map::parseAgvInfo (const String& agvInfoMsg, AGVInfo& agvInfo)
{
    DeserializationError error = deserializeJson (m_agvDoc, agvInfoMsg);

    if (error)
    {
        Serial.print (F ("AGVInfo deserializeJson() failed"));
        return false;
    }

    agvInfo.id                   = new String (m_agvDoc[ constants::map::jsonKey::id ].as< String > ());
    agvInfo.missionId            = new String (m_agvDoc[ constants::map::jsonKey::missionId ].as< String > ());
    agvInfo.leaveHomeActs        = new String (m_agvDoc[ constants::map::jsonKey::leaveHomeActs ].as< String > ());
    agvInfo.forwardDirHomingActs = new String (m_agvDoc[ constants::map::jsonKey::forwardDirHomingActs ].as< String > ());
    agvInfo.rewardDirHomingActs  = new String (m_agvDoc[ constants::map::jsonKey::rewardDirHomingActs ].as< String > ());

    agvInfo.currentLineCount = m_agvDoc[ constants::map::jsonKey::currentLineCount ];

    agvInfo.isHome       = m_agvDoc[ constants::map::jsonKey::isHome ];
    agvInfo.isMainBranch = m_agvDoc[ constants::map::jsonKey::isMainBranch ];
    agvInfo.isItemPicked = m_agvDoc[ constants::map::jsonKey::isItemPicked ];

    return true;
}

void Map::parseAgvInfoToString (const AGVInfo& agvInfo, String& agvInfoStr)
{
    m_agvDoc.clear ();

    m_agvDoc[ constants::map::jsonKey::id ]                   = *agvInfo.id;
    m_agvDoc[ constants::map::jsonKey::missionId ]            = *agvInfo.missionId;
    m_agvDoc[ constants::map::jsonKey::leaveHomeActs ]        = *agvInfo.leaveHomeActs;
    m_agvDoc[ constants::map::jsonKey::forwardDirHomingActs ] = *agvInfo.forwardDirHomingActs;
    m_agvDoc[ constants::map::jsonKey::rewardDirHomingActs ]  = *agvInfo.rewardDirHomingActs;
    m_agvDoc[ constants::map::jsonKey::currentLineCount ]     = agvInfo.currentLineCount;
    m_agvDoc[ constants::map::jsonKey::isHome ]               = agvInfo.isHome;
    m_agvDoc[ constants::map::jsonKey::isItemPicked ]         = agvInfo.isItemPicked;
    m_agvDoc[ constants::map::jsonKey::isMainBranch ]         = agvInfo.isMainBranch;

    serializeJsonPretty (m_agvDoc, agvInfoStr);
}

int Map::getHomeLineCnt () const
{
    return 1;
}

int Map::getImportLineCount () const
{
    // return 0;

    int lineCount = 0;

    JsonArray nodes = m_mapDoc[ "nodes" ];
    for (JsonObject node : nodes)
    {
        String role = node[ "role" ];
        if (role == "importPallet")
        {
            JsonObject data = node[ "data" ];
            lineCount       = data[ "lineCount" ];
            break;
        }
    }

    return lineCount;
}

int Map::getExportLineCount () const
{
    // return 7;

    int lineCount = 0;

    JsonArray nodes = m_mapDoc[ "nodes" ];
    for (JsonObject node : nodes)
    {
        String role = node[ "role" ];
        if (role == "exportPallet")
        {
            JsonObject data = node[ "data" ];
            lineCount       = data[ "lineCount" ];
            break;
        }
    }

    return lineCount;
}

bool Map::isInMainBranch (int lineCount)
{
    return lineCount == getImportLineCount () || lineCount == getExportLineCount ();
}

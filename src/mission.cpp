#include "mission.h"
#include <ArduinoJson.h>

DynamicJsonDocument missionObj (1024);

Mission::Mission ()
    : targetLineCount (0)
{
}

bool
Mission::parseMissionMsg (String missionStr)
{
    DeserializationError error = deserializeJson (missionObj, missionStr);

    if (error)
    {
        return false;
    }
    else if (!missionObj.containsKey (MISSION_TYPE_KEY) || !missionObj.containsKey (MISSION_LINECOUNT_KEY))
    {
        return false;
    }

    String type = missionObj["type"];
    this->type = type == "export" ? MissionType::exportMission : MissionType::importMission;
    targetLineCount = missionObj[MISSION_LINECOUNT_KEY];

    return true;
}

String
Mission::missionStatus2Str (MissionStatus status)
{
    switch (status)
    {
    case MissionStatus::missionstatus_no_mission:
        return "No Mission";
    case MissionStatus::missionstatus_taking:
        return "Taking Mission";
    case MissionStatus::missionstatus_running:
        return "Running Mission";
    case MissionStatus::missionstatus_done:
        return "Done Mission";
        return "Homing";
    }

    return "";
}

int
Mission::getTargetLineCount ()
{
    return targetLineCount;
}

MissionType
Mission::getType ()
{
    return type;
}

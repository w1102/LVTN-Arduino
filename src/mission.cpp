#include "mission.h"
#include <ArduinoJson.h>

cppQueue statusQueue (sizeof (MissionStatus), 10);
DynamicJsonDocument missionObj (1024);

Mission::Mission ()
{
    lastStatus = MissionStatus::missionstatus_no_mission;
    lineCount = 0;
}

bool
Mission::parseMission (String missionStr)
{
    DeserializationError error = deserializeJson (missionObj, missionStr);

    if (error)
    {
        Serial.print (F ("deserializeJson() failed: "));
        Serial.println (error.c_str ());
        return false;
    }
    else if (
        !missionObj.containsKey (MISSION_TYPE_KEY) || !missionObj.containsKey (MISSION_LINECOUNT_KEY))
    {
        printInvalid ();
        return false;
    }

    String type = missionObj["type"];
    this->type = type == "export" ? MissionType::exportMission : MissionType::importMission;
    lineCount = missionObj[MISSION_LINECOUNT_KEY];

    return true;
}

void
Mission::printInvalid ()
{
    Serial.print (F ("Mission invalid"));
    Serial.println ();
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

void
Mission::putStatus (MissionStatus status)
{
    lastStatus = status;
    statusQueue.push (&status);
}

MissionStatus
Mission::popStatus ()
{
    MissionStatus status;
    statusQueue.pop (&status);
    return status;
}

MissionStatus
Mission::getLastStatus ()
{
    MissionStatus status;
    statusQueue.peek (&status);
    return status;
}

bool
Mission::isStatusQueueEmpty ()
{
    return statusQueue.isEmpty ();
}

int
Mission::getLineCount ()
{
    return lineCount;
}

MissionType
Mission::getType ()
{
    return type;
}

void
Mission::clean ()
{
    statusQueue.clean ();
}

#include "mission.h"
#include <ArduinoJson.h>

#define MISSION_PHASE_1_KEY "phase1"
#define MISSION_PHASE_2_KEY "phase2"
#define MISSION_TARGET_KEY "target"
#define MISSION_ACTION_KEY "action"

#define SUM_IO 184
#define SUM_BYPASS 178
#define SUM_TURN_BACK 182
#define SUM_TURN_LEFT 192
#define SUM_TURN_RIGHT 198

#define SUM_IO_F 152
#define SUM_BYPASS_F 146
#define SUM_TURN_BACK_F 150
#define SUM_TURN_LEFT_F 160
#define SUM_TURN_RIGHT_F 166

DynamicJsonDocument missionObj (512);

void
parseMissionAction (cppQueue &actionQueue, String *actionStr)
{
    int idx = 0;

    while (idx + 3 <= actionStr->length ())
    {
        String act = actionStr->substring (idx, idx + 2);
        idx += 3;

        int sum = act[0] + act[1];

        switch (act[0] + act[1])
        {
        case SUM_IO:
            Serial.println ("io");
            break;
        case SUM_BYPASS:
            Serial.println ("bypass");
            break;
        case SUM_TURN_BACK:
            Serial.println ("back");
            break;
        case SUM_TURN_LEFT:
            Serial.println ("left");
            break;
        case SUM_TURN_RIGHT:
            Serial.println ("right");
            break;

        case SUM_IO_F:
            Serial.println ("force io");
            break;
        case SUM_BYPASS_F:
            Serial.println ("force bypass");
            break;
        case SUM_TURN_BACK_F:
            Serial.println ("force back");
            break;
        case SUM_TURN_LEFT_F:
            Serial.println ("force left");
            break;
        case SUM_TURN_RIGHT_F:
            Serial.println ("force right");
            break;

        default:
            break;
        }
    }
}

bool
parseMissionMsg (MissionData &mission, String missionStr)
{
    if (deserializeJson (missionObj, missionStr))
    {
        return false;
    }

    JsonObject phase1 = missionObj[MISSION_PHASE_1_KEY];
    JsonObject phase2 = missionObj[MISSION_PHASE_2_KEY];

    String phase1Action = phase1[MISSION_ACTION_KEY];
    String phase2Action = phase2[MISSION_ACTION_KEY];

    int phase1Target = phase1[MISSION_TARGET_KEY];
    int phase2Target = phase2[MISSION_TARGET_KEY];

    mission.phase1.target = phase1Target;
    mission.phase1.action = new String (phase1Action);
    mission.phase2.target = phase2Target;
    mission.phase2.action = new String (phase2Action);

    return true;
}
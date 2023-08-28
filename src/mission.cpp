#include "mission.h"
#include <ArduinoJson.h>

#define MISSION_PHASE_1_KEY "phase1"
#define MISSION_PHASE_2_KEY "phase2"
#define MISSION_TARGET_KEY "target"
#define MISSION_ACTION_KEY "action"
#define MISSION_STOP_MAIN_BRANCH_KEY "stopInMainBranch"

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

#define SUM_DONE_ORIGI 179
#define SUM_DONE_ORIGI_F 147

#define SUM_DONE_INCRE 111
#define SUM_DONE_DECRE 113

DynamicJsonDocument missionObj (512);

void
_missionPutAct (cppQueue *actQueue, ActType act, bool forceExcu = false)
{
    ActData actDat;
    actDat.type = act;
    actDat.forceExcu = forceExcu;

    actQueue->push (&actDat);
}

void
parseMissionAction (cppQueue *actionQueue, String *actionStr)
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
            _missionPutAct (actionQueue, act_io);
            break;
        case SUM_BYPASS:
            _missionPutAct (actionQueue, act_bypass);
            break;
        case SUM_TURN_BACK:
            _missionPutAct (actionQueue, act_turnBack);
            break;
        case SUM_TURN_LEFT:
            _missionPutAct (actionQueue, act_turnLeft);
            break;
        case SUM_TURN_RIGHT:
            _missionPutAct (actionQueue, act_turnRight);
            break;

        case SUM_IO_F:
            _missionPutAct (actionQueue, act_io, true);
            break;
        case SUM_BYPASS_F:
            _missionPutAct (actionQueue, act_bypass, true);
            break;
        case SUM_TURN_BACK_F:
            _missionPutAct (actionQueue, act_turnBack, true);
            break;
        case SUM_TURN_LEFT_F:
            _missionPutAct (actionQueue, act_turnLeft, true);
            break;
        case SUM_TURN_RIGHT_F:
            _missionPutAct (actionQueue, act_turnRight, true);
            break;

        case SUM_DONE_INCRE:
            _missionPutAct (actionQueue, act_done_incre);
            break;
        case SUM_DONE_DECRE:
            _missionPutAct (actionQueue, act_done_decre);
            break;
        case SUM_DONE_ORIGI:
            _missionPutAct (actionQueue, act_done_origi);
            break;
        case SUM_DONE_ORIGI_F:
            _missionPutAct (actionQueue, act_done_origi, true);
            break;

        default:
            break;
        }
    }

    Serial.println ("\n\n");
    delete actionStr;
    actionStr = nullptr;
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

    bool stopInMainBranch = missionObj[MISSION_STOP_MAIN_BRANCH_KEY];

    String phase1Action = phase1[MISSION_ACTION_KEY];
    String phase2Action = phase2[MISSION_ACTION_KEY];

    int phase1Target = phase1[MISSION_TARGET_KEY];
    int phase2Target = phase2[MISSION_TARGET_KEY];

    mission.stopInMainBranch = stopInMainBranch;
    mission.phase1.target = phase1Target;
    mission.phase1.action = new String (phase1Action);
    mission.phase2.target = phase2Target;
    mission.phase2.action = new String (phase2Action);

    return true;
}
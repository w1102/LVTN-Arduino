#ifndef MISSION_H_
#define MISSION_H_

#include "ArduinoJson.h"
#include "constants.h"
#include "cppQueue.h"
#include "types.h"
#include <Arduino.h>

class Mission
{
  public:
    Mission ();

    void setMission (int phase1Target, String *phase1Acts, int phase2Target, String *phase2Acts);
    void setHomingMission (int homeTarget, String *homeActs);
    bool isHomingMission();

    MissionPhase getPhase ();
    void turnNextPhase ();

    int getPhaseTarget ();
    String *getPhaseActs ();

    static bool parseMissionMsg (Mission &mission, String &missionMsg);
    static void parseMissionAct (cppQueue &actQueue, String *acts);

  private:
    static DynamicJsonDocument jsonObj;
    bool _isHomingMission;
    MissionPhase currentPhase;
    String *phase1Acts, *phase2Acts;
    int phase1Target, phase2Target;

    static void pushAct (cppQueue &actQueue, ActType type, bool forceExcu = false);
};

#endif // MISSION_H_

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
    enum Phase
    {
        phase1,
        phase2
    };

    Mission ();

    void setMission (String* id, int phase1Target, String* phase1Acts, int phase2Target, String* phase2Acts);
    void setHomingMission (int homeTarget, String* homeActs);
    bool isHomingMission ();

    Mission::Phase getPhase () const;
    void           turnNextPhase ();

    int     getPhaseTarget () const;
    String* getPhaseActs () const;

    String* id();

    static bool parseMissionMsg (Mission& mission, String& missionMsg);
    static void parseMissionAct (cppQueue& actQueue, String* acts);

  private:
    Mission::Phase currentPhase;
    String*        m_id;
    String*        m_phase1Acts;
    String*        m_phase2Acts;
    bool           m_isHomeMission;
    int            m_phase1Target;
    int            m_phase2Target;

    static void pushAct (cppQueue& actQueue, ActType type, bool forceExcu = false);
};

#endif // MISSION_H_

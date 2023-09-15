#include "mission.h"

DynamicJsonDocument Mission::jsonObj (512);

Mission::Mission ()
    : phase1Target { 0 }, phase1Acts { nullptr },
      phase2Target { 0 }, phase2Acts { nullptr },
      currentPhase { phase1 }, _isHomingMission { false } {};

void
Mission::setMission (int phase1Target, String *phase1Acts, int phase2Target, String *phase2Acts)
{

    currentPhase = phase1;

    this->phase1Target = phase1Target;
    this->phase1Acts = phase1Acts;

    this->phase2Target = phase2Target;
    this->phase2Acts = phase2Acts;

    if (_isHomingMission)
    {
        _isHomingMission = false;
    }
}

void
Mission::setHomingMission (int homeTarget, String *homeActs)
{
    setMission (homeTarget, homeActs, homeTarget, homeActs);
    currentPhase = phase2;

    if (_isHomingMission == false)
    {
        _isHomingMission = true;
    }
}

bool
Mission::isHomingMission ()
{
    return _isHomingMission;
}

MissionPhase
Mission::getPhase ()
{
    return currentPhase;
}

void
Mission::turnNextPhase ()
{
    if (currentPhase == phase1)
    {
        currentPhase = phase2;
    }
}

int
Mission::getPhaseTarget ()
{
    return currentPhase == phase1 ? phase1Target : phase2Target;
}

String *
Mission::getPhaseActs ()
{
    return currentPhase == phase1 ? phase1Acts : phase2Acts;
}

bool
Mission::parseMissionMsg (Mission &mission, String &missionMsg)
{
    if (deserializeJson (jsonObj, missionMsg))
    {
        Serial.print (F ("Mission deserializeJson() failed"));
        return false;
    }

    JsonObject phase1Obj = jsonObj[constants::mission::jsonKey::phase1];
    JsonObject phase2Obj = jsonObj[constants::mission::jsonKey::phase2];

    mission.setMission (
        phase1Obj[constants::mission::jsonKey::target].as<int> (),
        new String (phase1Obj[constants::mission::jsonKey::action].as<String> ()),
        phase2Obj[constants::mission::jsonKey::target].as<int> (),
        new String (phase2Obj[constants::mission::jsonKey::action].as<String> ()));

    return true;
}

void
Mission::parseMissionAct (cppQueue &actQueue, String *acts)
{
    for (
        int i = 0;
        i + constants::mission::actCodeTable::codeLength + 1 <= acts->length ();
        i += constants::mission::actCodeTable::codeLength + 1)
    /*
    codeLength + 1 with 1 is delimiter (';')
    */
    {
        String act = acts->substring (i, i + constants::mission::actCodeTable::codeLength);

        switch (act[0] + act[1])
        {
        case constants::mission::actCodeTable::io:
            pushAct (actQueue, act_io);
            break;
        case constants::mission::actCodeTable::bypass:
            pushAct (actQueue, act_bypass);
            break;
        case constants::mission::actCodeTable::turnBack:
            pushAct (actQueue, act_turnBack);
            break;
        case constants::mission::actCodeTable::turnLeft:
            pushAct (actQueue, act_turnLeft);
            break;
        case constants::mission::actCodeTable::turnRight:
            pushAct (actQueue, act_turnRight);
            break;
        case constants::mission::actCodeTable::done:
            pushAct (actQueue, act_done);
            break;

        case constants::mission::actCodeTable::forceIo:
            pushAct (actQueue, act_io, true);
            break;
        case constants::mission::actCodeTable::forceBypass:
            pushAct (actQueue, act_bypass, true);
            break;
        case constants::mission::actCodeTable::forceTurnBack:
            pushAct (actQueue, act_turnBack, true);
            break;
        case constants::mission::actCodeTable::forceTurnLeft:
            pushAct (actQueue, act_turnLeft, true);
            break;
        case constants::mission::actCodeTable::forceTurnRight:
            pushAct (actQueue, act_turnRight, true);
            break;
        case constants::mission::actCodeTable::forceDone:
            pushAct (actQueue, act_done, true);
            break;
        }
    }
}

void
Mission::pushAct (cppQueue &actQueue, ActType type, bool forceExcu)
{
    ActData act;
    act.type = type;
    act.forceExcu = forceExcu;

    actQueue.push (&act);
}

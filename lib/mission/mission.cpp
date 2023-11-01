#include "mission.h"

Mission::Mission ()
    : m_id { nullptr }
    , m_phase1Target { 0 }
    , m_phase1Acts { nullptr }
    , m_phase2Target { 0 }
    , m_phase2Acts { nullptr }
    , currentPhase { phase1 }
    , m_isHomeMission { false } {};

void Mission::setMission (String* id, int phase1Target, String* phase1Acts, int phase2Target, String* phase2Acts)
{
    m_id = id;

    m_phase1Target = phase1Target;
    m_phase1Acts   = phase1Acts;

    m_phase2Target = phase2Target;
    m_phase2Acts   = phase2Acts;

    currentPhase = phase1;

    if (m_isHomeMission)
        m_isHomeMission = false;
}

void Mission::setHomingMission (int homeTarget, String* homeActs)
{
    setMission (new String ("HO"), homeTarget, homeActs, homeTarget, homeActs);
    currentPhase = phase2;

    if (m_isHomeMission == false)
        m_isHomeMission = true;
}

bool Mission::isHomingMission ()
{
    return m_isHomeMission;
}

Mission::Phase Mission::getPhase () const
{
    return currentPhase;
}

void Mission::turnNextPhase ()
{
    if (currentPhase == phase1)
        currentPhase = phase2;
}

int Mission::getPhaseTarget () const
{
    return currentPhase == phase1 ? m_phase1Target : m_phase2Target;
}

String* Mission::getPhaseActs () const
{
    return currentPhase == phase1 ? m_phase1Acts : m_phase2Acts;
}

String* Mission::id() {
    return m_id;
}

bool Mission::parseMissionMsg (Mission& mission, String& missionMsg)
{
    DynamicJsonDocument doc (256);
    if (deserializeJson (doc, missionMsg))
    {
        Serial.print (F ("Mission deserializeJson() failed"));
        return false;
    }

    JsonObject phase1Obj = doc[ constants::mission::jsonKey::phase1 ];
    JsonObject phase2Obj = doc[ constants::mission::jsonKey::phase2 ];
    String*    id        = new String (doc[ constants::mission::jsonKey::id ]);

    int     phase1Target = phase1Obj[ constants::mission::jsonKey::target ].as< int > ();
    String* phase1Acts   = new String (phase1Obj[ constants::mission::jsonKey::action ].as< String > ());
    int     phase2Target = phase2Obj[ constants::mission::jsonKey::target ].as< int > ();
    String* phase2Acts   = new String (phase2Obj[ constants::mission::jsonKey::action ].as< String > ());

    mission.setMission (id, phase1Target, phase1Acts, phase2Target, phase2Acts);

    return true;
}

void Mission::parseMissionAct (cppQueue& actQueue, String* acts)
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

        switch (act[ 0 ] + act[ 1 ])
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

void Mission::pushAct (cppQueue& actQueue, ActType type, bool forceExcu)
{
    ActData act;
    act.type      = type;
    act.forceExcu = forceExcu;

    actQueue.push (&act);
}

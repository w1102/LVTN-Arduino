#include "main.fsm.h"

/*
Implementing a state machine to manage main actions:
    * line follower
    * tasking mission
    * ...
*/

// ----------------------------------------------------------------------------
// prototype declarations
// clang-format off
// 

/* global variable declarations */
QueueHandle_t mainstatusQueue;

/* state declarations */
class MainState_initialize;
class MainState_idle;
class MainState_move;
class MainState_bypass;
class MainState_io;
class MainState_turnLeft;
class MainState_turnRight;
class MainState_turnBack;
class MainState_done;

/* helper declarations */

void onInterval(TimerHandle_t t);
inline void putMainStatus(MainStatus status);

// ----------------------------------------------------------------------------
// State: MainState_initialize
// clang-format on
//

class MainState_initialize : public MainManager
{
  public:
    void
    entry () override
    {
        mainstatusQueue = xQueueCreate (CONF_MAINFSM_QUEUE_LENGTH, sizeof (MainStatus));

        trigger ("MainState_initialize_Trigger",
                 [=] ()
                 {
                     xQueueReceive (currentLineCountQueue, &currentLineCount, portMAX_DELAY);
                     putMainStatus (mainstatus_idle);
                     transit<MainState_idle> ();
                 });
    }
};

// ----------------------------------------------------------------------------
// State: MainState_idle
//

class MainState_idle : public MainManager
{
  public:
    void
    entry () override
    {
        timerInit (onInterval, pdMS_TO_TICKS (100), true);
        timerStart ();
    }

    void
    exit () override
    {
        timerDelete ();
    }

    void
    react (mainevent_interval const &) override
    {
        transit<MainState_move> (
            [=] () {
                
            },
            [=] () -> bool
            {
                return taskingMissionIfAvailable () || returnHomeIfAvailable ();
            });
    }

  private:
    bool
    taskingMissionIfAvailable ()
    {
        if (xQueueReceive (missionQueue, &mission, 0) == pdTRUE)
        {
            if (currentLineCount > targetLineCount)
            {
                pushAct (act_turnBack, true);
            }

            currentPhase = phase1;
            parseMissionAction (actQueue, mission.phase1.action);

            return true;
        }

        return false;
    }

    bool
    returnHomeIfAvailable ()
    {
        xSemaphoreTake (storageMapMutex, portMAX_DELAY);
        int homeLineCount = storageMap.getHomeLinecount ();
        xSemaphoreGive (storageMapMutex);

        if (homeLineCount != currentLineCount)
        {
            targetLineCount = homeLineCount - 1;
            pushAct (act_turnBack);
            pushAct (act_bypass);
            pushAct (act_doneMission, true);

            return true;
        }

        return false;
    }
};

// ----------------------------------------------------------------------------
// State: MainState_missionRecive
//

class MainState_move : public MainManager
{
  public:
    void
    entry () override
    {
        setPos = 0;
        currentPos = 0;
        computedPos = 0;
        pid = new PID (&currentPos, &computedPos, &setPos, KP, KI, KD, AUTOMATIC);

        pid->SetSampleTime (CONF_MAINFSM_INTERVAL_MS);
        pid->SetMode (AUTOMATIC);
        pid->SetOutputLimits (-1024, 1024);

        rhsMotor->forward ();
        lhsMotor->forward ();

        timerInit (onInterval, pdMS_TO_TICKS (CONF_MAINFSM_INTERVAL_MS), true);
        timerStart ();
    }

    void
    exit () override
    {
        timerDelete ();
    }

    void
    react (mainevent_interval const &) override
    {
        ActData act;
        if (actQueue.peek (&act) && act.forceExcu)
        {
            sendMainEvent (mainevent_dispatch_act {});
            return;
        }

        currentPos = makerLine->readPosition ();

        if (currentPos == STOP_POSITION)
        {
            stopHandle ();
        }
        else
        {
            moveHanlde ();
        }
    }

    void
    react (mainevent_dispatch_act const &) override
    {
        ActData act = popAct ();

        switch (act.type)
        {
        case ActType::act_turnLeft:
            transit<MainState_turnLeft> ();
            break;
        case ActType::act_turnRight:
            transit<MainState_turnRight> ();
            break;
        case ActType::act_turnBack:
            transit<MainState_turnBack> ();
            break;
        case ActType::act_bypass:
            transit<MainState_bypass> ();
            break;
        case ActType::act_io:
            transit<MainState_io> ();
            break;
        case ActType::act_doneMission:
            transit<MainState_done> ();
            break;
        }
    }

  private:
    double setPos,
        currentPos,
        computedPos;
    PID *pid;

    void
    stopHandle ()
    {
        timerStop ();

        rhsMotor->stop ();
        lhsMotor->stop ();

        if (!actQueue.isEmpty ())
        {
            sendMainEvent (mainevent_dispatch_act {});
        }
        else
        {
            currentLineCount = targetLineCount < currentLineCount ? currentLineCount - 1 : currentLineCount + 1;

            if (currentLineCount == targetLineCount)
            {
                dispatchAct ();
            }
            else
            {
                transit<MainState_bypass> ();
            }
        }
    }

    void
    moveHanlde ()
    {
        pid->Compute ();

        int baseSpeed = CONF_MAINFSM_MID_SPEED;

        if (abs (targetLineCount - currentLineCount) <= 1 || !actQueue.isEmpty ())
        {
            baseSpeed = CONF_MAINFSM_LOW_SPEED;
        }

        rhsMotor->setSpeed (baseSpeed + this->computedPos);
        lhsMotor->setSpeed (baseSpeed - this->computedPos);
    }

    void
    dispatchAct ()
    {
        MissionType missionType;

        // xSemaphoreTake (missionMutex, portMAX_DELAY);
        // missionType = mission.getType ();
        // xSemaphoreGive (missionMutex);

        if (homing)
        {
            // pushAct (MainActType::act_turnBack);
            // pushAct (MainActType::act_bypass);
            // pushAct (MainActType::act_doneMission, true);
        }
        else if (missionType == MissionType::exportMission && itemPicked)
        {

            // pushAct (MainActType::act_io);
            // pushAct (MainActType::act_turnBack, true);

            // for homing
            homing = true;

            xSemaphoreTake (storageMapMutex, portMAX_DELAY);
            targetLineCount = storageMap.getHomeLinecount () - 1;
            xSemaphoreGive (storageMapMutex);
        }
        else if (missionType == MissionType::exportMission)
        {
            // pushAct (MainActType::act_bypass);
            // pushAct (MainActType::act_turnLeft, true);
            // pushAct (MainActType::act_io);
            // pushAct (MainActType::act_turnBack, true);
            // pushAct (MainActType::act_bypass);
            // pushAct (MainActType::act_turnLeft, true);

            xSemaphoreTake (storageMapMutex, portMAX_DELAY);
            targetLineCount = storageMap.getExportLineCount ();
            xSemaphoreGive (storageMapMutex);
        }
        else if (missionType == MissionType::importMission && itemPicked)
        {
            // pushAct (MainActType::act_bypass);
            // pushAct (MainActType::act_turnLeft, true);
            // pushAct (MainActType::act_io);
            // pushAct (MainActType::act_turnBack, true);
            // pushAct (MainActType::act_bypass);
            // pushAct (MainActType::act_turnRight, true);

            homing = true;

            xSemaphoreTake (storageMapMutex, portMAX_DELAY);
            targetLineCount = storageMap.getHomeLinecount () - 1;
            xSemaphoreGive (storageMapMutex);
        }
        else if (missionType == MissionType::importMission)
        {
            // pushAct (MainActType::act_io);
            // pushAct (MainActType::act_turnBack, true);

            // xSemaphoreTake (missionMutex, portMAX_DELAY);
            // targetLineCount = mission.getTargetLineCount ();
            // xSemaphoreGive (missionMutex);
        }

        sendMainEvent (mainevent_dispatch_act {});
    }
};

class MainState_bypass : public MainManager
{
    void
    entry () override
    {
        timerInit (onInterval, pdMS_TO_TICKS (CONF_MAINFSM_INTERVAL_MS), true);
        timerStart ();
    }

    void
    exit () override
    {
        timerDelete ();
    }

    void
    react (mainevent_interval const &) override
    {
        if (makerLine->readPosition () != STOP_POSITION)
        {
            rhsMotor->stop ();
            lhsMotor->stop ();

            transit<MainState_move> ();
        }
        else
        {
            rhsMotor->setSpeed (CONF_MAINFSM_LOW_SPEED);
            rhsMotor->forward ();

            lhsMotor->setSpeed (CONF_MAINFSM_LOW_SPEED);
            lhsMotor->forward ();
        }
    }
};

class MainState_turnLeft : public MainManager
{
  public:
    void
    entry () override
    {
        step = 0;

        timerInit (onInterval, pdMS_TO_TICKS (CONF_MAINFSM_INTERVAL_MS), true);
        timerStart ();
    }

    void
    exit ()
    {
        timerDelete ();
    }

    void
    react (mainevent_interval const &) override
    {
        int pos = makerLine->readPosition (RIGHT_POSITION);

        if (step == 0 && pos >= LEFT_POSITION)
        {
            step = 1;
        }
        else if (step == 0)
        {
            rhsMotor->reward ();
            rhsMotor->setSpeed (CONF_MAINFSM_HIGH_SPEED);
            lhsMotor->forward ();
            lhsMotor->setSpeed (CONF_MAINFSM_HIGH_SPEED);
        }
        else if (pos >= -1 && pos <= 1)
        {
            timerStop ();

            rhsMotor->stop ();
            lhsMotor->stop ();

            transit<MainState_move> ();
        }
        else
        {
            rhsMotor->stop ();
            lhsMotor->forward ();
            lhsMotor->setSpeed (CONF_MAINFSM_HIGH_SPEED);
        }
    }

  private:
    int step;
};

class MainState_turnRight : public MainManager
{
  public:
    void
    entry () override
    {
        step = 0;

        timerInit (onInterval, pdMS_TO_TICKS (CONF_MAINFSM_INTERVAL_MS), true);
        timerStart ();
    }

    void
    exit ()
    {
        timerDelete ();
    }

    void
    react (mainevent_interval const &) override
    {
        int pos = makerLine->readPosition (LEFT_POSITION);

        if (step == 0 && pos <= RIGHT_POSITION)
        {
            step = 1;
        }
        else if (step == 0)
        {
            lhsMotor->reward ();
            lhsMotor->setSpeed (CONF_MAINFSM_HIGH_SPEED);
            rhsMotor->forward ();
            rhsMotor->setSpeed (CONF_MAINFSM_HIGH_SPEED);
        }
        else if (pos >= -1 && pos <= 1)
        {
            timerStop ();

            rhsMotor->stop ();
            lhsMotor->stop ();

            transit<MainState_move> ();
        }
        else
        {
            lhsMotor->stop ();
            rhsMotor->forward ();
            rhsMotor->setSpeed (CONF_MAINFSM_HIGH_SPEED);
        }
    }

  private:
    int step;
};

class MainState_io : public MainManager
{
  public:
    void
    entry () override
    {
        timerInit (onInterval, 3000, false);
        timerStart ();
    }

    void
    exit () override
    {
        itemPicked = !itemPicked;
        timerDelete ();
    }

    void
    react (mainevent_interval const &) override
    {
        transit<MainState_move> ();
    }
};

class MainState_turnBack : public MainManager
{
  public:
    void
    entry () override
    {
        waitCounter = 0;

        timerInit (onInterval, CONF_MAINFSM_INTERVAL_MS, true);
        timerStart ();
    }

    void
    exit () override
    {
        timerDelete ();
    }

    void
    react (mainevent_interval const &) override
    {
        if (waitCounter < 100)
        {
            waitCounter++;
            rotation ();
        }
        else
        {
            int pos = makerLine->readPosition (STOP_POSITION);

            if (pos >= -1 && pos <= 1)
            {
                timerStop ();

                rhsMotor->stop ();
                lhsMotor->stop ();

                transit<MainState_move> ();
            }
            else
            {
                rotation ();
            }
        }
    }

  private:
    int waitCounter;

    void
    rotation ()
    {
        this->rhsMotor->reward ();
        this->rhsMotor->setSpeed (CONF_MAINFSM_HIGH_SPEED);
        this->lhsMotor->forward ();
        this->lhsMotor->setSpeed (CONF_MAINFSM_HIGH_SPEED);
    }
};

class MainState_done : public MainManager
{
    void
    entry () override
    {
        homing = false;
        itemPicked = false;

        xSemaphoreTake (storageMapMutex, portMAX_DELAY);
        currentLineCount = storageMap.getHomeLinecount ();
        xSemaphoreGive (storageMapMutex);

        MissionStatus done = missionstatus_done;
        xQueueSend (missionStatusQueue, &done, portMAX_DELAY);

        putMainStatus (MainStatus::mainstatus_idle);
        transit<MainState_idle> ();
    }
};

// ----------------------------------------------------------------------------
// Base state: default implementations
//

MainManager::MainManager () {}

MainManager::~MainManager () {}

int MainManager::targetLineCount = 0;
int MainManager::currentLineCount = 0;
bool MainManager::itemPicked = false;
bool MainManager::homing = false;
MissionData MainManager::mission;
MissionPhase MainManager::currentPhase;
cppQueue MainManager::actQueue (sizeof (ActData), 20);

L298N *MainManager::lhsMotor = new L298N (M1_EN, M1_PA, M1_PB);
L298N *MainManager::rhsMotor = new L298N (M2_EN, M2_PA, M2_PB);
MakerLine *MainManager::makerLine = new MakerLine (
    SENSOR_1,
    SENSOR_2,
    SENSOR_3,
    SENSOR_4,
    SENSOR_5);

void
MainManager::pushAct (ActType act, bool forceExcu)
{
    ActData _act;
    _act.type = act;
    _act.forceExcu = forceExcu;

    actQueue.push (&_act);
}

ActData
MainManager::popAct ()
{
    ActData act;
    actQueue.pop (&act);
    return act;
}

void
MainManager::timerInit (TimerCallbackFunction_t cb, TickType_t xTimerPeriodInTicks, UBaseType_t autoReaload)
{
    _timer = xTimerCreate (
        "MainManager timeout timer", // timer name
        xTimerPeriodInTicks,         // period in ticks
        autoReaload,                 // auto reload
        NULL,                        // timer id
        cb);                         // callback
}

void
MainManager::timerStart ()
{
    if (_timer == nullptr)
    {
        return;
    }

    if (!xTimerIsTimerActive (_timer))
    {
        xTimerStart (_timer, 0);
    }
}

void
MainManager::timerStop ()
{
    if (_timer == nullptr)
    {
        return;
    }

    if (xTimerIsTimerActive (_timer))
    {
        xTimerStop (_timer, 0);
    }
}

void
MainManager::timerDelete ()
{
    if (_timer == nullptr)
    {
        return;
    }
    timerStop ();
    xTimerDelete (_timer, 1000);
}

FSM_INITIAL_STATE (MainManager, MainState_initialize)

// ----------------------------------------------------------------------------
// Helper definition
//
void
onInterval (TimerHandle_t t)
{
    sendMainEvent (mainevent_interval {});
}

void
mainStatusLoop ()
{
    MainStatus currentStatus;

    /*  wait for data to become available in the queue  */
    if (xQueueReceive (mainstatusQueue, &currentStatus, portMAX_DELAY) == pdFALSE)
    {
        return;
    }

    /* lock mainstatusMutex if it's availabel, if not then wait */
    if (xSemaphoreTake (mainstatusMutex, portMAX_DELAY) == pdFALSE)
    {
        return;
    }

    mainstatus = currentStatus;

    /* unlock mainstatusMutex */
    xSemaphoreGive (mainstatusMutex);
}

inline void
putMainStatus (MainStatus status)
{
    xQueueSend (mainstatusQueue, &status, 0);
}
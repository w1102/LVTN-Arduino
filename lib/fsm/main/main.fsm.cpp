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
        pinMode (MAGNET, OUTPUT);
        digitalWrite (MAGNET, CONF_MAINFSM_MAGNET_OFF);

        servo->setPeriodHertz (50);
        servo->attach (SERVO, CONF_MAINFSM_SERVO_US_LOW, CONF_MAINFSM_SERVO_US_HIGH);
        servo->write (CONF_MAINFSM_SERVO_PUSH_IN_POS);

        mainstatusQueue = xQueueCreate (CONF_MAINFSM_QUEUE_LENGTH, sizeof (MainStatus));

        dpQueue.dispatch (
            [=] ()
            {
                xQueueReceive (global::currentLineCountQueue, &currentLineCount, portMAX_DELAY);
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
        timerInit (onInterval, constants::global::intervalMs, true);
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
            [=] ()
            {
                leaveHome ();
                goMainBranch ();

                MissionStatus missionRunning = missionstatus_running;
                xQueueSend (global::missionStatusQueue, &missionRunning, 0);

                MainStatus mainRunning = mainstatus_runMission;
                xQueueSend (mainstatusQueue, &mainRunning, 0);
            },
            [=] () -> bool
            {
                return isTaskingIOMission () || isTaskingHomingMission ();
            });
    }

  private:
    void
    leaveHome ()
    {
        if (isHome)
        {
            String *leaveHomeActs = new String ("TB;Bp;");
            parseMissionAction (&actQueue, leaveHomeActs);

            isHome = false;
            isMainBranch = false;
        }
    }

    void
    goMainBranch ()
    {
        if (isMainBranch && currentLineCount > targetLineCount)
        {
            robotDir = robotDir == forward ? reward : forward;
            pushAct (act_turnBack, true);
        }
        else if (!isMainBranch && currentLineCount > targetLineCount)
        {
            robotDir = reward;
            pushAct (act_turnRight, true);
        }
        else if (!isMainBranch)
        {
            robotDir = forward;
            pushAct (act_turnLeft, true);
        }
    }

    bool
    isTaskingHomingMission ()
    {
        if (!isHome)
        {
            // ...
            // target line count for homing

            targetLineCount = 1;
            homing = true;

            return true;
        }
        else
        {
            return false;
        }
    }

    bool
    isTaskingIOMission ()
    {
        if (xQueueReceive (global::missionQueue, &mission, 0) == pdTRUE)
        {
            currentPhase = phase1;
            targetLineCount = mission.phase1.target;

            return true;
        }
        else
        {
            return false;
        }
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
        bool actPeeked = actQueue.peek (&nextAct);

        if (actPeeked && nextAct.forceExcu == true)
        {
            sendMainEvent (mainevent_dispatch_act {});
            return;
        }

        if (obstacleCheck () && (!actPeeked || nextAct.type != act_io))
        {
            obstacleAction ();
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

        case ActType::act_done_decre:
            currentLineCount--;
            transit<MainState_done> ();
            break;
        case ActType::act_done_incre:
            currentLineCount++;
            transit<MainState_done> ();
            break;
        case ActType::act_done_origi:
            transit<MainState_done> ();
            break;
        }
    }

  private:
    double setPos,
        currentPos,
        computedPos;
    PID *pid;

    ActData nextAct;
    ActData prevAct;

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

        rhsMotor->forward ();
        rhsMotor->setSpeed (baseSpeed + this->computedPos);

        lhsMotor->forward ();
        lhsMotor->setSpeed (baseSpeed - this->computedPos);
    }

    void
    dispatchAct ()
    {
        if (homing)
        {
            String *homingAction = new String (robotDir == forward ? "Bp;TL;Do;" : "Bp;TR;Do;");
            parseMissionAction (&actQueue, homingAction);
        }
        else if (currentPhase == phase1)
        {
            parseMissionAction (&actQueue, mission.phase1.action);
            currentPhase = phase2;
            targetLineCount = mission.phase2.target;
        }
        else if (currentPhase == phase2)
        {
            parseMissionAction (&actQueue, mission.phase2.action);
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
        if (obstacleCheck ())
        {
            obstacleAction ();
            return;
        }

        if (makerLine->readPosition (STOP_POSITION) != STOP_POSITION)
        {
            timerStop ();
            rhsMotor->stop ();
            lhsMotor->stop ();

            dpQueue.dispatch (
                [=] ()
                {
                    vTaskDelay (constants::main::bypassDelayMs);
                    transit<MainState_move> ();
                });
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
        if (obstacleCheck ())
        {
            obstacleAction ();
            return;
        }

        int pos = makerLine->readPosition (STOP_POSITION);

        if (step == 0 && pos == LEFT_POSITION)
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
            lhsMotor->setSpeed (CONF_MAINFSM_MID_SPEED);
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
        if (obstacleCheck ())
        {
            obstacleAction ();
            return;
        }

        int pos = makerLine->readPosition (STOP_POSITION);

        if (step == 0 && pos == RIGHT_POSITION)
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
            rhsMotor->setSpeed (CONF_MAINFSM_MID_SPEED);
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
        itemPicked = !itemPicked;
        dpQueue.dispatch (
            [=] ()
            {
                servoPutOut ();
                digitalWrite (MAGNET, itemPicked ? CONF_MAINFSM_MAGNET_ON : CONF_MAINFSM_MAGNET_OFF);
                vTaskDelay (CONF_MAINFSM_SERVO_DELAY_TIME);
                servoPutIn ();
                vTaskDelay (CONF_MAINFSM_SERVO_DELAY_TIME);

                transit<MainState_move> ();
            });
    }

  private:
    void
    servoPutOut ()
    {
        for (int i = CONF_MAINFSM_SERVO_PUSH_IN_POS; i <= CONF_MAINFSM_SERVO_PUSH_OUT_POS; i++)
        {
            servo->write (i);
            vTaskDelay (CONF_MAINFSM_INTERVAL_MS);
        }
    }

    void
    servoPutIn ()
    {
        for (int i = CONF_MAINFSM_SERVO_PUSH_OUT_POS; i >= CONF_MAINFSM_SERVO_PUSH_IN_POS; i--)
        {
            servo->write (i);
            vTaskDelay (CONF_MAINFSM_INTERVAL_MS);
        }
    }
};

class MainState_turnBack : public MainManager
{
  public:
    void
    entry () override
    {
        step = 0;

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
        int pos = makerLine->readPosition (STOP_POSITION);

        if (step == 0 && (pos == LEFT_POSITION || pos == LEFT_POSITION - 1))
        {
            step = 1;
        }
        else if (step == 0)
        {
            rotation (CONF_MAINFSM_HIGH_SPEED);
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
            rotation (CONF_MAINFSM_MID_SPEED);
        }
    }

  private:
    int step = 0;

    void
    rotation (int speed)
    {
        rhsMotor->reward ();
        rhsMotor->setSpeed (speed);
        lhsMotor->forward ();
        lhsMotor->setSpeed (speed);
    }
};

class MainState_done : public MainManager
{
    void
    entry () override
    {
        if (homing)
        {
            homing = false;
            isHome = true;
        }

        rhsMotor->stop ();
        lhsMotor->stop ();

        MissionStatus missionDone = missionstatus_done;
        xQueueSend (global::missionStatusQueue, &missionDone, 0);

        MainStatus mainIdle = mainstatus_idle;
        xQueueSend (mainstatusQueue, &mainIdle, 0);

        isMainBranch = mission.stopInMainBranch;

        trigger ("MainState_done_trigger", [=] ()
                 { transit<MainState_idle> (); });
    }
};

// ----------------------------------------------------------------------------
// Base state: default implementations
//

MainManager::MainManager ()
{
}

MainManager::~MainManager () {}

dispatch_queue MainManager::dpQueue (
    constants::main::dpQueueName,
    constants::main::dpQueuethreadCnt,
    constants::main::dpQueueStackDepth);

int MainManager::targetLineCount = 0;
int MainManager::currentLineCount = 0;
bool MainManager::itemPicked = false;
bool MainManager::homing = false;
bool MainManager::isHome = true;
bool MainManager::isMainBranch = false;
Direction MainManager::robotDir = forward;
MissionData MainManager::mission;
MissionPhase MainManager::currentPhase;
cppQueue MainManager::actQueue (sizeof (ActData), 20);

L298N *MainManager::lhsMotor = new L298N (M1_EN, M1_PA, M1_PB);
L298N *MainManager::rhsMotor = new L298N (M2_EN, M2_PA, M2_PB);
MakerLine *MainManager::makerLine = new MakerLine (SENSOR_1, SENSOR_2, SENSOR_3, SENSOR_4, SENSOR_5);
Servo *MainManager::servo = new Servo ();

bool
MainManager::obstacleCheck ()
{
    if (xSemaphoreTake (global::ultrasonicThresholdDistanceSync, 0) == pdTRUE)
    {
        xSemaphoreGive (global::ultrasonicThresholdDistanceSync);
        return false;
    }
    else
    {
        return true;
    }
}

void
MainManager::obstacleAction ()
{
    rhsMotor->stop ();
    lhsMotor->stop ();
}

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
    if (xSemaphoreTake (global::mainstatusMutex, portMAX_DELAY) == pdFALSE)
    {
        return;
    }

    global::mainstatus = currentStatus;

    /* unlock mainstatusMutex */
    xSemaphoreGive (global::mainstatusMutex);
}

inline void
putMainStatus (MainStatus status)
{
    xQueueSend (mainstatusQueue, &status, 0);
}
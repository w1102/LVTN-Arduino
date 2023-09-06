#include "main.fsm.h"

// ----------------------------------------------------------------------------
// Forward declarations
//

class MainState_initialize;
class MainState_idle;
class MainState_move;
class MainState_bypass;
class MainState_io;
class MainState_turnLeft;
class MainState_turnRight;
class MainState_turnBack;
class MainState_done;

// ----------------------------------------------------------------------------
// State implementations
//

class MainState_initialize : public MainManager
{
  public:
    void
    entry () override
    {
        pinMode (MAGNET, OUTPUT);
        digitalWrite (MAGNET, constants::main::magnetOff);

        servo->setPeriodHertz (constants::main::servoPeriod);
        servo->attach (SERVO, constants::main::servoUsLow, constants::main::servoUsHigh);
        servo->write (constants::main::servoPushIn);

        dpQueue.dispatch (
            [=] ()
            {
                xQueueReceive (global::currentLineCountQueue, &currentLineCount, portMAX_DELAY);
                transit<MainState_idle> (
                    [=] ()
                    {
                        pushStatus (mainstatus_idle);
                    });
            });
    }
};

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
                xQueueSend (global::missionStatusQueue, &missionRunning, portMAX_DELAY);

                pushStatus (mainstatus_runMission);
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
        if (!isHome)
        {
            return;
        }

        String *leaveHomeActs = new String ("TB;Bp;");
        parseMissionAction (&actQueue, leaveHomeActs);

        isHome = false;
        isMainBranch = false;
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

class MainState_move : public MainManager
{
  public:
    void
    entry () override
    {
        pid = new PID (
            &currentPos,
            &computedPos,
            &setPos,
            constants::main::kp,
            constants::main::ki,
            constants::main::kd,
            AUTOMATIC);

        pid->SetSampleTime (constants::global::intervalMs);
        pid->SetMode (AUTOMATIC);
        pid->SetOutputLimits (-1024, 1024);

        timerInit (onInterval, constants::global::intervalMs, true);
        timerStart ();
    }

    void
    exit () override
    {
        delete pid;
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
    double setPos { 0 }, currentPos { 0 }, computedPos { 0 };
    PID *pid;
    ActData nextAct;

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

        int baseSpeed = constants::main::midSpeed;

        if (abs (targetLineCount - currentLineCount) <= 1 || !actQueue.isEmpty ())
        {
            baseSpeed = constants::main::lowSpeed;
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
            rhsMotor->setSpeed (constants::main::lowSpeed);
            rhsMotor->forward ();

            lhsMotor->setSpeed (constants::main::lowSpeed);
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
        timerInit (onInterval, constants::global::intervalMs, true);
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
            rhsMotor->setSpeed (constants::main::higSpeed);
            lhsMotor->forward ();
            lhsMotor->setSpeed (constants::main::higSpeed);
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
            lhsMotor->setSpeed (constants::main::midSpeed);
        }
    }

  private:
    int step { 0 };
};

class MainState_turnRight : public MainManager
{
  public:
    void
    entry () override
    {
        timerInit (onInterval, constants::global::intervalMs, true);
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
            lhsMotor->setSpeed (constants::main::higSpeed);
            rhsMotor->forward ();
            rhsMotor->setSpeed (constants::main::higSpeed);
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
            rhsMotor->setSpeed (constants::main::midSpeed);
        }
    }

  private:
    int step { 0 };
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
                digitalWrite (MAGNET, itemPicked ? constants::main::magnetOn : constants::main::magnetOff);
                vTaskDelay (constants::main::ioDelayMs);

                servoPutIn ();
                vTaskDelay (constants::main::ioDelayMs);

                transit<MainState_move> ();
            });
    }

  private:
    void
    servoPutOut ()
    {
        for (int i = constants::main::servoPushIn; i <= constants::main::servoPushOut; i++)
        {
            servo->write (i);
            vTaskDelay (constants::main::servoDelayMs);
        }
    }

    void
    servoPutIn ()
    {
        for (int i = constants::main::servoPushOut; i >= constants::main::servoPushIn; i--)
        {
            servo->write (i);
            vTaskDelay (constants::main::servoDelayMs);
        }
    }
};

class MainState_turnBack : public MainManager
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
        int pos = makerLine->readPosition (STOP_POSITION);

        if (step == 0 && (pos == LEFT_POSITION || pos == LEFT_POSITION - 1))
        {
            step = 1;
        }
        else if (step == 0)
        {
            rotation (constants::main::higSpeed);
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
            rotation (constants::main::midSpeed);
        }
    }

  private:
    int step { 0 };

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

        dpQueue.dispatch (
            [=] ()
            {
                transit<MainState_idle> (
                    [=] ()
                    {
                        MissionStatus missionDone = missionstatus_done;
                        xQueueSend (global::missionStatusQueue, &missionDone, portMAX_DELAY);

                        pushStatus (mainstatus_idle);

                        isMainBranch = mission.stopInMainBranch;
                    });
            });
    }
};

// ----------------------------------------------------------------------------
// Initial state definition
//

FSM_INITIAL_STATE (MainManager, MainState_initialize)

// ----------------------------------------------------------------------------
// Base class implementations
//

dispatch_queue MainManager::dpQueue (
    constants::main::dpQueueName,
    constants::main::dpQueuethreadCnt,
    constants::main::dpQueueStackDepth);

int MainManager::targetLineCount { 0 }, MainManager::currentLineCount { 0 };
bool MainManager::itemPicked { false }, MainManager::homing { false }, MainManager::isHome { true }, MainManager::isMainBranch { false };

Direction MainManager::robotDir { forward };
MissionData MainManager::mission;
MissionPhase MainManager::currentPhase;
cppQueue MainManager::actQueue (sizeof (ActData), constants::global::queueCnt);

L298N *MainManager::lhsMotor { new L298N (M1_EN, M1_PA, M1_PB) };
L298N *MainManager::rhsMotor { new L298N (M2_EN, M2_PA, M2_PB) };
MakerLine *MainManager::makerLine { new MakerLine (SENSOR_1, SENSOR_2, SENSOR_3, SENSOR_4, SENSOR_5) };
Servo *MainManager::servo { new Servo () };

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

void
MainManager::pushStatus (MainStatus status)
{
    dpQueue.dispatch (
        [=] ()
        {
            xSemaphoreTake (global::mainstatusMutex, portMAX_DELAY);
            global::mainstatus = status;
            xSemaphoreGive (global::mainstatusMutex);
        });
}

void
MainManager::onInterval (TimerHandle_t t)
{
    sendMainEvent (mainevent_interval {});
}

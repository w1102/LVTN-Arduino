#include "main.fsm.h"

// ----------------------------------------------------------------------------
// Forward declarations
//

class MainState_initialize;
class MainState_idle;
class MainState_dispatchAction;
class MainState_move;
class MainState_obstacle;
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

        servo.setPeriodHertz (constants::main::servoPeriod);
        servo.attach (SERVO, constants::main::servoUsLow, constants::main::servoUsHigh);
        servo.write (constants::main::servoPushIn);

        dpQueue.dispatch (
            [=] ()
            {
                xQueueReceive (global::agvInfoQueue, &agvInfo, portMAX_DELAY);
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
        goHomeSync = xSemaphoreCreateBinary ();

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

        if (xQueueReceive (global::missionQueue, &mission, 0) == pdTRUE)
        {
            timerStop ();
            leaveHome ();
            goMainBranch ();
        }
        else if (!agvInfo.isHome)
        {
            timerStop ();
            goMainBranch ();

            dpQueue.dispatch (
                [=]
                {
                    xSemaphoreTake (goHomeSync, portMAX_DELAY);

                    mission.setHomingMission (
                        1, agvInfo.direction == forward
                               ? agvInfo.forwardDirHomingActs
                               : agvInfo.rewardDirHomingActs);
                });
        }
        else
        {
            return;
        }

        MissionStatus missionRunning = missionstatus_running;
        xQueueSend (global::missionStatusQueue, &missionRunning, portMAX_DELAY);
        pushStatus (mainstatus_runMission);

        if (agvInfo.isMainBranch)
        {
            agvInfo.direction = agvInfo.direction == forward ? reward : forward;
            xSemaphoreGive (goHomeSync);
            transit<MainState_turnBack> ();
        }
        else
        {
            transit<MainState_dispatchAction> ();
        }
    }

  private:
    xSemaphoreHandle goHomeSync;

    void
    leaveHome ()
    {
        if (agvInfo.isHome == false)
        {
            return;
        }

        Mission::parseMissionAct (action::queue, agvInfo.leaveHomeActs);

        agvInfo.isHome = false;
        agvInfo.isMainBranch = false;
    }

    void
    goMainBranch ()
    {
        if (!agvInfo.isMainBranch && agvInfo.currentLineCount > mission.getPhaseTarget ())
        {
            agvInfo.direction = reward;
            action::push (act_turnRight, true);
        }
        else if (!agvInfo.isMainBranch)
        {
            agvInfo.direction = forward;
            action::push (act_turnLeft, true);
        }
    }
};

class MainState_dispatchAction : public MainManager
{
  public:
    void
    entry () override
    {
        if (!isHaveForceAction ())
        {
            transit<MainState_move> ();
        }

        ActData act = action::pop ();

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
        case ActType::act_done:
            transit<MainState_done> ();
            break;
        }
    }
};

class MainState_move : public MainManager
{
  public:
    void
    entry () override
    {
        pid.SetSampleTime (constants::global::intervalMs);
        pid.SetMode (AUTOMATIC);
        pid.SetOutputLimits (-1023, 1023);

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
        transit<MainState_dispatchAction> ([] {}, isHaveForceAction);
        transit<MainState_obstacle> (
            [] {}, [=]
            { return getUltrasonicDistance () <= constants::ultrasonic::stopDistance; });

        currentPos = makerLine.readPosition ();

        if (currentPos == STOP_POSITION)
        {
            stopAgv ();
            stopHandle ();
        }
        else
        {
            sendMainEvent (mainevent_move {});
        }
    }

    void
    react (mainevent_move const &) override
    {
        pid.Compute ();

        int baseSpeed = constants::main::midSpeed;

        if (
            abs (mission.getPhaseTarget () - agvInfo.currentLineCount) <= 1 
            || !action::queue.isEmpty ()
            || ultrasonicDistance <= constants::ultrasonic::warnDistance)
        {
            baseSpeed = constants::main::lowSpeed;
        }

        rhsMotor.forward ();
        rhsMotor.setSpeed (baseSpeed + this->computedPos);

        lhsMotor.forward ();
        lhsMotor.setSpeed (baseSpeed - this->computedPos);
    }

  private:
    double setPos { 0 }, currentPos { 0 }, computedPos { 0 };
    float ultrasonicDistance;
    PID pid { PID (
        &currentPos, &computedPos, &setPos,
        constants::main::kp, constants::main::ki, constants::main::kd, AUTOMATIC) };

    void
    stopHandle ()
    {
        if (!action::queue.isEmpty ())
        {
            transit<MainState_dispatchAction> ();
        }
        else
        {
            agvInfo.currentLineCount = mission.getPhaseTarget () < agvInfo.currentLineCount
                                           ? agvInfo.currentLineCount - 1
                                           : agvInfo.currentLineCount + 1;

            if (agvInfo.currentLineCount == mission.getPhaseTarget ())
            {
                String *acts = mission.getPhaseActs ();
                Mission::parseMissionAct (action::queue, acts);

                xSemaphoreTake (global::mapMutex, portMAX_DELAY);
                agvInfo.isMainBranch = global::map.lineCountIsInMainBranch (mission.getPhaseTarget ());
                xSemaphoreGive (global::mapMutex);

                if (!agvInfo.isMainBranch && mission.getPhase () == phase1)
                {
                    String lastAct = acts->substring (
                        acts->length () - constants::mission::actCodeTable::codeLength - 1,
                        acts->length () + constants::mission::actCodeTable::codeLength);

                    switch (lastAct[0] + lastAct[1])
                    {
                    case constants::mission::actCodeTable::turnLeft:
                    case constants::mission::actCodeTable::forceTurnLeft:
                        agvInfo.direction = forward;
                        break;
                    case constants::mission::actCodeTable::turnRight:
                    case constants::mission::actCodeTable::forceTurnRight:
                        agvInfo.direction = reward;
                        break;
                    }
                }

                dpQueue.dispatch (
                    [=] ()
                    {
                        xQueueSend (global::agvInfoQueue, &agvInfo, portMAX_DELAY);
                    });

                if (!mission.isHomingMission ())
                {
                    delete acts;
                }
                mission.turnNextPhase ();
                transit<MainState_dispatchAction> ();
            }
            else
            {
                dpQueue.dispatch (
                    [=] ()
                    {
                        xQueueSend (global::agvInfoQueue, &agvInfo, portMAX_DELAY);
                    });
                transit<MainState_bypass> ();
            }
        }
    }

    float
    getUltrasonicDistance ()
    {
        xSemaphoreTake (global::ultrasonicDistanceMutex, portMAX_DELAY);
        ultrasonicDistance = global::ultrasonicDistance;
        xSemaphoreGive (global::ultrasonicDistanceMutex);

        return ultrasonicDistance;
    }
};

class MainState_obstacle : public MainManager
{
  public:
    void
    entry () override
    {
        stopAgv();

        dpQueue.dispatch (
            [=] ()
            {
                xSemaphoreTake (global::ultrasonicDistanceStopsync, portMAX_DELAY);
                transit<MainState_move> (
                    [=] ()
                    {
                        xSemaphoreGive (global::ultrasonicDistanceStopsync);
                    });
            });
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
        if (makerLine.readPosition (STOP_POSITION) != STOP_POSITION)
        {
            timerStop ();

            dpQueue.dispatch (
                [=] ()
                {
                    vTaskDelay (constants::main::stateDelayMs);
                    rhsMotor.stop ();
                    lhsMotor.stop ();
                    vTaskDelay (constants::main::stateDelayMs);

                    if (isHaveForceAction ())
                    {
                        transit<MainState_dispatchAction> ();
                    }
                    else
                    {
                        transit<MainState_move> ();
                    }
                });
        }
        else
        {
            rhsMotor.setSpeed (constants::main::lowSpeed);
            rhsMotor.forward ();

            lhsMotor.setSpeed (constants::main::lowSpeed);
            lhsMotor.forward ();
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
        int pos = makerLine.readPosition (STOP_POSITION);

        if (step == 0 && pos == LEFT_POSITION)
        {
            step = 1;
        }
        else if (step == 0)
        {
            rhsMotor.reward ();
            rhsMotor.setSpeed (constants::main::higSpeed);
            lhsMotor.forward ();
            lhsMotor.setSpeed (constants::main::higSpeed);
        }
        else if (pos >= -1 && pos <= 1)
        {
            timerStop ();

            rhsMotor.stop ();
            lhsMotor.stop ();

            if (isHaveForceAction ())
            {
                transit<MainState_dispatchAction> ();
            }
            else
            {
                transit<MainState_move> ();
            }
        }
        else
        {
            rhsMotor.stop ();
            lhsMotor.forward ();
            lhsMotor.setSpeed (constants::main::midSpeed);
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
        step = 0;
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
        int pos = makerLine.readPosition (STOP_POSITION);

        if (step == 0 && pos == RIGHT_POSITION)
        {
            step = 1;
        }
        else if (step == 0)
        {
            lhsMotor.reward ();
            lhsMotor.setSpeed (constants::main::higSpeed);
            rhsMotor.forward ();
            rhsMotor.setSpeed (constants::main::higSpeed);
        }
        else if (pos >= -1 && pos <= 1)
        {
            timerStop ();

            rhsMotor.stop ();
            lhsMotor.stop ();

            if (isHaveForceAction ())
            {
                transit<MainState_dispatchAction> ();
            }
            else
            {
                transit<MainState_move> ();
            }
        }
        else
        {
            lhsMotor.stop ();
            rhsMotor.forward ();
            rhsMotor.setSpeed (constants::main::midSpeed);
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
        agvInfo.isItemPicked = !agvInfo.isItemPicked;
        dpQueue.dispatch (
            [=] ()
            {
                servoPutOut ();
                digitalWrite (MAGNET, agvInfo.isItemPicked ? constants::main::magnetOn : constants::main::magnetOff);
                vTaskDelay (constants::main::ioDelayMs);
                servoPutIn ();
                vTaskDelay (constants::main::ioDelayMs);

                if (isHaveForceAction ())
                {
                    transit<MainState_dispatchAction> ();
                }
                else
                {
                    transit<MainState_move> ();
                }
            });
    }

  private:
    void
    servoPutOut ()
    {
        for (int i = constants::main::servoPushIn; i <= constants::main::servoPushOut; i++)
        {
            servo.write (i);
            vTaskDelay (constants::main::servoDelayMs);
        }
    }

    void
    servoPutIn ()
    {
        for (int i = constants::main::servoPushOut; i >= constants::main::servoPushIn; i--)
        {
            servo.write (i);
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
        step = 0;
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
        int pos = makerLine.readPosition (12);

        if (step == 0 && pos == LEFT_POSITION)
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

            rhsMotor.stop ();
            lhsMotor.stop ();

            if (isHaveForceAction ())
            {
                transit<MainState_dispatchAction> ();
            }
            else
            {
                transit<MainState_move> ();
            }
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
        rhsMotor.reward ();
        rhsMotor.setSpeed (speed);
        lhsMotor.forward ();
        lhsMotor.setSpeed (speed);
    }
};

class MainState_done : public MainManager
{
    void
    entry () override
    {
        stopAgv ();

        // dpQueue.dispatch (
        //     [=] ()
        //     {
        transit<MainState_idle> (
            [=] ()
            {
                if (mission.isHomingMission ())
                {
                    agvInfo.isHome = true;
                }

                xQueueSend (global::agvInfoQueue, &agvInfo, portMAX_DELAY);

                MissionStatus missionDone = missionstatus_done;
                xQueueSend (global::missionStatusQueue, &missionDone, portMAX_DELAY);

                pushStatus (mainstatus_idle);
            });
        //   });
    }
};

// ----------------------------------------------------------------------------
// Initial state definition
//

FSM_INITIAL_STATE (MainManager, MainState_initialize)
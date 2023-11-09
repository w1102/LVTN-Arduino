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

class MainState_initialize: public MainManager
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
            [ = ] ()
            {
                xQueueReceive (global::agvInfoQueue, &m_info, portMAX_DELAY);
                transit< MainState_idle > (
                    [ = ] ()
                    {
                        putMainStatus (mainstatus_idle);
                    });
            });
    }
};

class MainState_idle: public MainManager
{
  public:
    void entry () override
    {
        timerInit (onInterval, constants::global::intervalMs, true);
        timerStart ();
    }

    void exit () override
    {
        timerDelete ();
    }

    void react (mainevent_interval const&) override
    {
        if (xQueueReceive (global::missionQueue, &mission, 0) == pdTRUE)
        {
            timerStop ();
            leaveHome ();
            goMainBranch ();
        }
        else if (!m_info.isHome)
        {
            timerStop ();

            xSemaphoreTake (global::mapMutex, portMAX_DELAY);
            const int homeTarget = global::map.getHomeLineCnt ();
            xSemaphoreGive (global::mapMutex);

            mission.setHomingMission (
                homeTarget,
                m_info.currentLineCount > homeTarget
                    ? new String ("BP;TR;Do;")
                    : new String ("BP;TL;Do;"));

            goMainBranch ();
        }
        else if (*m_info.missionId != "")
        {
            *m_info.missionId = "";
            xQueueSend (global::agvInfoQueue, &m_info, portMAX_DELAY);
            return;
        }
        else
            return;

        m_info.missionId = mission.id ();

        MissionStatus missionRunning = missionstatus_running;
        xQueueSend (global::missionStatusQueue, &missionRunning, portMAX_DELAY);
        putMainStatus (mainstatus_runMission);

        if (m_info.isMainBranch)
            transit< MainState_turnBack > ();
        else
            transit< MainState_dispatchAction > ();
    }

  private:
    void leaveHome ()
    {
        if (!m_info.isHome)
            return;

        Mission::parseMissionAct (action::queue, m_info.leaveHomeActs);

        m_info.isHome       = false;
        m_info.isMainBranch = false;
    }

    void goMainBranch ()
    {
        if (!m_info.isMainBranch && m_info.currentLineCount > mission.getPhaseTarget ())
            action::push (act_turnRight, true);
        else if (!m_info.isMainBranch)
            action::push (act_turnLeft, true);
    }
};

class MainState_dispatchAction: public MainManager
{
  public:
    void
        entry () override
    {
        if (!isHaveForceAction ())
        {
            transit< MainState_move > ();
        }

        ActData act = action::pop ();

        switch (act.type)
        {
        case ActType::act_turnLeft:
            transit< MainState_turnLeft > ();
            break;
        case ActType::act_turnRight:
            transit< MainState_turnRight > ();
            break;
        case ActType::act_turnBack:
            transit< MainState_turnBack > ();
            break;
        case ActType::act_bypass:
            transit< MainState_bypass > ();
            break;
        case ActType::act_io:
            transit< MainState_io > ();
            break;
        case ActType::act_done:
            transit< MainState_done > ();
            break;
        }
    }
};

class MainState_move: public MainManager
{
  public:
    void entry () override
    {
        pid.SetSampleTime (constants::global::intervalMs);
        pid.SetMode (AUTOMATIC);
        pid.SetOutputLimits (-1023, 1023);

        timerInit (onInterval, constants::global::intervalMs, true);
        timerStart ();
    }

    void exit () override
    {
        timerDelete ();
    }

    void react (mainevent_interval const&) override
    {
        if (isHaveForceAction ())
            transit< MainState_dispatchAction > ();

        if (getUltrasonicDistance () <= constants::ultrasonic::stopDistance)
            transit< MainState_obstacle > ();

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

    void react (mainevent_move const&) override
    {
        pid.Compute ();

        int baseSpeed = constants::main::midSpeed;

        if (
            abs (mission.getPhaseTarget () - m_info.currentLineCount) <= 1 || !action::queue.isEmpty () || ultrasonicDistance <= constants::ultrasonic::warnDistance)
        {
            baseSpeed = constants::main::lowSpeed;
        }

        lhsMotor.forward ();
        lhsMotor.setSpeed (baseSpeed + this->computedPos);

        rhsMotor.forward ();
        rhsMotor.setSpeed (baseSpeed - this->computedPos);
    }

  private:
    double setPos { 0 }, currentPos { 0 }, computedPos { 0 };
    float  ultrasonicDistance;
    PID    pid { PID (
        &currentPos,
        &computedPos,
        &setPos,
        constants::main::kp,
        constants::main::ki,
        constants::main::kd,
        AUTOMATIC) };

    void stopHandle ()
    {
        if (!action::queue.isEmpty ())
            transit< MainState_dispatchAction > ();
        else
        {
            m_info.currentLineCount = mission.getPhaseTarget () < m_info.currentLineCount
                                          ? m_info.currentLineCount - 1
                                          : m_info.currentLineCount + 1;

            if (m_info.currentLineCount == mission.getPhaseTarget ())
            {
                String* acts = mission.getPhaseActs ();
                Mission::parseMissionAct (action::queue, acts);

                xSemaphoreTake (global::mapMutex, portMAX_DELAY);
                m_info.isMainBranch = global::map.isInMainBranch (mission.getPhaseTarget ());
                xSemaphoreGive (global::mapMutex);

                xQueueSend (global::agvInfoQueue, &m_info, portMAX_DELAY);

                delete acts;

                mission.turnNextPhase ();
                transit< MainState_dispatchAction > ();
            }
            else
            {
                dpQueue.dispatch (
                    [ = ] ()
                    {
                        xQueueSend (global::agvInfoQueue, &m_info, portMAX_DELAY);
                    });
                transit< MainState_bypass > ();
            }
        }
    }

    float getUltrasonicDistance ()
    {
        xSemaphoreTake (global::ultrasonicDistanceMutex, portMAX_DELAY);
        ultrasonicDistance = global::ultrasonicDistance;
        xSemaphoreGive (global::ultrasonicDistanceMutex);

        return ultrasonicDistance;
    }
};

class MainState_obstacle: public MainManager
{
  public:
    void
        entry () override
    {
        stopAgv ();

        dpQueue.dispatch (
            [ = ] ()
            {
                xSemaphoreTake (global::ultrasonicDistanceStopsync, portMAX_DELAY);
                transit< MainState_move > (
                    [ = ] ()
                    {
                        xSemaphoreGive (global::ultrasonicDistanceStopsync);
                    });
            });
    }
};

class MainState_bypass: public MainManager
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
        react (mainevent_interval const&) override
    {
        if (makerLine.readPosition (STOP_POSITION) != STOP_POSITION)
        {
            timerStop ();

            dpQueue.dispatch (
                [ = ] ()
                {
                    vTaskDelay (constants::main::stateDelayMs);
                    rhsMotor.stop ();
                    lhsMotor.stop ();
                    vTaskDelay (constants::main::stateDelayMs);

                    if (isHaveForceAction ())
                    {
                        transit< MainState_dispatchAction > ();
                    }
                    else
                    {
                        transit< MainState_move > ();
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

class MainState_turnLeft: public MainManager
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
        react (mainevent_interval const&) override
    {
        int pos = makerLine.readPosition (STOP_POSITION);

        if (step == 0 && pos == LEFT_POSITION)
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
                transit< MainState_dispatchAction > ();
            }
            else
            {
                transit< MainState_move > ();
            }
        }
        else
        {
            lhsMotor.stop ();
            rhsMotor.forward ();
            rhsMotor.setSpeed (constants::main::lowSpeed);
        }
    }

  private:
    int step { 0 };
};

class MainState_turnRight: public MainManager
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
        react (mainevent_interval const&) override
    {
        int pos = makerLine.readPosition (STOP_POSITION);

        if (step == 0 && pos == RIGHT_POSITION)
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
                transit< MainState_dispatchAction > ();
            }
            else
            {
                transit< MainState_move > ();
            }
        }
        else
        {
            rhsMotor.stop ();
            lhsMotor.forward ();
            lhsMotor.setSpeed (constants::main::lowSpeed);
        }
    }

  private:
    int step { 0 };
};

class MainState_io: public MainManager
{
  public:
    void entry () override
    {
        m_info.isItemPicked = !m_info.isItemPicked;
        dpQueue.dispatch (
            [ = ] ()
            {
                rhsMotor.forward ();
                rhsMotor.setSpeed (constants::main::lowSpeed);
                lhsMotor.forward ();
                lhsMotor.setSpeed (constants::main::lowSpeed);

                while (getUltrasonicDistance () > constants::ultrasonic::ioDistance)
                    vTaskDelay (pdMS_TO_TICKS (constants::global::intervalMs));

                rhsMotor.stop ();
                lhsMotor.stop ();

                vTaskDelay (constants::main::ioDelayMs);
                servoPutOut ();
                digitalWrite (MAGNET, m_info.isItemPicked ? constants::main::magnetOn : constants::main::magnetOff);
                vTaskDelay (constants::main::ioDelayMs);
                servoPutIn ();
                vTaskDelay (constants::main::ioDelayMs);

                if (isHaveForceAction ())
                {
                    transit< MainState_dispatchAction > ();
                }
                else
                {
                    transit< MainState_move > ();
                }
            });
    }

  private:
    void servoPutOut ()
    {
        for (int i = constants::main::servoPushIn; i <= constants::main::servoPushOut; i++)
        {
            servo.write (i);
            vTaskDelay (constants::main::servoDelayMs);
        }
    }

    void servoPutIn ()
    {
        for (int i = constants::main::servoPushOut; i >= constants::main::servoPushIn; i--)
        {
            servo.write (i);
            vTaskDelay (constants::main::servoDelayMs);
        }
    }

    float getUltrasonicDistance ()
    {
        xSemaphoreTake (global::ultrasonicDistanceMutex, portMAX_DELAY);
        float ultrasonicDistance = global::ultrasonicDistance;
        xSemaphoreGive (global::ultrasonicDistanceMutex);

        return ultrasonicDistance;
    }
};

class MainState_turnBack: public MainManager
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
        react (mainevent_interval const&) override
    {
        int pos = makerLine.readPosition (12);

        if (step == 0 && pos == LEFT_POSITION)
        {
            step = 1;
        }
        else if (step == 0)
        {
            rotation (m_info.isItemPicked 
            ? constants::main::veryLowSpeed 
            : constants::main::higSpeed);
        }
        else if (pos >= -1 && pos <= 1)
        {
            timerStop ();

            rhsMotor.stop ();
            lhsMotor.stop ();

            if (isHaveForceAction ())
            {
                transit< MainState_dispatchAction > ();
            }
            else
            {
                transit< MainState_move > ();
            }
        }
        else
        {
            rotation (constants::main::lowSpeed);
        }
    }

  private:
    int step { 0 };

    void
        rotation (int speed)
    {
        lhsMotor.reward ();
        lhsMotor.setSpeed (speed);
        rhsMotor.forward ();
        rhsMotor.setSpeed (speed);
    }
};

class MainState_done: public MainManager
{
    void
        entry () override
    {
        stopAgv ();

        // dpQueue.dispatch (
        //     [=] ()
        //     {
        transit< MainState_idle > (
            [ = ] ()
            {
                if (mission.isHomingMission ())
                {
                    m_info.isHome = true;
                }

                xQueueSend (global::agvInfoQueue, &m_info, portMAX_DELAY);

                MissionStatus missionDone = missionstatus_done;
                xQueueSend (global::missionStatusQueue, &missionDone, portMAX_DELAY);

                putMainStatus (mainstatus_idle);
            });
        //   });
    }
};

// ----------------------------------------------------------------------------
// Initial state definition
//

FSM_INITIAL_STATE (MainManager, MainState_initialize)

#ifndef MAIN_FSM_H_
#define MAIN_FSM_H_

/*
Implementing a state machine to manage main actions:
    * line follower
    * tasking mission
    * ...
*/

#include "PID_v1.h"
#include "config.h"
#include "global.extern.h"
#include "gpio.h"
#include "l298n.h"
#include "makerline.h"
#include "network.fsm.h"
#include "tinyfsm.hpp"
#include "trigger.h"
#include "types.h"
#include <Arduino.h>
#include <cppQueue.h>
#include <math.h>
#include "ESP32Servo.h"

// ----------------------------------------------------------------------------
// Event declarations
// clang-format off
//

struct mainevent : tinyfsm::Event {};

struct mainevent_interval: mainevent {};

struct mainevent_dispatch_act: mainevent {};

// ----------------------------------------------------------------------------
// MainManager (FSM base class) declaration
// clang-format on
//

class MainManager : public tinyfsm::Fsm<MainManager>
{
  public:
    MainManager ();
    ~MainManager ();

    /* default reaction for unhandled events */
    void react (tinyfsm::Event const &) {};

    /* react events in some states */
    virtual void react (mainevent_interval const &) {};
    virtual void react (mainevent_dispatch_act const &) {};

    /* entry actions in some states */
    virtual void entry (void) {};

    /* exit actions in some states */
    virtual void exit (void) {};

  private:
    TimerHandle_t _timer;

  protected:
    static L298N *lhsMotor;
    static L298N *rhsMotor;
    static MakerLine *makerLine;
    static Servo *servo;
    static int targetLineCount;
    static int currentLineCount;
    static bool itemPicked;

    static bool homing;
    static bool isHome;

    static bool isMainBranch;
    static Direction robotDir;

    static cppQueue actQueue;
    static MissionData mission;
    static MissionPhase currentPhase;

    bool obstacleCheck();
    void obstacleAction();

    void pushAct (ActType act, bool forceExcu = false);
    ActData popAct ();

    void timerInit (TimerCallbackFunction_t, TickType_t, UBaseType_t autoReaload = false);
    void timerStart ();
    void timerStop ();
    void timerDelete ();
};

using MainManagerFSM = tinyfsm::Fsm<MainManager>;

// ----------------------------------------------------------------------------
// helper declaration
//

/* send a main event into MainManager state machine */
template <typename E>
void
sendMainEvent (E const &event)
{
    MainManagerFSM::dispatch<E> (event);
}

/* polling this function to perform main status update */
void mainStatusLoop ();

#endif // MAIN_FSM_H_

#ifndef MAIN_FSM_H_
#define MAIN_FSM_H_

/*
Implementing a state machine to manage main actions:
    * line follower
    * tasking mission
    * ...
*/

#include "ESP32Servo.h"
#include "PID_v1.h"
#include "action.h"
#include "constants.h"
#include "cppQueue.h"
#include "dispatchqueue.h"
#include "global.h"
#include "gpio.h"
#include "l298n.h"
#include "makerline.h"
#include "mission.h"
#include "network.fsm.h"
#include "tinyfsm.hpp"
#include "types.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Event declarations
// clang-format off
//

struct mainevent : tinyfsm::Event {};

struct mainevent_interval: mainevent {};

struct mainevent_dispatch_act: mainevent {};
struct mainevent_move: mainevent {};

// ----------------------------------------------------------------------------
// MainManager (FSM base class) declaration
//

class MainManager : public tinyfsm::Fsm<MainManager>
{
  public:
    /* default reaction for unhandled events */
    void react (tinyfsm::Event const &) {};

    /* react events in some states */
    virtual void react (mainevent_interval const &) {};
    virtual void react (mainevent_dispatch_act const &) {};
    virtual void react (mainevent_move const &) {};

    /* entry actions in some states */
    virtual void entry (void) {};

    /* exit actions in some states */
    virtual void exit (void) {};

  private:
    TimerHandle_t _timer;

  protected:
    static L298N lhsMotor, rhsMotor;
    static MakerLine makerLine;
    static Servo servo;
    
    static AGVInfo agvInfo;

    static Mission mission;

    static dispatch_queue dpQueue;

    static void onInterval (TimerHandle_t t);
    static bool isHaveForceAction();

    void pushStatus (MainStatus status);
    void stopAgv();
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
sendMainEvent (E const &event, bool condition = true)
{
  if (condition == false) {
    return;
  }
    MainManagerFSM::dispatch<E> (event);
}

#endif // MAIN_FSM_H_

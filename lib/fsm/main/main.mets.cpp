#include "main.fsm.h"

void MainManager::timerInit (TimerCallbackFunction_t cb, TickType_t xTimerPeriodInTicks, UBaseType_t autoReaload)
{
    _timer = xTimerCreate (
        "MainIntervalTimer", // timer name
        xTimerPeriodInTicks, // period in ticks
        autoReaload,         // auto reload
        NULL,                // timer id
        cb);                 // callback
}

void MainManager::timerStart ()
{
    if (_timer == nullptr)
        return;

    if (!xTimerIsTimerActive (_timer))
        xTimerStart (_timer, 0);
}

void MainManager::timerStop ()
{
    if (_timer == nullptr)
        return;

    if (xTimerIsTimerActive (_timer))
        xTimerStop (_timer, 0);
}

void MainManager::timerDelete ()
{
    if (_timer == nullptr)
        return;

    timerStop ();
    xTimerDelete (_timer, portMAX_DELAY);
}

void MainManager::putMainStatus (MainStatus status)
{
    dpQueue.dispatch (
        [ = ] ()
        {
            xSemaphoreTake (global::mainstatusMutex, portMAX_DELAY);
            global::mainstatus = status;
            xSemaphoreGive (global::mainstatusMutex);
        });
}

void MainManager::onInterval (TimerHandle_t t)
{
    sendMainEvent (mainevent_interval {});
}

bool MainManager::isHaveForceAction ()
{
    ActData nextAct;

    if (action::queue.peek (&nextAct))
        return nextAct.forceExcu;

    return false;
}

void MainManager::stopAgv ()
{
    timerStop ();
    rhsMotor.stop ();
    lhsMotor.stop ();
}

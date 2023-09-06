#include "mainstatus.task.h"

void
mainStatusTask (void *params)
{
    pinMode (ERR_LED_3, OUTPUT);
    pinMode (RUN_LED, OUTPUT);
    digitalWrite (RUN_LED, 0);
    digitalWrite (ERR_LED_3, 0);

    for (;;)
    {
        xSemaphoreTake (global::mainstatusMutex, portMAX_DELAY);
        MainStatus currentStatus = global::mainstatus;
        xSemaphoreGive (global::mainstatusMutex);

        switch (currentStatus)
        {
        case MainStatus::mainstatus_initialize:
            break;
        case MainStatus::mainstatus_idle:
            blinkLed (RUN_LED, constants::mainStatus::idleBlinkTimes);
            break;
        case MainStatus::mainstatus_runMission:
            blinkLed (RUN_LED, constants::mainStatus::runBlinkTimes);
            break;
        default:
            blinkLed (
                RUN_LED,
                constants::mainStatus::undefineErrorBlinkTimes,
                constants::mainStatus::delayMs,
                0);
            break;
        }

        vTaskDelay (constants::mainStatus::delayMs);
    }
}
#include "mainstatus.task.h"

#include "config.h"
#include "global.h"
#include "gpio.h"
#include "helper.h"

void mainStatusTask(void *params)
{
    pinMode(ERR_LED_3, OUTPUT);
    pinMode(RUN_LED, OUTPUT);
    digitalWrite(RUN_LED, 0);
    digitalWrite(ERR_LED_3, 0);

    for (;;)
    {
        xSemaphoreTake(global::mainstatusMutex, portMAX_DELAY);
        MainStatus currentStatus = global::mainstatus;
        xSemaphoreGive(global::mainstatusMutex);

        switch (currentStatus)
        {
        case MainStatus::mainstatus_initialize:
            blinkLed(ERR_LED_3, CONF_MAINSTATUS_INIT_BLINKTIMES);
            break;
        case MainStatus::mainstatus_idle:
            blinkLed(RUN_LED, CONF_MAINSTATUS_IDLE_BLINKTIMES);
            break;
        case MainStatus::mainstatus_runMission:
            blinkLed(RUN_LED, CONF_MAINSTATUS_RUN_BLINKTIMES);
            break;
        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(CONF_MAINSTATUS_DELAY_MS));
    }
}
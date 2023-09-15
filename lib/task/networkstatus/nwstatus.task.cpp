#include "nwstatus.task.h"

void
nwStatusTask (void *params)
{
    pinMode (ERR_LED_1, OUTPUT);
    digitalWrite (ERR_LED_1, 0);

    for (;;)
    {
        xSemaphoreTake (global::nwstatusMutex, portMAX_DELAY);
        NwStatus currentStatus = global::nwstatus;
        xSemaphoreGive (global::nwstatusMutex);

        switch (currentStatus)
        {
        case nwstatus_good:
            digitalWrite (ERR_LED_1, 0);
            break;
        case nwstatus_wifi_not_found:
            blinkLed (ERR_LED_1, constants::networkStatus::wifiConnectBlinkTimes);
            break;
        case nwstatus_wifi_config:
            blinkLed (ERR_LED_1, constants::networkStatus::wifiConfigBlinkTimes);
            break;
        case nwstatus_mqtt_not_found:
            blinkLed (ERR_LED_1, constants::networkStatus::mqttConnectBlinkTimes);
            break;
        case nwstatus_error:
            blinkLed (
                ERR_LED_1,
                constants::networkStatus::undefineErrorBlinkTimes,
                constants::networkStatus::delayMs,
                0);
            break;
        }

        vTaskDelay (constants::networkStatus::delayMs);
    }
}

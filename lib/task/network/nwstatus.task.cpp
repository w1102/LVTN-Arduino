#include "nwstatus.task.h"

#include "config.h"
#include "global.extern.h"
#include "gpio.h"
#include "helper.h"

void
nwStatusTask (void *params)
{
    pinMode (ERR_LED_1, OUTPUT);
    digitalWrite (ERR_LED_1, 0);

    for (;;)
    {
        NwStatus currentStatus;
        
        xSemaphoreTake (nwstatusMutex, portMAX_DELAY);
        currentStatus = nwstatus;
        xSemaphoreGive (nwstatusMutex);

        switch (currentStatus)
        {
        case nwstatus_good:
            digitalWrite (ERR_LED_1, 0);
            break;
        case nwstatus_wifi_not_found:
            blinkLed (ERR_LED_1, CONF_NWSTATUSTASK_WIFI_CONNECT_BLINKTIMES);
            break;
        case nwstatus_wifi_config:
            blinkLed (ERR_LED_1, CONF_NWSTATUSTASK_WIFI_CONFIG_BLINKTIMES);
            break;
        case nwstatus_mqtt_not_found:
            blinkLed (ERR_LED_1, CONF_NWSTATUSTASK_MQTT_CONNECT_BLINKTIMES);
            break;
        case nwstatus_error:
            static const uint longDelayMs = 500;
            blinkLed (
                ERR_LED_1,
                CONF_NWSTATUSTASK_UNDEFINE_ERROR_BLINKTIMES,
                longDelayMs,
                0);
            break;
        }

        vTaskDelay (pdMS_TO_TICKS (CONF_NWSTATUSTASK_DELAY_MS));
    }
}

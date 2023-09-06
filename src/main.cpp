#include <Arduino.h>

#include "config.h"
#include "constants.h"
#include "eepromrw.h"
#include "global.h"
#include "main.task.h"
#include "network.task.h"
#include "ultrasonic.h"

void
setup ()
{
    Serial.begin (115200);

    global::initMutex ();
    eepromInit ();

    xTaskCreatePinnedToCore (
        networkTask,                        // Function that should be called
        constants::network::taskName,       // Name of the task (for debugging)
        constants::network::taskStackDepth, // Stack size (bytes)
        NULL,                               // Parameter to pass
        constants::network::taskPriority,   // Task priority
        NULL,                               // Task handle
        constants::network::taskRunningCore // Run on core
    );

    xTaskCreatePinnedToCore (
        mainTask,                 // Function that should be called
        CONF_MAINTASK_NAME,       // Name of the task (for debugging)
        CONF_MAINTASK_STACK_SIZE, // Stack size (bytes)
        NULL,                     // Parameter to pass
        CONF_MAINTASK_PRIORITY,   // Task priority
        NULL,                     // Task handle
        CONF_MAINTASK_RUN_CORE    // Run on core
    );

    xTaskCreatePinnedToCore (
        ultrasonicTask,                 // Function that should be called
        CONF_ULTRASONICTASK_NAME,       // Name of the task (for debugging)
        CONF_ULTRASONICTASK_STACK_SIZE, // Stack size (bytes)
        NULL,                           // Parameter to pass
        CONF_ULTRASONICTASK_PRIORITY,   // Task priority
        NULL,                           // Task handle
        CONF_ULTRASONICTASK_RUN_CORE    // Run on core
    );
}

void
loop ()
{
    vTaskDelay (portMAX_DELAY);
}

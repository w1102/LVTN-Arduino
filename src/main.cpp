#include <Arduino.h>

#include "config.h"
#include "eepromrw.h"
#include "global.h"
#include "main.task.h"
#include "network.task.h"
#include "ultrasonic.h"

void
setup ()
{
    Serial.begin (115200);

    initMutex ();
    eepromInit ();
    // eepromClean();

    xTaskCreatePinnedToCore (
        networkTask,            // Function that should be called
        CONF_NWSTASK_NAME,      // Name of the task (for debugging)
        CONF_NWTASK_STACK_SIZE, // Stack size (bytes)
        NULL,                   // Parameter to pass
        CONF_NWTASK_PRIORITY,   // Task priority
        NULL,                   // Task handle
        CONF_NWTASK_RUN_CORE    // Run on core
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

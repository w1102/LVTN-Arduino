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
        mainTask,                          // Function that should be called
        constants::task::main::name,       // Name of the task (for debugging)
        constants::task::main::stackDepth, // Stack size (bytes)
        NULL,                              // Parameter to pass
        constants::task::main::priority,   // Task priority
        NULL,                              // Task handle
        constants::task::main::runningCore // Run on core
    );

    xTaskCreatePinnedToCore (
        ultrasonicTask,                          // Function that should be called
        constants::task::ultrasonic::name,       // Name of the task (for debugging)
        constants::task::ultrasonic::stackDepth, // Stack size (bytes)
        NULL,                                    // Parameter to pass
        constants::task::ultrasonic::priority,   // Task priority
        NULL,                                    // Task handle
        constants::task::ultrasonic::runningCore // Run on core
    );
}

void
loop ()
{
    vTaskDelay (portMAX_DELAY);
}

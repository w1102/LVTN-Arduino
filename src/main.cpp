#include <Arduino.h>

#include "config.h"
#include "constants.h"
#include "eepromrw.h"
#include "global.h"
#include "main.task.h"
#include "mainstatus.task.h"
#include "network.task.h"
#include "nwstatus.task.h"
#include "ultrasonic.h"

void
setup ()
{
    Serial.begin (115200);

    global::init ();
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
        nwStatusTask,                        // Function that should be called
        constants::networkStatus::taskName,       // Name of the task (for debugging)
        constants::networkStatus::taskStackDepth, // Stack size (bytes)
        NULL,                               // Parameter to pass
        constants::networkStatus::taskPriority,   // Task priority
        NULL,                               // Task handle
        constants::networkStatus::taskRunningCore // Run on core
    );

    xTaskCreatePinnedToCore (
        mainTask,                        // Function that should be called
        constants::main::taskName,       // Name of the task (for debugging)
        constants::main::taskStackDepth, // Stack size (bytes)
        NULL,                            // Parameter to pass
        constants::main::taskPriority,   // Task priority
        NULL,                            // Task handle
        constants::main::taskRunningCore // Run on core
    );

    xTaskCreatePinnedToCore (
        mainStatusTask,                        // Function that should be called
        constants::mainStatus::taskName,       // Name of the task (for debugging)
        constants::mainStatus::taskStackDepth, // Stack size (bytes)
        NULL,                                  // Parameter to pass
        constants::mainStatus::taskPriority,   // Task priority
        NULL,                                  // Task handle
        constants::mainStatus::taskRunningCore // Run on core
    );

    xTaskCreatePinnedToCore (
        ultrasonicTask,                        // Function that should be called
        constants::ultrasonic::taskName,       // Name of the task (for debugging)
        constants::ultrasonic::taskStackDepth, // Stack size (bytes)
        NULL,                                  // Parameter to pass
        constants::ultrasonic::taskPriority,   // Task priority
        NULL,                                  // Task handle
        constants::ultrasonic::taskRunningCore // Run on core
    );
}

void
loop ()
{
    vTaskDelay (portMAX_DELAY);
}

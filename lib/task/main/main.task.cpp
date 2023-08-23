#include "main.task.h"

#include "global.extern.h"
#include "gpio.h"
#include "main.fsm.h"
#include "mainstatus.task.h"

void mainTask(void *params)
{
    MainManagerFSM::start();

    xTaskCreatePinnedToCore(
        mainStatusTask,             // Function that should be called
        CONF_MAINSTATUS_NAME,       // Name of the task (for debugging)
        CONF_MAINSTATUS_STACK_SIZE, // Stack size (bytes)
        NULL,                       // Parameter to pass
        CONF_MAINSTATUS_PRIORITY,   // Task priority
        NULL,                       // Task handle
        CONF_MAINSTATUS_RUN_CORE);  // Run on core

    for (;;)
    {
        mainStatusLoop();
    }
}

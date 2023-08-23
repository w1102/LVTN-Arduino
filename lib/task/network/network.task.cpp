#include "network.task.h"

#include "config.h"
#include "global.extern.h"
#include "gpio.h"
#include "network.fsm.h"
#include "nwstatus.task.h"
 
void networkTask(void *params)
{
    NetworkManagerFSM::start();

    xTaskCreatePinnedToCore(
        nwStatusTask,            // Function that should be called
        CONF_NWSTATUSTASK_NAME,       // Name of the task (for debugging)
        CONF_NWSTATUSTASK_STACK_SIZE, // Stack size (bytes)
        NULL,                    // Parameter to pass
        CONF_NWSTATUSTASK_PRIORITY,   // Task priority
        NULL,                    // Task handle
        CONF_NWSTATUSTASK_RUN_CORE);   // Run on core

    for (;;)
    {
        nwStatusLoop();
    }
}

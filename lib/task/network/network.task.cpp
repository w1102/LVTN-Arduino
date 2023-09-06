#include "network.task.h"

void
networkTask (void *params)
{
    NetworkManagerFSM::start ();

    for (;;)
    {
        vTaskDelay (portMAX_DELAY);
    }
}

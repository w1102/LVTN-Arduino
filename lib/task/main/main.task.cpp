#include "main.task.h"

void mainTask(void *params)
{
    MainManagerFSM::start();

    for (;;)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

#include "trigger.h"

typedef struct
{
    TickType_t delay;
    triggerCallback callback;
} TriggerData;

void
triggerTask (void *param)
{
    TriggerData *data = (TriggerData *)param;

    vTaskDelay (data->delay);
    data->callback ();

    delete data;
    vTaskDelete (NULL);
}

void
trigger (const char *triggerName, triggerCallback callback, TickType_t triggerAfter)
{
    TriggerData *data = new TriggerData {};
    data->delay = triggerAfter;
    data->callback = callback;

    xTaskCreatePinnedToCore (
        triggerTask,              // Function that should be called
        triggerName,              // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE, // Stack size (bytes)
        data,                     // Parameter to pass
        1,                        // Task priority
        NULL,                     // Task handle
        1                         // Run on core
    );
}
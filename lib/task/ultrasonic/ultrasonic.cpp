#include "ultrasonic.h"

void
ultrasonicTask (void *params)
{

    pinMode (SR04_TRIG, OUTPUT);
    pinMode (SR04_ECHO, INPUT);

    float duration, distance;
    bool semaphoreTaked = false;

    for (;;)
    {
        digitalWrite (SR04_TRIG, LOW);
        delayMicroseconds (2);
        digitalWrite (SR04_TRIG, HIGH);
        delayMicroseconds (10);
        digitalWrite (SR04_TRIG, LOW);
        delayMicroseconds (30);

        duration = pulseIn (SR04_ECHO, HIGH);
        distance = duration * CONF_ULTRASONICTASK_SPEED_OF_SOUND / 2;

        //xQueueSend(ultrasonicDistanceQueue, &distance, 0);

        if (distance < CONF_ULTRASONICTASK_MIN_DISTANCE && !semaphoreTaked)
        {
            xSemaphoreTake (ultrasonicThresholdDistanceSync, portMAX_DELAY);
            semaphoreTaked = true;
        }
        else if (distance > CONF_ULTRASONICTASK_MIN_DISTANCE && semaphoreTaked)
        {
            xSemaphoreGive (ultrasonicThresholdDistanceSync);
            semaphoreTaked = false;
        }

        vTaskDelay (pdMS_TO_TICKS (CONF_ULTRASONICTASK_INTERVAL));
    }
}
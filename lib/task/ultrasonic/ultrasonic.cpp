#include "ultrasonic.h"

void
ultrasonicTask (void *params)
{

    pinMode (SR04_TRIG, OUTPUT);
    pinMode (SR04_ECHO, INPUT);

    float duration, distance;
    bool isLocked = false;

    for (;;)
    {
        digitalWrite (SR04_TRIG, LOW);
        delayMicroseconds (2);
        digitalWrite (SR04_TRIG, HIGH);
        delayMicroseconds (10);
        digitalWrite (SR04_TRIG, LOW);
        delayMicroseconds (30);

        duration = pulseIn (SR04_ECHO, HIGH);
        distance = duration * constants::ultrasonic::speedOfSound / 2.0;

        xSemaphoreTake (global::ultrasonicDistanceMutex, portMAX_DELAY);
        global::ultrasonicDistance = distance;
        xSemaphoreGive (global::ultrasonicDistanceMutex);

        if (distance < constants::ultrasonic::stopDistance && !isLocked)
        {
            xSemaphoreTake (global::ultrasonicDistanceStopsync, portMAX_DELAY);
            isLocked = true;
        }
        else if (distance > constants::ultrasonic::stopDistance && isLocked)
        {
            xSemaphoreGive (global::ultrasonicDistanceStopsync);
            isLocked = false;
        }

        vTaskDelay (constants::global::intervalMs);
    }
}
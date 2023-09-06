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
        distance = duration * constants::config::ultrasonic::speedOfSound / 2.0;

        if (distance < constants::config::ultrasonic::minDistance && !isLocked)
        {
            xSemaphoreTake (ultrasonicThresholdDistanceSync, portMAX_DELAY);
            isLocked = true;
        }
        else if (distance > constants::config::ultrasonic::minDistance && isLocked)
        {
            xSemaphoreGive (ultrasonicThresholdDistanceSync);
            isLocked = false;
        }

        vTaskDelay (constants::global::intervalMs);
    }
}
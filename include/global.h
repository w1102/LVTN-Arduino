#ifndef GLOBAL_H_
#define GLOBAL_H_

/* This file contains the declaration of global variables that can be included, and it can only be included in the main.cpp file */

#include "map.h"
#include "mission.h"
#include "types.h"
#include <Arduino.h>

namespace global
{

    inline NwStatus nwstatus = nwstatus_wifi_not_found;
    inline xSemaphoreHandle nwstatusMutex;

    inline MainStatus mainstatus = mainstatus_initialize;
    inline xSemaphoreHandle mainstatusMutex;

    inline StorageMap storageMap;
    inline xSemaphoreHandle storageMapMutex;

    inline QueueHandle_t currentLineCountQueue;
    inline QueueHandle_t missionStatusQueue;
    inline QueueHandle_t missionQueue;

    inline xSemaphoreHandle missionSync;

    inline xSemaphoreHandle ultrasonicThresholdDistanceSync;
    inline QueueHandle_t ultrasonicDistanceQueue;

    inline void
    initMutex ()
    {
        missionSync = xSemaphoreCreateBinary ();
        xSemaphoreGive (missionSync);

        ultrasonicThresholdDistanceSync = xSemaphoreCreateBinary ();
        xSemaphoreGive (ultrasonicThresholdDistanceSync);

        nwstatusMutex = xSemaphoreCreateMutex ();
        mainstatusMutex = xSemaphoreCreateMutex ();
        storageMapMutex = xSemaphoreCreateMutex ();

        currentLineCountQueue = xQueueCreate (CONF_GLO_QUEUE_LENGTH, sizeof (int));
        missionStatusQueue = xQueueCreate (CONF_GLO_QUEUE_LENGTH, sizeof (MissionStatus));
        missionQueue = xQueueCreate (CONF_GLO_QUEUE_LENGTH, sizeof (MissionData));
        ultrasonicDistanceQueue = xQueueCreate (CONF_GLO_QUEUE_LENGTH, sizeof (int));
    }
}

#endif // GLOBAL_H_

#ifndef GLOBAL_H_
#define GLOBAL_H_

/* This file contains the declaration of global variables that can be included, and it can only be included in the main.cpp file */

#include "constants.h"
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

    inline Map map;
    inline xSemaphoreHandle mapMutex;

    inline QueueHandle_t agvInfoQueue;
    inline QueueHandle_t currentLineCountQueue;
    inline QueueHandle_t missionStatusQueue;
    inline QueueHandle_t missionQueue;

    inline xSemaphoreHandle missionSync;

    inline xSemaphoreHandle ultrasonicDistanceStopsync;
    inline xSemaphoreHandle ultrasonicDistanceMutex;
    inline float ultrasonicDistance;

    inline void
    init ()
    {
        missionSync = xSemaphoreCreateBinary ();
        xSemaphoreGive (missionSync);

        ultrasonicDistanceStopsync = xSemaphoreCreateBinary ();
        xSemaphoreGive(ultrasonicDistanceStopsync);

        ultrasonicDistanceMutex = xSemaphoreCreateMutex ();
        nwstatusMutex = xSemaphoreCreateMutex ();
        mainstatusMutex = xSemaphoreCreateMutex ();
        mapMutex = xSemaphoreCreateMutex ();

        agvInfoQueue = xQueueCreate(constants::global::queueCnt, sizeof(AGVInfo));
        currentLineCountQueue = xQueueCreate (constants::global::queueCnt, sizeof (int));
        missionStatusQueue = xQueueCreate (constants::global::queueCnt, sizeof (MissionStatus));
        missionQueue = xQueueCreate (constants::global::queueCnt, sizeof (Mission));
    }
}

#endif // GLOBAL_H_

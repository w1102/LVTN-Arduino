#ifndef GLOBAL_H_
#define GLOBAL_H_

/* This file contains the declaration of global variables that can be included, and it can only be included in the main.cpp file */

#include "map.h"
#include "mission.h"
#include "types.h"
#include <Arduino.h>

NwStatus nwstatus = nwstatus_wifi_not_found;
xSemaphoreHandle nwstatusMutex;

MainStatus mainstatus = mainstatus_initialize;
xSemaphoreHandle mainstatusMutex;

StorageMap storageMap;
xSemaphoreHandle storageMapMutex;

QueueHandle_t currentLineCountQueue;
QueueHandle_t missionStatusQueue;
QueueHandle_t missionQueue;

xSemaphoreHandle missionSync;

void
initMutex ()
{
    missionSync = xSemaphoreCreateBinary();
    xSemaphoreGive(missionSync);

    nwstatusMutex = xSemaphoreCreateMutex();
    mainstatusMutex = xSemaphoreCreateMutex ();
    storageMapMutex = xSemaphoreCreateMutex ();
    
    currentLineCountQueue = xQueueCreate (CONF_GLO_QUEUE_LENGTH, sizeof (int));
    missionStatusQueue = xQueueCreate(CONF_GLO_QUEUE_LENGTH, sizeof(MissionStatus));
    missionQueue = xQueueCreate(CONF_GLO_QUEUE_LENGTH, sizeof(MissionData));
}

#endif // GLOBAL_H_

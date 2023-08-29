#ifndef GLOBAL_EXTERN_H_
#define GLOBAL_EXTERN_H_

/* This file contains the declaration of global variables that can be included in multiple other files */

#include "types.h"
#include "map.h"
#include "mission.h"
#include <Arduino.h>

extern NwStatus nwstatus;
extern xSemaphoreHandle nwstatusMutex;

extern MainStatus mainstatus;
extern xSemaphoreHandle mainstatusMutex;

extern StorageMap storageMap;
extern xSemaphoreHandle storageMapMutex;

extern QueueHandle_t currentLineCountQueue;
extern QueueHandle_t missionStatusQueue;
extern QueueHandle_t missionQueue;

extern xSemaphoreHandle missionSync;

extern xSemaphoreHandle ultrasonicThresholdDistanceSync;
extern QueueHandle_t ultrasonicDistanceQueue;


#endif // GLOBAL_EXTERN_H_

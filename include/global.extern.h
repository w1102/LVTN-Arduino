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

extern Mission mission;
extern xSemaphoreHandle missionMutex;

extern QueueHandle_t currentLineCountQueue;
extern QueueHandle_t missionStatusQueue;

extern xSemaphoreHandle missionSync;

#endif // GLOBAL_EXTERN_H_

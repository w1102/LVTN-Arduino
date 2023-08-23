#ifndef TYPES_H_
#define TYPES_H_

#include <Arduino.h>

typedef enum
{
    nwstatus_good,
    nwstatus_error,
    nwstatus_wifi_not_found,
    nwstatus_wifi_config,
    nwstatus_mqtt_not_found
} NwStatus;

typedef enum
{
    mainstatus_initialize,
    mainstatus_idle,
    mainstatus_runMission,
    mainstatus_error
} MainStatus;

typedef enum
{
    mainact_none,
    mainact_turnLeft,
    mainact_turnRight,
    mainact_turnBack,
    mainact_bypass,
    mainact_io,
    mainact_doneMission
} MainActType;

typedef struct
{
    MainActType type;
    bool forceExcu;
} MainAct;

typedef enum
{
    missionstatus_no_mission,
    missionstatus_taking,
    missionstatus_running,
    missionstatus_done
} MissionStatus;

typedef enum
{
    importMission,
    exportMission
} MissionType;

typedef struct
{
    int width, height;
} MapSize;

typedef struct
{
    int x, y;
} MapObjPos;

typedef enum
{
    exportPallet = 0,
    importPallet,
    StoragePallet,
    line
} MapObjRole;

typedef struct
{
    int lineCount;
    char palletId[15];
} MapObjData;

typedef struct
{
    MapObjPos pos;
    MapObjRole role;
    MapObjData data;
} MapObj;

#endif // TYPES_H_

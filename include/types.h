#ifndef TYPES_H_
#define TYPES_H_

#include <Arduino.h>
#include "cppQueue.h"

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

typedef enum {
    forward,
    reward
} Direction;

typedef enum
{
    act_none,
    act_turnLeft,
    act_turnRight,
    act_turnBack,
    act_bypass,
    act_io,
    act_done_origi,
    act_done_incre,
    act_done_decre
} ActType;

typedef struct
{
    ActType type;
    bool forceExcu;
} ActData;

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


typedef enum {
    phase1,
    phase2,
    phase3,
} MissionPhase;


typedef struct {
    int target;
    String *action;
} MissionPhaseData;

typedef struct {
    MissionPhaseData phase1;
    MissionPhaseData phase2;
    bool stopInMainBranch;
} MissionData;

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

#ifndef TYPES_H_
#define TYPES_H_

#include "cppQueue.h"
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
    forward = 0,
    reward
} Direction;

typedef struct
{
    String *id, *leaveHomeActs, *forwardDirHomingActs, *rewardDirHomingActs;
    Direction direction;
    int currentLineCount;
    bool isHome, isMainBranch, isItemPicked;
} AGVInfo;

typedef enum
{
    act_none,
    act_turnLeft,
    act_turnRight,
    act_turnBack,
    act_bypass,
    act_io,
    act_done
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

typedef enum
{
    phase1,
    phase2
} MissionPhase;

typedef struct
{
    int target;
    String *action;
} MissionPhaseData;

typedef struct
{
    MissionPhaseData phase1;
    MissionPhaseData phase2;
    bool stopInMainBranch;
    MissionPhase currentPhase;
    bool isHomingMission;
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

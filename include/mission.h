#ifndef MISSION_H_
#define MISSION_H_

#include "config.h"
#include "cppQueue.h"
#include "types.h"
#include <Arduino.h>

void parseMissionAction (cppQueue *actionQueue, String *actionStr);

bool parseMissionMsg (MissionData &mission, String missionStr);

#endif // MISSION_H_
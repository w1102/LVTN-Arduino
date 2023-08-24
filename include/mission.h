#ifndef MISSION_H_
#define MISSION_H_

#include "types.h"
#include <Arduino.h>

#define MISSION_TYPE_KEY "type"
#define MISSION_LINECOUNT_KEY "lineCount"

class Mission
{
  public:
    Mission();
    bool parseMissionMsg(String missionStr);

    String missionStatus2Str(MissionStatus status);

    int getTargetLineCount();
    MissionType getType();

  private:
    MissionType type;
    int targetLineCount;
};

#endif // MISSION_H_
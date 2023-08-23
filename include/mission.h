#ifndef MISSION_H_
#define MISSION_H_

#include "types.h"
#include <Arduino.h>
#include <cppQueue.h>

#define MISSION_TYPE_KEY "type"
#define MISSION_LINECOUNT_KEY "lineCount"

class Mission
{
  public:
    Mission();
    bool parseMission(String missionStr);

    String missionStatus2Str(MissionStatus status);

    void putStatus(MissionStatus status);
    MissionStatus popStatus();
    MissionStatus getLastStatus();
    bool isStatusQueueEmpty();

    int getLineCount();
    MissionType getType();

    void clean();

  private:
    MissionType type;
    int lineCount;
    MissionStatus lastStatus;

    void printInvalid();
};

#endif // MISSION_H_
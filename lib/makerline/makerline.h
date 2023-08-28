#ifndef MAKER_LINE_H_
#define MAKER_LINE_H_
 
#define NUM_OF_SENS 5

#define STOP_POSITION 5

#define NEAR_LEFT_POSITION 3
#define LEFT_POSITION 4

#define NEAR_RIGHT_POSITION -3
#define RIGHT_POSITION -4


#define PREV_DEFAULT 10

#include "Arduino.h"

class MakerLine
{
   public:
    MakerLine(const uint8_t sen1,
              const uint8_t sen2,
              const uint8_t sen3,
              const uint8_t sen4,
              const uint8_t sen5);

    int8_t readPosition(int defaultPos = PREV_DEFAULT);

   private:
    const uint8_t numOfSens = NUM_OF_SENS;
    uint8_t sensGpioMap[NUM_OF_SENS];
    uint8_t senRawValue;

    void readRaw();
};

#endif // MAKER_LINE_H_
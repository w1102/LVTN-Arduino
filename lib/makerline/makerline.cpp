#include "makerline.h"

MakerLine::MakerLine(const uint8_t sen1,
                     const uint8_t sen2,
                     const uint8_t sen3,
                     const uint8_t sen4,
                     const uint8_t sen5)
{
    this->sensGpioMap[0] = sen1;
    this->sensGpioMap[1] = sen2;
    this->sensGpioMap[2] = sen3;
    this->sensGpioMap[3] = sen4;
    this->sensGpioMap[4] = sen5;

    for (auto senGpio : sensGpioMap)
    {
        pinMode(senGpio, INPUT);
    }
}

void MakerLine::readRaw()
{
    this->senRawValue = 0;

    for (uint8_t i = 0; i < this->numOfSens; i++)
    {
        this->senRawValue |= digitalRead(sensGpioMap[i]) << (this->numOfSens - i - 1);
    }
}

int8_t MakerLine::readPosition(int defaultPos)
{
    readRaw();

    int8_t position = 0;
    static int8_t prev_position = 0;

    switch (this->senRawValue)
    {
    case B00001:
        position = 4;
        break;
    case B00011:
        position = 3;
        break;
    case B00010:
        position = 2;
        break;
    case B00110:
        position = 1;
        break;

    case B01110:
    case B00100:
        position = 0;
        break;

    case B01100:
        position = -1;
        break;
    case B01000:
        position = -2;
        break;
    case B11000:
        position = -3;
        break;
    case B10000:
        position = -4;
        break;

    case B11110:
    case B11100:
        position = RIGHT_POSITION;
        break;

    case B01111:
    case B00111:
        position = LEFT_POSITION;
        break;

    case B10001:
    case B11111:
        position = STOP_POSITION;
        break;

    default:
        position = defaultPos == PREV_DEFAULT ? prev_position : defaultPos;
    }

    prev_position = position;
    return position;
}
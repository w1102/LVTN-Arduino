#include "helper.h"
#include <iostream>
#include <cstdlib>

uint blinkLed(uint led, uint blinkTimes, uint onTime, uint offTime)
{
    uint totalTimeMs = 0;

    for (uint i = 0; i < blinkTimes; i++)
    {
        totalTimeMs += onTime + offTime;

        digitalWrite(led, 1);
        vTaskDelay(pdMS_TO_TICKS(onTime));

        digitalWrite(led, 0);
        vTaskDelay(pdMS_TO_TICKS(offTime));
    }

    return totalTimeMs;
}

int str2number(const char* str, int length) {
    // Tạo một chuỗi tạm thời có độ dài tương ứng
    char* tempStr = new char[length + 1];
    strncpy(tempStr, str, length);
    tempStr[length] = '\0'; // Đảm bảo kết thúc chuỗi

    int result = std::atoi(tempStr);

    delete[] tempStr; // Giải phóng bộ nhớ

    return result;
}

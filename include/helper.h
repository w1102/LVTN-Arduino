#ifndef HELPER_H_
#define HELPER_H_

#include <Arduino.h>

/* Perform blinking LED, return the total time used in milliseconds. */
uint blinkLed(uint led, uint blinkTimes, uint onTime = 150, uint offTime = 100);

int str2number(const char* str, int length);

#endif // HELPER_H_

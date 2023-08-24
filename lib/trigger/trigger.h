#ifndef TRIGGER_H_
#define TRIGGER_H_

#include <Arduino.h>
#include <functional>

//typedef void (*triggerCallback)();
typedef  std::function<void(void)> triggerCallback;

void trigger(const char* triggerName, triggerCallback callback, TickType_t triggerAfter = pdMS_TO_TICKS(100));






#endif // TRIGGER_H_
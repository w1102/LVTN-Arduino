#ifndef NETWORK_TASK_H
#define NETWORK_TASK_H

#include <Arduino.h>
#include "config.h"
#include "gpio.h"
#include "network.fsm.h"

void networkTask(void *params);

#endif // NETWORK_TASK_H

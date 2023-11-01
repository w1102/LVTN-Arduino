#include "main.fsm.h"

dispatch_queue MainManager::dpQueue (
    constants::main::dpQueueName,
    constants::main::dpQueuethreadCnt,
    constants::main::dpQueueStackDepth);

AGVInfo MainManager::m_info;
Mission MainManager::mission;

L298N MainManager::lhsMotor (M1_EN, M1_PA, M1_PB);
L298N MainManager::rhsMotor (M2_EN, M2_PA, M2_PB);
MakerLine MainManager::makerLine (SENSOR_1, SENSOR_2, SENSOR_3, SENSOR_4, SENSOR_5);
Servo MainManager::servo;

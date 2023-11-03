#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <Arduino.h>

namespace constants
{
    namespace global
    {
        inline constexpr uint8_t queueCnt { 10 };
        inline constexpr uint8_t intervalMs { pdMS_TO_TICKS (5) };
    }

    namespace ultrasonic
    {
        inline constexpr uint16_t taskStackDepth { configMINIMAL_STACK_SIZE + 2048 };
        inline constexpr uint8_t taskPriority { 5 };
        inline constexpr uint8_t taskRunningCore { ARDUINO_RUNNING_CORE };
        inline constexpr char taskName[] { "ultrasonic" };

        inline constexpr float speedOfSound { 0.034 };
        inline constexpr float warnDistance { 20.0 };
        inline constexpr float stopDistance { 6.0 };
    }

    namespace network
    {
        inline constexpr uint8_t maxConnectionAttempts { 3 };
        inline constexpr uint16_t connectTimeoutMs { pdMS_TO_TICKS (3000) };
        inline constexpr uint16_t trackingInterval { pdMS_TO_TICKS (500) };

        inline constexpr uint16_t taskStackDepth { configMINIMAL_STACK_SIZE + 6144 };
        inline constexpr uint8_t taskPriority { 10 };
        inline constexpr uint8_t taskRunningCore { ARDUINO_RUNNING_CORE };
        inline constexpr char taskName[] { "network" };

        inline constexpr uint16_t dpQueueStackDepth { configMINIMAL_STACK_SIZE + 2048 };
        inline constexpr uint8_t dpQueuethreadCnt { 2 };
        inline constexpr char dpQueueName[] { "NetworkDpQueue" };

        inline constexpr char mqttHost[] { "mqtt.tranquang.net" };
        inline constexpr uint16_t mqttPort { 1883 };
        inline constexpr char mqttUsername[] { "quang" };
        inline constexpr char mqttPassword[] { "1102" };
        inline constexpr uint8_t mqttQos { 0 };
        inline constexpr char mqttMapTopic[] { "quang/logistics/map" };
        inline constexpr char mqttAgvInfoTopic[] { "quang/logistics/agvinfo" };
        inline constexpr char mqttMissionTopic[] { "quang/logistics/mission" };

    }

    namespace networkStatus
    {
        inline constexpr uint16_t delayMs { pdMS_TO_TICKS (600) };
        inline constexpr uint8_t wifiConnectBlinkTimes { 1 };
        inline constexpr uint8_t wifiConfigBlinkTimes { 2 };
        inline constexpr uint8_t mqttConnectBlinkTimes { 3 };
        inline constexpr uint8_t undefineErrorBlinkTimes { 1 };

        inline constexpr uint16_t taskStackDepth { configMINIMAL_STACK_SIZE };
        inline constexpr uint8_t taskPriority { 1 };
        inline constexpr uint8_t taskRunningCore { ARDUINO_RUNNING_CORE };
        inline constexpr char taskName[] { "networkStatus" };
    }

    namespace main
    {
        inline constexpr double kp { 80 };
        inline constexpr double ki { 6 };
        inline constexpr double kd { 0.1 };

        inline constexpr uint16_t lowSpeed { 600 };
        inline constexpr uint16_t midSpeed { 750 };
        inline constexpr uint16_t higSpeed { 850 };

        inline constexpr uint8_t stateDelayMs { pdMS_TO_TICKS (25) };
        inline constexpr uint16_t ioDelayMs { pdMS_TO_TICKS (1500) };

        inline constexpr uint16_t servoDelayMs { pdMS_TO_TICKS (10) };
        inline constexpr uint8_t servoPeriod { 50 };
        inline constexpr uint16_t servoUsLow { 400 };
        inline constexpr uint16_t servoUsHigh { 2400 };
        inline constexpr uint8_t servoPushIn { 0 };
        inline constexpr uint8_t servoPushOut { 50 };

        inline constexpr uint8_t magnetOn { false };
        inline constexpr uint8_t magnetOff { true };

        inline constexpr uint16_t taskStackDepth { configMINIMAL_STACK_SIZE + 13312 };
        inline constexpr uint8_t taskPriority { 20 };
        inline constexpr uint8_t taskRunningCore { 0 };
        inline constexpr char taskName[] { "main" };

        inline constexpr uint16_t dpQueueStackDepth { configMINIMAL_STACK_SIZE + 2048 };
        inline constexpr uint8_t dpQueuethreadCnt { 2 };
        inline constexpr char dpQueueName[] { "MainDpQueue" };
    }

    namespace mainStatus
    {
        inline constexpr uint16_t delayMs { pdMS_TO_TICKS (600) };
        inline constexpr uint8_t initBlinkTimes { 1 };
        inline constexpr uint8_t idleBlinkTimes { 1 };
        inline constexpr uint8_t runBlinkTimes { 2 };
        inline constexpr uint8_t undefineErrorBlinkTimes { 1 };

        inline constexpr uint16_t taskStackDepth { configMINIMAL_STACK_SIZE };
        inline constexpr uint8_t taskPriority { 0 };
        inline constexpr uint8_t taskRunningCore { ARDUINO_RUNNING_CORE };
        inline constexpr char taskName[] { "mainStatus" };
    }

    namespace map {
       namespace jsonKey {
        inline constexpr char id[] {"id"};
        inline constexpr char missionId[]{"missionId"};
        inline constexpr char leaveHomeActs[] {"leaveHomeActs"};
        inline constexpr char forwardDirHomingActs[] {"forwardDirHomingActs"};
        inline constexpr char rewardDirHomingActs[] {"rewardDirHomingActs"};
        inline constexpr char currentLineCount[] {"currentLineCount"};
        inline constexpr char isHome[]{"isHome"};
        inline constexpr char isMainBranch[]{"isMainBranch"};
        inline constexpr char isItemPicked[] {"isItemPicked"};
       } 
    }

    namespace mission
    {
        namespace jsonKey
        {
            inline constexpr char id[]{"missionId"};
            inline constexpr char phase1[] { "phase1" };
            inline constexpr char phase2[] { "phase2" };
            inline constexpr char target[] { "target" };
            inline constexpr char action[] { "action" };
        }

        namespace actCodeTable
        {
            inline constexpr uint8_t codeLength { 2 };
            
            inline constexpr uint8_t io { 'I' + 'o' };
            inline constexpr uint8_t bypass {'B' + 'p'};
            inline constexpr uint8_t turnBack {'T' + 'b'};
            inline constexpr uint8_t turnLeft {'T' + 'l'};
            inline constexpr uint8_t turnRight {'T' + 'r'};
            inline constexpr uint8_t done{'D' + 'o'};

            inline constexpr uint8_t forceIo {'I' + 'O'};
            inline constexpr uint8_t forceBypass {'B' + 'P'};
            inline constexpr uint8_t forceTurnBack {'T' + 'B'};
            inline constexpr uint8_t forceTurnLeft {'T' + 'L'};
            inline constexpr uint8_t forceTurnRight {'T' + 'R'};
            inline constexpr uint8_t forceDone{'D' + 'O'};
        }
    }

    namespace eeprom {
        inline constexpr uint8_t wifiCredentialsLen {32};
        inline constexpr uint8_t wifiSsidAddr {0};
        inline constexpr uint8_t wifiSsidLen {wifiCredentialsLen};
        inline constexpr uint8_t wifiPskAddr {wifiSsidAddr + wifiCredentialsLen};
        inline constexpr uint8_t wifiPskLen {wifiCredentialsLen};

        inline constexpr uint8_t totalSize {wifiSsidLen + wifiPskLen};

    }

} // namespace constants

#endif // CONSTANTS_H_

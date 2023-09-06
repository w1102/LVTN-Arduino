#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <Arduino.h>

namespace constants
{
    namespace global
    {
        inline constexpr uint8_t queueCnt { 15 };
        inline constexpr uint8_t intervalMs { pdMS_TO_TICKS (5) };
    }

    namespace task
    {
        namespace ultrasonic
        {
            inline constexpr uint16_t stackDepth { configMINIMAL_STACK_SIZE + 2048 };
            inline constexpr uint8_t priority { 5 };
            inline constexpr uint8_t runningCore { ARDUINO_RUNNING_CORE };
            inline constexpr char name[] { "ultrasonic" };
        }

        namespace networkStatus
        {
            inline constexpr uint16_t stackDepth { configMINIMAL_STACK_SIZE };
            inline constexpr uint8_t priority { 1 };
            inline constexpr uint8_t runningCore { ARDUINO_RUNNING_CORE };
            inline constexpr char name[] { "networkStatus" };
        }

        namespace main
        {
            inline constexpr uint16_t stackDepth { configMINIMAL_STACK_SIZE + 13312 };
            inline constexpr uint8_t priority { 20 };
            inline constexpr uint8_t runningCore { 0 };
            inline constexpr char name[] { "main" };
        }

        namespace mainStatus
        {
            inline constexpr uint16_t stackDepth { configMINIMAL_STACK_SIZE };
            inline constexpr uint8_t priority { 0 };
            inline constexpr uint8_t runningCore { ARDUINO_RUNNING_CORE };
            inline constexpr char name[] { "mainStatus" };
        }
    }

    namespace config
    {
        namespace ultrasonic
        {
            inline constexpr float speedOfSound { 0.034 };
            inline constexpr float minDistance { 7.0 };
        }

    }

    namespace network
    {
        inline constexpr uint8_t maxConnectionAttempts {3}; 
        inline constexpr uint16_t connectTimeoutMs{pdMS_TO_TICKS(3000)};
        inline constexpr uint16_t trackingInterval{pdMS_TO_TICKS(500)};

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
        inline constexpr char mqttLocationTopic[] { "quang/logistics/botlocation" };
        inline constexpr char mqttMissionTopic[] { "quang/logistics/mission" };

    }

} // namespace constants

#endif // CONSTANTS_H_

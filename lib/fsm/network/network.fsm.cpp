#include "network.fsm.h"
// #include "global.extern.h"

// ----------------------------------------------------------------------------
// Forward declarations
//

class NwState_initialize;
class NwState_connectWiFi;
class NwState_configWiFi;
class NwState_connectMqtt;
class NwState_subscribeMqtt;
class NwState_idle;
class NwState_error;

// ----------------------------------------------------------------------------
// State implementations
//

class NwState_initialize : public NetworkManager
{
  public:
    void
    entry () override
    {
        retainSyncMsg = xSemaphoreCreateBinary ();
        xSemaphoreGive (retainSyncMsg);

        MQTT.setServer (constants::network::mqttHost, constants::network::mqttPort);
        MQTT.setCredentials (constants::network::mqttUsername, constants::network::mqttPassword);

        MQTT.onConnect (onMqttConnect);
        MQTT.onDisconnect (onMqttDisconnect);
        MQTT.onSubscribe (onMqttSubscribe);
        MQTT.onMessage (onMqttMessage);

        WiFi.onEvent (onWiFiEvent);

        transit<NwState_connectWiFi> (
            [=] ()
            {
                pushStatus (nwstatus_wifi_not_found);
            });
    }
};

class NwState_connectWiFi : public NetworkManager
{
  public:
    void
    entry () override
    {
        eepromReadWifiCredentials (ssid, pswd);

        WiFi.mode (WIFI_STA);
        WiFi.begin (ssid.c_str (), pswd.c_str ());

        timerInit ();
        timerStart ();
    }

    void
    react (nwevent_wifi_disconnected const &) override
    {
    }

    void
    react (nwevent_timeout const &) override
    {
        if (++connectionAttempts < constants::network::maxConnectionAttempts)
        {
            WiFi.mode (WIFI_STA);
            WiFi.begin (ssid.c_str (), pswd.c_str ());
            timerStart ();
        }
        else
        {
            transit<NwState_configWiFi> (
                [=]
                {
                    pushStatus (nwstatus_wifi_config);
                });
        }
    }

    void
    react (nwevent_wifi_connected const &) override
    {
        transit<NwState_connectMqtt> (
            [=] ()
            {
                pushStatus (nwstatus_mqtt_not_found);
            });
    }

  private:
    String ssid, pswd;
    uint connectionAttempts { 0 };
};

class NwState_configWiFi : public NetworkManager
{
  public:
    void
    entry () override
    {
        WiFi.mode (WIFI_AP_STA);
        WiFi.beginSmartConfig (SC_TYPE_ESPTOUCH_V2);
    }

    void
    react (nwevent_sc_done const &) override
    {
        transit<NwState_connectMqtt> (
            [=] ()
            {
                pushStatus (nwstatus_mqtt_not_found);
                eepromSaveWifiCredentials (WiFi.SSID (), WiFi.psk ());
            });
    }
};

class NwState_connectMqtt : public NetworkManager
{
  public:
    void
    entry () override
    {
        MQTT.connect ();
        timerInit ();
        timerStart ();
    }

    void
    exit () override
    {
        timerDelete ();
    }

    void
    react (nwevent_timeout const &) override
    {
        if (++connectionAttempts < constants::network::maxConnectionAttempts)
        {
            MQTT.connect ();
            timerStart ();
        }
        else
        {
            transit<NwState_error> (
                [=] ()
                {
                    pushStatus (nwstatus_error);
                });
        }
    }

    void
    react (nwevent_mqtt_connected const &) override
    {
        transit<NwState_subscribeMqtt> ();
    }

  private:
    uint connectionAttempts { 0 };
};

class NwState_subscribeMqtt : public NetworkManager
{
    void
    entry () override
    {
        xSemaphoreTake (retainSyncMsg, portMAX_DELAY);

        MQTT.subscribe (constants::network::mqttMapTopic, constants::network::mqttQos);
        MQTT.subscribe (constants::network::mqttLocationTopic, constants::network::mqttQos);
        MQTT.subscribe (constants::network::mqttMissionTopic, constants::network::mqttQos);
    }

    void
    react (nwevent_mqtt_msg_map const &e) override
    {
        mapMsg.concat (e.payload, e.len);

        if (e.index + e.len != e.total)
        {
            return;
        }
        else
        {
            String cpyMapMsg = String (mapMsg);
            mapMsg.clear ();

            dpQueue.dispatch (
                [=] () mutable
                {
                    xSemaphoreTake (global::storageMapMutex, portMAX_DELAY);
                    global::storageMap.parseMapMsg (cpyMapMsg) && xSemaphoreGive (retainSyncMsg);
                    xSemaphoreGive (global::storageMapMutex);
                });
        }
    }

    void
    react (nwevent_mqtt_msg_lineCount const &e) override
    {
        String locationMsg = String (e.payload, e.len);

        dpQueue.dispatch (
            [=, isValidLineCount { false }, lineCount { 0 }] () mutable
            {
                xSemaphoreTake (retainSyncMsg, portMAX_DELAY);
                xSemaphoreGive (retainSyncMsg);

                xSemaphoreTake (global::storageMapMutex, portMAX_DELAY);
                isValidLineCount = global::storageMap.parseLineCountMsg (lineCount, locationMsg);
                xSemaphoreGive (global::storageMapMutex);

                if (isValidLineCount)
                {
                    xQueueSend (global::currentLineCountQueue, &lineCount, portMAX_DELAY);
                    transit<NwState_idle> (
                        [=] ()
                        {
                            pushStatus (nwstatus_good);
                        });
                }
            });
    }

    void
    react (nwevent_mqtt_disconnected const &) override
    {
        transit<NwState_connectMqtt> (
            [=] ()
            {
                pushStatus (nwstatus_mqtt_not_found);
            });
    }

  private:
    String mapMsg = "";
};

class NwState_idle : public NetworkManager
{
    void
    entry () override
    {
        timerInit (constants::network::trackingInterval);
        timerStart ();
    }

    void
    exit () override
    {
        timerDelete ();
    }

    void
    react (nwevent_mqtt_disconnected const &)
    {
        transit<NwState_connectMqtt> (
            [=] ()
            {
                pushStatus (nwstatus_mqtt_not_found);
            });
    }

    void
    react (nwevent_mqtt_msg_mission const &e) override
    {
        String missionMsg = String (e.payload, e.len);

        dpQueue.dispatch (
            [=, mission { MissionData {} }, tasking { MissionStatus::missionstatus_taking }] () mutable
            {
                if (parseMissionMsg (mission, missionMsg))
                {
                    xQueueSend (global::missionStatusQueue, &tasking, portMAX_DELAY);
                    xQueueSend (global::missionQueue, &mission, portMAX_DELAY);
                }
            });
    }

    void
    react (nwevent_timeout const &) override
    {
        MissionStatus missionStatus;
        if (xQueueReceive (global::missionStatusQueue, &missionStatus, 0) == pdTRUE)
        {
            // Serial.printf ("tracking: %d\n", missionStatus);
        }
        timerStart ();
    }
};

class NwState_error : public NetworkManager
{
};

// ----------------------------------------------------------------------------
// Initial state definition
//

FSM_INITIAL_STATE (NetworkManager, NwState_initialize)

// ----------------------------------------------------------------------------
// Base class implementations
//

AsyncMqttClient NetworkManager::MQTT;
SemaphoreHandle_t NetworkManager::retainSyncMsg;

dispatch_queue NetworkManager::dpQueue (
    constants::network::dpQueueName,
    constants::network::dpQueuethreadCnt,
    constants::network::dpQueueStackDepth);

void
NetworkManager::react (nwevent_wifi_disconnected const &)
{
    transit<NwState_connectWiFi> (
        [=] ()
        {
            pushStatus (nwstatus_wifi_not_found);
        });
}

void
NetworkManager::timerInit (TickType_t period)
{
    _timer = xTimerCreate (
        "NetworkTimer", // timer name
        period,         // period in ticks
        pdTRUE,         // auto reload
        NULL,           // timer id
        onTimeout);     // callback
}

void
NetworkManager::timerStart ()
{
    if (_timer == nullptr)
    {
        return;
    }

    if (!xTimerIsTimerActive (_timer))
    {
        xTimerStart (_timer, portMAX_DELAY);
    }
}

void
NetworkManager::timerStop ()
{
    if (_timer == nullptr)
    {
        return;
    }

    if (xTimerIsTimerActive (_timer))
    {
        xTimerStop (_timer, portMAX_DELAY);
    }
}

void
NetworkManager::timerDelete ()
{

    if (_timer == nullptr)
    {
        return;
    }

    timerStop ();
    xTimerDelete (_timer, portMAX_DELAY);
}

void
NetworkManager::pushStatus (NwStatus status)
{
    dpQueue.dispatch (
        [=] ()
        {
            xSemaphoreTake (global::nwstatusMutex, portMAX_DELAY);
            global::nwstatus = status;
            xSemaphoreGive (global::nwstatusMutex);
        });
}

void
NetworkManager::onTimeout (TimerHandle_t xTimer)
{
    sendNwEvent (nwevent_timeout {});
}

void
NetworkManager::onWiFiEvent (WiFiEvent_t event, arduino_event_info_t eventInfo)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        sendNwEvent (nwevent_wifi_connected {});
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        sendNwEvent (nwevent_wifi_disconnected {});
        break;
    case ARDUINO_EVENT_SC_SEND_ACK_DONE:
        sendNwEvent (nwevent_sc_done {});
        break;
    default:
        break;
    }
}

void
NetworkManager::onMqttConnect (bool sessionPresent)
{
    sendNwEvent (nwevent_mqtt_connected {});
}

void
NetworkManager::onMqttDisconnect (AsyncMqttClientDisconnectReason reason)
{
    sendNwEvent (nwevent_mqtt_disconnected {});
}

void
NetworkManager::onMqttSubscribe (uint16_t packetId, uint8_t qos)
{
}

void
NetworkManager::onMqttPublish (uint16_t packetId)
{
}

void
NetworkManager::onMqttMessage (char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{

    nwevent_mqtt_msg retainEvent;
    retainEvent.len = len;
    retainEvent.index = index;
    retainEvent.total = total;
    retainEvent.payload = payload;

    /* handle mission */
    if (strcmp (constants::network::mqttMissionTopic, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_msg_mission { retainEvent });
        return;
    }

    /* handle retain map */
    if (strcmp (constants::network::mqttMapTopic, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_msg_map { retainEvent });
        return;
    }

    /* handle retain location */
    if (strcmp (constants::network::mqttLocationTopic, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_msg_lineCount { retainEvent });
        return;
    }
}

#include "network.fsm.h"

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
        eeprom::readWifiCredentials (ssid, pswd);

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
                eeprom::saveWifiCredentials (WiFi.SSID (), WiFi.psk ());
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
        MQTT.subscribe (constants::network::mqttAgvInfoTopic, constants::network::mqttQos);
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
            String cpyMapMsg (mapMsg);
            mapMsg.clear ();

            dpQueue.dispatch (
                [=] () mutable
                {
                    xSemaphoreTake (global::mapMutex, portMAX_DELAY);
                    global::map.parseMapMsg (cpyMapMsg) && xSemaphoreGive (retainSyncMsg);
                    xSemaphoreGive (global::mapMutex);
                });
        }
    }

    void
    react (nwevent_mqtt_msg_agvInfo const &e) override
    {
        String agvInfoMsg (e.payload, e.len);

        dpQueue.dispatch (
            [&, agvInfoMsg] () mutable
            {
                xSemaphoreTake (retainSyncMsg, portMAX_DELAY) && xSemaphoreGive (retainSyncMsg);

                transit<NwState_idle> (
                    [=] () mutable
                    {
                        xQueueSend (global::agvInfoQueue, &agvInfo, portMAX_DELAY);
                        pushStatus (nwstatus_good);
                    },
                    [&, agvInfoMsg] () mutable
                    {
                        return Map::parseAgvInfo (agvInfo, agvInfoMsg);
                    });
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
    String mapMsg;
    AGVInfo agvInfo;
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
            [=, mission { Mission() } , tasking { MissionStatus::missionstatus_taking }] () mutable
            {
                if (Mission::parseMissionMsg (mission, missionMsg))
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
            // send mission info
        }

        AGVInfo agvInfo;
        if (xQueueReceive (global::agvInfoQueue, &agvInfo, 0) == pdTRUE)
        {
            String agvInfoSerialized;
            Map::parseAgvInfoToString (agvInfo, agvInfoSerialized);

            MQTT.publish (
                "quang/test",
                constants::network::mqttQos,
                true, agvInfoSerialized.c_str (),
                agvInfoSerialized.length ());
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
    if (strcmp (constants::network::mqttAgvInfoTopic, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_msg_agvInfo { retainEvent });
        return;
    }
}

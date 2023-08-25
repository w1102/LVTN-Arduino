#include "network.fsm.h"
#include "AsyncMqttClient.h"
#include "WiFi.h"
#include "global.extern.h"
/*

// ----------------------------------------------------------------------------
// prototype declarations
// clang-format off
//

/* global variable declarations */
AsyncMqttClient MQTT;
QueueHandle_t nwstatusQueue;

/* forward state declarations */
class NwState_initialize;
class NwState_connectWiFi;
class NwState_configWiFi;
class NwState_connectMqtt;
class NwState_subscribeMqtt;
class NwState_idle;
class NwState_tracking;
class NwState_error;

/* callback declarations */
void onTimeout (TimerHandle_t xTimer);
void onWiFiEvent (WiFiEvent_t event, arduino_event_info_t eventInfo);
void onMqttConnect (bool sessionPresent);
void onMqttDisconnect (AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe (uint16_t packetId, uint8_t qos);
void onMqttPublish (uint16_t packetId);
void onMqttMessage (char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

/* helper declarations */

/* put network status into queue */
inline void putNwStatus (NwStatus status);

// ----------------------------------------------------------------------------
// State: NwState_initialize
// clang-format on
//

class NwState_initialize : public NetworkManager
{
  public:
    void
    entry () override
    {
        nwstatusQueue = xQueueCreate (CONF_GLO_QUEUE_LENGTH, sizeof (NwStatus));
        xSemaphoreTake (missionSync, portMAX_DELAY);

        MQTT.setServer (NW_MQTT_HOST, NW_MQTT_PORT);
        MQTT.setCredentials (NW_MQTT_USERNAME, NW_MQTT_PASSWORD);
        MQTT.onConnect (onMqttConnect);
        MQTT.onDisconnect (onMqttDisconnect);
        MQTT.onSubscribe (onMqttSubscribe);
        MQTT.onMessage (onMqttMessage);

        WiFi.onEvent (onWiFiEvent);

        transit<NwState_connectWiFi> (
            [=] ()
            {
                Serial.println ("connect wifi");
                putNwStatus (nwstatus_wifi_not_found);
            });
    }
};

// ----------------------------------------------------------------------------
// State: NwState_connectWiFi
//

class NwState_connectWiFi : public NetworkManager
{
  public:
    void
    entry () override
    {
        eepromReadWifiCredentials (ssid, pswd);

        WiFi.mode (WIFI_STA);
        WiFi.begin (ssid.c_str (), pswd.c_str ());

        timerInit (onTimeout);
        timerStart ();
    }

    void
    react (nwevent_wifi_disconnected const &) override
    {
    }

    void
    react (nwevent_timeout const &) override
    {
        if (++connectionAttempts < NW_MAX_CONNECTION_ATTEMPTS)
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
                    Serial.println ("config wifi");
                    putNwStatus (nwstatus_wifi_config);
                });
        }
    }

    void
    react (nwevent_wifi_connected const &) override
    {
        transit<NwState_connectMqtt> (
            [=] ()
            { putNwStatus (nwstatus_mqtt_not_found); });
    }

  private:
    String ssid, pswd;
    uint connectionAttempts = 0;
};

// ----------------------------------------------------------------------------
// State: NwState_configWiFi
//

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
                putNwStatus (nwstatus_mqtt_not_found);
                eepromSaveWifiCredentials (WiFi.SSID (), WiFi.psk ());
            });
    }
};

// ----------------------------------------------------------------------------
// State: NwState_connectMqtt
//

class NwState_connectMqtt : public NetworkManager
{
  public:
    void
    entry () override
    {
        timerInit (onTimeout);

        MQTT.connect ();
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
        if (++connectionAttempts < NW_MAX_CONNECTION_ATTEMPTS)
        {
            MQTT.connect ();
            timerStart ();
        }
        else
        {
            transit<NwState_error> (
                [=] ()
                {
                    Serial.println ("error state");
                    putNwStatus (nwstatus_error);
                });
        }
    }

    void
    react (nwevent_mqtt_connected const &) override
    {
        transit<NwState_subscribeMqtt> (
            [=] ()
            {
                Serial.println ("subscribe mqtt");
            });
    }

  private:
    uint connectionAttempts = 0;
};

// ----------------------------------------------------------------------------
// State: NwState_subscribeMqtt
//

class NwState_subscribeMqtt : public NetworkManager
{
    void
    entry () override
    {
        TickType_t waitMapRetainMs = pdMS_TO_TICKS (500);
        timerInit (onTimeout, waitMapRetainMs);

        subscription ();
    }

    void
    exit () override
    {
        timerDelete ();
    }

    void
    react (nwevent_mqtt_msg_map const &e) override
    {
        fullPayload += String (e.payload, e.len);

        if (e.index + e.len != e.total)
        {
            return;
        }

        xSemaphoreTake (storageMapMutex, portMAX_DELAY);
        bool isValidMap = storageMap.parseMapMsg (fullPayload);
        xSemaphoreGive (storageMapMutex);

        if (isValidMap)
        {
            isMapRetained = true;
            transitWhenRetained ();
        }

        fullPayload = "";
    }

    void
    react (nwevent_mqtt_msg_lineCount const &e) override
    {
        locationPayload = String (e.payload, e.len);

        if (!isMapRetained)
        {
            timerStart ();
        }
        else
        {
            retainLocation ();
        }
    }

    void
    react (nwevent_timeout const &) override
    {
        if (!isMapRetained)
        {
            timerStart ();
        }
        else
        {
            retainLocation ();
        }
    }

    void
    react (nwevent_mqtt_disconnected const &) override
    {
        transit<NwState_connectMqtt> ();
    }

  private:
    bool isMapRetained = false;
    bool isLocationRetained = false;
    String fullPayload = "";
    String locationPayload = "";

    void
    retainLocation ()
    {
        xSemaphoreTake (storageMapMutex, portMAX_DELAY);
        int currentLineCount = storageMap.parseLineCountMsg (locationPayload);
        xSemaphoreGive (storageMapMutex);

        xQueueSend (currentLineCountQueue, &currentLineCount, portMAX_DELAY);

        isLocationRetained = true;
        transitWhenRetained ();
    }

    void
    transitWhenRetained ()
    {
        if (isMapRetained && isLocationRetained)
        {
            transit<NwState_idle> ();
        }
    }

    void
    subscription ()
    {
        MQTT.subscribe (NW_MQTT_TOPICNAME_MAP, NW_MQTT_TOPIC_QOS);
        MQTT.subscribe (NW_MQTT_TOPICNAME_BOTLOCATION, NW_MQTT_TOPIC_QOS);
        MQTT.subscribe (NW_MQTT_TOPICNAME_MISSION, NW_MQTT_TOPIC_QOS);
    }
};

// ----------------------------------------------------------------------------
// State: NwState_idle
//

class NwState_idle : public NetworkManager
{
    void
    entry () override
    {
        putNwStatus (nwstatus_good);
    }

    void
    react (nwevent_mqtt_disconnected const &)
    {
        transit<NwState_connectMqtt> ();
    }

    void
    react (nwevent_mqtt_msg_mission const &e) override
    {
        String payload = String (e.payload, e.len);

        MissionData mission;
        if (parseMissionMsg (mission, payload))
        {
            xQueueSend (missionQueue, &mission, 0);
        }

        // xSemaphoreTake (missionMutex, portMAX_DELAY);
        // bool isMissionValid = mission.parseMissionMsg (payload);
        // xSemaphoreGive (missionMutex);

        // transit<NwState_tracking> (
        //     [=] ()
        //     {
        //         xSemaphoreGive (missionSync);

        //         MissionStatus tasking = missionstatus_taking;
        //         xQueueSend (missionStatusQueue, &tasking, portMAX_DELAY);
        //     },
        //     [=] () -> bool
        //     { return isMissionValid; });
    }
};

// ----------------------------------------------------------------------------
// State: NwState_tracking
//

class NwState_tracking : public NetworkManager
{
  public:
    void
    entry () override
    {
        timerInit (onTimeout, pdMS_TO_TICKS (CONF_NWFSM_TRACKING_INTERVAL_MS));
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
        xSemaphoreTake (missionSync, 0);

        MissionStatus missionStatus;

        if (xQueueReceive (missionStatusQueue, &missionStatus, 0) == pdTRUE)
        {
            transit<NwState_idle> ([=] () {},
                                   [=] () -> bool
                                   { return missionStatus == missionstatus_done; });
        }
        else
        {
            timerStart ();
        }
    }

    void
    react (nwevent_mqtt_disconnected const &) override
    {
        transit<NwState_connectMqtt> (
            [=] ()
            { putNwStatus (nwstatus_mqtt_not_found); });
    }
};

// ----------------------------------------------------------------------------
// State: NwState_error
//

class NwState_error : public NetworkManager
{
};

// ----------------------------------------------------------------------------
// Base state: default implementations
//

void
NetworkManager::react (nwevent_wifi_disconnected const &)
{
    transit<NwState_connectWiFi> (
        [=] ()
        { putNwStatus (nwstatus_wifi_not_found); });
}

void
NetworkManager::timerInit (TimerCallbackFunction_t cb, TickType_t period)
{
    _timer = xTimerCreate (
        "NetworkManager timeout timer", // timer name
        period,                         // period in ticks
        pdTRUE,                         // auto reload
        NULL,                           // timer id
        cb);                            // callback
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

// ----------------------------------------------------------------------------
// Initial state definition
//

FSM_INITIAL_STATE (NetworkManager, NwState_initialize)

// ----------------------------------------------------------------------------
// Helper definition
//

void
nwStatusLoop ()
{
    NwStatus currentStatus;

    /*  wait for data to become available in the queue  */
    if (xQueueReceive (nwstatusQueue, &currentStatus, portMAX_DELAY) == pdFALSE)
    {
        return;
    }

    xSemaphoreTake (nwstatusMutex, portMAX_DELAY);
    nwstatus = currentStatus;
    xSemaphoreGive (nwstatusMutex);
}

inline void
putNwStatus (NwStatus status)
{
    xQueueSend (nwstatusQueue, &status, 0);
}

// ----------------------------------------------------------------------------
// callback definition
//

void
onTimeout (TimerHandle_t xTimer)
{
    sendNwEvent (nwevent_timeout {});
}

void
onWiFiEvent (WiFiEvent_t event, arduino_event_info_t eventInfo)
{
    Serial.print ("Wifi event: ");
    Serial.println (event);

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
onMqttConnect (bool sessionPresent)
{
    sendNwEvent (nwevent_mqtt_connected {});
}

void
onMqttDisconnect (AsyncMqttClientDisconnectReason reason)
{
    sendNwEvent (nwevent_mqtt_disconnected {});
}

void
onMqttSubscribe (uint16_t packetId, uint8_t qos)
{
}

void
onMqttPublish (uint16_t packetId)
{
}

void
onMqttMessage (char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{

    nwevent_mqtt_msg retainEvent;
    retainEvent.len = len;
    retainEvent.index = index;
    retainEvent.total = total;
    retainEvent.payload = payload;

    /* handle mission */
    if (strcmp (NW_MQTT_TOPICNAME_MISSION, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_msg_mission { retainEvent });
        return;
    }

    /* handle retain map */
    if (strcmp (NW_MQTT_TOPICNAME_MAP, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_msg_map { retainEvent });
        return;
    }

    /* handle retain location */
    if (strcmp (NW_MQTT_TOPICNAME_BOTLOCATION, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_msg_lineCount { retainEvent });
        return;
    }
}

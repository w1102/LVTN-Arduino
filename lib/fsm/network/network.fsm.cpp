#include "network.fsm.h"
#include "AsyncMqttClient.h"
#include "WiFi.h"
#include "global.extern.h"
/*
Implementing a state machine to manage WiFi and MQTT connections

Using the `TinyFSM` library to implement a Mealy state machine
Link: https://github.com/digint/tinyfsm

Using the `Async MQTT` library to implement an asynchronous MQTT client
Link: https://github.com/marvinroger/async-mqtt-client
*/

// ----------------------------------------------------------------------------
// prototype declarations
// clang-format off
// 

/* global variable declarations */
AsyncMqttClient MQTT;
QueueHandle_t nwstatusQueue;

/* state declarations */
class NwState_initialize;
class NwState_connectWiFi;
class NwState_configWiFi;
class NwState_connectMqtt;
class NwState_subscribeMqtt;
class NwState_idle;
class NwState_tracking;
class NwState_error;


/* callback declarations */
void onTimeout(TimerHandle_t xTimer);
void onWiFiEvent(WiFiEvent_t event, arduino_event_info_t eventInfo);
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe(uint16_t packetId, uint8_t qos);
void onMqttPublish(uint16_t packetId);
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

/* helper declarations */

/* put network status into queue */
inline void putNwStatus(NwStatus status);

// ----------------------------------------------------------------------------
// State: NwState_initialize
// clang-format on
// initialize WiFi, MQTT and switch to NwState_connectWiFi

class NwState_initialize : public NetworkManager
{
  public:
    void
    entry () override
    {
        /* network status queue init */
        const uint nwstatusMaxStoredItems = 5;
        nwstatusQueue = xQueueCreate (nwstatusMaxStoredItems, sizeof (NwStatus));

        /* mqtt init */
        /* NW_MQTT_HOST, NW_MQTT_PORT, ... must be read from EEPROM */
        /* and it can be configurable by user by webserver or smartconfig(esptouch-v2) */
        /* but not implemented yet */
        MQTT.setServer (NW_MQTT_HOST, NW_MQTT_PORT);
        MQTT.setCredentials (NW_MQTT_USERNAME, NW_MQTT_PASSWORD);

        /* handle mqtt event */
        MQTT.onConnect (onMqttConnect);
        MQTT.onDisconnect (onMqttDisconnect);
        MQTT.onSubscribe (onMqttSubscribe);
        MQTT.onMessage (onMqttMessage);

        /* handle wifi event */
        WiFi.onEvent (onWiFiEvent);

        transit<NwState_connectWiFi> ();
    }
};

// ----------------------------------------------------------------------------
// State: NwState_connectWiFi
//
// Perform WiFi connection, will attempt to connect 3 times with 3-second interval each time
// If the connection is successful, switch to NwState_connectMqtt
// If failed to connect for more than 3 attempts, switch to NwState_configWiFi

class NwState_connectWiFi : public NetworkManager
{
  public:
    NwState_connectWiFi () : connectionAttempts { 0 } {}

    void
    entry () override
    {
        Serial.println ("connect wifi");

        putNwStatus (nwstatus_wifi_not_found);
        eepromReadWifiCredentials (ssid, pswd);
        timerInit (onTimeout);

        WiFi.mode (WIFI_STA);
        WiFi.begin (ssid.c_str (), pswd.c_str ());

        timerStart ();
    }

    void
    exit () override
    {
        timerDelete ();
    }

    void
    react (nwevent_wifi_disconnected const &) override
    {
        /* disable nwevent_wifi_disconnected event */
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
            transit<NwState_configWiFi> ();
        }
    }

    void
    react (nwevent_wifi_connected const &) override
    {
        transit<NwState_connectMqtt> ();
    }

  private:
    String ssid, pswd;
    uint connectionAttempts = 0;
};

// ----------------------------------------------------------------------------
// State: NwState_configWiFi
//
// Perform smartconfig
// When the cellphone sends login information, it will be saved to EEPROM
// When successfully connected to WiFi based on the information above, switch to NwState_connectMqtt
// In the future, need to implement additional functionality to restart smartconfig when failed to connect with the login information above

class NwState_configWiFi : public NetworkManager
{
  public:
    void
    entry () override
    {
        Serial.println ("config wifi");

        putNwStatus (nwstatus_wifi_config);

        WiFi.mode (WIFI_AP_STA);
        WiFi.beginSmartConfig (SC_TYPE_ESPTOUCH_V2);
    }

    void
    react (nwevent_sc_got_credentials const &credentials) override
    {
        eepromSaveWifiCredentials (
            String (credentials.ssid),
            String (credentials.pswd));
    }

    void
    react (nwevent_sc_done const &) override
    {
        transit<NwState_connectMqtt> ();
    }

  private:
};

// ----------------------------------------------------------------------------
// State: NwState_connectMqtt
//
// Perform MQTT connection, will attempt to connect 3 times with 3-second interval each time
// If the connection is successful, switch to NwState_subscribeMqtt
// If failed to connect for more than 3 attempts, switch to NwState_error

class NwState_connectMqtt : public NetworkManager
{
  public:
    NwState_connectMqtt () : connectionAttempts { 0 } {}

    void
    entry () override
    {
        Serial.println ("connect mqtt");

        putNwStatus (nwstatus_mqtt_not_found);
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
            transit<NwState_error> ();
        }
    }

    void
    react (nwevent_mqtt_connected const &) override
    {
        transit<NwState_subscribeMqtt> ();
    }

  private:
    uint connectionAttempts;
};

// ----------------------------------------------------------------------------
// State: NwState_subscribeMqtt
//
// Perform MQTT topic subscription
// If subscription is successful, switch to NwState_idle
// Note: Currently, if subscribinga non-existent topic, it also switches to NwState_idle. This needs to be reconsidered in the future.

class NwState_subscribeMqtt : public NetworkManager
{
    void
    entry () override
    {
        Serial.println ("subscribe mqtt");

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
    react (nwevent_mqtt_retain_map const &e) override
    {
        fullPayload += String (e.payload, e.len);

        if (e.index + e.len == e.total)
        {
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
    }

    void
    react (nwevent_mqtt_retain_location const &e) override
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
        /* mqtt subscription */
        /* NW_MQTT_TOPICNAME_BOTLOCATION, NW_MQTT_TOPIC_QOS, ... must be read from EEPROM */
        /* and it can be configurable by user by webserver or smartconfig(esptouch-v2) */
        /* but not implemented yet */
        MQTT.subscribe (NW_MQTT_TOPICNAME_BOTLOCATION, NW_MQTT_TOPIC_QOS);
        MQTT.subscribe (NW_MQTT_TOPICNAME_MAP, NW_MQTT_TOPIC_QOS);
        MQTT.subscribe (NW_MQTT_TOPICNAME_MISSION, NW_MQTT_TOPIC_QOS);
    }
};

// ----------------------------------------------------------------------------
// State: NwState_idle
//
// Idle state, when all states have been completed

class NwState_idle : public NetworkManager
{
    void
    entry () override
    {
        Serial.println ("idle state");
        putNwStatus (nwstatus_good);
    }

    void
    react (nwevent_mqtt_disconnected const &)
    {
        transit<NwState_connectMqtt> ();
    }

    void
    react (nwevent_mqtt_take_mission const &e) override
    {
        String payload = String (e.payload, e.len);

        xSemaphoreTake (missionMutex, portMAX_DELAY);
        bool isMissionValid = mission.parseMission (payload);
        if (isMissionValid)
        {
            mission.putStatus (MissionStatus::missionstatus_taking);
        }
        xSemaphoreGive (missionMutex);

        if (isMissionValid)
        {
            transit<NwState_tracking> ();
        }
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
        xSemaphoreTake (missionMutex, portMAX_DELAY);

        if (!mission.isStatusQueueEmpty ())
        {
            MissionStatus status = mission.popStatus ();
            xSemaphoreGive (missionMutex);
            // must implement push status into mqtt
            // ...
        }
        else if (mission.getLastStatus () == MissionStatus::missionstatus_done)
        {
            mission.clean ();
            xSemaphoreGive (missionMutex);
            transit<NwState_idle> ();
        }
        else
        {
            xSemaphoreGive (missionMutex);
        }

        timerStart ();
    }

    void
    react (nwevent_mqtt_disconnected const &) override
    {
        transit<NwState_connectMqtt> ();
    }
};

// ----------------------------------------------------------------------------
// State: NwState_error
//
// Handling errors if they occur
// This state has not been implemented yet.
// It is assumed that if an error occurs, it will be stuck here.

class NwState_error : public NetworkManager
{
    void
    entry () override
    {
        Serial.println ("error state");

        putNwStatus (nwstatus_error);
    }
};

// ----------------------------------------------------------------------------
// Base state: default implementations
//

void
NetworkManager::react (nwevent_wifi_disconnected const &)
{
    transit<NwState_connectWiFi> ();
}

void
NetworkManager::timerInit (TimerCallbackFunction_t cb, TickType_t period)
{
    int id = 145;
    this->_timer = xTimerCreate (
        "NetworkManager timeout timer", // timer name
        period,                         // period in ticks
        pdFALSE,                        // auto reload
        (void *)id,                     // timer id
        cb);                            // callback
}

void
NetworkManager::timerStart ()
{

    if (this->_timer == nullptr)
    {
        return;
    }

    if (!xTimerIsTimerActive (this->_timer))
    {
        xTimerStart (this->_timer, 0);
    }
}

void
NetworkManager::timerStop ()
{
    if (this->_timer == nullptr)
    {
        return;
    }

    if (xTimerIsTimerActive (this->_timer))
    {
        xTimerStop (this->_timer, 0);
    }
}

void
NetworkManager::timerDelete ()
{

    if (this->_timer == nullptr)
    {
        return;
    }

    timerStop ();
    xTimerDelete (this->_timer, 0);
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
    case ARDUINO_EVENT_SC_GOT_SSID_PSWD:
    {
        nwevent_sc_got_credentials credentials {};

        memcpy (credentials.ssid, eventInfo.sc_got_ssid_pswd.ssid, 32);
        memcpy (credentials.pswd, eventInfo.sc_got_ssid_pswd.password, 32);

        sendNwEvent (credentials);
    }
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

    nwevent_mqtt_retain retainEvent;
    retainEvent.len = len;
    retainEvent.index = index;
    retainEvent.total = total;
    retainEvent.payload = payload;

    /* handle mission */
    if (strcmp (NW_MQTT_TOPICNAME_MISSION, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_take_mission { retainEvent });
        return;
    }

    /* handle retain map */
    if (strcmp (NW_MQTT_TOPICNAME_MAP, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_retain_map { retainEvent });
        return;
    }

    /* handle retain location */
    if (strcmp (NW_MQTT_TOPICNAME_BOTLOCATION, topic) == 0)
    {
        sendNwEvent (nwevent_mqtt_retain_location { retainEvent });
        return;
    }
}

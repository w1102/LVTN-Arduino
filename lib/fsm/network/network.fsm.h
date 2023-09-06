#ifndef NETWORK_FSM_H_
#define NETWORK_FSM_H_

#include "AsyncMqttClient.h"
#include "WiFi.h"
#include "constants.h"
#include "dispatchqueue.h"
#include "eepromrw.h"
#include "global.h"
#include "gpio.h"
#include "main.fsm.h"
#include "tinyfsm.hpp"
#include <Arduino.h>

// ----------------------------------------------------------------------------
// Event declarations
// clang-format off
//

struct nwevent : tinyfsm::Event {};

struct nwevent_timeout : nwevent {};

struct nwevent_wifi_connected : nwevent {};
struct nwevent_wifi_disconnected : nwevent {};

struct nwevent_sc_done : nwevent {};

struct nwevent_mqtt_connected: nwevent {};
struct nwevent_mqtt_disconnected: nwevent {};

struct nwevent_mqtt_msg: nwevent {
    char *payload;
    size_t len, index, total;  
};

struct nwevent_mqtt_msg_map: nwevent_mqtt_msg {};
struct nwevent_mqtt_msg_lineCount: nwevent_mqtt_msg {};
struct nwevent_mqtt_msg_mission: nwevent_mqtt_msg {};


// ----------------------------------------------------------------------------
// NetworkManager (FSM base class) declaration
// clang-format on
//

class NetworkManager : public tinyfsm::Fsm<NetworkManager>
{
  public:
    NetworkManager () : _timer { nullptr } {};

    /* default reaction for unhandled events */
    void react (tinyfsm::Event const &) {};

    /* default reaction for nwevent_wifi_disconnected events */
    virtual void react (nwevent_wifi_disconnected const &);

    /* react events in some states */
    virtual void react (nwevent_timeout const &) {};
    virtual void react (nwevent_wifi_connected const &) {};
    virtual void react (nwevent_sc_done const &) {};
    virtual void react (nwevent_mqtt_disconnected const &) {};
    virtual void react (nwevent_mqtt_connected const &) {};
    virtual void react (nwevent_mqtt_msg_map const &) {};
    virtual void react (nwevent_mqtt_msg_lineCount const &) {};
    virtual void react (nwevent_mqtt_msg_mission const &) {};

    /* entry actions in some states */
    virtual void entry (void) {};

    /* exit actions in some states */
    virtual void exit (void) {};

  private:
    TimerHandle_t _timer;

  protected:
    static AsyncMqttClient MQTT;
    static SemaphoreHandle_t retainSyncMsg;
    static dispatch_queue dpQueue;

    static void onTimeout (TimerHandle_t xTimer);
    static void onWiFiEvent (WiFiEvent_t event, arduino_event_info_t eventInfo);
    static void onMqttConnect (bool sessionPresent);
    static void onMqttDisconnect (AsyncMqttClientDisconnectReason reason);
    static void onMqttSubscribe (uint16_t packetId, uint8_t qos);
    static void onMqttPublish (uint16_t packetId);
    static void onMqttMessage (char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

    void timerInit (TickType_t period = constants::network::connectTimeoutMs);
    void timerStart ();
    void timerStop ();
    void timerDelete ();

    void pushStatus (NwStatus status);
};

using NetworkManagerFSM = tinyfsm::Fsm<NetworkManager>;

// ----------------------------------------------------------------------------
// helper declaration
//

/* Send a network event into NetworkManager state machine */
template <typename E>
void
sendNwEvent (E const &event)
{
    NetworkManagerFSM::dispatch<E> (event);
}

#endif // NETWORK_FSM_H_

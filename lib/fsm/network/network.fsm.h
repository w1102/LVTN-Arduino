#ifndef NETWORK_FSM_H_
#define NETWORK_FSM_H_

/*
Implementing a state machine to manage WiFi and MQTT connections

Using the `TinyFSM` library to implement a Mealy state machine
Link: https://github.com/digint/tinyfsm

Using the `Async MQTT` library to implement an asynchronous MQTT client
Link: https://github.com/marvinroger/async-mqtt-client
*/

#define NW_CONNECTION_TIMEOUT_IN_MS 3000
#define NW_MAX_CONNECTION_ATTEMPTS 3

#define NW_MQTT_HOST "mqtt.tranquang.net"
#define NW_MQTT_PORT 1883
#define NW_MQTT_USERNAME "quang"
#define NW_MQTT_PASSWORD "1102"

#define NW_MQTT_TOPIC_QOS 0
#define NW_MQTT_TOPICNAME_MAP "quang/logistics/map"
#define NW_MQTT_TOPICNAME_BOTLOCATION "quang/logistics/botlocation"
#define NW_MQTT_TOPICNAME_MISSION "quang/logistics/mission"

#include "eepromrw.h"
#include "gpio.h"
#include "helper.h"
#include "main.fsm.h"
#include "tinyfsm.hpp"
#include <Arduino.h>
#include "trigger.h"

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
    // virtual void react(nwevent_sc_got_credentials const &) {};
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
    void timerInit (
        TimerCallbackFunction_t cb,
        TickType_t period = pdMS_TO_TICKS (NW_CONNECTION_TIMEOUT_IN_MS));
    void timerStart ();
    void timerStop ();
    void timerDelete ();
};

using NetworkManagerFSM = tinyfsm::Fsm<NetworkManager>;

// ----------------------------------------------------------------------------
// helper declaration
//

/* send a network event into NetworkManager state machine */
template <typename E>
void
sendNwEvent (E const &event)
{
    NetworkManagerFSM::dispatch<E> (event);
}

/* polling this function to perform network status update */
void nwStatusLoop ();

#endif // NETWORK_FSM_H_

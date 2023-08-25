#ifndef CONFIG_H_
#define CONFIG_H_

// ----------------------------------------------------------------------------
// Global configuration
//
#define CONF_GLO_QUEUE_LENGTH 10

// ----------------------------------------------------------------------------
// PID configuration
//
#define KP 100
#define KI 0
#define KD 0

// ----------------------------------------------------------------------------
// Main task configuration
//

#define CONF_MAINTASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 12000)
#define CONF_MAINTASK_PRIORITY (configMAX_PRIORITIES - 1)
#define CONF_MAINTASK_RUN_CORE (0)
#define CONF_MAINTASK_NAME "MainTask"

// ----------------------------------------------------------------------------
// Main status configuration
//
#define CONF_MAINSTATUS_STACK_SIZE (configMINIMAL_STACK_SIZE)
#define CONF_MAINSTATUS_PRIORITY (1)
#define CONF_MAINSTATUS_RUN_CORE (ARDUINO_RUNNING_CORE)
#define CONF_MAINSTATUS_NAME "mainStatusTask"

#define CONF_MAINSTATUS_DELAY_MS 500
#define CONF_MAINSTATUS_INIT_BLINKTIMES 1
#define CONF_MAINSTATUS_IDLE_BLINKTIMES 1
#define CONF_MAINSTATUS_RUN_BLINKTIMES 2
#define CONF_NWSTATUSTASK_UNDEFINE_ERROR_BLINKTIMES 1

// ----------------------------------------------------------------------------
// Network task configuration
//

#define CONF_NWTASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 6000)
#define CONF_NWTASK_PRIORITY (5)
#define CONF_NWTASK_RUN_CORE (ARDUINO_RUNNING_CORE)
#define CONF_NWSTASK_NAME "NetworkTask"

// ----------------------------------------------------------------------------
// Network status task configuration
//

#define CONF_NWSTATUSTASK_STACK_SIZE (configMINIMAL_STACK_SIZE)
#define CONF_NWSTATUSTASK_PRIORITY (1)
#define CONF_NWSTATUSTASK_RUN_CORE (ARDUINO_RUNNING_CORE)
#define CONF_NWSTATUSTASK_NAME "NetworkStatusTask"

#define CONF_NWSTATUSTASK_DELAY_MS 500
#define CONF_NWSTATUSTASK_WIFI_CONNECT_BLINKTIMES 1
#define CONF_NWSTATUSTASK_WIFI_CONFIG_BLINKTIMES 2
#define CONF_NWSTATUSTASK_MQTT_CONNECT_BLINKTIMES 3
#define CONF_NWSTATUSTASK_UNDEFINE_ERROR_BLINKTIMES 1


// ----------------------------------------------------------------------------
// Network FSM configuration
//

#define CONF_NWFSM_TRACKING_INTERVAL_MS 500

// ----------------------------------------------------------------------------
// Main FSM configuration
//

#define CONF_MAINFSM_INTERVAL_MS 5
#define CONF_MAINFSM_QUEUE_LENGTH 10
#define CONF_MAINFSM_LOW_SPEED 550
#define CONF_MAINFSM_MID_SPEED 750
#define CONF_MAINFSM_HIGH_SPEED 900
#define CONF_MAINFSM_TURN_WAIT 25


#endif // CONFIG_H_

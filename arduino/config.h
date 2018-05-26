#ifndef __CONFIG_H__
#define __CONFIG_H__

#define THIS_ID              "5af1ae79d10de93c0ae652d9"

// Pin configuration
#define PIN_BT_SERIAL_RX     2 // This pin has to be connected to the TX pin of the bluetooth ic
#define PIN_BT_SERIAL_TX     3 // This pin has to be connected to the RX pin of the bluetooth ic
#define PIN_SONIC_ECHO       8
#define PIN_SONIC_TRIGGER    9
#define PIN_SWITCH           10
#define SONIC_MAX_DISTANCE   250

// Timeouts
#define LID_CLOSED_TIMEOUT   5000

#endif

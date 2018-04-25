#ifndef __CONFIG_H__
#define __CONFIG_H__

// Serial ID of the Arduino (for identification).
// Max 7 bits.
#define SERIAL_ID            42

// Address of the sink for communication.
#define SINK_ADDRESS         ((1 << 8) - 1)

// Pin configuration
#define PIN_RECEIVER         2
#define PIN_TRANSMITTER      8
#define PIN_SONIC_ECHO       9
#define PIN_SONIC_TRIGGER    10
#define PIN_SWITCH           11
#define SONIC_MAX_DISTANCE   200

#endif

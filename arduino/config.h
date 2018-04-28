#ifndef __CONFIG_H__
#define __CONFIG_H__

// Common addresses
#define THIS_ADDRESS         42
#define BROADCAST_ADDRESS    ((1 << 8) - 1)

// Pin configuration
#define PIN_RECEIVER         2
#define PIN_TRANSMITTER      8
#define PIN_SONIC_ECHO       9
#define PIN_SONIC_TRIGGER    10
#define PIN_SWITCH           11
#define SONIC_MAX_DISTANCE   200

#endif

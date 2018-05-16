#include <NewPing.h>
#include <SoftwareSerial.h>

#include "config.h"

SoftwareSerial bt(PIN_BT_SERIAL_RX, PIN_BT_SERIAL_TX);
NewPing sonar(PIN_SONIC_TRIGGER, PIN_SONIC_ECHO);

bool perform_measurement(unsigned long* val) {

    // Wait for the lid to close
    Serial.print("Waiting for lid closure...\n");
    bool timeout = true;
    unsigned long start = millis();
    while (millis() - start <= LID_CLOSED_TIMEOUT) {
        if (digitalRead(PIN_SWITCH) == HIGH) {
            timeout = false;
            break;
        }
    }
    
    if (timeout) {
        Serial.print("Timeout.\n");
        return false;
    } else {
    
        // Perform 3 measurements and take the avg
        unsigned long m1 = sonar.ping_cm();
        unsigned long m2 = sonar.ping_cm();
        unsigned long m3 = sonar.ping_cm();
        unsigned long m = (m1 + m2 + m3) / 3;
    
        Serial.print("Measured value: ");
        Serial.print(m);
        Serial.print("\n");
    
        *val = m;
        return true;
        
    }
}

void setup() {
    Serial.begin(9600);
    bt.begin(9600);
    Serial.print("Initialized.\n");
}

void loop() {
    if (bt.available() > 0) {
        int command = bt.read();

        unsigned long m = 0;
        switch (command) {

            case 0:
                Serial.print("Name request. Sending " THIS_ID ".\n");
                bt.print(THIS_ID);
                break;

            case 1:
                Serial.print("Measurement request.\n");
                if (perform_measurement(&m)) {
                    bt.write((unsigned char) m);
                } else {
                    bt.write((unsigned char) -1);
                }
                break;

            default:
                // Ignore any invalid command
                Serial.print("Invalid command.\n");
                break;

        }
    }
}

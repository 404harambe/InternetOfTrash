#include <NewPing.h>

#include "config.h"
#include "NetworkProtocol.h"
#include "util.h"

#define LID_CLOSED_TIMEOUT 5000

NetworkProtocol protocol(THIS_ADDRESS, digitalPinToInterrupt(PIN_RECEIVER), PIN_TRANSMITTER);
NewPing sonar(PIN_SONIC_TRIGGER, PIN_SONIC_ECHO);

class PerformMeasurementCallback : public NetworkProtocol::MessageCallback {
public:
    void Run(NetworkProtocol::Message* msg) override {
    
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
            msg->SetType(NetworkProtocol::MessageType::ErrorLid);
            Serial.print("Timeout.\n");
        } else {
      
            // Perform 3 measurements and take the avg
            unsigned long m1 = sonar.ping_cm();
            unsigned long m2 = sonar.ping_cm();
            unsigned long m3 = sonar.ping_cm();
            unsigned long m = (m1 + m2 + m3) / 3;
      
            Serial.print("Measured value: ");
            Serial.print(m);
            Serial.print("\n");
      
            // Fill the measurement
            msg->SetType(NetworkProtocol::MessageType::Success);
            msg->SetMeasurement(m);
            
        }
    }
};

PerformMeasurementCallback performMeasurement;

void setup() {
  Serial.begin(9600);
  protocol.SetMeasurementCallback(&performMeasurement);

  Serial.print("Initialized.\n");
}

void loop() {
  protocol.Loop();
}

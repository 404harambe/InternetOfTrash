#include <NewPing.h>

#include "config.h"
#include "NetworkProtocol.h"
#include "util.h"

#define LID_CLOSED_TIMEOUT 10000
#define SEND_RETRIES 3

NetworkProtocol protocol(THIS_ADDRESS, digitalPinToInterrupt(PIN_RECEIVER), PIN_TRANSMITTER);
NewPing sonar(PIN_SONIC_TRIGGER, PIN_SONIC_ECHO);

static bool PacketFilter(NetworkProtocol* protocol, NetworkProtocol::Packet* packet) {
    // Receive both packets directed to this node and to the special broadcast address.
    return packet->dest == protocol->Address() || packet->dest == BROADCAST_ADDRESS;
}

void setup() {
  Serial.begin(9600);
  protocol.SetPacketFilter(PacketFilter);
}

void loop() {

    // Check if new data is available for us
    NetworkProtocol::Message* msg = protocol.Receive();
    if (msg != NULL) {

        // We don't actually care about the contents of the message,
        // since we only can do one thing: take a measurement.
        Serial.print("Message received.\n");
        addr_t sender = msg->Source();
        msg->Free();

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
        } else {
    
            // Perform 3 measurements and take the avg
            unsigned long m1 = sonar.ping_cm();
            unsigned long m2 = sonar.ping_cm();
            unsigned long m3 = sonar.ping_cm();
            unsigned long m = (m1 + m2 + m3) / 3;

            Serial.print("Measured value: ");
            Serial.print(m);
            Serial.print("\n");

            Serial.print("Sending message...\n");

            // Send the measurement back to the sender
            unsigned long msg = htonl(m);
            for (int retries = SEND_RETRIES; retries > 0; retries--) {
                if (protocol.Send(sender, (const unsigned char*) &msg, sizeof(msg))) {
                    Serial.print("Success!\n");
                    break;
                } else {
                    Serial.print("Failed.\n");
                }
                Serial.print("Trying again...\n");
            }
        }

        Serial.print("Waiting for another incoming message...\n");

    }

}

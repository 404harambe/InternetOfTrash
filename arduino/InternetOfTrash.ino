#include "config.h"
#include "NetworkProtocol.h"

NetworkProtocol protocol(SERIAL_ID, PIN_RECEIVER, PIN_TRANSMITTER);

void setup() {
  Serial.begin(9600);
}

void loop() {

  // Check if new data is available for us
  NetworkProtocol::Message* msg = NULL;
  if (protocol.Receive(&msg)) {
    Serial.print("Data received: ");
    Serial.print((const char*) msg->Contents());
    Serial.print(" from #");
    Serial.print(msg->Source());
    Serial.print("\n");
    msg->Free();
  }

  // Send a message to the sink
  protocol.Send(SINK_ADDRESS, (const unsigned char*) "HELLO", 6);
  Serial.print("Message sent.\n");

  delay(1);
}

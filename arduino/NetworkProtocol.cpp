#define NETWORK_PROTOCOL_DEBUG
#include "NetworkProtocol.h"


NetworkProtocol::NetworkProtocol(addr_t myself, unsigned int rxPin, unsigned int txPin)
    : _myself(myself)
{   
    // Configure the RCSwitches for both tx and rx
    _rx.enableReceive(rxPin);
    _rx.setProtocol(0);
    _tx.enableTransmit(txPin);
    _tx.setProtocol(0);
    _tx.setRepeatTransmit(10);
}

void NetworkProtocol::RequestMeasurement(MessageCallback* cb) {

    // Send the broadcast message
    Message msg(_myself, NetworkProtocol::BroadcastAddress, 0, MessageType::Request);
    Transmit(msg);

    // Wait for incoming messages
    unsigned long start = millis();
    do {
        if (ReadIncomingMessage(msg)) {
            // We expect only a response, either positive or negative
            if (msg.IsError() || msg.Type() == MessageType::Success) {
                TransmitAck(msg);
                cb->Run(&msg);
            }
        }
    } while (millis() - start < _requestTimeout);

}

void NetworkProtocol::RequestMeasurement(addr_t dest, MessageCallback* cb) {

    // Send the message
    Message msg(_myself, dest, 0, MessageType::Request);
    Transmit(msg);

    // Wait for incoming messages
    unsigned long start = millis();
    do {
        if (ReadIncomingMessage(msg)) {
            // We expect only a response, either positive or negative,
            // and only from the specific destination
            if (msg.Source() == dest && (msg.IsError() || msg.Type() == MessageType::Success)) {
                TransmitAck(msg);
                cb->Run(&msg);
                break;
            }
        }
    } while (millis() - start < _requestTimeout);

}

void NetworkProtocol::Loop() {

    // Check that the user actually provided the callback for the measurement
    if (_measurementCallback == nullptr) {
        log("Missing measurement callback. Please call SetMeasurementCallback during setup.");
        return;
    }
    
    // Check if there's a request for us
    Message msg;
    if (ReadIncomingMessage(msg) && msg.Type() == MessageType::Request) {
        
        // Perform the measurement
        Message res(_myself, msg.Source(), 0, MessageType::Request);
        _measurementCallback->Run(&res);

        // Check that the callback set the appropriate type
        if (!res.IsError() && res.Type() != MessageType::Success) {
            log("Invalid message type set by measurement callback. Dropping message.");
            return;
        }

        // Send the response every second until we receive the ack
        unsigned long start = millis();
        do {
            Transmit(res);

            // Wait one second for the ack
            unsigned long start2 = millis();
            do {
                if (ReadIncomingMessage(msg) && msg.Type() == MessageType::Ack) {
                    return;
                }
            } while (millis() - start2 < 1000);
            
        } while (millis() - start < _ackTimeout);
    
    }

}


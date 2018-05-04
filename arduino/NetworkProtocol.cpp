#include "NetworkProtocol.h"

#ifdef NETWORK_PROTOCOL_DEBUG

#ifdef RPI
    #include <iostream>
    #define __print std::cout <<
#else
    #define __print Serial.print 
#endif

static void log(const char* message) {
    __print("[NETWORK_PROTOCOL] ");
    __print(message);
    __print("\n");
}

static void log(const char* message, NetworkProtocol::Packet& packet) {
    __print("[NETWORK_PROTOCOL] ");
    __print(message);
    __print(" { ");
    switch (packet.type) {
        case NetworkProtocol::PacketType::Handshake:
            __print("<Handshake>");
            break;
        case NetworkProtocol::PacketType::Data:
            __print("<Data>");
            break;
        case NetworkProtocol::PacketType::Ack:
            __print("<Ack>");
            break;
        default:
            __print("<Unknown>");
            break;
    }
    __print(", Source = ");
    __print((int) packet.src);
    __print(", Destination = ");
    __print((int) packet.dest);
    __print(", Seqno = ");
    __print((int) packet.seqno);
    __print(", Data = ");
    __print(packet.data);
    __print(" }\n");
}

#undef __print

#else
#define log(...)
#endif


static bool DefaultPacketFilter(NetworkProtocol* protocol, NetworkProtocol::Packet* packet) {
    return protocol->Address() == packet->dest;
}

NetworkProtocol::NetworkProtocol(addr_t myself, unsigned int rxPin, unsigned int txPin)
    : _myself(myself),
      _filter(DefaultPacketFilter)
{   
    // Configure the RCSwitches for both tx and rx
    _rx.enableReceive(rxPin);
    _rx.setProtocol(0);
    _tx.enableTransmit(txPin);
    _tx.setProtocol(0);
    _tx.setRepeatTransmit(10);
}

addr_t NetworkProtocol::Address() const {
    return _myself;
}

void NetworkProtocol::SetPacketFilter(PacketFilter filter) {
    _filter = filter;
}

bool NetworkProtocol::Send(addr_t dest, const unsigned char* data, uint32_t len) {

    // Check parameters
    if (len > NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH) {
        return false;
    }

    Packet packet;
    packet.src = _myself;
    packet.dest = dest;
    packet.type = PacketType::Handshake;
    packet.seqno = 0;
    packet.data = len;

    // Send the handshake first
    if (!TransmitAndWait(packet)) {
        return false;
    }

    // Then send the data
    packet.type = PacketType::Data;
    for (unsigned int i = 0; i < len; i += 4) {
        packet.data = 0;
        memcpy(&packet.data, data + i, MIN(len - i, 4));
        if (!TransmitAndWait(packet)) {
            return false;
        }
    }

    return true;
}

NetworkProtocol::Message* NetworkProtocol::Receive() {

    Message* out = FindCompletedMessage();

    if (!_rx.available()) {
        return out;
    }

    // Unpack the received packet
    uint64_t rcvRaw = _rx.getReceivedValue();
    _rx.resetAvailable();
    Packet rcv;
    if (!Packet::Parse(rcvRaw, rcv)) {
        return out;
    }

    // Discard the packet if it's not for us
    if (!_filter(this, &rcv)) {
        return out;
    }

    // Process the incoming packet
    if (ProcessIncomingPacket(rcv) && out == NULL) {
        return FindCompletedMessage();
    } else {
        return out;
    }
}

bool NetworkProtocol::TransmitAndWait(Packet& p) {

    // Build the packet
    uint64_t packet = Packet::Serialize(p);

    for (int retries = 0; retries < NETWORK_PROTOCOL_SEND_RETRIES; retries++) {

        // Send!
        _tx.send(packet, 64);
        log("Sent packet:", p);

        // Wait for the ack
        unsigned int start = millis();
        do {
            if (_rx.available()) {
		   
                uint64_t rcvRaw = _rx.getReceivedValue();
                _rx.resetAvailable();
                
                // Unpack the received packet
                Packet rcv;
                if (!Packet::Parse(rcvRaw, rcv)) {
                    continue;
                }

                // Discard the packet if it's not directed to us and if it's not an ack
                if (_filter(this, &rcv)) {
                    log("Received packet:", rcv);
                    if (rcv.type == PacketType::Ack) {
#ifdef NETWORK_PROTOCOL_DEBUG
                        char buf[100];
                        sprintf(buf, "Recognized ACK. Expected seqno: %d. Actual seqno: %d", p.seqno, rcv.seqno);
                        log(buf);
#endif
                        if (rcv.seqno == p.seqno) {
                            p.seqno++;
                            return true;
                        }
                    } else {
                        // The packet is still directed to us, maybe it's from another message
                        ProcessIncomingPacket(rcv);
                    }
                }

            }
        } while (millis() - start <= NETWORK_PROTOCOL_SEND_TIMEOUT);

    }

    // Timeout
    return false;
    
}

bool NetworkProtocol::ProcessIncomingPacket(Packet& packet) {

    log("Received packet:", packet);

    // Check if there's an already message with partial data for this source
    Message* msg = FindMessageBySource(packet.src);
    if (msg == NULL) {

        // Only an handshake can start a new message
        if (packet.type != PacketType::Handshake || packet.data > NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH) {
            return false;
        }

        // Find a free message and initialize it
        msg = FindFreeMessage();
        if (msg == NULL) {
            return false;
        }
        msg->Init(packet.src, packet.data);
        SendAck(packet.src, packet.seqno);
        return false;
        
    } else {
    
        // Parts of this message have already been received, so this must be
        // either some other data, or the termination.
        switch (packet.type) {

            case PacketType::Handshake:
                // We received an handshake for an already incoming message.
                // Maybe we lost some packet?
                msg->Free();
                if (packet.data > NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH) {
                    return false;
                }
                msg->Init(packet.src, packet.data);
                SendAck(packet.src, packet.seqno);
                return false;

            case PacketType::Data:
#ifdef NETWORK_PROTOCOL_DEBUG
                char buf[100];
                sprintf(buf, "Recognized DATA. Expected seqno: %d. Actual seqno: %d", msg->ExpectedSeqno(), packet.seqno);
                log(buf);
#endif
                if (msg->ExpectedSeqno() != packet.seqno) {
                    SendAck(packet.src, msg->ExpectedSeqno() - 1);
                    return false;
                }
                if (msg->Received() == msg->Len()) {
                    return false;
                }
                msg->Append(packet.data);
                SendAck(packet.src, packet.seqno);
                return msg->IsCompleted();

            default:
                return false;
                
        }
        
    }
}

void NetworkProtocol::SendAck(addr_t dest, uint8_t seqno) {
    Packet ack;
    ack.src = _myself;
    ack.dest = dest;
    ack.type = PacketType::Ack;
    ack.seqno = seqno;
    ack.data = 0;
    _tx.send(Packet::Serialize(ack), 64);
    log("Sending ACK:", ack);
}

NetworkProtocol::Message* NetworkProtocol::FindFreeMessage() {
    for (int i = 0; i < NETWORK_PROTOCOL_MAX_INCOMING_MESSAGES; i++) {
        if (_messages[i].IsFree()) {
            return &_messages[i];
        }
    }
    return NULL;
}

NetworkProtocol::Message* NetworkProtocol::FindCompletedMessage() {
    for (int i = 0; i < NETWORK_PROTOCOL_MAX_INCOMING_MESSAGES; i++) {
        if (_messages[i].IsCompleted()) {
            return &_messages[i];
        }
    }
    return NULL;
}

NetworkProtocol::Message* NetworkProtocol::FindMessageBySource(addr_t src) {
    for (int i = 0; i < NETWORK_PROTOCOL_MAX_INCOMING_MESSAGES; i++) {
        if (!_messages[i].IsFree() && _messages[i].Source() == src) {
            return &_messages[i];
        }
    }
    return NULL;
}

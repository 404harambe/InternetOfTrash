#include "NetworkProtocol.h"
#include "crc16.h"


#define htons(x) ( ((x)<< 8 & 0xFF00) | \
                   ((x)>> 8 & 0x00FF) )
#define ntohs(x) htons(x)


/** Checks if the address is not valid. */
static inline bool CheckAddress(addr_t addr) {
    return addr <= 127;
}

/** Helper function to build a packet. */
static inline uint32_t BuildPacket(NetworkProtocol::PacketType type, addr_t src, addr_t dest, uint16_t data) {

    // Unless this is a Data packet, the payload will always be a little endian integer
    if (type != NetworkProtocol::PacketType::Data) {
        data = htons(data);
    }
    
    return (((uint32_t) type) << 30)
         | (((uint32_t) (src & 127)) << 23)
         | (((uint32_t) (dest & 127)) << 16)
         | ((uint32_t) data);
}

/** Destructures a packet into its components. */
static inline void UnpackPacket(uint32_t packet, NetworkProtocol::PacketType* type, addr_t* src, addr_t* dest, uint16_t* data) {
    *type = (NetworkProtocol::PacketType) (packet >> 30);
    *src = (packet >> 23) & 127;
    *dest = (packet >> 16) & 127;
    *data = packet & 0x0000ffff;

    // Unless this is a Data packet, the payload will always be a little endian integer
    if (*type != NetworkProtocol::PacketType::Data) {
        *data = ntohs(*data);
    }
}



NetworkProtocol::NetworkProtocol(addr_t myself, unsigned int rxPin, unsigned int txPin)
    : _myself(myself)
{   
    // Configure the RCSwitches for both tx and rx
    _rx.enableReceive(rxPin);
    _rx.setProtocol(0);
    _tx.enableTransmit(txPin);
    _tx.setProtocol(0);
    _tx.setRepeatTransmit(1);
}

bool NetworkProtocol::Send(addr_t dest, const unsigned char* data, uint16_t len) {

    // Check parameters
    if (!CheckAddress(dest)) {
        return false;
    }
    if (len > NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH) {
        return false;
    }

    uint16_t expected_ack = 0;

    // Send the handshake first
    if (!TransmitAndWait(PacketType::Handshake, dest, len, &expected_ack)) {
        return false;
    }

    // Then send the data a pair of bytes at a time
    unsigned int i = 0;
    while (true) {
        uint16_t chunk;
        if (len - i == 1) {
            chunk = ((uint16_t) data[i]) << 8;
        } else {
            chunk = ((uint16_t*) data)[i];
        }

        if (!TransmitAndWait(PacketType::Data, dest, chunk, &expected_ack)) {
            return false;
        }
        
        i += 2;
    }

    // In the end, send the crc16 as termination
    if (!TransmitAndWait(PacketType::Termination, dest, crc16(data, len), &expected_ack)) {
        return false;
    }

    return true;
}

NetworkProtocol::Message* NetworkProtocol::Receive() {

    Message* out = FindCompletedMessage();

    if (!_rx.available()) {
        return out;
    }

    // Unpack the received packet
    PacketType type;
    addr_t src;
    addr_t dest;
    uint16_t data;
    UnpackPacket(_rx.getReceivedValue(), &type, &src, &dest, &data);
    _rx.resetAvailable();

    // Discard the packet if it's not for us
    if (dest != _myself) {
        return out;
    }

    // Process the incoming packet
    if (ProcessIncomingPacket(type, src, data) && out == NULL) {
        return FindCompletedMessage();
    } else {
        return out;
    }
}

bool NetworkProtocol::TransmitAndWait(PacketType type, addr_t dest, uint16_t data, uint16_t* expected_ack) {

    // Build and transmit the packet
    uint32_t packet = BuildPacket(type, _myself, dest, data);
    _tx.send(packet, 32);

    // Wait for the ack
    unsigned int start = millis();
    do {
        if (_rx.available()) {
            
            // Unpack the received packet
            PacketType type;
            addr_t src;
            addr_t dest;
            uint16_t data;
            UnpackPacket(_rx.getReceivedValue(), &type, &src, &dest, &data);
            _rx.resetAvailable();

            // Discard the packet if it's not directed to us and if it's not an ack
            if (dest == _myself) {
                if (type == PacketType::Ack) {
                    if (data == *expected_ack) {
                        (*expected_ack)++;
                        return true;
                    }
                } else {
                    // The packet is still directed to us, maybe it's from another message
                    ProcessIncomingPacket(type, src, data);
                }
            }

        }
    } while (millis() - start <= NETWORK_PROTOCOL_TIMEOUT);

    // Timeout
    return false;
    
}

bool NetworkProtocol::ProcessIncomingPacket(PacketType type, addr_t src, uint16_t data) {

    // Check if there's an already message with partial data for this source
    Message* msg = FindMessageBySource(src);
    if (msg == NULL) {

        // Only an handshake can start a new message
        if (type != PacketType::Handshake || data > NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH) {
            return false;
        }

        // Find a free message and initialize it
        msg = FindFreeMessage();
        if (msg == NULL) {
            return false;
        }
        msg->Init(src, data);
        return false;
        
    } else {
    
        // Parts of this message have already been received, so this must be
        // either some other data, or the termination.
        switch (type) {

            case PacketType::Handshake:
                // We received an handshake for an already incoming message.
                // Maybe we lost the termination packet?
                msg->Free();
                if (data > NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH) {
                    return false;
                }
                msg->Init(src, data);
                return false;

            case PacketType::Data:
                if (msg->Received() == msg->Len()) {
                    return false;
                }
                msg->Append(data >> 8);
                msg->Append(data & 0x00ff);
                return false;

            case PacketType::Termination:
                if (msg->Received() != msg->Len()) {
                    msg->Free();
                    return false;
                }
                if (crc16(msg->Contents(), msg->Len()) != data) {
                    msg->Free();
                    return false;
                }
                return true;

            default:
                return false;
                
        }
        
    }
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

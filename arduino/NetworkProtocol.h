/**
 * RCSwitch can send messages up to 32 bits in a single batch.
 * This library builds on RCSwitch to create a reliable (let's be honest, it's way far from 100% reliable)
 * serial transmission channel than can work with arbitrary message lengths.
 * Define `NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH` before including this header to change the default value of 40 bytes.
 *
 *
 * Protocol details
 * ================
 *
 * The format of the packets is the following:
 * 
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | T | Sender Addr |  Dest Addr  |             Data              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - `T` is the packet type. Depending on this value, the field `Data` might assume
 *   different meanings. There are three possible packet types defined:
 *   - T = 00 => Handshake packet, the `Data` will contain the length in bytes of the whole message
 *               (in little endian).
 *   - T = 01 => Data packet. The `Data` field will contain the actual payload of the message.
 *   - T = 10 => Termination packet. The `Data` field will contain a CRC16 of the whole message.
 *   - T = 11 => Ack packet. The `Data` field contains the sequence number of the acked packet
 *               (in little endian).
 *
 * - `Sender Addr` is the address of the sender. This is a 7 bit string identifying
 *   the originator of the message.
 *
 * - `Dest Addr` is the address of the destination of the message. This is also a 7 bit string.
 *
 * - `Data` is a 16 bit field whose meaning varies depending on the value of `T`.
 *
 * The sender sends a single message at a time, and waits for the ack coming from the receiver.
 * As an example, this is a possible exchange of messages:
 *
 * Sender                          Receiver
 *   |           Handshake            |
 *   |------------------------------->|
 *   |                                |
 *   |             Ack #1             |
 *   |<-------------------------------|
 *   |                                |
 *   |           Data: "HE"           |
 *   |------------------------------->|
 *   |                                |
 *   |             Ack #2             |
 *   |<-------------------------------|
 *   |                                |
 *   |           Data: "LO"           |
 *   |------------------------------->|
 *   |                                |
 *   |             Ack #3             |
 *   |<-------------------------------|
 *   |                                |
 *   |           Termination          |
 *   |------------------------------->|
 *   |                                |
 *   |             Ack #4             |
 *   |<-------------------------------|
 *
 * If the sender does not receive an Ack, it keeps waiting, and bails out after a timeout.
 */

#ifndef __NETWORK_PROTOCOL_H__
#define __NETWORK_PROTOCOL_H__

#ifndef NETWORK_PROTOCOL_MAX_INCOMING_MESSAGES
#define NETWORK_PROTOCOL_MAX_INCOMING_MESSAGES 6
#endif

#ifndef NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH
#define NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH 40 /* Must be even */
#endif

#ifndef NETWORK_PROTOCOL_TIMEOUT
#define NETWORK_PROTOCOL_TIMEOUT 1000
#endif

#include <RCSwitch.h>

/** Addresses are 7 bit identifiers. */
typedef unsigned char addr_t;

class NetworkProtocol {
public:

    /** A message received from the network. */
    class Message {
    public:
        inline addr_t Source() { return _src; }
        inline uint16_t Len() { return _len; }
        inline const unsigned char* Contents() { return (const unsigned char*) _contents; }
        
        inline void Free() {
            _free = true;
        }

        friend class NetworkProtocol;

    private:
        bool _free = true;
        addr_t _src;
        uint16_t _len;
        uint16_t _received;
        unsigned char _contents[NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH];

        inline uint16_t Received() { return _received; }
        inline bool IsFree() { return _free; }
        inline bool IsCompleted() { return !_free && _received == _len; }
        
        inline void Append(unsigned char c) {
            if (_received < _len) {
                _contents[_received++] = c;
            }
        }

        inline void Init(addr_t src, uint16_t len) {
            _free = false;
            _src = src;
            _len = len;
            _received = 0;
        }
    };

    enum class PacketType: unsigned char {
        Handshake = 0,
        Data = 1,
        Termination = 2,
        Ack = 3
    };

    NetworkProtocol(addr_t myself, unsigned int rxPin, unsigned int txPin);

    /**
     * Sends an arbitrary array of bytes to the given destination.
     * This method blocks until the receiver has acknowledged all the data.
     *
     * It can send at most `NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH` bytes.
     *
     * Returns `true` if the message was successfully sent, `false` otherwise.
     */
    bool Send(addr_t dest, const unsigned char* data, uint16_t len);

    /**
     * Checks if a new packet for this node is available.
     * This method does not block, and returns a proper message only if new data is available.
     * If it returns `false`, the value of `*out` is left untouched.
     *
     * It can receive at most `NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH` bytes.
     */
    bool Receive(Message** out);

    
private:

    addr_t _myself;
    RCSwitch _tx;
    RCSwitch _rx;
    Message _messages[NETWORK_PROTOCOL_MAX_INCOMING_MESSAGES];

    Message* FindFreeMessage();
    Message* FindCompletedMessage();
    Message* FindMessageBySource(addr_t src);

    bool TransmitAndWait(PacketType type, addr_t dest, uint16_t data, uint16_t* expected_ack);
    bool ProcessIncomingPacket(PacketType type, addr_t src, uint16_t data);
};

#endif

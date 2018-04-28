/**
 * RCSwitch can send messages up to 64 bits in a single batch.
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
 * |  Sender Addr  |   Dest Addr   | T |  Seq Num  |     CRC8      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                              Data                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - `Sender Addr` is the address of the sender. This is a 8 bit string identifying
 *   the originator of the message.
 *
 * - `Dest Addr` is the address of the destination of the message. This is also a 8 bit string.
 *
 * - `T` is the packet type. Depending on this value, the field `Data` might assume
 *   different meanings. There are three possible packet types defined:
 *   - T = 00 => Handshake packet, the `Data` will contain the length in bytes of the whole message
 *               (in little endian).
 *   - T = 01 => Data packet. The `Data` field will contain the actual payload of the message.
 *   - T = 10 => Ack packet. The `Data` field is meaningless. This packet is sent by the receiver
 *               to acknowledge a received packet.
 *   - T = 11 => Reserved for future use.
 *
 * - `Seq Num` is the sequence number of the packet. Note the we count whole packets in sequence numbers,
 *   not bytes like TCP. The length of the field is 6 bits.
 *
 * - `CRC8` is a crc to check for packet integrity. It is computed on the whole packet,
 *   with the `CRC8` field set to all 0.
 *
 * - `Data` is a 32 bit field whose meaning varies depending on the value of `T`.
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
 *
 */

#ifndef __NETWORK_PROTOCOL_H__
#define __NETWORK_PROTOCOL_H__

#ifndef NETWORK_PROTOCOL_MAX_INCOMING_MESSAGES
#define NETWORK_PROTOCOL_MAX_INCOMING_MESSAGES 6
#endif

#ifndef NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH
#define NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH 40 /* Must be dividible by 4 */
#endif

#ifndef NETWORK_PROTOCOL_SEND_RETRIES
#define NETWORK_PROTOCOL_SEND_RETRIES 10
#endif

#ifndef NETWORK_PROTOCOL_SEND_TIMEOUT
#define NETWORK_PROTOCOL_SEND_TIMEOUT 300
#endif

//#define NETWORK_PROTOCOL_DEBUG

#include "RCSwitch.h"
#include "util.h"
#include "crc8.h"

/** Addresses are 8 bit identifiers. */
typedef uint8_t addr_t;

class NetworkProtocol {
public:

    /** A message received from the network. */
    class Message {
    public:
        Message()
            : _free(true),
              _src(0),
              _len(0),
              _received(0),
              _expected_seqno(1) // Since a Message is allocated after an handshake,
                                 // we wait for a Data packet, which should be the second packet.
        {
        }
    
        inline addr_t Source() { return _src; }
        inline uint32_t Len() { return _len; }
        inline const unsigned char* Contents() { return (const unsigned char*) _contents; }
        
        inline void Free() {
            _free = true;
        }

        friend class NetworkProtocol;

    private:
        bool _free;
        addr_t _src;
        uint32_t _len;
        uint32_t _received; // Blocks of 4 bytes
        uint8_t _expected_seqno;
        uint32_t _contents[NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH / 4];

        inline uint32_t Received() { return _received; }
        inline uint8_t ExpectedSeqno() { return _expected_seqno; }
        inline bool IsFree() { return _free; }
        inline bool IsCompleted() { return !_free && _received * 4 >= _len; }
        
        inline void Append(uint32_t data) {
            _contents[_received++] = data;
            _expected_seqno++;
        }

        inline void Init(addr_t src, uint32_t len) {
            _free = false;
            _src = src;
            _len = len;
            _received = 0;
            _expected_seqno = 1;
        }
    };

    enum class PacketType: unsigned char {
        Handshake = 0,
        Data = 1,
        Ack = 2,
        Reserved = 3
    };

    struct Packet {
        uint8_t                      src;
        uint8_t                      dest;
        NetworkProtocol::PacketType  type;
        uint8_t                      seqno;
        uint8_t                      crc;
        uint32_t                     data;
    
        static inline bool Parse(uint64_t raw, Packet& packet) {
    
            // Unpack the fields
            packet.src = (uint8_t) (raw >> 56 & 0x000000FFUL);
            packet.dest = (uint8_t) (raw >> 48 & 0x000000FFUL);
            packet.type = (NetworkProtocol::PacketType) (raw >> 46 & 0x00000003UL);
            packet.seqno = (uint8_t) (raw >> 40 & 0x0000003FUL);
            packet.crc = (uint8_t) (raw >> 32 & 0x000000FFUL);
            packet.data = (uint32_t) (raw & 0x00000000FFFFFFFFULL);
    
            // Ensure that the data field is parsed correctly in case of an handshake packet
            if (packet.type == PacketType::Handshake) {
                packet.data = ntohl(packet.data);
            }
    
            // Check CRC
            uint64_t crc_zeroed = raw & 0xFFFFFF00FFFFFFFFULL;
            return packet.crc == crc8(&crc_zeroed, sizeof(uint64_t));
            
        }
    
        static inline uint64_t Serialize(Packet& packet) {
    
            // Ensure that the data field is parsed correctly in case of an handshake packet
            uint32_t data = packet.data;
            if (packet.type == PacketType::Handshake) {
                data = htonl(data);
            }
        
            uint64_t res =
                   ((uint64_t) packet.src) << 56
                 | ((uint64_t) packet.dest) << 48
                 | ((uint64_t) packet.type & 0x03U) << 46
                 | ((uint64_t) (packet.seqno & 0x3FU)) << 40
                 | (uint64_t) data;

            res |= ((uint64_t) crc8(&res, sizeof(uint64_t))) << 32;
            return res;
        }
    };

    /** Custom callback to decide which packets should be discarded. */
    typedef bool (*PacketFilter)(NetworkProtocol*, Packet*);

    NetworkProtocol(addr_t myself, unsigned int rxPin, unsigned int txPin);

    /**
     * Sends an arbitrary array of bytes to the given destination.
     * This method blocks until the receiver has acknowledged all the data.
     *
     * It can send at most `NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH` bytes.
     *
     * Returns `true` if the message was successfully sent, `false` otherwise.
     */
    bool Send(addr_t dest, const unsigned char* data, uint32_t len);

    /**
     * Checks if a new packet for this node is available.
     * This method does not block, and returns a proper message only if new data is available.
     * If no message is available, `NULL` is returned.
     *
     * It can receive at most `NETWORK_PROTOCOL_MAX_MESSAGE_LENGTH` bytes.
     */
    Message* Receive();

    /** Returns the address associated to this node. */
    addr_t Address() const;

    /**
     * Sets a function that acts as a filter for the packets.
     * Note that this function will see raw unprocessed packets, not whole messages.
     */
    void SetPacketFilter(PacketFilter filter);

    
private:

    const addr_t _myself;
    RCSwitch _tx;
    RCSwitch _rx;
    Message _messages[NETWORK_PROTOCOL_MAX_INCOMING_MESSAGES];
    PacketFilter _filter;

    Message* FindFreeMessage();
    Message* FindCompletedMessage();
    Message* FindMessageBySource(addr_t src);

    bool TransmitAndWait(Packet& packet);
    bool ProcessIncomingPacket(Packet& packet);
    void SendAck(addr_t dest, uint8_t seqno);
};

#endif

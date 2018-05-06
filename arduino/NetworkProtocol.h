/**
 * RCSwitch can send messages up to 64 bits in a single batch.
 * This library implements a simple protocol to talk to the bins and request measurements.
 *
 * The format of the packets is the following:
 * 
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         Sender address        |          Dest address         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |          Measurement          |     Type      |      CRC8     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * - `Sender address` is the address of the sender. This is a 16 bit string identifying
 *   the originator of the message.
 *
 * - `Dest address` is the address of the destination of the message. This is also a 16 bit string.
 *   If the value of `Dest address` is 2^17 - 1, the message is intended to be a broadcast message.
 *
 * - `Measument` is the value of the performed measument, or 0 if it is not relevant.
 *   This is a 16 bit little endian integer.
 *
 * - `Type` is the packet type:
 *   - Type = 00000000 => Request for a measurement.
 *   - Type = 00000001 => Positive response for a measurement request.
 *   - Type = 00000010 => Acknoledgement of a packet.
 *   - Type = 10000000 => Generic error.
 *   - Type = 10000001 => Lid open, cannot take measurement.
 *
 * - `CRC8` is a crc to check for packet integrity. It is computed on the whole packet,
 *   with the `CRC8` field set to all 0.
 *
 */

#ifndef __NETWORK_PROTOCOL_H__
#define __NETWORK_PROTOCOL_H__

#if defined(RPI) && defined(NETWORK_PROTOCOL_DEBUG)
    #include <iostream>
#endif

#include "RCSwitch.h"
#include "util.h"
#include "crc8.h"

/** Addresses are 16 bit identifiers. */
typedef uint16_t addr_t;

/** Implementation of a simple protocol to talk to the bins. */
class NetworkProtocol {
public:

    enum class MessageType : unsigned char {
        Request = 0,
        Success = 1,
        Ack = 2,

        // Errors
        Error = (1 << 7),
        ErrorLid = (1 << 7) | 1
    };

    /** A message received from the network. */
    class Message {
    public:
        Message()
            : _src(0),
              _dest(0),
              _measurement(0),
              _type(MessageType::Request)
        {
        }

        Message(addr_t src, addr_t dest, uint16_t measurement, MessageType type)
            : _src(src),
              _dest(dest),
              _measurement(measurement),
              _type(type)
        {
        }

        inline addr_t Source() { return _src; }
        inline addr_t Destination() { return _dest; }
        inline uint16_t Measurement() { return _measurement; }
        inline MessageType Type() { return _type; }
        
        inline bool IsError() { return ((unsigned char) _type & (1 << 7)) != 0; }
        
        inline void SetSource(addr_t val) { _src = val; }
        inline void SetDestination(addr_t val) { _dest = val; }
        inline void SetMeasurement(uint16_t val) { _measurement = val; }
        inline void SetType(MessageType val) { _type = val; }

        inline uint64_t Serialize() {
            
            // Join all the fields
            uint64_t ret =
                ((uint64_t) _src) << 48
                | ((uint64_t) _dest) << 32
                | ((uint64_t) htons(_measurement)) << 16
                | ((uint64_t) _type) << 8;

           // Compute the crc
           return ret | crc8(&ret, sizeof(uint64_t));
           
        }

        inline bool Parse(uint64_t raw) {

            // Unpack the fields
            _src = (uint16_t) (raw >> 48 & 0x0000FFFFUL);
            _dest = (uint16_t) (raw >> 32 & 0x0000FFFFUL);
            _measurement = ntohs((uint16_t) (raw >> 16 & 0x0000FFFFUL));
            _type = (MessageType) (raw >> 8 & 0x000000FFUL);
            uint8_t crc = (uint8_t) (raw & 0x00000000000000FFULL);

            // Check CRC
            uint64_t crc_zeroed = raw & 0xFFFFFFFFFFFFFF00ULL;
            return crc == crc8(&crc_zeroed, sizeof(uint64_t));
        
        }
        
    private:
        addr_t _src;
        addr_t _dest;
        uint16_t _measurement;
        MessageType _type;
    };


    /** Callback accepting a Message. */
    class MessageCallback {
    public:
        virtual void Run(Message*) = 0;
        virtual ~MessageCallback() {}
    };


    /** Special broadcast address. */
    const addr_t BroadcastAddress = (addr_t) ((1UL << 17) - 1UL);


    /** Creates a new instance of the NetworkProtocol class. */
    NetworkProtocol(addr_t myself, unsigned int rxPin, unsigned int txPin);

    /** Returns the address associated to this node. */
    addr_t Address() const {
        return _myself;
    }

    /**
     * Requests an updated measurement from all the available bins.
     * The callback will be invoked for every received message.
     */
    void RequestMeasurement(MessageCallback* cb);

    /**
     * Requests an updated measurement from a specific bin.
     * The callback will be invoked at most once.
     */
    void RequestMeasurement(addr_t dest, MessageCallback* cb);

    /**
     * Invoke the function in the `loop` function of the Arduino sketch
     * to allow proccesing of the incoming messages.
     */
    void Loop();

    /**
     * Sets the callback that will be used to perform the measurement.
     * The callback is expected to perform the measurement and fill the
     * fields `Measurement` and `Type` of the given message.
     */
    inline void SetMeasurementCallback(MessageCallback* cb) {
        _measurementCallback = cb;
    }

    /** Sets the timeout to wait before stopping listening for answers. */
    inline void SetRequestTimeout(unsigned long ms) {
        _requestTimeout = ms;
    }

    /** Sets the timeout to wait when waiting for an ack of the measurement sent. */
    inline void SetAckTimeout(unsigned long ms) {
        _ackTimeout = ms;
    }
    
private:
    const addr_t _myself;
    RCSwitch _rx;
    RCSwitch _tx;

    MessageCallback* _measurementCallback = nullptr;
    unsigned long _requestTimeout = 10000;
    unsigned long _ackTimeout = 7000;

    
#ifdef NETWORK_PROTOCOL_DEBUG

#ifdef RPI
    #define __print std::cout <<
#else
    #define __print Serial.print 
#endif

    static void log(const char* message) {
        __print("[NETWORK_PROTOCOL] ");
        __print(message);
        __print("\n");
    }
    
    static void log(const char* message, NetworkProtocol::Message& msg) {
        __print("[NETWORK_PROTOCOL] ");
        __print(message);
        __print(" { ");
        switch (msg.Type()) {
            case NetworkProtocol::MessageType::Request:
                __print("<Request>");
                break;
            case NetworkProtocol::MessageType::Success:
                __print("<Success>");
                break;
            case NetworkProtocol::MessageType::Ack:
                __print("<Ack>");
                break;
            case NetworkProtocol::MessageType::Error:
                __print("<Error>");
                break;
            case NetworkProtocol::MessageType::ErrorLid:
                __print("<ErrorLid>");
                break;
            default:
                __print("<Unknown>");
                break;
        }
        __print(", Source = ");
        __print((int) msg.Source());
        __print(", Destination = ");
        __print((int) msg.Destination());
        __print(", Measurement = ");
        __print((int) msg.Measurement());
        __print(" }\n");
    }

#undef __print

#else
#define log(...)
#endif


    inline void Transmit(Message& msg) {
        _tx.send(msg.Serialize(), 64);
        log("Sent:", msg);
    }
    
    inline void TransmitAck(Message& msg) {
        Message ack(_myself, msg.Source(), 0, MessageType::Ack);
        Transmit(ack);
    }
    
    inline bool ReadIncomingMessage(Message& msg) {
        if (_rx.available()) {
            log("DATA");
            uint64_t raw = _rx.getReceivedValue();
            _rx.resetAvailable();
            if (msg.Parse(raw)) {
                log("Raw packet:", msg);
                if (msg.Destination() == _myself || msg.Destination() == BroadcastAddress) {
                    log("Received:", msg);
                    return true;
                }
            }
        }
        return false;
    }
    
};

#endif

/***************************************************************************
    CannonBoard Hardware Interface. 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "globals.hpp"
#include <string>

#ifdef CANNONBOARD
#include <boost/thread/mutex.hpp>
#endif

class CallbackAsyncSerial;
  
struct Packet
{
    // Packet Length including CBALL Header
    const static int LENGTH = 15;

    // Message Count
    uint8_t msg_count;

    // Last message received
    uint8_t msg_received;

    uint8_t status;
    uint8_t di1;
    uint8_t di2;
    uint8_t mci;     // Moving Cabinet: Motor Limit Values
    uint8_t ai0;     // Acceleration
    uint8_t ai1;     // Moving Cabinet: Motor Position
    uint8_t ai2;     // Steering
    uint8_t ai3;     // Braking
};

struct IncomingStatus
{
    const static uint8_t FOUND     = 0x01; // Packet Found
    const static uint8_t CSUM      = 0x02; // Last Packet Checksum was OK
    const static uint8_t PAST_FAIL = 0x04; // Checksum has failed at some point in the past
    const static uint8_t RESET     = 0x08; // Interface is resetting
    const static uint8_t MISSED    = 0x10; // Interface missed a packet (using message counter)
};

class Interface
{

public:
    // Incoming Packets Found
    uint32_t stats_found_in;
    // Incoming Packets Not Found
    uint32_t stats_notfound_in;
    // Incoming Packets With Error
    uint32_t stats_error_in;
    // Outbound Packets Found
    uint32_t stats_found_out;
    // Outbound Packets Missed
    uint32_t stats_missed_out;
    // Outbound Packets With Error
    uint32_t stats_error_out;

    Interface();
    ~Interface();
    void init(const std::string& port, unsigned int baud);
    void reset_stats();
    void close();
    void start();
    void stop();
    void write(uint8_t dig_out, uint8_t mc_out);
    bool started();
    Packet* get_packet();

#ifdef CANNONBOARD
private:
    const static bool DEBUG = false;

    // Mutex Declaration
    boost::mutex mtx;

    // Serial Port Handler
    CallbackAsyncSerial* serial;

    // Circular Array Buffer
    const static int BUFFER_SIZE = 1024;
    uint8_t buffer[BUFFER_SIZE];
    int32_t head;

    // Packet storage
    Packet packet;
    Packet packet_copy;
    enum
    {
        PACKET_NOT_FOUND = -1,
        CHECKSUM_ERROR   = -2,
    };

    // Message Types
    const static uint8_t MSG_RESET  = 0;
    const static uint8_t MSG_OUTRUN = 1;
    
    // Previous Message Index
    int16_t prev_msg;

    // Write Message Count
    unsigned char write_count;

    // Is Serial interface open and operational
    bool is_started;

    void reset_interface();
    void received(const char *data, unsigned int len);
    int find_packet();
    bool is_checksum_ok(int offset);

    uint8_t c_get(int i);
    int c_add(int a, int b);
    int c_sub(int a, int b);
#endif
};


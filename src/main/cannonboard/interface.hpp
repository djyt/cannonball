#pragma once

#include "globals.hpp"
#include <boost/thread/mutex.hpp>

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

class Interface
{

public:
    uint32_t stats_found;
    uint32_t stats_missed;
    uint32_t stats_error;

    Interface();
    ~Interface();
    void init(const std::string& port, unsigned int baud);
    void close();
    void start();
    void stop();
    void write(uint8_t dig_out, uint8_t mc_out);
    bool started();
    Packet get_packet();


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
    enum
    {
        PACKET_NOT_FOUND = -1,
        CHECKSUM_ERROR   = -2,
    };
    
    // Previous Message Index
    int16_t prev_msg;

    // Write Message Count
    unsigned char write_count;

    // Is Serial interface open and operational
    bool is_started;

    void received(const char *data, unsigned int len);
    int find_packet();
    bool is_checksum_ok(int offset);

    uint8_t c_get(int i);
    int c_add(int a, int b);
    int c_sub(int a, int b);
};
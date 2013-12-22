#include "interface.hpp"
#include "asyncserial.hpp"

Interface::Interface()
{
    serial     = NULL;
    is_started = false;
}

Interface::~Interface()
{
    close();
}

void Interface::init()
{
    try
    {
        close();

        // Setup Serial Port
        serial = new CallbackAsyncSerial("COM6", 57600);

        // Reset Circular Array
        memset(&buffer, 0, BUFFER_SIZE);
        head = 0;
    }
    catch (boost::system::system_error& e)
    {
        std::cerr << "Error Opening Serial Interface: " << e.what() << std::endl;
    }
}

void Interface::close()
{
    if (serial != NULL)
    {
        try
        {
            serial->close();
        }
        catch (boost::system::system_error& e)
        {
            std::cerr << "Error Closing Serial Interface: " << e.what() << std::endl;
        }
        delete serial;
        serial     = NULL;
        is_started = false;
    }
}

void Interface::start()
{
    if (serial != NULL && serial->isOpen())
    {
        prev_msg   = -1;
        is_started = true;

        // Use bind to create a callback to a class member
        serial->setCallback(boost::bind(&Interface::received, this, _1, _2));
    }
}

void Interface::stop()
{
    if (serial != NULL && serial->isOpen())
    {
        is_started = false;

        serial->clearCallback();
    }
}

bool Interface::started()
{
    return is_started;
}

Packet Interface::get_packet()
{
    boost::lock_guard<boost::mutex> guard(mtx);
    return packet;
}

// ------------------------------------------------------------------------------------------------
// Serial CallBack
// ------------------------------------------------------------------------------------------------

void Interface::received(const char *data, unsigned int len)
{
    int32_t new_head = head + len;

    // Copy directly into circular array
    if (new_head < BUFFER_SIZE)
    {
        memcpy(buffer + head, data, len);
    }
    // Spanned the join of the circular array
    else
    {
        int len1 = len - (new_head - BUFFER_SIZE);
        int len2 = len - len1;
        memcpy(buffer + head, data, len1);
        memcpy(buffer, data + len1, len2);
    }

    // Increment Head Position
    head = c_add(head, len);

    // Find Most Recent Packet
    int packet_index = find_packet();
    if (packet_index >= 0) // Checksum OK, Packet Found
    {
        uint8_t msg_count = c_get(packet_index);
        
        // New Packet
        if (msg_count != prev_msg)
        {
            boost::lock_guard<boost::mutex> guard(mtx);

            prev_msg            = msg_count;
            packet.msg_count    = msg_count;
            packet.msg_received = c_get(++packet_index);
            packet.status       = c_get(++packet_index);
            packet.di1          = c_get(++packet_index);
            packet.di2          = c_get(++packet_index);
            packet.mci          = c_get(++packet_index);
            packet.di3          = c_get(++packet_index);
            packet.di4          = c_get(++packet_index);
            packet.ai1          = c_get(++packet_index);
            packet.ai2          = c_get(++packet_index);
            packet.ai3          = c_get(++packet_index);
            packet.ai4          = c_get(++packet_index);
            packet.ai5          = c_get(++packet_index);
            packet.ai6          = c_get(++packet_index);
            packet.ai7          = c_get(++packet_index);
            packet.ai8          = c_get(++packet_index);
        }
    }
    else if (packet_index == PACKET_NOT_FOUND)
    {

    }
    else if (packet_index == CHECKSUM_ERROR)
    {

    }
}

int Interface::find_packet()
{
    // Start by searching back at least one full packet length
    int index  = c_sub(head, Packet::LENGTH);

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        if (buffer[c_add(index, 0)] == 'C' && 
            buffer[c_add(index, 1)] == 'B' && 
            buffer[c_add(index, 2)] == 'A' && 
            buffer[c_add(index, 3)] == 'L' &&
            buffer[c_add(index, 4)] == 'L')
        {
            int address = c_add(index, 5);
            if (is_checksum_ok(address))
                return address;
            else
                return CHECKSUM_ERROR;
        }

        if (--index < 0)
            index += BUFFER_SIZE;
    }
    return PACKET_NOT_FOUND;
}

bool Interface::is_checksum_ok(int offset)
{
    char check = 0;

    for (int i = 0; i < 16; i++)
    {
        check += buffer[offset];

        if (++offset >= BUFFER_SIZE)
            offset = 0;
    }
    return check == buffer[c_add(offset, 3)];
}

// ------------------------------------------------------------------------------------------------
// Circular Buffer Functions
// ------------------------------------------------------------------------------------------------

uint8_t Interface::c_get(int i)
{
    return buffer[i % BUFFER_SIZE];
}

int Interface::c_add(int a, int b)
{    
    int value = a + b;
    if (value >= BUFFER_SIZE)
        value -= BUFFER_SIZE;
    return value;
}

int Interface::c_sub(int a, int b)
{
    int value = a - b;
    if (value < 0)
        value += BUFFER_SIZE;
    return value;
}
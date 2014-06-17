/***************************************************************************
    Cabinet Diagnostics.

    Diagnostics Modes. Based On Original Code, with modifications.
    Mostly of use for real cabinets.

    - CRT Check
    - CannonBoard Interface Check
    - Motor Hardware Test
    - Brake/Start Lamp Test
    - Control Input Test
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "../stdint.hpp"

class Interface;
struct Packet;

class CabDiag
{
public:
    enum
    {
        STATE_INTERFACE,
        STATE_CRT,
        STATE_INPUT,
        STATE_OUTPUT,
        STATE_MOTORT,
    };

    CabDiag(Interface* cannonboard);
    ~CabDiag(void);
    bool tick(Packet* packet);
    void set(uint8_t);
private:
    Interface* cannonboard;

    bool init;
    bool done;
    uint8_t state;
    uint8_t counter;

    // Can user press start to exit mode?
    bool press_start_to_exit;

    void reset();

    void init_interface();
    void tick_interface(Packet* packet);
    void init_output();
    void tick_output();
    void init_input();
    void tick_input(Packet* packet);
    void init_crt();
    void blit_box();
    void blit1_block(uint32_t adr, uint32_t data);
    void blit7_block(uint32_t* adr, uint32_t data);
    void init_motor_test();
};
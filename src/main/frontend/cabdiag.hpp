/***************************************************************************
    Cabinet Diagnostics.

    Diagnostics Modes. Based On Original Code, with modifications.
    Mostly of use for real cabinets.

    - CRT Check
    - SMARTYPI Interface Check
    - Motor Hardware Test
    - Brake/Start Lamp Test
    - Control Input Test
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "../stdint.hpp"

class CabDiag
{
public:
    enum
    {
        STATE_CRT,
        STATE_INPUT,
        STATE_OUTPUT,
        STATE_MOTORT,
    };

    CabDiag();
    ~CabDiag(void);
    bool tick();
    void set(uint8_t);

private:
    bool init;
    bool done;
    uint8_t state;
    uint8_t counter;

    // Can user press start to exit mode?
    bool press_start_to_exit;

    void reset();

    void init_output();
    void tick_output();
    void init_input();
    void tick_input();
    void init_crt();
    void blit_box();
    void blit1_block(uint32_t adr, uint32_t data);
    void blit7_block(uint32_t* adr, uint32_t data);
    void init_motor_test();
    void tick_motor();
};
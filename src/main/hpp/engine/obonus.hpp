/***************************************************************************
    Bonus Points Code.
    
    This is the code that displays your bonus points on completing the game.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

class OBonus
{
public:
    // Bonus Control    
    int8_t bonus_control;
    enum
    {
        BONUS_DISABLE = 0x0,    // 0  = Bonus Mode Disabled
        BONUS_INIT = 0x4,       // 4  = Init Bonus Mode
        BONUS_TICK = 0x8,       // 8  = Tick Down Bonus Time
        BONUS_SEQ0 = 0xC,       // C  = End Seq Animation Stage #0 - Stages used to set Ferrari frames
        BONUS_SEQ1 = 0x10,      // 10 = End Seq Animation Stage #1
        BONUS_SEQ2 = 0x14,      // 14 = End Seq Animation Stage #2
        BONUS_SEQ3 = 0x18,      // 18 = End Seq Animation Stage #3
        BONUS_END = 0x1C,       // 1C = End Bonus Sequence
    };

    // Bonus State
    //
    // 0 = Init Bonus Points Text & Calculate Bonus Score
    // 1 = Decrement Bonus Seconds
    // 2 = Clear Bonus Text
    // 3 = Do Not Execute Bonus Text Block
    int8_t bonus_state;
    enum
    {
        BONUS_TEXT_INIT = 0,
        BONUS_TEXT_SECONDS = 1,
        BONUS_TEXT_CLEAR = 2,
        BONUS_TEXT_DONE = 3,
    };

    // Timer used by bonus mode logic (Added from Rev. A onwards)
    int16_t bonus_timer;

    OBonus(void);
    ~OBonus(void);
    void init();

    void do_bonus_text();

private:
    // Bonus seconds (for seconds countdown at bonus stage)
    //
    // Stored as hex, but should be converted to decimal for true value.
    //
    // So 0x314 would be 78.8 seconds
    int16_t bonus_secs;

    // Timing counter used in bonus code logic
    int16_t bonus_counter;

    void init_bonus_text();
    void blit_bonus_secs();
    void decrement_bonus_secs();
};

extern OBonus obonus;


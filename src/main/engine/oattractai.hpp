/***************************************************************************
    Ferrari AI and Logic Routines.
    Used by Attract Mode and the end of game Bonus Sequence. 

    This code contains a port of the original AI and a new enhanced AI.

    Enhanced AI Bug-Fixes:
    ----------------------
    - AI is much better at driving tracks without crashing into scenery.
    - No weird juddering when turning corners.
    - No brake light flickering.
    - Can drive any stage in the game competently. 
    - Selects a true random route, rather than a pre-defined route. 
    - Can handle split tracks correctly.

    It still occasionally collides with scenery on the later stages, but
    I think this is ok - as we want to demo a few collisions!

    Notes on the original AI:
    -------------------------
    The final behaviour of the AI differs from the original game.
    
    This is because the core Ferrari logic the AI relies on is in turn
    dependent on timing behaviour between the two 68k CPUs.
    
    Differences in timing lead to subtle differences in the road x position 
    at that particular point of time. Over time, these differences are 
    magnified. 
    
    MAME does not accurately reproduce the arcade machine with regard to
    this. And the Saturn conversion also exhibits different behaviour.
    
    The differences are relatively minor but noticeable.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

class OAttractAI
{
public:
    OAttractAI(void);
    ~OAttractAI(void);

    void init();
    void tick_ai_enhanced();

    void tick_ai();
    void check_road_bonus();
    void set_steering_bonus();

private:
    int8_t last_stage;
    void check_road();
    void set_steering();
};

extern OAttractAI oattractai;

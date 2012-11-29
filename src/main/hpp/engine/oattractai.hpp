/***************************************************************************
    Ferrari AI and Logic Routines.
    Used by Attract Mode and the end of game Bonus Sequence. 
    
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

    void tick_ai();

    void check_road_bonus();
    void set_steering_bonus();

private:
    void check_road();
    void set_steering();
};

extern OAttractAI oattractai;

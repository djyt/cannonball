/***************************************************************************
    Animated Sprites. 
    
    This format is essentially a deviation from the normal sprites used in
    the game.
    
    Some of the entries in the block of memory are replaced and used for 
    other purposes, which can be seen below.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "oentry.hpp"

class oanimsprite
{
public:
    // Base Sprite
    oentry* sprite;

    //+0x06 [Byte] Sprite/Object Index Being Processed
    //             1 = Car Door
    //             2 = Ferrari Interior
    //             3 = Car Shadow
    //             4 = Man Sprite
    //             5 = Man Shadow
    //             6 = Female Sprite
    //             7 = Female Shadow
    //             8 = Trophy Person
    //             9 = Trophy Shadow
    //             A = After Effects (e.g. smoke cloud on genie animation)

    //+0x1E [Long] Reference to the CURRENT block of animation data.
    uint32_t anim_addr_curr;

    //+0x22 [Long] Reference to the NEXT block of animation data.
    uint32_t anim_addr_next;

    //+0x26 [Word] Animation Frame Number
    int16_t anim_frame;

    //+0x28 [Word] Frame Delay (Before increment to next frame)
    uint8_t frame_delay;

    //+0x2A [Word] Increment End Sequence Position When Set
    uint16_t anim_props;

    //+0x2C [Word] Animation State
    int16_t anim_state;

    void init(oentry* s)
    {
        sprite = s;
        sprite->function_holder = -1;
        anim_addr_curr = 0;
        anim_addr_next = 0;
        anim_frame = 0;
        frame_delay = 0;
        anim_props = 0;
        anim_state = 0;
    }
};
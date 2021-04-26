/***************************************************************************
    Sprite Entry.
    
    This class represents a single sprite object, commonly used by OutRun.
    
    In the original codebase, each object consists of a 64 byte block of 
    memory. Each of these blocks forms part of a jump table that is iterated 
    each tick. The sprite referenced the address of the routine it used.
    
    Unfortunately, the system is messy and the usage of each 64 byte 
    entry differs dependent on the game object. 
    
    However, the majority are similar. And as such, I've tried to convert
    this structure to a more manageable class. For the conversion, rather
    than dynamically editing a jump table, I've called the sprite routines
    from the main code.
    
    It's not perfect, but struck a reasonable balance between clarity and
    being able to debug the conversion.
    
    All in-game objects that populate the gameworld use this structure.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.hpp"

class oentry
{
public:
	// +00 [Byte] Bit 7 Enables/Disables Address
	//            Bit 6
	//            Bit 5 Set to Draw Sprite
	//            Bit 4 Set to Enable Shadows
	//            Bit 3 Traffic Sprite - Set to Use Large Shadow, Unset for Small Shadow (If Shadows Enabled)
	//            Bit 2 Set if road_width >= 0x118
	//            Bit 1
	//            Bit 0 Set to Horizontally Flip Image
	uint8_t control;

	// +01 [Byte] Index Number of Jump 0,1,2,3 etc.
	uint8_t jump_index;

	// +02 [Long] Jump Address
	int8_t function_holder;

	// +06 [Byte] Multiple Uses. Used to identify sprites.
	// E.g. Passenger Sprites: Denote Man (0) or Woman (1) Sprite.
    // Course Map Sprites: Denote piece of map (just iterates from 0 upwards)
	uint8_t id;

	// +07 [Byte] Sprite Priority (In relation to tilemaps) & Shadow Settings
	// Default = 3
	uint8_t shadow;

	// +08 [Byte] Entry Number For Zoom Lookup Table 
    //            Looked up from ROM [0x30000 + (offset * 8)]
    //            0 = Hide Sprite
	uint8_t zoom;

	// +09 [Byte] 1/ Draw Routine To Use. [4-bits] dddd----
	//	 		  2/ Draw Properties.     [4-bits]: ----yyxx
	//
	// For draw routine see SetupSpriteRoutine function
	//
	// xx 0 = Anchor Centre. 01 = Anchor Left. 10 = Anchor Right
	// yy 0 = Anchor Centre. 01 = Anchor Top.  10 = Anchor Bottom.
	uint8_t draw_props;

    enum
    {
        CENTRE = 0,
        LEFT = 1,
        RIGHT = 2,
        TOP = 4,
        BOTTOM = 8
    };

	// +0A [Byte] Sprite Colour Palette: Source index into ROM.
	//            For the car sprite:
	//            0-2 = Wheels turning
	//            3-5 = Brake lights on, Wheels turning
	uint16_t pal_src;

	// +0B [Byte] Sprite Colour Palette: Destination index into PALETTE RAM. 0 - 7F.
	uint8_t pal_dst;

	// +0C [Word] Sprite X (Screen/Camera) \   (Set from world co-ordinates)
	// +0E [Word] Sprite Y (Screen/Camera) /
	int16_t x, y;

	// +10 [Word] Sprite Width
	uint16_t width;

	// +14 [Word] Sprite to Sprite Priority (Lower Number = Draw Earlier)
	uint16_t priority;

	// +16 [Word] Entry Number In Sprite Destination Table 
	// This is the offset address in memory we want to copy the sprite to.
	// And is necessary because it ties in with sprite ordering.
	uint16_t dst_index;

	// +18 [Long] Address of actual sprite data we want to render. Offset 1/2 into ROM[0x20000]. 
	uint32_t addr;

	// +1C [Word] Distance into screen or Sprite to Road Priority (High Number = Closer to camera)
	uint16_t road_priority;

	// +1E [Word] Sprite Counter Reload Value (For Animations etc.)
	int16_t reload;

	// +20 [Word] Sprite Counter (For Animations etc.)
	uint16_t counter;

	// +22 [Word] Sprite X (World) COPY
	int16_t xw1;

	// +24 [Long] Sprite Z / Zoom (World) - 0x10000 = Most distant value
	int32_t z;

    // +28 [Word] Original Traffic Speed
    int16_t traffic_speed;

	// +2A [Word] Sprite Type (Actual Object Index)
	uint16_t type;

	// +2C [Word] Sprite X (World) COPY
	int16_t xw2;

    // +2E [Word] Bit 2 - Denote car is close to other traffic [z-axis]
    //            Bit 1 - Denote traffic on RHS
    //            Bit 0 - Denote traffic on LHS
           
    // Note: Set to 0xFF on collision
    uint8_t traffic_proximity;

    // +30 [Word]
    uint8_t traffic_fx;

    // +32 [Word] Original Traffic Speed
    int16_t traffic_orig_speed;

    // +36 [Word] Nearby Traffic Speed
    int16_t traffic_near_speed;

	// +38 [Word] Sprite Y (World)
	uint16_t yw;

    // +3A [Word] Passenger Props (Passenger Sprites Only)
    int16_t pass_props;

	// Initalize to default values
	void init(uint8_t i)
	{
        control = 0;
        jump_index = i;
		function_holder = -1;
		id = 0;
		shadow = 3;
		zoom = 0;
		draw_props = 0;
		pal_src = 0;
		pal_dst = 0;
		x = 0;
		y = 0;
		width = 0;
		priority = 0;
		dst_index = 0;
		addr = 0;
		road_priority = 0;
		reload = 0;
		counter = 0;
        xw1 = 0;
        z = 0;
        traffic_speed = 0;
        type = 0;
        xw2 = 0;
        traffic_proximity = 0;
        traffic_fx = 0;
        traffic_orig_speed = 0;
        traffic_near_speed = 0;
        yw = 0;
        pass_props = 0;
	}
};
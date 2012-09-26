#pragma once

// ------------------------------------------------------------------------------------------------
// Settings
// ------------------------------------------------------------------------------------------------

// Frame Rate (either 30 which is the same as the original or 60)
const int FRAMES_PER_SECOND = 60;

const bool DEBUG_LEVEL = false;

// Fix bugs present in the original game
const bool FIX_BUGS = true;

// Sega OutRun Screen Properties
const uint16_t S16_WIDTH = 320;
const uint16_t S16_HEIGHT = 224;

// 0 = Easy, 1 = Normal, 2 = Hard, 3 = Very Hard (Note these aren't same numbers as original)
const uint8_t DIP_TRAFFIC = 1;

// 0 = Easy, 1 = Normal, 2 = Hard, 3 = Very Hard
const uint8_t DIP_TIME = 0;

// Automatic Gears
const bool GEAR_AUTO = false;

enum 
{
	BIT_0 = 0x01,
	BIT_1 = 0x02,
	BIT_2 = 0x04,
	BIT_3 = 0x08,
	BIT_4 = 0x10,
	BIT_5 = 0x20,
	BIT_6 = 0x40,
	BIT_7 = 0x80,
    BIT_8 = 0x100,
    BIT_9 = 0x200,
    BIT_A = 0x400,
};
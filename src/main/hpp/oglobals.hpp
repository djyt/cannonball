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

enum 
{
	GS_INIT = 0,				//0  = Initalize Game
	GS_ATTRACT = 1,				//1  = Attract Mode
	GS_INIT_BEST1 = 2,			//2  = Load Best Outrunners
	GS_BEST1 = 3,				//3  = Best Outrunners (Attract Mode)
	GS_INIT_LOGO = 4,			//4  = Load Outrun Logo
	GS_LOGO = 5,				//5  = Outrun Logo (Attract Mode)
	GS_INIT_MUSIC = 6,			//6  = Load Music Selection Screen
	GS_MUSIC = 7,				//7  = Music Selection Screen
	GS_INIT_GAME = 8,			//8  = Loading In-Game
	GS_START1 = 9,				//9  = Start Game, Car Driving In
	GS_START2 = 10,				//A  = Start Game, Countdown
	GS_START3 = 11,				//B  = Start Game, Countdown 2
	GS_INGAME = 12,				//C  = Start Game, User in control
	GS_INIT_BONUS = 13,			//D  = Load Bonus Points
	GS_BONUS = 14,				//E  = Display Bonus Points
	GS_INIT_GAMEOVER = 15,	    //F  = Load Game Over
	GS_GAMEOVER = 16,           //10 = Game Over Text
	GS_INIT_MAP = 17,			//11 = Load Course Map
	GS_MAP = 18,				//12 = Course Map
	GS_INIT_BEST2 = 19,			//13 = Load Best Outrunners
	GS_BEST2 = 20,			    //14 = Best Outrunners
	GS_REINIT = 21,				//15 = Reinitalize Game (after outrunners screen)
};

// Road width at merge point
const uint16_t RD_WIDTH_MERGE = 0xD4;

// Max speed of car
const uint32_t MAX_SPEED = 0x1260000;
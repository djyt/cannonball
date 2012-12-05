/***************************************************************************
    OutRun Engine Entry Point.

    This is the hub of the ported OutRun code.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.hpp"
#include "roms.hpp"
#include "globals.hpp"

#include "sdl/video.hpp"

#include "frontend/config.hpp"

// Main include for Ported OutRun Code
#include "oaddresses.hpp"
#include "otiles.hpp"
#include "oentry.hpp"
#include "osprite.hpp"
#include "osprites.hpp"
#include "oferrari.hpp"
#include "osmoke.hpp"
#include "otraffic.hpp"
#include "olevelobjs.hpp"
#include "ologo.hpp"
#include "oroad.hpp"
#include "oinitengine.hpp"
#include "oinputs.hpp"
#include "ohud.hpp"
#include "ocrash.hpp"
#include "oattractai.hpp"
#include "oanimseq.hpp"
#include "ostats.hpp"
#include "omap.hpp"
#include "ohiscore.hpp"
#include "omusic.hpp"
#include "obonus.hpp"
#include "audio/osoundint.hpp"

// Globals
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

class Outrun
{
public:
    // Service Mode Toggle: Not implemented yet.
    bool service_mode;

    // Frame counter
	uint32_t frame;

    // Tick Logic. Used when running at non-standard > 30 fps
    bool tick_frame;

    // Tick Counter (always syncd to 30 fps to flash text and other stuff)
    uint32_t tick_counter;

    // Main game state
    int8_t game_state;

	Outrun();
	~Outrun();
	void init();
	void tick();
	void vint();

private:    
    // Set to debug a particular level
    static const uint8_t LOAD_LEVEL = 0;

    // Car Increment Backup for attract mode
    uint32_t car_inc_bak;

    // Debug to denote when fork has been chosen
    int8_t fork_chosen;
	void jump_table();
	void init_jump_table();
	void main_switch();
    void controls();
    bool decrement_timers();
};

extern Outrun outrun;

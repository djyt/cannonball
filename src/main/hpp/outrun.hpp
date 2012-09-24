#pragma once

#include "stdint.hpp"
#include "roms.hpp"
#include "options.hpp"

// Main include for Ported OutRun Code
#include "oglobals.hpp"
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

class Outrun
{
public:
    Options options;

    bool pause;

    bool service_mode;

    // Frame counter
	uint32_t frame;

    // Tick Logic. Used when running at non-standard > 30 fps
    bool tick_frame;

    // Tick Counter (always syncd to 30 fps to flash text and other stuff)
    uint32_t tick_counter;

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

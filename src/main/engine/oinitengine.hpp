/***************************************************************************
    Core Game Engine Routines.
    
    - The main loop which advances the level onto the next segment.
    - Code to directly control the road hardware. For example, the road
      split and bonus points routines.
    - Code to determine whether to initialize certain game modes
      (Crash state, Bonus points, road split state) 
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

class OInitEngine
{
public:
    // Debug: Camera X Offset
    int16_t camera_x_off;

    // Is the in-game engine active?
    bool ingame_engine;

    // Time to wait before enabling ingame_engine after crash
    int16_t ingame_counter;

	// Road Segment Master.
	// Points to location in ROM used to setup road_seg_addrx and other variables
	uint32_t road_seg_master;

	// road_seg_addr1 Format: [4 byte boundaries]

	// [+0] Segment Position [Word]
	// [+2] Number Of Sprites In Segment [Byte]
	// [+3] Sprite Data Entry Number From Lookup Table * 4 [Byte]
	uint32_t road_seg_addr1;

	// road_seg_addr2 Format: [8 byte boundaries]
	//
	// [+0] Segment Position
	// [+2] Set = Denotes Road Height Info. Unset = Denotes Road Width
	// [+4] Segment Road Width / Segment Road Height Index
	// [+6] Segment Width Adjustment SIGNED (Speed at which width is adjusted)
	uint32_t road_seg_addr2;

	// road_seg_addr3 [6 byte boundaries]
	//
	// [+0] Segment Position
	// [+2] Unknown data
	// [+4] Segment Road type (1 = Straight, 2 = Slight Bend, 3 = Severe Bend)
	uint32_t road_seg_addr3;

    // Road Split State
    // 0 = No Road Split
    // 1 = Init Road Split
    // 2 = Road Split
    // 3 = Beginning of split. User must choose.
    // 4 = Road physically splits into two individual roads
    // 5 = Road fully split. Init remove other road
    // 6 = Road split. Only one road drawn.
    // 7 = Unknown
    // 8 = Init Road Merge before checkpoint sign
    // 9 = Road Merge before checkpoint sign
    // A = Unknown
    // B = Checkpoint sign 
    // C = Unused
    // D = Unused
    // E = Unused
    // F = Unused
    // 10 = Init Bonus Points Sequence
    // 11 = Bonus Points Sequence
    uint16_t rd_split_state;
    enum {SPLIT_NONE, SPLIT_INIT, SPLIT_CHOICE1, SPLIT_CHOICE2};

    // Upcoming Road Type:
    // 0 = No change
    // 1 = Straight road
    // 2 = Right Bend
    // 3 = Left Bend
    int16_t road_type;
    int16_t road_type_next;
    enum {ROAD_NOCHANGE, ROAD_STRAIGHT, ROAD_RIGHT, ROAD_LEFT};

    // End Of Stage Properties
    //
    // Bit 0: Denotes in road-split code. Road physically splitting.
    // Bit 1: Attract Mode: Set to loop back to Stage 1
    // Bit 2: Attract Mode: Set to loop back to Stage 1
    // Bit 3: Denote setup_sky_change done
    uint8_t end_stage_props;

    uint32_t car_increment; // NEEDS REPLACING. Implementing here as a quick hack so routine works

    // Car X Position              
    // 0000 = Centre of two road generators
    //
    // Turning Right REDUCES value, Turning Left INCREASES value
    //    
    // 0xxx [pos] = Road Generator 1 Position (0 - xxx from centre)
    // Fxxx [neg] = Road Generator 2 Position (0 + xxx from centre)
    int16_t car_x_pos;
    int16_t car_x_old;

    // Checkpoint Marker

    // 0  = Checkpoint Not Past
    // -1 = Checkpoint Past
    int8_t checkpoint_marker;

    // Something to do with the increment / curve of the road
    int16_t road_curve;
    int16_t road_curve_next;

    // Road split logic handling to remove split
    // 0 = Normal Road Rendering
    // 1 = Road Has Split, Old Road now removed
    int8_t road_remove_split;

    // Route Selected
    // -1 = Left
    // 0  = Right
    // But confusingly, these values get swapped by a not instruction
    int8_t route_selected;

	OInitEngine();
	~OInitEngine();
	void init();
    void debug_load_level(uint8_t);

    void init_road_seg_master();
    void init_crash_bonus();
    void update_road();
    void update_engine();
    void set_granular_position();

    void init_bonus(); // moved here for debugging purposes

    static const uint8_t stage_data[];

private:
    // Road width at merge point
    const static uint16_t RD_WIDTH_MERGE = 0xD4;

    // Road width of next section
    int16_t road_width_next;

    // Speed at which adjustment to road section occurs
    int16_t road_width_adj;

    // Road Width Change 
    // 0 = No
    // -1 = In Progress
    int16_t change_width;

    int16_t granular_rem;

    uint16_t pos_fine_old;

    // Road Original Width. Used when adjustments are being made during road split, and end sequence initialisation
    int16_t road_width_orig;

    // Used by road merge logic, to control width of road
    int16_t road_width_merge;
    
    // ------------------------------------------------------------------------
    // Route Information
    // ------------------------------------------------------------------------

    // Used as part of road split code.
    // 0 = Route Info Not Updated
    // 1 = Route Info Updated
    int8_t route_updated;
    
    void setup_stage1();
    void check_road_split();
    void check_stage();
    void init_split1();
    void init_split2();
    void init_split3();
    void init_split4();
    void init_split5();
    void init_split6();
    void init_split7();
    void init_split9();
    void init_split10();
    void bonus1();
    void bonus2();
    void bonus3();
    void bonus4();
    void bonus5();
    void bonus6();
    void init_split_next_level();
    void test_bonus_mode(bool);
};

extern OInitEngine oinitengine;
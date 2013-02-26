/***************************************************************************
    Road Rendering & Control

    This is a complete port of the 68000 SUB CPU Program ROM.
    
    The original code consists of a shared Sega library and some routines
    which are OutRun specific.
    
    Some of the original code is not used and is therefore not ported.
    
    This is the most complex area of the game code, and an area of the code
    in need of refactoring.

    Useful background reading on road rendering:
    http://www.extentofthejam.com/pseudo/

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/


#pragma once

class ORoad
{
public:
    bool debug_road;

    uint32_t road_pos;        // 0x6: Current Road Position (addressed as long and word)
    int16_t tilemap_h_target; // 0xA: Tilemap H target

	// Stage Lookup Offset - Used to retrieve various data from in-game tables
	//
	// Stage Map:
	//
	// 24  23  22  21  20 
	//   1B  1A  19  18
	//     12  11  10
	//       09  08 
	//         00 
	//
	// Increments by 8 each stage.
	// Also increments by +1 during the road split section from the values shown above.
	//
	// 0x38: Set to -8 during bonus mode section
	int16_t stage_lookup_off;

	// These pointers rotate and select the current chunk of road data to blit
	uint16_t road_p0; // 0x3A: Road Pointer 0
	uint16_t road_p1; // 0x3C: Road Pointer 1 (Working road data)
	uint16_t road_p2; // 0x3E: Road Pointer 2 (Chunk of road to be blitted)
	uint16_t road_p3; // Ox40: Road Pointer 3 (Horizon Y Position)

    // 0x4C: Road Width Backup
	int16_t road_width_bak;

    // 0x4E: Car X Backup
	int16_t car_x_bak;

    // 0x66: Road Height Lookup 
	uint16_t height_lookup;

    // 0x6C: Change in road position
    int32_t road_pos_change; 
	
	// 0x5E: Instruct CPU 1 to load bonus road. Set Bit 1.
	uint8_t road_load_bonus;

	// 0x306: Road Control
	uint8_t road_ctrl;
	enum 
	{
		ROAD_OFF = 0,         // Both Roads Off
		ROAD_R0 = 1,          // Road 0
		ROAD_R1 = 2,          // Road 1
		ROAD_BOTH_P0 = 3,     // Both Roads (Road 0 Priority) [DEFAULT]
		ROAD_BOTH_P1 = 4,     // Both Roads (Road 1 Priority) 
		ROAD_BOTH_P0_INV = 5, // Both Roads (Road 0 Priority) (Road Split. Invert Road 1)
		ROAD_BOTH_P1_INV = 6, // Both Roads (Road 1 Priority) (Road Split. Invert Road 1)
		ROAD_R0_SPLIT = 7,    // Road 0 (Road Split.)
		ROAD_R1_SPLIT = 8,    // Road 1 (Road Split. Invert Road 1)
	};

	// 0x30B: Road Load Split.
	// This should be set to tell CPU 1 to init the road splitting code
	// 0    = Do Not Load
	// 0xFF = Load
	int8_t road_load_split;

	// 0x314: Road Width
	// There are two road generators.
	//
	// The second road is drawn at an offset from the first, so that it either appears as one solid road, or as two separate roads.
	//
	// 00 = 3 lanes
	// D4 = 6 lanes
	//
	// Once the distance is greater than F0 or so, it's obvious there are two independent roads.
	int32_t road_width;// DANGER! USED AS LONG AND WORD

	// 0x420: Offset Into Road Data [Current Road Position * 4]
    // Moved from private for tracked
	uint16_t road_data_offset;

	// 0x4F0: Start Address of Road Data For Current Stage In ROM
    // TODO - move back to being private at some stage
	uint32_t stage_addr;

    // 0x510: Horizon Y Position
	int16_t horizon_y2;
    int16_t horizon_y_bak;

	// 0x53C: Granular Position. More fine than other positioning info. Used to choose road background colour.
	uint16_t pos_fine;

    // 0x732 - [long] Base Horizon Y-Offset. Adjusting this almost has the effect of raising the camera. 
    // Stage 1 is 0x240
    // Higher values = higher horizon.
    // Note: This is adjusted mid-stage for Stage 2, but remains constant for Stage 1.
	int32_t horizon_base;

	// 0x736: 0 = Base Horizon Value Not Set. 1 = Value Set.
	uint8_t horizon_set;

    const static uint16_t ARRAY_LENGTH = 0x200;

	// 60800 - 60BFF: Road X-Positions [Before H-Scroll Is Applied] - Same Data For Both Roads
	int16_t road_x[ARRAY_LENGTH];

	// 60C00 - 60FFF: Road 0 H-Scroll Adjusted Positions
	int16_t road0_h[ARRAY_LENGTH];
	
	// 61000 - 613FF: Road 1 H-Scroll Adjusted Positions
	int16_t road1_h[ARRAY_LENGTH];

	// 61400 - 617FF: Not sure what this is yet
	int16_t road_unk[ARRAY_LENGTH];

	// 61800 - 637FF: Road Y-Positions	
    //
    // Consists of three separate blocks of data:
    //
    // Offset 0x000: Source Data. List of sequential numbers to indicate when to read next road value from source rom.
    //               Numbers iterate down sequentially, until a hill, when they will rise again
    // Offset 0x280: Priority information for road (during elevated sections). Used to hide sprites
    // Offset 0x400: Destination Data. Final converted data to be output to road hardware
    //
    // This format is repeated four times, due to the way values rotate through road ram
	int16_t road_y[0x1000];

	ORoad();
	~ORoad();
	void init();
	void tick();
    int16_t get_road_y(uint16_t);

private:
	uint16_t stage_loaded; // 0x4: Current Stage Backup (So we know when to load next stage road data)

	uint32_t road_pos_old; // 0x410: Road Position Backup

	// 60530 - [word] Distance into section of track, for height #1
    // Ranges from 0x100 - 0x1FF
	uint16_t height_start;

	// 0x536 - [word] Controls switch statement when processing road height
    //                0 = Clear Road Height Segment
    //                1 = Init Next Road Height Segment
    //                2 = Use Elevation
    //                3 = 
    //                4 = 
    //                5 = Set Base Horizon
	uint16_t height_ctrl;

	// 0x542: Granular Position Backup.
	uint16_t pos_fine_old;

	// 0x544 - [word] Difference between granular positions. 
	int16_t pos_fine_diff;

	// 0x70E - [word] Counter. Counts to 7. Denotes Interpolated track section currently being written.
	int8_t counter;

	// 0x710 - Index Into Height Data (generally specifies hill type for specific section of road)
    // The results of the value will differ depending on which road height section currently on.
    // Not to be confused with 6072A, which is a much larger number.
    // Hack to see different values: wpset 60710,2,r,1,{w@60710 = 5; g;}
	int16_t height_index;

	// 0x712 - [long] Final Height Value. Takes Horizon and distance into screen into account.
	int32_t height_final;

	// 0x716 - [word] Increment Value For 0x710 to adjust lookup from height data
	uint16_t height_inc;

	// 0x718 - [word] This stores the position into the current road segment we're on. 
    // Derived from the granular position and used in conjunction with road height.
    // As a hack try wpset 60718,2,r,1,{w@60718 = 0x6b5; g;}
    // you'll stall at a position on the current road segment after value is set.
	uint16_t height_step;

	// 0x71A - [word] Jump Table Control For Road Height (Read from Road Data).
	uint16_t height_ctrl2;

	// 0x71C - [long] Stores current position into road height data. This is an actual address.
	uint32_t height_addr;

	// 0x720 - [word] Elevation Flag
	int16_t elevation;
	enum {DOWN = -1, FLAT = 0, UP = 1};
								 
	// 0x722 - [word] Road Height Index. Working copy of 60066.
	uint16_t height_lookup_wrk;

	// 0x724 - [word] Ascend/Descent Modifier Combo
	int16_t updowncombo;

	// 0x726 - [word] Distance Control
	uint16_t dist_ctrl;

	// 0x728 
	uint16_t do_height_inc;

	// 0x72A - [word] Distance into section of track, for height #2
    //             Ranges from 0x100 - 0x1FF
	uint16_t height_end;

	// 0x72C: Up Multiplier
	int8_t up_mult;

	// 0x72E: Down Multiplier
	int8_t down_mult;

	// 0x73A: 0 = Base Horizon Value Not Set. 1 = Value Set.
	uint32_t horizon_mod;

	// 60700: Lengths of the 7 road segments 60700 - 6070D
	uint16_t section_lengths[7];
	int8_t length_offset;
	uint32_t a1_lookup;

    // Registers - todo: refactor these
    int32_t change_per_entry; // [d2]
    int32_t d5_o;
    uint32_t a3_o;
    uint32_t y_addr;
    int16_t scanline;
    int32_t total_height;

	// -------------------------------------------------------------------------
	// Locations in Road RAM
	// -------------------------------------------------------------------------
	static const uint32_t HW_HSCROLL_TABLE0 = 0x80400;
	static const uint32_t HW_HSCROLL_TABLE1 = 0x80800;
	static const uint32_t HW_BGCOLOR = 0x80C00;

	void init_road_code();
	void set_default_hscroll();
	void clear_road_ram();
	void init_stage1();
	void do_road();
	void rotate_values();
	void check_load_road();
	
	void setup_road_x();
	void setup_x_data();
	void set_tilemap_x();
	void add_next_road_pos(uint32_t*);
	void create_curve(int16_t&, int16_t&,
                      const int32_t, const int32_t, const int16_t, const int16_t);

	void setup_hscroll();
	void do_road_offset(int16_t*, int16_t, bool);

	void setup_road_y();
	void init_height_seg();

	void init_elevation(uint32_t&);
	void do_elevation();
	void init_elevation_hill(uint32_t&);
	void do_elevation_hill();
    void init_level_4d(uint32_t&);
    void do_level_4d();
	void init_horizon_adjust(uint32_t&);
	void do_horizon_adjust();

	void set_road_y();
	void set_y_interpolate();
	void set_y_horizon();
	void set_y_2044();
	void pos_done();
	void read_next_height();
	void set_elevation();

	void set_horizon_y();
	void do_road_data();
	
	void blit_roads();
	void blit_road(uint32_t);
	
	void output_hscroll(int16_t*, uint32_t);
	void copy_bg_color();
};

extern ORoad oroad;
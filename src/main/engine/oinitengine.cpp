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

#include "trackloader.hpp"

#include "engine/oanimseq.hpp"
#include "engine/obonus.hpp"
#include "engine/ocrash.hpp"
#include "engine/oferrari.hpp"
#include "engine/ohud.hpp"
#include "engine/oinputs.hpp"
#include "engine/olevelobjs.hpp"
#include "engine/omusic.hpp"
#include "engine/ooutputs.hpp"
#include "engine/ostats.hpp"
#include "engine/outils.hpp"
#include "engine/opalette.hpp"
#include "engine/osmoke.hpp"
#include "engine/otiles.hpp"
#include "engine/otraffic.hpp"
#include "engine/oinitengine.hpp"

OInitEngine oinitengine;

// Continuous Mode Level Ordering
const static uint8_t CONTINUOUS_LEVELS[] = {0, 0x8, 0x9, 0x10, 0x11, 0x12, 0x18, 0x19, 0x1A, 0x1B, 0x20, 0x21, 0x22, 0x23, 0x24};

OInitEngine::OInitEngine()
{
}


OInitEngine::~OInitEngine()
{
}

// Source: 0x8360
void OInitEngine::init(int8_t level)
{
    ostats.game_completed  = 0;

    ingame_engine          = false;
    ingame_counter         = 0;
    ostats.cur_stage       = 0;
    oroad.stage_lookup_off = level ? level : 0;
    rd_split_state         = SPLIT_NONE;
    road_type              = ROAD_NOCHANGE;
    road_type_next         = ROAD_NOCHANGE;
    end_stage_props        = 0;
    car_increment          = 0;
    car_x_pos              = 0;
    car_x_old              = 0;
    checkpoint_marker      = 0;
    road_curve             = 0;
    road_curve_next        = 0;
    road_remove_split      = 0;
    route_selected         = 0;
    
    road_width_next        = 0;
    road_width_adj         = 0;
    change_width           = 0;
    granular_rem           = 0;
    pos_fine_old           = 0;
    road_width_orig        = 0;
    road_width_merge       = 0;
    route_updated          = 0;

	init_road_seg_master();

    // Road Renderer: Setup correct stage address 
    if (level)
        trackloader.init_path(oroad.stage_lookup_off);

	opalette.setup_sky_palette();
	opalette.setup_ground_color();
	opalette.setup_road_centre();
	opalette.setup_road_stripes();
	opalette.setup_road_side();
	opalette.setup_road_colour();   
    otiles.setup_palette_hud();                     // Init Default Palette for HUD
    osprites.copy_palette_data();                   // Copy Palette Data to RAM
    otiles.setup_palette_tilemap();                 // Setup Palette For Tilemap
    setup_stage1();                                 // Setup Misc stuff relating to Stage 1
    otiles.reset_tiles_pal();                       // Reset Tiles, Palette And Road Split Data
    ocrash.clear_crash_state();

    // The following is set up specifically for time trial mode
    if (level)
    {  
        otiles.init_tilemap_palette(level);
        oroad.road_ctrl  = ORoad::ROAD_BOTH_P0;
        oroad.road_width = RD_WIDTH_MERGE;        // Setup a default road width
    }

    osoundint.reset();
}

// Source: 0x8402
void OInitEngine::setup_stage1()
{
    oroad.road_width = 0x1C2 << 16;     // Force display of two roads at start
    ostats.score = 0;
    ostats.clear_stage_times();
    oferrari.reset_car();               // Reset Car Speed/Rev Values
    outrun.outputs->set_digital(OOutputs::D_EXT_MUTE);
    outrun.outputs->set_digital(OOutputs::D_SOUND);
    osoundint.engine_data[sound::ENGINE_VOL] = 0x3F;
    ostats.extend_play_timer = 0;
    checkpoint_marker = 0;              // Denote not past checkpoint marker
    otraffic.set_max_traffic();         // Set Number Of Enemy Cars Based On Dip Switches
    ostats.clear_route_info();
    osmoke.setup_smoke_sprite(true);
}

// Initialise Master Segment Address For Stage
//
// 1. Read Internal Stage Number from Stage Data Table (Using the lookup offset)
// 2. Load the master address, using the stage number as an index.
//
// Source: 0x8C80

void OInitEngine::init_road_seg_master()
{
    trackloader.init_track(oroad.stage_lookup_off);
}

//
// Check Road Width
// Source: B85A
//
// Potentially Update Width Of Road
//
// ADDRESS 2 - Road Segment Data [8 byte boundaries]:
//
// Word 1 [+0]: Segment Position
// Word 2 [+2]: Set = Denotes Road Height Info. Unset = Denotes Road Width
// Word 3 [+4]: Segment Road Width / Segment Road Height Index
// Word 4 [+6]: Segment Width Adjustment SIGNED (Speed at which width is adjusted)

void OInitEngine::update_road()
{
    check_road_split(); // Check/Process road split if necessary
    uint32_t addr = 0;
    uint16_t d0 = trackloader.read_width_height(&addr);
    // Update next road section
    if (d0 <= oroad.road_pos >> 16)
    {
        // Skip road width adjustment if set and adjust height
        if (trackloader.read_width_height(&addr) == 0)
        {
            // ROM:0000B8A6 skip_next_width
            if (oroad.height_lookup == 0)
                 oroad.height_lookup = trackloader.read_width_height(&addr); // Set new height lookup section
        }
        else
        {
            // ROM:0000B87A
            int16_t width  = trackloader.read_width_height(&addr); // Segment road width
            int16_t change = trackloader.read_width_height(&addr); // Segment adjustment speed

            if (width != (int16_t) (oroad.road_width >> 16))
            {
                if (width <= (int16_t) (oroad.road_width >> 16))
                    change = -change;

                road_width_next = width;
                road_width_adj  = change;
                change_width = -1; // Denote road width is changing
            }
        }
        trackloader.wh_offset += 8;
    }

    // ROM:0000B8BC set_road_width    
    // Width of road is changing & car is moving
    if (change_width != 0 && car_increment >> 16 != 0)
    {
        int32_t d0 = ((car_increment >> 16) * road_width_adj) << 4;
        oroad.road_width += d0; // add long here
        if (d0 > 0)
        {    
            if (road_width_next < (int16_t) (oroad.road_width >> 16))
            {
                change_width = 0;
                oroad.road_width = road_width_next << 16;
            }
        }
        else
        {
            if (road_width_next >= (int16_t) (oroad.road_width >> 16))
            {
                change_width = 0;
                oroad.road_width = road_width_next << 16;
            }
        }
    }
    // ------------------------------------------------------------------------------------------------
    // ROAD SEGMENT FORMAT
    //
    // Each segment of road is 6 bytes in memory, consisting of 3 words
    // Each road segment is a significant length of road btw :)
    //
    // ADDRESS 3 - Road Segment Data [6 byte boundaries]
    //
    // Word 1 [+0]: Segment Position (used with 0x260006 car position)
    // Word 2 [+2]: Segment Road Curve
    // Word 3 [+4]: Segment Road type (1 = Straight, 2 = Right Bend, 3 = Left Bend)
    //
    // 60a08 = address of next road segment? (e.g. A0 = 0x0001DD86)
    // ------------------------------------------------------------------------------------------------

    // ROM:0000B91C set_road_type: 

    int16_t segment_pos = trackloader.read_curve(0);

    if (segment_pos != -1)
    {
        int16_t d1 = segment_pos - 0x3C;

        if (d1 <= (int16_t) (oroad.road_pos >> 16))
        {
            road_curve_next = trackloader.read_curve(2);
            road_type_next  = trackloader.read_curve(4);
        }

        if (segment_pos <= (int16_t) (oroad.road_pos >> 16))
        {
            road_curve = trackloader.read_curve(2);
            road_type  = trackloader.read_curve(4);
            trackloader.curve_offset += 6;
            road_type_next = 0;
            road_curve_next = 0;
        }
    }
}

// Carries on from the above in the original code
void OInitEngine::update_engine()
{   
    // ------------------------------------------------------------------------
    // TILE MAP OFFSETS
    // ROM:0000B986 setup_shadow_offset:
    // Setup the shadow offset based on how much we've scrolled left/right. Lovely and subtle!
    // ------------------------------------------------------------------------

    update_shadow_offset();

    // ------------------------------------------------------------------------
    // Main Car Logic Block
    // ------------------------------------------------------------------------

    oferrari.move();

    if (oferrari.car_ctrl_active)
    {
        oferrari.set_curve_adjust();
        oferrari.set_ferrari_x();
        oferrari.do_skid();
        oferrari.check_wheels();
        oferrari.set_ferrari_bounds();
    }

    oferrari.do_sound_score_slip();

    // ------------------------------------------------------------------------
    // Setup New Sprite Scroll Speed. Based On Granular Difference.
    // ------------------------------------------------------------------------
    set_granular_position();
    set_fine_position();

    // Draw Speed & Hud Stuff
    if (outrun.game_state >= GS_START1 && outrun.game_state <= GS_BONUS)
    {
        // Convert & Blit Car Speed
        ohud.blit_speed(0x110CB6, car_increment >> 16);
        ohud.blit_text1(HUD_KPH1);
        ohud.blit_text1(HUD_KPH2);

        // Blit High/Low Gear
        if (config.controls.gear == config.controls.GEAR_BUTTON && !config.cannonboard.enabled)
        {
            if (oinputs.gear)
                ohud.blit_text_new(9, 26, "H");
            else
                ohud.blit_text_new(9, 26, "L");
        }

        if (config.engine.layout_debug)
            ohud.draw_debug_info(oroad.road_pos, oroad.height_lookup_wrk, trackloader.read_sprite_pattern_index());
    }

    if (olevelobjs.spray_counter > 0)
        olevelobjs.spray_counter--;

    if (olevelobjs.sprite_collision_counter > 0)
        olevelobjs.sprite_collision_counter--;

    opalette.setup_sky_cycle();
}

void OInitEngine::update_shadow_offset()
{
    int16_t shadow_off = oroad.tilemap_h_target & 0x3FF;
    if (shadow_off > 0x1FF)
        shadow_off = -shadow_off + 0x3FF;
    shadow_off >>= 2;
    if (oroad.tilemap_h_target & BIT_A)
        shadow_off = -shadow_off; // reverse direction of shadow
    osprites.shadow_offset = shadow_off;
}

// Check for Road Split
//
// - Checks position in level and determine whether to init road split
// - Processes road split if initialized
//
// Source: 868E
void OInitEngine::check_road_split()
{
    // Check whether to initialize the next level
    ostats.init_next_level();

    switch (rd_split_state)
    {
        // State 0: No road split. Check current road position with ROAD_END.
        case SPLIT_NONE:
            if (oroad.road_pos >> 16 <= ROAD_END) return; 
            check_stage(); // Do Split - Split behaviour depends on stage
            break;

        // State 1: (Does this ever need to be called directly?)
        case SPLIT_INIT:
            init_split1();
            break;
        
        // State 2: Road Split
        case SPLIT_CHOICE1:
            if (oroad.road_pos >> 16 >= 0x3F)  
                init_split2();            
            break;

        // State 3: Beginning of split. User must choose.
        case SPLIT_CHOICE2:
            init_split2();
            break;

        // State 4: Road physically splits into two individual roads
        case 4:
            init_split3();
            break;

        // State 5: Road fully split. Init remove other road
        case 5:
            if (!road_curve)
                rd_split_state = 6; // and fall through
            else
                break;
        
        // State 6: Road split. Only one road drawn.
        case 6:
            init_split5();
            break;

        // Stage 7
        case 7:
            init_split6();
            break;

        // State 8: Init Road Merge before checkpoint sign
        case 8:
            otraffic.traffic_split = -1;
            rd_split_state = 9;
            break;

        // State 9: Road Merge before checkpoint sign
        case 9:
            otraffic.traffic_split = 0;
        case 0x0A:
            init_split9();
            break;

        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
            init_split10();
            break;
        
        // Init Bonus Sequence
        case 0x10:
            init_bonus(oroad.stage_lookup_off - 0x20);
            break;

        case 0x11:
            bonus1();
            break;

        case 0x12:
            bonus2();
            break;

        case 0x13:
            bonus3();
            break;

        case 0x14:
            bonus4();
            break;

        case 0x15:
            bonus5();
            break;

        case 0x16:
        case 0x17:
        case 0x18:
            bonus6();
            break;
    }
}

// ------------------------------------------------------------------------------------------------
// Check Stage Info To Determine What To Do With Road
//
// Stage 1-4: Init Road Split
// Stage 5: Init Bonus
// Stage 5 ATTRACT: Loop to Stage 1
// ------------------------------------------------------------------------------------------------
void OInitEngine::check_stage()
{
    // Time Trial Mode
    if (outrun.cannonball_mode == Outrun::MODE_TTRIAL)
    {
        // Store laptime and reset
        uint8_t* laptimes = outrun.ttrial.laptimes[outrun.ttrial.current_lap];

        laptimes[0] = ostats.stage_times[0][0];
        laptimes[1] = ostats.stage_times[0][1];
        laptimes[2] = ostats.stage_times[0][2];
        ostats.stage_times[0][0] = 
        ostats.stage_times[0][1] = 
        ostats.stage_times[0][2] = 0;

        // Check for new best laptime
        int16_t counter = ostats.stage_counters[outrun.ttrial.current_lap];
        if (counter < outrun.ttrial.best_lap_counter)
        {
            outrun.ttrial.best_lap_counter = counter;
            outrun.ttrial.best_lap[0] = laptimes[0];
            outrun.ttrial.best_lap[1] = laptimes[1];
            outrun.ttrial.best_lap[2] = ostats.lap_ms[laptimes[2]];

            // Draw best laptime
            ostats.extend_play_timer = 0x80;
            ohud.blit_text1(TEXT1_LAPTIME1);
            ohud.blit_text1(TEXT1_LAPTIME2);
            ohud.draw_lap_timer(0x110554, laptimes, ostats.lap_ms[laptimes[2]]);

            outrun.ttrial.new_high_score = true;
        }

        if (outrun.game_state == GS_INGAME)
        {
            // More laps to go, loop the course
            if (++outrun.ttrial.current_lap < outrun.ttrial.laps)
            {
                // Update lap number
                oroad.road_pos = 0;
                oroad.tilemap_h_target = 0;
                init_road_seg_master();
            }
            else 
            {
                // Set correct finish segment for final 5 stages, otherwise just default to first one.
                oroad.stage_lookup_off = oroad.stage_lookup_off < 0x20 ? 0x20 : oroad.stage_lookup_off;
                ostats.time_counter = 1;
                init_bonus(oroad.stage_lookup_off - 0x20);
            }
        }
    }
    else if (outrun.cannonball_mode == Outrun::MODE_CONT)
    {
        oroad.road_pos         = 0;
        oroad.tilemap_h_target = 0;
        
        if ((ostats.cur_stage + 1) == 15)
        {
            if (outrun.game_state == GS_INGAME)
                init_bonus(outils::random() % 5);
            else
                reload_stage1();
        }
        else
        {
            oroad.stage_lookup_off = CONTINUOUS_LEVELS[++ostats.cur_stage];
            init_road_seg_master();
            osprites.clear_palette_data();

            // Init next tilemap
            otiles.set_vertical_swap(); // Tell tilemap to v-scroll off/on

            // Reload smoke data
            osmoke.setup_smoke_sprite(true);

            // Update palette
            oinitengine.end_stage_props |= BIT_1; // Don't bump stage offset when fetching next palette
            oinitengine.end_stage_props |= BIT_2;
            opalette.pal_manip_ctrl = 1;
            opalette.setup_sky_change();
            
            // Denote Checkpoint Passed
            checkpoint_marker = -1;

            // Cycle Music every 5 stages
            if (outrun.game_state == GS_INGAME)
            {
                if (ostats.cur_stage == 5 || ostats.cur_stage == 10)
                {
                    switch (omusic.music_selected)
                    {
                        // Cycle in-built sounds
                        case sound::MUSIC_BREEZE:
                            omusic.music_selected = sound::MUSIC_SPLASH;
                            osoundint.queue_sound(sound::MUSIC_SPLASH2); // Play without rev effect
                            break;
                        case sound::MUSIC_SPLASH:
                            omusic.music_selected = sound::MUSIC_MAGICAL;
                            osoundint.queue_sound(sound::MUSIC_MAGICAL2); // Play without rev effect
                            break;
                        case sound::MUSIC_MAGICAL:
                            omusic.music_selected = sound::MUSIC_BREEZE;
                            osoundint.queue_sound(sound::MUSIC_BREEZE2); // Play without rev effect
                            break;
                    }                 
                }
            }              
        }
    }

    // Stages 0-4, do road split
    else if (ostats.cur_stage <= 3)
    {
        rd_split_state = SPLIT_INIT;
        init_split1();
    }
    // Stage 5: Init Bonus
    else if (outrun.game_state == GS_INGAME)
    {
        init_bonus(oroad.stage_lookup_off - 0x20);
    }
    // Stage 5 Attract Mode: Reload Stage 1
    else
    {
        reload_stage1();
    }
}

void OInitEngine::reload_stage1()
{
    oroad.road_pos         = 0;
    oroad.tilemap_h_target = 0;
    ostats.cur_stage       = -1;
    oroad.stage_lookup_off = -8;

    ostats.clear_route_info();

    end_stage_props |= BIT_1; // Loop back to stage 1 (Used by tilemap code)
    end_stage_props |= BIT_2;
    end_stage_props |= BIT_3;
    osmoke.setup_smoke_sprite(true);
    init_split_next_level();
}

// ------------------------------------------------------------------------------------------------
// Road Split 1
// Init Road Split & Begin Road Split
// Called When We're Not On The Final Stage
// ------------------------------------------------------------------------------------------------
void OInitEngine::init_split1()
{
    rd_split_state = SPLIT_CHOICE1;

    oroad.road_load_split  = -1;
    oroad.road_ctrl        = ORoad::ROAD_BOTH_P0_INV; // Both Roads (Road 0 Priority) (Road Split. Invert Road 0)
    road_width_orig        = oroad.road_width >> 16;
    oroad.road_pos         = 0;
    oroad.tilemap_h_target = 0;
    trackloader.init_track_split();
}

// ------------------------------------------------------------------------------------------------
// Road Split 2: Beginning of split. User must choose.
// ------------------------------------------------------------------------------------------------
void OInitEngine::init_split2()
{
    rd_split_state = SPLIT_CHOICE2;

    // Manual adjustments to the road width, based on the current position
    int16_t pos = (((oroad.road_pos >> 16) - 0x3F) << 3) + road_width_orig;
    oroad.road_width = (pos << 16) | (oroad.road_width & 0xFFFF);
    if (pos > 0xFF)
    {
        route_updated &= ~BIT_0;
        init_split3();
    }
}

// ------------------------------------------------------------------------------------------------
// Road Split 3: Road physically splits into two individual roads
// ------------------------------------------------------------------------------------------------
void OInitEngine::init_split3()
{
    rd_split_state = 4;

    int16_t pos = (((oroad.road_pos >> 16) - 0x3F) << 3) + road_width_orig;
    oroad.road_width = (pos << 16) | (oroad.road_width & 0xFFFF);

    if (route_updated & BIT_0 || pos <= 0x168)
    {
        if (oroad.road_width >> 16 > 0x300)
            init_split4();
        return;
    }

    route_updated |= BIT_0; // Denote route info updated
    route_selected = 0;

    // Go Left
    if (car_x_pos > 0)
    {
        route_selected = -1;
        uint8_t inc = 1 << (3 - ostats.cur_stage);

        // One of the following increment values

        // Stage 1 = +8 (1 << 3 - 0)
        // Stage 2 = +4 (1 << 3 - 1)
        // Stage 3 = +2 (1 << 3 - 2)
        // Stage 4 = +1 (1 << 3 - 3)
        // Stage 5 = Road doesn't split on this stage

        ostats.route_info += inc;
        oroad.stage_lookup_off++;
    }
    // Go Right / Continue

    end_stage_props |= BIT_0;                                 // Set end of stage property (road splitting)
    osmoke.load_smoke_data |= BIT_0;                          // Denote we should load smoke sprite data
    ostats.routes[0]++;                                       // Set upcoming stage number to store route info
    ostats.routes[ostats.routes[0]] = ostats.route_info;      // Store route info for course map screen

    if (oroad.road_width >> 16 > 0x300)
        init_split4();
}

// ------------------------------------------------------------------------------------------------
// Road Split 4: Road Fully Split, Remove Other Road
// ------------------------------------------------------------------------------------------------

void OInitEngine::init_split4()
{
    rd_split_state = 5; // init_split4

     // Set Appropriate Road Control Value, Dependent On Route Chosen
    if (route_selected != 0)
        oroad.road_ctrl = ORoad::ROAD_R0_SPLIT;
    else
        oroad.road_ctrl = ORoad::ROAD_R1_SPLIT;

    // Denote road split has been removed (for enemy traFfic logic)
    road_remove_split |= BIT_0;
       
    if (!road_curve)
        init_split5();
}

// ------------------------------------------------------------------------------------------------
// Road Split 5: Only Draw One Section Of Road - Wait For Final Curve
// ------------------------------------------------------------------------------------------------

void OInitEngine::init_split5()
{
    rd_split_state = 6;
    if (road_curve)
        init_split6();
}

// ------------------------------------------------------------------------------------------------
// Road Split 6 - Car On Final Curve Of Split
// ------------------------------------------------------------------------------------------------
void OInitEngine::init_split6()
{
    rd_split_state = 7;
    if (!road_curve)
        init_split7();
}

// ------------------------------------------------------------------------------------------------
// Road Split 7: Init Road Merge Before Checkpoint (From Normal Section Of Road)
// ------------------------------------------------------------------------------------------------

void OInitEngine::init_split7()
{
    rd_split_state = 8;

    oroad.road_ctrl = ORoad::ROAD_BOTH_P0;
    route_selected = ~route_selected; // invert bits
    int16_t width2 = (oroad.road_width >> 16) << 1;
    if (route_selected == 0) 
        width2 = -width2;
    car_x_pos += width2;
    car_x_old += width2;
    road_width_orig = oroad.road_pos >> 16;
    road_width_merge = oroad.road_width >> 19; // (>> 16 and then >> 3)
    road_remove_split &= ~BIT_0; // Denote we're back to normal road handling for enemy traffic logic
}

// ------------------------------------------------------------------------------------------------
// Road Split 9 - Do Road Merger. Road Gets Narrower Again.
// ------------------------------------------------------------------------------------------------
void OInitEngine::init_split9()
{
    rd_split_state = 10;

    // Calculate narrower road width to merge roads
    uint16_t d0 = (road_width_merge - ((oroad.road_pos >> 16) - road_width_orig)) << 3;

    if (d0 <= RD_WIDTH_MERGE)
    {
        oroad.road_width = (RD_WIDTH_MERGE << 16) | (oroad.road_width & 0xFFFF);
        init_split10();
    }
    else
        oroad.road_width = (d0 << 16) | (oroad.road_width & 0xFFFF);
}


// ------------------------------------------------------------------------------------------------
// Road Split A: Checkpoint Sign
// ------------------------------------------------------------------------------------------------
void OInitEngine::init_split10()
{
    rd_split_state = 11;

    if (oroad.road_pos >> 16 > 0x180)
    {
        rd_split_state = 0;
        init_split_next_level();
    }
}

// ------------------------------------------------------------------------------------------------
// Road Split B: Init Next Level
// ------------------------------------------------------------------------------------------------
void OInitEngine::init_split_next_level()
{
    oroad.road_pos = 0;
    oroad.tilemap_h_target = 0;
    ostats.cur_stage++;
    oroad.stage_lookup_off += 8;    // Increment lookup to next block of stages
    ostats.route_info += 0x10;      // Route Info increments by 10 at each stage
    ohud.do_mini_map();
    init_road_seg_master();

    // Clear sprite palette lookup
    if (ostats.cur_stage)
        osprites.clear_palette_data();
}

// ------------------------------------------------------------------------------------------------
// Bonus Road Mode Control
// ------------------------------------------------------------------------------------------------

// Initialize new segment of road data for bonus sequence
// Source: 0x8A04
void OInitEngine::init_bonus(int16_t seq)
{
    oroad.road_ctrl = ORoad::ROAD_BOTH_P0_INV;
    oroad.road_pos  = 0;
    oroad.tilemap_h_target = 0;
    oanimseq.end_seq = (uint8_t) seq; // Set End Sequence (0 - 4)
    trackloader.init_track_bonus(oanimseq.end_seq);
    outrun.game_state = GS_INIT_BONUS;
    rd_split_state = 0x11;
    bonus1();
}

void OInitEngine::bonus1()
{
    if (oroad.road_pos >> 16 >= 0x5B)
    {
        otraffic.bonus_lhs = 1; // Force traffic spawn on LHS during bonus mode
        rd_split_state = 0x12;
        bonus2();
    }
}

void OInitEngine::bonus2()
{
    if (oroad.road_pos >> 16 >= 0xB6)
    {
        otraffic.bonus_lhs = 0; // Disable forced traffic spawn
        road_width_orig = oroad.road_width >> 16;
        rd_split_state = 0x13;
        bonus3();
    }
}

// Stretch the road to a wider width. It does this based on the car's current position.
void OInitEngine::bonus3()
{
    // Manual adjustments to the road width, based on the current position
    int16_t pos = (((oroad.road_pos >> 16) - 0xB6) << 3) + road_width_orig;
    oroad.road_width = (pos << 16) | (oroad.road_width & 0xFFFF);
    if (pos > 0xFF)
    {
        route_selected = 0;
        if (car_x_pos > 0)
            route_selected = ~route_selected; // invert bits
        rd_split_state = 0x14;
        bonus4();
    }
}

void OInitEngine::bonus4()
{
    // Manual adjustments to the road width, based on the current position
    int16_t pos = (((oroad.road_pos >> 16) - 0xB6) << 3) + road_width_orig;
    oroad.road_width = (pos << 16) | (oroad.road_width & 0xFFFF);
    if (pos > 0x300)
    {
         // Set Appropriate Road Control Value, Dependent On Route Chosen
        if (route_selected != 0)
            oroad.road_ctrl = ORoad::ROAD_R0_SPLIT;
        else
            oroad.road_ctrl = ORoad::ROAD_R1_SPLIT;

        // Denote road split has been removed (for enemy traFfic logic)
        road_remove_split |= BIT_0;
        rd_split_state = 0x15;
        bonus5();
    }
}

// Check for end of curve. Init next state when ended.
void OInitEngine::bonus5()
{
    if (!road_curve)
    {
        rd_split_state = 0x16;
        bonus6();
    }
}

// This state simply checks for the end of bonus mode
void OInitEngine::bonus6()
{
    if (obonus.bonus_control >= OBonus::BONUS_END)
        rd_split_state = 0;
}

// SetGranularPosition
//
// Source: BD3E
//
// Uses the car increment value to set the granular position.
// The granular position is used to finely scroll the road by CPU 1 and smooth zooming of scenery.
//
// pos_fine is the (road_pos >> 16) * 10
//
// Notes:
// Disable with - bpset bd3e,1,{pc = bd76; g}

void OInitEngine::set_granular_position()
{
    uint16_t car_inc16 = car_increment >> 16;

    uint16_t result = car_inc16 / 0x40;
    uint16_t rem = car_inc16 % 0x40;

    granular_rem += rem;
    // When the overall counter overflows past 0x40, we must carry a 1 to the unsigned divide :)
    if (granular_rem >= 0x40)
    {
        granular_rem -= 0x40;
        result++;
    }
    oroad.pos_fine += result;
}

void OInitEngine::set_fine_position()
{
    uint16_t d0 = oroad.pos_fine - pos_fine_old;
    if (d0 > 0xF)
        d0 = 0xF;

    d0 <<= 0xB;
    osprites.sprite_scroll_speed = d0;

    pos_fine_old = oroad.pos_fine;
}

// Check whether to initalize crash or bonus sequence code
//
// Source: 0x984E
void OInitEngine::init_crash_bonus()
{
    if (outrun.game_state == GS_MUSIC) return;

    if (ocrash.skid_counter > 6 || ocrash.skid_counter < -6)
    {
        // do_skid:
        if (otraffic.collision_traffic == 1)
        {   
            otraffic.collision_traffic = 2;
            uint8_t rnd = outils::random() & otraffic.collision_mask;
            if (rnd == otraffic.collision_mask)
            {
                // Try to launch crash code and perform a spin
                if (ocrash.coll_count1 == ocrash.coll_count2)
                {
                    if (!ocrash.spin_control1)
                    {
                        ocrash.spin_control1 = 1;
                        ocrash.skid_counter_bak = ocrash.skid_counter;
                        test_bonus_mode(true); // 9924 fall through
                        return;
                    }
                    test_bonus_mode(false); // finalise skid               
                    return;
                }
                else
                {
                    test_bonus_mode(true); // test_bonus_mode
                    return;
                }
            }
        }
    }
    else if (ocrash.spin_control2 == 1)
    {
        // 9894
        ocrash.spin_control2 = 2;
        if (ocrash.coll_count1 == ocrash.coll_count2)
        {
            ocrash.enable();
        }
        test_bonus_mode(false); // finalise skid
        return;
    }
    else if (ocrash.spin_control1 == 1)
    {
        // 98c0
        ocrash.spin_control1 = 2;
        ocrash.enable();
        test_bonus_mode(false); // finalise skid
        return;
    }

    // 0x9924: Section Of Code
    if (ocrash.coll_count1 == ocrash.coll_count2) 
    {
        test_bonus_mode(true);  // test_bonus_mode
    }
    else
    {
        ocrash.enable();
        test_bonus_mode(false); // finalise skid
    }
}

// Source: 0x993C
void OInitEngine::test_bonus_mode(bool do_bonus_check)
{
    // Bonus checking code 
    if (do_bonus_check && obonus.bonus_control)
    {
        // Do Bonus Text Display
        if (outrun.cannonball_mode != Outrun::MODE_TTRIAL && obonus.bonus_state < 3)
            obonus.do_bonus_text();

        // End Seq Animation Stage #0
        if (obonus.bonus_control == OBonus::BONUS_SEQ0)
        {
            if (outrun.cannonball_mode == Outrun::MODE_TTRIAL)
                outrun.game_state = GS_INIT_GAMEOVER;
            else
                oanimseq.init_end_seq();
        }
    }

   // finalise_skid:
   if (!ocrash.skid_counter)
       otraffic.collision_traffic = 0;
}
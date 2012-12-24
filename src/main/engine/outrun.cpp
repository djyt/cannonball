/***************************************************************************
    OutRun Engine Entry Point.

    This is the hub of the ported OutRun code.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/outrun.hpp"
#include "engine/opalette.hpp"
#include "engine/outils.hpp"

Outrun outrun;

/*
    Known Core Engine Issues:

    - Road split. Minor bug on positioning traffic on correct side of screen for one frame or so at the point of split.
      Most noticeable in 60fps mode. 
      The Dreamcast version exhibits a bug where the road renders on the wrong side of the screen for one frame at this point.
      The original version (and Cannonball) has a problem where the cars face the wrong direction for one frame. 

    Bugs Present In Original 1986 Release:

    - Millisecond displays incorrectly on Extend Time screen [fixed]
    - Erroneous values in sprite zooming table [fixed]
    - Shadow popping into position randomly. Try setting car x position to 0x1E2. (0x260050) [fixed]
    - Stage 2a: Incomplete arches due to lack of sprite slots [fixed]
    - Best OutRunners screen looks odd after Stage 2 Gateway
    - Stage 3c: Clouds overlapping trees

*/

Outrun::Outrun()
{
}

Outrun::~Outrun()
{
}

void Outrun::init()
{
    game_state = GS_INIT;

    video.clear_text_ram();

    frame = 0;
    tick_frame = true;
    tick_counter = 0;
    ohiscore.init_def_scores();  // Initialize default hi-score entries
    config.load_scores();        // Load saved hi-score entries
    ostats.init();
    oinitengine.init();
    osoundint.init();
    init_jump_table();
    outils::reset_random_seed(); // Ensure we match the genuine boot up of the original game each time

    if (LOAD_LEVEL)
        oinitengine.debug_load_level(LOAD_LEVEL);
}


void Outrun::tick()
{
    /*if (input.has_pressed(Input::END_SEQ))
    {
        game_state = GS_INIT_BONUS;
        oroad.road_width = 0x90;
        ostats.time_counter = 2;
        oroad.stage_lookup_off = 0x23;
        oinitengine.route_selected = -1;
        oinitengine.init_bonus();
    }*/

    frame++;

    // Non standard FPS.
    // Determine whether to tick the current frame.
    if (config.fps != 30)
    {
        if (config.fps == 60)
            tick_frame = frame & 1;
        else if (config.fps == 120)
            tick_frame = (frame & 3) == 1;
    }

    if (tick_frame)
    {
        tick_counter++;       
        controls();      // Analogue Controls
    }
    oinputs.do_gear();   // Digital Gear

    // Only tick the road cpu twice for every time we tick the main cpu
    // The timing here isn't perfect, as normally the road CPU would run in parallel with the main CPU.
    // We can potentially hack this by calling the road CPU twice.
    // Most noticeable with clipping sprites on hills.
      
    // 30 FPS 
    // Updates Game Logic 1/2 frames
    // Updates V-Blank 1/2 frames
    if (config.fps == 30 && config.tick_fps == 30)
    {
        jump_table();
        oroad.tick();
        vint();
        vint();
    }
    // 30/60 FPS Hybrid. (This is the same as the original game)
    // Updates Game Logic 1/2 frames
    // Updates V-Blank 1/1 frames
    else if (config.fps == 60 && config.tick_fps == 30)
    {
        if (tick_frame)
        {
            jump_table();
            oroad.tick();
        }
        vint();
    }
    // 60 FPS. Smooth Mode. 
    // Updates Game Logic 1/1 frames
    // Updates V-Blank 1/1 frames
    else
    {
        jump_table();
        oroad.tick();
        vint();
    }
}

// Vertical Interrupt
void Outrun::vint()
{
    otiles.write_tilemap_hw();

    if (osprites.do_sprite_swap)
    {
        osprites.do_sprite_swap = false;

        video.sprite_layer->swap();

        // Do Sprite RAM Swap and copy new palette data if necessary
        osprites.copy_palette_data();
    }
    otiles.update_tilemaps();

    if (config.fps < 120 || (frame & 1))
    {
        opalette.cycle_sky_palette();
        opalette.fade_palette();
        // ... 
        ostats.do_timers();
        ohud.draw_timer1(ostats.time_counter);
        oinputs.do_credits();
        oinitengine.set_granular_position();
    }
}

void Outrun::jump_table()
{
    if (tick_frame)
    {
        main_switch();                  // Address #1 (0xB128) - Main Switch
        oinputs.adjust_inputs();        // Address #2 (0x74D8) - Adjust Analogue Inputs
    }

    switch (game_state)
    {
        // ----------------------------------------------------------------------------------------
        // Couse Map Specific Code
        // ----------------------------------------------------------------------------------------
        case GS_MAP:
            omap.tick();
            break;


        case GS_MUSIC:
            osprites.tick();
            olevelobjs.do_sprite_routine();

            if (!outrun.tick_frame)
            {
                omusic.check_start();
                omusic.blit();
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Best OutRunners Entry (EditJumpTable3 Entries)
        // ----------------------------------------------------------------------------------------
        case GS_INIT_BEST2:
        case GS_BEST2:
            osprites.tick();
            olevelobjs.do_sprite_routine();

            if (!outrun.tick_frame)
            {
                // Check for start button if credits are remaining and set state to Music Selection
                if (ostats.credits && input.has_pressed(Input::START))
                    game_state = GS_INIT_MUSIC;
            }
            break;

        case GS_REINIT:
            break;

        // ----------------------------------------------------------------------------------------
        // Core Game Engine Routines
        // ----------------------------------------------------------------------------------------
        case GS_LOGO:
            if (!outrun.tick_frame)
                ologo.blit();
        
        default:
            if (tick_frame) osprites.tick();                // Address #3 Jump_SetupSprites
            olevelobjs.do_sprite_routine();                 // replaces calling each sprite individually
            if (!config.engine.disable_traffic)
                otraffic.tick();                            // Spawn & Tick Traffic
            if (tick_frame) oinitengine.init_crash_bonus(); // Initalize crash sequence or bonus code
            oferrari.tick();
            if (oferrari.state != OFerrari::FERRARI_END_SEQ)
            {
                oanimseq.flag_seq();
                ocrash.tick();
                osmoke.draw_ferrari_smoke(&osprites.jump_table[OSprites::SPRITE_SMOKE1]); // Do Left Hand Smoke
                oferrari.draw_shadow();                                                   // (0xF1A2) - Draw Ferrari Shadow
                osmoke.draw_ferrari_smoke(&osprites.jump_table[OSprites::SPRITE_SMOKE2]); // Do Right Hand Smoke
            }
            else
            {
                osmoke.draw_ferrari_smoke(&osprites.jump_table[OSprites::SPRITE_SMOKE1]); // Do Left Hand Smoke
                osmoke.draw_ferrari_smoke(&osprites.jump_table[OSprites::SPRITE_SMOKE2]); // Do Right Hand Smoke
            }
            break;
    }

    osprites.sprite_copy();
}

// Source: 0xB15E
void Outrun::main_switch()
{
    switch (game_state)
    {
        case GS_INIT:  
            osoundint.has_booted = true;
            oferrari.car_inc_old = car_inc_bak >> 16;
            oinitengine.car_increment = car_inc_bak;
            oferrari.car_ctrl_active = true;
            ostats.time_counter = 0x15;
            ostats.frame_counter = ostats.frame_reset;
            // todo: turn screen on
            game_state = GS_ATTRACT;
            // fall through
            
        // ----------------------------------------------------------------------------------------
        // Attract Mode
        // ----------------------------------------------------------------------------------------
        case GS_ATTRACT:
            // Set credits to 1 if in free play mode (note display still reads Free Play)
            if (ostats.free_play)
                ostats.credits = 1;

            ohud.draw_credits();
            ohud.draw_copyright_text();
            ohud.draw_insert_coin();
            if (ostats.credits)
                game_state = GS_INIT_MUSIC;
            else if (decrement_timers())
            {
                car_inc_bak = oinitengine.car_increment;
                game_state = GS_INIT_BEST1;
            }
            break;

        case GS_INIT_BEST1:
            oferrari.car_ctrl_active = false;
            oinitengine.car_increment = 0;
            oferrari.car_inc_old = 0;
            ostats.time_counter = 5;
            ostats.frame_counter = ostats.frame_reset;
            ohiscore.init();
            osoundint.queue_sound(sound::FM_RESET);
            game_state = GS_BEST1;

        case GS_BEST1:
            ohud.draw_copyright_text();
            ohiscore.display_scores();
            ohud.draw_credits();
            ohud.draw_insert_coin();
            if (ostats.credits)
                game_state = GS_INIT_MUSIC;
            else if (decrement_timers())
                game_state = GS_INIT_LOGO;
            break;

        case GS_INIT_LOGO:
            video.clear_text_ram();
            oferrari.car_ctrl_active = false;
            oinitengine.car_increment = 0;
            oferrari.car_inc_old = 0;
            ostats.time_counter = 5;
            ostats.frame_counter = ostats.frame_reset;
            osoundint.queue_sound(0);
            ologo.enable(sound::FM_RESET);
            game_state = GS_LOGO;

        case GS_LOGO:
            ohud.draw_credits();
            ohud.draw_copyright_text();
            ohud.draw_insert_coin();
            ologo.tick();

            if (ostats.credits)
                game_state = GS_INIT_MUSIC;
            else if (decrement_timers())
            {
                ologo.disable();
                game_state = GS_INIT; // Resume attract mode
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Music Select Screen
        // ----------------------------------------------------------------------------------------

        case GS_INIT_MUSIC:
            omusic.enable();
            game_state = GS_MUSIC;

        case GS_MUSIC:
            ohud.draw_credits();
            ohud.draw_insert_coin();
            omusic.check_start(); // Check for start button
            omusic.tick();
            if (decrement_timers())
            {
                omusic.disable();
                game_state = GS_INIT_GAME;
            }
            break;
        // ----------------------------------------------------------------------------------------
        // In-Game
        // ----------------------------------------------------------------------------------------

        case GS_INIT_GAME:
            //ROM:0000B3E0                 bclr    #5,(ppi1_value).l                   ; Turn screen off (not activated until PPI written to)
            //ROM:0000B3E8                 move.w  #-1,(ingame_active1).l              ; Denote in-game engine is active
            //ROM:0000B3F0                 clr.l   (prev_game_time).l                  ; Reset overall game time
            //ROM:0000B3F6                 move.w  #-1,(ingame_active2).l
            video.clear_text_ram();
            oferrari.car_ctrl_active = true;
            oinitengine.init();
            init_jump_table();
            // Timing Hack to ensure horizon is correct
            // Note that the original code disables the screen, and waits for the second CPU's interrupt instead
            oroad.tick();
            oroad.tick();
            oroad.tick();
            osoundint.queue_sound(sound::STOP_CHEERS);
            osoundint.queue_sound(sound::VOICE_GETREADY);
            osoundint.queue_sound(omusic.music_selected);
            ostats.time_counter = ostats.TIME[config.engine.dip_time * 40]; // Set time to begin level with
            ostats.frame_counter = ostats.frame_reset + 50;                 // set this to 49 for testing purposes
            ohud.draw_main_hud();
            ostats.credits--;                                   // Update Credits
            ohud.blit_text1(TEXT1_CLEAR_START);
            ohud.blit_text1(TEXT1_CLEAR_CREDITS);
            oroad.road_width = 0x1C2 << 16;
            osoundint.queue_sound(sound::INIT_CHEERS);
            game_state = GS_START1;
            // fall through

        //  Start Game - Car Driving In
        case GS_START1:
        case GS_START2:
            if (--ostats.frame_counter < 0)
            {
                osoundint.queue_sound(sound::SIGNAL1);
                ostats.frame_counter = ostats.frame_reset;
                game_state++;
            }
            break;

        case GS_START3:
            if (--ostats.frame_counter < 0)
            {
                osoundint.queue_sound(sound::SIGNAL2);
                osoundint.queue_sound(sound::STOP_CHEERS);
                ostats.frame_counter = ostats.frame_reset;
                game_state++;
            }
            break;

        case GS_INGAME:
            if (decrement_timers())
                game_state = GS_INIT_GAMEOVER;
            break;

        // ----------------------------------------------------------------------------------------
        // Bonus Mode
        // ----------------------------------------------------------------------------------------
        case GS_INIT_BONUS:
            ostats.frame_counter = ostats.frame_reset;
            obonus.bonus_control = OBonus::BONUS_INIT;  // Initialize Bonus Mode Logic
            oroad.road_load_bonus |= BIT_0;             // Instruct CPU 1 to load bonus road section
            ostats.game_completed |= BIT_0;             // Denote game completed
            obonus.bonus_timer = 3600;                  // Safety Timer Added in Rev. A Roms
            game_state = GS_BONUS;

        case GS_BONUS:
            if (--obonus.bonus_timer < 0)
            {
                obonus.bonus_control = OBonus::BONUS_DISABLE;
                game_state = GS_INIT_GAMEOVER;
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Display Game Over Text
        // ----------------------------------------------------------------------------------------
        case GS_INIT_GAMEOVER:
            oferrari.car_ctrl_active = false; // -1
            oinitengine.car_increment = 0;
            oferrari.car_inc_old = 0;
            ostats.time_counter = 3;
            ostats.frame_counter = ostats.frame_reset;
            ohud.blit_text2(TEXT2_GAMEOVER);
            osoundint.queue_sound(sound::NEW_COMMAND);
            game_state = GS_GAMEOVER;

        case GS_GAMEOVER:
            if (decrement_timers())
                game_state = GS_INIT_MAP;
            break;

        // ----------------------------------------------------------------------------------------
        // Display Course Map
        // ----------------------------------------------------------------------------------------
        case GS_INIT_MAP:
            oferrari.car_ctrl_active = false; // -1
            video.clear_text_ram();
            osprites.disable_sprites();
            otraffic.disable_traffic();
            osprites.clear_palette_data();
            oinitengine.car_increment = 0;
            oferrari.car_inc_old = 0;
            osprites.spr_cnt_main = 0;
            osprites.spr_cnt_shadow = 0;
            oroad.road_ctrl = ORoad::ROAD_BOTH_P0;
            oroad.horizon_base = -0x3FF;
            otiles.fill_tilemap_color(0xABD); //  Paint pinkish colour on tilemap 16
            ohud.blit_text2(TEXT2_COURSEMAP);
            game_state = GS_MAP;
            // fall through
            omap.init_sprites = true;

        case GS_MAP:
            break;

        // ----------------------------------------------------------------------------------------
        // Best OutRunners / Score Entry
        // ----------------------------------------------------------------------------------------
        case GS_INIT_BEST2:
            // bsr.w   EndGame
            osprites.disable_sprites();
            otraffic.disable_traffic();
            // bsr.w   EditJumpTable3
            osprites.clear_palette_data();
            olevelobjs.hiscore_entries(); // Setup default sprites for screen
            ocrash.coll_count1   = 0;
            ocrash.coll_count2   = 0;
            ocrash.crash_counter = 0;
            ocrash.skid_counter  = 0;
            ocrash.spin_control1 = 0;
            oferrari.car_ctrl_active = false; // -1
            oinitengine.car_increment = 0;
            oferrari.car_inc_old = 0;
            ostats.time_counter = 0x30;
            ostats.frame_counter = ostats.frame_reset;
            ohiscore.init();
            osoundint.queue_sound(sound::NEW_COMMAND);
            game_state = GS_BEST2;
            // fall through

        case GS_BEST2:
            ohiscore.tick(); // Do High Score Logic

            // Check for start button if credits are remaining and set state to Music Selection
            if (ostats.credits && input.has_pressed(Input::START))
            {
                game_state = GS_INIT_MUSIC;
            }

            ohud.draw_credits();

            // If countdown has expired
            if (decrement_timers())
            {
                //ROM:0000B700                 bclr    #5,(ppi1_value).l                   ; Turn screen off (not activated until PPI written to)
                oferrari.car_ctrl_active = true; // 0 : Allow road updates
                oinitengine.init();
                init_jump_table();
                //ROM:0000B716                 bclr    #0,(byte_260550).l
                game_state = GS_REINIT;          // Reinit game to attract mode
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Reinitialize Game After High Score Entry
        // ----------------------------------------------------------------------------------------
        case GS_REINIT:
            video.clear_text_ram();
            game_state = GS_INIT;
            break;
    }

    oinitengine.check_road_width();

    // --------------------------------------------------------------------------------------------
    // Debugging Only
    // --------------------------------------------------------------------------------------------
    if (DEBUG_LEVEL)
    {
        if (oinitengine.rd_split_state != 0)
        {
            if (!fork_chosen)
            {
                if (oinitengine.camera_x_off < 0)
                    fork_chosen = -1;
                else
                    fork_chosen = 1;        
            }
        }
        else if (fork_chosen)
            fork_chosen = 0;

        // Hack to allow user to choose road fork with left/right
        if (fork_chosen == -1)
        {
            oroad.road_width_bak = oroad.road_width >> 16; 
            oroad.car_x_bak = -oroad.road_width_bak; 
            oinitengine.car_x_pos = oroad.car_x_bak;
        }
        else
        {
            oroad.road_width_bak = oroad.road_width >> 16; 
            oroad.car_x_bak = oroad.road_width_bak; 
            oinitengine.car_x_pos = oroad.car_x_bak;
        } 
    }

}

// Setup Jump Table. Move from ROM to RAM.
//
// Source Address: 0x7E1C
// Input:          Sprite To Copy
// Output:         None
//
// ROM Format [0xF000 - 0xF1F5]
//
// Word 1: Number of entries [7D]
// Long 1: Address 1 (address of jump information)
// ...
// Long x: Address x
//
// Each address in the jump table is a pointer into ROM containing 0x1F words 
// of info (so info is at 0x40 boundary in bytes)
//
// RAM Format[0x61800]
//
// 0x00 byte: If high byte set, take jump
// 0x01 byte: Index number
// 0x02 long: Address to jump to
void Outrun::init_jump_table()
{
    // Reset value to restore car increment to during attract mode
    car_inc_bak = 0;

    osprites.init();
    if (!LOAD_LEVEL) olevelobjs.default_entries();
    otraffic.init();
    osmoke.init();
    oroad.init();
    opalette.init();
    oinputs.init();
    obonus.init();

    video.tile_layer->set_x_clamp(video.tile_layer->RIGHT);
    video.sprite_layer->set_x_clip(false);
}

void Outrun::controls()
{
    // ------------------------------------------------------------------------
    // DEBUG CONTROLS
    // ------------------------------------------------------------------------
    if (DEBUG_LEVEL)
    {
        if (input.is_pressed(Input::LEFT))
             oinitengine.camera_x_off += 5;
        if (input.is_pressed(Input::RIGHT))
             oinitengine.camera_x_off -= 5;

        // Start/Stop 
        if (input.has_pressed(Input::BUTTON3))
        {
            if (oinitengine.car_increment != 0xd0 << 16)
                oinitengine.car_increment = 0xd0 << 16;
            else
                oinitengine.car_increment = 0;
        }

        if (input.is_pressed(Input::UP))
            oinitengine.car_increment = 0x20 << 16;
        else if (oinitengine.car_increment != 0xd0 << 16)
            oinitengine.car_increment = 0;

        if (input.is_pressed(Input::BUTTON1))
        {
            oroad.horizon_base += -20;
            if (oroad.horizon_base < 0x100)
                oroad.horizon_base = 0x100;
        }

        if (input.is_pressed(Input::BUTTON2))
        {
            oroad.horizon_base += 20;
            if (oroad.horizon_base > 0x6A0)
                oroad.horizon_base = 0x6A0;
        }
    }

    // ------------------------------------------------------------------------
    // NORMAL CONTROLS
    // ------------------------------------------------------------------------
    else
    {
        oinputs.simulate_analog();
    }
}

// -------------------------------------------------------------------------------
// Decrement Game Time
// 
// Decrements Frame Count, and Overall Time Counter
//
// Returns true if timer expired.
// Source: 0xB736
// -------------------------------------------------------------------------------
bool Outrun::decrement_timers()
{
    // Cheat
    if (config.engine.freeze_timer)
        return false;

    if (--ostats.frame_counter >= 0)
        return false;

    ostats.frame_counter = ostats.frame_reset;

    ostats.time_counter = outils::bcd_sub(1, ostats.time_counter);
    
    return (ostats.time_counter < 0);
}
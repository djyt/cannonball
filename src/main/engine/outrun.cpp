/***************************************************************************
    OutRun Engine Entry Point.

    This is the hub of the ported OutRun code.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "setup.hpp"
#include "main.hpp"
#include "trackloader.hpp"
#include "../utils.hpp"
#include "engine/oattractai.hpp"
#include "engine/oanimseq.hpp"
#include "engine/obonus.hpp"
#include "engine/ocrash.hpp"
#include "engine/oferrari.hpp"
#include "engine/ohiscore.hpp"
#include "engine/ohud.hpp"
#include "engine/oinputs.hpp"
#include "engine/olevelobjs.hpp"
#include "engine/ologo.hpp"
#include "engine/omap.hpp"
#include "engine/omusic.hpp"
#include "engine/ooutputs.hpp"
#include "engine/osmoke.hpp"
#include "engine/outrun.hpp"
#include "engine/opalette.hpp"
#include "engine/ostats.hpp"
#include "engine/otiles.hpp"
#include "engine/otraffic.hpp"
#include "engine/outils.hpp"
#include "cannonboard/interface.hpp"

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
    - Stage 3c: Clouds overlapping trees [unable to fix easily]
    - Sometimes the Ferrari stalls on the start-line on game restart. Happens in Attract Mode too.
    - On completion screen, some of the side crowd graphics are misplaced. Japanese version only [fixed]

*/

Outrun::Outrun()
{
    outputs = new OOutputs();
}

Outrun::~Outrun()
{
    delete outputs;
}

void Outrun::init()
{
    freeze_timer = cannonball_mode == MODE_TTRIAL ? true : config.engine.freeze_timer;
    video.enabled = false;
    select_course(config.engine.jap != 0, config.engine.prototype != 0);
    video.clear_text_ram();

    tick_counter = 0;

    // CannonBoard Config: When Used in original cabinet
    if (config.cannonboard.enabled && config.cannonboard.cabinet == config.cannonboard.CABINET_MOVING)
        init_motor_calibration();
    else
        boot();
}

void Outrun::boot()
{
    game_state = config.engine.layout_debug ? GS_INIT_GAME : GS_INIT;
    // Initialize default hi-score entries
    ohiscore.init_def_scores();
    // Load saved hi-score entries
    config.load_scores(cannonball_mode == Outrun::MODE_ORIGINAL ? FILENAME_SCORES : FILENAME_CONT);        
    ostats.init(cannonball_mode == MODE_TTRIAL);
    init_jump_table();
    oinitengine.init(cannonball_mode == MODE_TTRIAL ? ttrial.level : 0);
    osoundint.init();
    outils::reset_random_seed(); // Ensure we match the genuine boot up of the original game each time
}

void Outrun::tick(Packet* packet, bool tick_frame)
{
    this->tick_frame = tick_frame;
    //Packet* packet = config.cannonboard.enabled ? &cannonboard->get_packet() : NULL;

    if (game_state >= GS_START1 && game_state <= GS_INGAME)
    {
        if (input.has_pressed(Input::VIEWPOINT))
        {
            int mode = oroad.get_view_mode() + 1;
            if (mode > ORoad::VIEW_INCAR)
                mode = ORoad::VIEW_ORIGINAL;

            oroad.set_view_mode(mode);
        }
    }

    if (cannonball::tick_frame)
    {
        tick_counter++;
        oinputs.tick(packet); // Do Controls
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
        jump_table(packet);
        oroad.tick();
        vint();
        vint();
    }
    // 30/60 FPS Hybrid. (This is the same as the original game)
    // Updates Game Logic 1/2 frames
    // Updates V-Blank 1/1 frames
    else if (config.fps == 60 && config.tick_fps == 30)
    {
        if (cannonball::tick_frame)
        {
            jump_table(packet);
            oroad.tick();
        }
        vint();
    }
    // 60 FPS. Smooth Mode. 
    // Updates Game Logic 1/1 frames
    // Updates V-Blank 1/1 frames
    else
    {
        jump_table(packet);
        oroad.tick();
        vint();
    }

    // Draw FPS
    if (config.video.fps_count)
        ohud.draw_fps_counter(cannonball::fps_counter);
}

// Vertical Interrupt
void Outrun::vint()
{
    otiles.write_tilemap_hw();
    osprites.update_sprites();
    otiles.update_tilemaps(cannonball_mode == MODE_ORIGINAL ? ostats.cur_stage : 0);

    if (config.fps < 120 || (cannonball::frame & 1))
    {
        opalette.cycle_sky_palette();
        opalette.fade_palette();
        // ... 
        ostats.do_timers();
        if (cannonball_mode != MODE_TTRIAL) ohud.draw_timer1(ostats.time_counter);
        oinputs.do_credits();
        oinitengine.set_granular_position();
    }
}

void Outrun::jump_table(Packet* packet)
{
    if (tick_frame && game_state != GS_CALIBRATE_MOTOR)
    {
        main_switch();                  // Address #1 (0xB128) - Main Switch
        oinputs.adjust_inputs();        // Address #2 (0x74D8) - Adjust Analogue Inputs
    }

    switch (game_state)
    {
        case GS_REINIT:
        case GS_CALIBRATE_MOTOR:
            break;
 
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

        // ----------------------------------------------------------------------------------------
        // Core Game Engine Routines
        // ----------------------------------------------------------------------------------------
        case GS_LOGO:
            if (!cannonball::tick_frame)
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

    // Motor Code
    if (tick_frame)
    {
        if (game_state == GS_CALIBRATE_MOTOR)
        {
            if (outputs->calibrate_motor(packet->ai1, packet->mci, 0))
            {
                video.enabled     = false;
                video.clear_text_ram();
                oroad.horizon_set = 0;
                boot();
            }
        }
        else
        {
            if (config.controls.haptic && config.controls.analog)
                outputs->tick(OOutputs::MODE_FFEEDBACK, oinputs.input_steering);
            else if (config.cannonboard.enabled)
                outputs->tick(OOutputs::MODE_CABINET, packet->ai1, config.cannonboard.cabinet);
        }
    }

    if (config.cannonboard.enabled && config.cannonboard.debug)
    {
        uint16_t x = 1;
        uint16_t y = 5;
        ohud.blit_text_new(x, y, "AI0 ACCEL");   ohud.blit_text_new(x + 10, y, Utils::to_hex_string(packet->ai0).c_str(), OHud::PINK); x += 13;
        ohud.blit_text_new(x, y, "AI2 WHEEL");   ohud.blit_text_new(x + 10, y, Utils::to_hex_string(packet->ai2).c_str(), OHud::PINK); x += 13;
        ohud.blit_text_new(x, y, "AI3 BRAKE");   ohud.blit_text_new(x + 10, y, Utils::to_hex_string(packet->ai3).c_str(), OHud::PINK); x += 13;
      
        x = 1;
        y = 6;
        ohud.blit_text_new(x, y, "AI1 MOTOR"); ohud.blit_text_new(x + 10, y, Utils::to_hex_string(packet->ai1).c_str(), OHud::PINK); x += 13;
        ohud.blit_text_new(x, y, "MC OUT");    ohud.blit_text_new(x + 10, y, Utils::to_hex_string(outputs->hw_motor_control).c_str(), OHud::PINK); x += 13;
        ohud.blit_text_new(x, y, "MC IN");     ohud.blit_text_new(x + 10, y, Utils::to_hex_string(packet->mci).c_str(), OHud::PINK);

        x = 1;
        y = 7;
        ohud.blit_text_new(x, y, "DI1");     ohud.blit_text_new(x + 10, y, Utils::to_hex_string(packet->di1).c_str(), OHud::PINK); x += 13;
        ohud.blit_text_new(x, y, "DI2");     ohud.blit_text_new(x + 10, y, Utils::to_hex_string(packet->di2).c_str(), OHud::PINK); x += 13;
        ohud.blit_text_new(x, y, "DIG OUT"); ohud.blit_text_new(x + 10, y, Utils::to_hex_string(outputs->dig_out).c_str(), OHud::PINK); x += 13;
    }
}

// Source: 0xB15E
void Outrun::main_switch()
{
    switch (game_state)
    {
        case GS_INIT:  
            init_attract();
            // fall through
            
        // ----------------------------------------------------------------------------------------
        // Attract Mode
        // ----------------------------------------------------------------------------------------
        case GS_ATTRACT:
            tick_attract();
            check_freeplay_start();
            break;

        case GS_INIT_BEST1:
            oferrari.car_ctrl_active = false;
            oinitengine.car_increment = 0;
            oferrari.car_inc_old = 0;
            ostats.time_counter = 5;
            ostats.frame_counter = ostats.frame_reset;
            ohiscore.init();
            osoundint.queue_sound(sound::FM_RESET);
            #ifdef COMPILE_SOUND_CODE
            cannonball::audio.clear_wav();
            #endif
            game_state = GS_BEST1;

        case GS_BEST1:
            ohud.draw_copyright_text();
            ohiscore.display_scores();
            ohud.draw_credits();
            ohud.draw_insert_coin();
            check_freeplay_start();
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

            check_freeplay_start();
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
            //ROM:0000B3E8                 move.w  #-1,(ingame_active1).l              ; Denote in-game engine is active
            //ROM:0000B3F0                 clr.l   (prev_game_time).l                  ; Reset overall game time
            //ROM:0000B3F6                 move.w  #-1,(ingame_active2).l
            video.clear_text_ram();
            oferrari.car_ctrl_active = true;
            init_jump_table();
            oinitengine.init(cannonball_mode == MODE_TTRIAL ? ttrial.level : 0);
            // Timing Hack to ensure horizon is correct
            // Note that the original code disables the screen, and waits for the second CPU's interrupt instead
            oroad.tick();
            oroad.tick();
            oroad.tick();
            osoundint.queue_sound(sound::STOP_CHEERS);
            osoundint.queue_sound(sound::VOICE_GETREADY);
            
            #ifdef COMPILE_SOUND_CODE
            if (omusic.music_selected >= 0 && omusic.music_selected <= 2)
            {
                cannonball::audio.load_wav(config.sound.custom_music[omusic.music_selected].filename.c_str());
                osoundint.queue_sound(sound::REVS); // queue revs sound manually
            }
            else
            #endif
                osoundint.queue_sound(omusic.music_selected);
            
            if (!freeze_timer)
                ostats.time_counter = ostats.TIME[config.engine.dip_time * 40]; // Set time to begin level with
            else
                ostats.time_counter = 0x30;

            ostats.frame_counter = ostats.frame_reset + 50;
            ostats.credits--;                                   // Update Credits
            ohud.blit_text1(TEXT1_CLEAR_START);
            ohud.blit_text1(TEXT1_CLEAR_CREDITS);
            osoundint.queue_sound(sound::INIT_CHEERS);
            video.enabled = true;
            game_state = GS_START1;
            ohud.draw_main_hud();
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
                if (cannonball_mode == MODE_TTRIAL)
                {
                    ohud.clear_timetrial_text();
                }

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
            oroad.road_load_end   |= BIT_0;             // Instruct CPU 1 to load end road section
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
            if (cannonball_mode != MODE_TTRIAL)
            {
                oferrari.car_ctrl_active = false; // -1
                oinitengine.car_increment = 0;
                oferrari.car_inc_old = 0;
                ostats.time_counter = 3;
                ostats.frame_counter = ostats.frame_reset;
                ohud.blit_text2(TEXT2_GAMEOVER);
            }
            else
            {
                ohud.blit_text_big(7, ttrial.new_high_score ? "NEW RECORD" : "BAD LUCK");

                ohud.blit_text1(TEXT1_LAPTIME1);
                ohud.blit_text1(TEXT1_LAPTIME2);
                ohud.draw_lap_timer(0x110554, ttrial.best_lap, ttrial.best_lap[2]);

                ohud.blit_text_new(9,  14, "OVERTAKES          - ");
                ohud.blit_text_new(31, 14, Utils::to_string((int) ttrial.overtakes).c_str(), OHud::GREEN);
                ohud.blit_text_new(9,  16, "VEHICLE COLLISIONS - ");
                ohud.blit_text_new(31, 16, Utils::to_string((int) ttrial.vehicle_cols).c_str(), OHud::GREEN);
                ohud.blit_text_new(9,  18, "CRASHES            - ");
                ohud.blit_text_new(31, 18, Utils::to_string((int) ttrial.crashes).c_str(), OHud::GREEN);
            }
            osoundint.queue_sound(sound::NEW_COMMAND);
            game_state = GS_GAMEOVER;

        case GS_GAMEOVER:
            if (cannonball_mode == MODE_ORIGINAL)
            {
                if (decrement_timers())
                    game_state = GS_INIT_MAP;
            }
            else if (cannonball_mode == MODE_CONT)
            {
                if (decrement_timers())
                    init_best_outrunners();
            }
            else if (cannonball_mode == MODE_TTRIAL)
            {
                if (outrun.tick_counter & BIT_4)
                    ohud.blit_text1(10, 20, TEXT1_PRESS_START);
                else
                    ohud.blit_text1(10, 20, TEXT1_CLEAR_START);

                if (input.is_pressed(Input::START))
                    cannonball::state = cannonball::STATE_INIT_MENU;
            }
            break;

        // ----------------------------------------------------------------------------------------
        // Display Course Map
        // ----------------------------------------------------------------------------------------
        case GS_INIT_MAP:
            omap.init();
            ohud.blit_text2(TEXT2_COURSEMAP);
            game_state = GS_MAP;
            // fall through

        case GS_MAP:
            break;

        // ----------------------------------------------------------------------------------------
        // Best OutRunners / Score Entry
        // ----------------------------------------------------------------------------------------
        case GS_INIT_BEST2:
            oroad.set_view_mode(ORoad::VIEW_ORIGINAL, true);
            // bsr.w   EndGame
            osprites.disable_sprites();
            otraffic.disable_traffic();
            // bsr.w   EditJumpTable3
            osprites.clear_palette_data();
            olevelobjs.init_hiscore_sprites();
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
            osoundint.queue_sound(sound::FM_RESET);
            #ifdef COMPILE_SOUND_CODE
            cannonball::audio.clear_wav();
            #endif
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
                init_jump_table();
                oinitengine.init(cannonball_mode == MODE_TTRIAL ? ttrial.level : 0);
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

    oinitengine.update_road();
    oinitengine.update_engine();

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
    if (cannonball_mode != MODE_TTRIAL) 
    {
        otraffic.init_stage1_traffic();      // Hard coded traffic in right hand lane
        if (trackloader.display_start_line)
            olevelobjs.init_startline_sprites(); // Hard coded start line sprites (not part of level data)
    }
    else if (trackloader.display_start_line)
        olevelobjs.init_timetrial_sprites();

    otraffic.init();
    osmoke.init();
    oroad.init();
    otiles.init();
    opalette.init();
    oinputs.init();
    obonus.init();
    outputs->init();

    video.tile_layer->set_x_clamp(video.tile_layer->RIGHT);
    video.sprite_layer->set_x_clip(false);
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
    if (freeze_timer && game_state == GS_INGAME)
        return false;

    if (--ostats.frame_counter >= 0)
        return false;

    ostats.frame_counter = ostats.frame_reset;

    ostats.time_counter = outils::bcd_sub(1, ostats.time_counter);
    
    return (ostats.time_counter < 0);
}

// -------------------------------------------------------------------------------
// CannonBoard: Motor Calibration
// -------------------------------------------------------------------------------

void Outrun::init_motor_calibration()
{
    otiles.init();
    opalette.init();
    oinputs.init();
    outputs->init();

    video.tile_layer->set_x_clamp(video.tile_layer->RIGHT);
    video.sprite_layer->set_x_clip(false);

    otiles.fill_tilemap_color(0x4F60); // Fill Tilemap Light Blue

    video.enabled        = true;
    osoundint.has_booted = true;

    oroad.init();
    oroad.horizon_set    = 1;
    oroad.horizon_base   = -0x3FF;
    game_state           = GS_CALIBRATE_MOTOR;


    // Write Palette To RAM
    uint32_t dst = 0x120000;
    const static uint32_t PAL_SERVICE[] = {0xFF, 0xFF00FF, 0xFF00FF, 0xFF0000};
    video.write_pal32(&dst, PAL_SERVICE[0]);
    video.write_pal32(&dst, PAL_SERVICE[1]);
    video.write_pal32(&dst, PAL_SERVICE[2]);
    video.write_pal32(&dst, PAL_SERVICE[3]);
}

// -------------------------------------------------------------------------------
// Attract Mode Control
// -------------------------------------------------------------------------------

void Outrun::init_attract()
{
    video.enabled             = true;
    osoundint.has_booted      = true;
    oferrari.car_ctrl_active  = true;
    oferrari.car_inc_old      = car_inc_bak >> 16;
    oinitengine.car_increment = car_inc_bak;
    ostats.time_counter       = config.engine.new_attract ? 0x80 : 0x15;
    ostats.frame_counter      = ostats.frame_reset;
    attract_counter           = 0;
    attract_view              = 0;
    oattractai.init();
    game_state = cannonball_mode == MODE_TTRIAL ? GS_INIT_MUSIC : GS_ATTRACT;
}

void Outrun::tick_attract()
{
    ohud.draw_credits();
    ohud.draw_copyright_text();
    ohud.draw_insert_coin();

    // Enhanced Attract Mode (Switch Between Views)
    if (config.engine.new_attract)
    {
        if (++attract_counter > 240)
        {
            const static uint8_t VIEWS[] = {ORoad::VIEW_ORIGINAL, ORoad::VIEW_ELEVATED, ORoad::VIEW_INCAR};

            attract_counter = 0;
            if (++attract_view > 2)
                attract_view = 0;
            bool snap = VIEWS[attract_view] == ORoad::VIEW_INCAR;
            oroad.set_view_mode(VIEWS[attract_view], snap);
        }
    }

    if (ostats.credits)
        game_state = GS_INIT_MUSIC;

    else if (decrement_timers())
    {
        car_inc_bak = oinitengine.car_increment;
        game_state = GS_INIT_BEST1;
    }
}

void Outrun::check_freeplay_start()
{
    if (config.engine.freeplay)
    {
        if (input.is_pressed(Input::START))
        {
            if (!ostats.credits)
                ostats.credits = 1;
        }
    }
}

// -------------------------------------------------------------------------------
// Best OutRunners Initialization
// -------------------------------------------------------------------------------

void Outrun::init_best_outrunners()
{
    video.enabled = false;
    video.sprite_layer->set_x_clip(false); // Stop clipping in wide-screen mode.
    otiles.fill_tilemap_color(0); // Fill Tilemap Black
    osprites.disable_sprites();
    oroad.horizon_base = 0x154;
    ohiscore.setup_pal_best();    // Setup Palettes
    ohiscore.setup_road_best();
    game_state = GS_INIT_BEST2;
}

// -------------------------------------------------------------------------------
// Remap ROM addresses and select course.
// -------------------------------------------------------------------------------

void Outrun::select_course(bool jap, bool prototype)
{
    if (jap)
    {
        roms.rom0p = &roms.j_rom0;
        roms.rom1p = &roms.j_rom1;

        // Main CPU
        adr.tiles_def_lookup      = TILES_DEF_LOOKUP_J;
        adr.tiles_table           = TILES_TABLE_J;
        adr.sprite_master_table   = SPRITE_MASTER_TABLE_J;
        adr.sprite_type_table     = SPRITE_TYPE_TABLE_J;
        adr.sprite_def_props1     = SPRITE_DEF_PROPS1_J;
        adr.sprite_def_props2     = SPRITE_DEF_PROPS2_J;
        adr.sprite_cloud          = SPRITE_CLOUD_FRAMES_J;
        adr.sprite_minitree       = SPRITE_MINITREE_FRAMES_J;
        adr.sprite_grass          = SPRITE_GRASS_FRAMES_J;
        adr.sprite_sand           = SPRITE_SAND_FRAMES_J;
        adr.sprite_stone          = SPRITE_STONE_FRAMES_J;
        adr.sprite_water          = SPRITE_WATER_FRAMES_J;
        adr.sprite_ferrari_frames = SPRITE_FERRARI_FRAMES_J;
        adr.sprite_skid_frames    = SPRITE_SKID_FRAMES_J;
        adr.sprite_pass_frames    = SPRITE_PASS_FRAMES_J;
        adr.sprite_pass1_skidl    = SPRITE_PASS1_SKIDL_J;
        adr.sprite_pass1_skidr    = SPRITE_PASS1_SKIDR_J;
        adr.sprite_pass2_skidl    = SPRITE_PASS2_SKIDL_J;
        adr.sprite_pass2_skidr    = SPRITE_PASS2_SKIDR_J;
        adr.sprite_crash_spin1    = SPRITE_CRASH_SPIN1_J;
        adr.sprite_crash_spin2    = SPRITE_CRASH_SPIN2_J;
        adr.sprite_bump_data1     = SPRITE_BUMP_DATA1_J;
        adr.sprite_bump_data2     = SPRITE_BUMP_DATA2_J;
        adr.sprite_crash_man1     = SPRITE_CRASH_MAN1_J;
        adr.sprite_crash_girl1    = SPRITE_CRASH_GIRL1_J;
        adr.sprite_crash_flip     = SPRITE_CRASH_FLIP_J;
        adr.sprite_crash_flip_m1  = SPRITE_CRASH_FLIP_MAN1_J;
        adr.sprite_crash_flip_g1  = SPRITE_CRASH_FLIP_GIRL1_J;
        adr.sprite_crash_flip_m2  = SPRITE_CRASH_FLIP_MAN2_J;
        adr.sprite_crash_flip_g2  = SPRITE_CRASH_FLIP_GIRL2_J;
        adr.sprite_crash_man2     = SPRITE_CRASH_MAN2_J;
        adr.sprite_crash_girl2    = SPRITE_CRASH_GIRL2_J;
        adr.smoke_data            = SMOKE_DATA_J;
        adr.spray_data            = SPRAY_DATA_J;
        adr.anim_ferrari_frames   = ANIM_FERRARI_FRAMES_J;
        adr.anim_endseq_obj1      = ANIM_ENDSEQ_OBJ1_J;
        adr.anim_endseq_obj2      = ANIM_ENDSEQ_OBJ2_J;
        adr.anim_endseq_obj3      = ANIM_ENDSEQ_OBJ3_J;
        adr.anim_endseq_obj4      = ANIM_ENDSEQ_OBJ4_J;
        adr.anim_endseq_obj5      = ANIM_ENDSEQ_OBJ5_J;
        adr.anim_endseq_obj6      = ANIM_ENDSEQ_OBJ6_J;
        adr.anim_endseq_obj7      = ANIM_ENDSEQ_OBJ7_J;
        adr.anim_endseq_obj8      = ANIM_ENDSEQ_OBJ8_J;
        adr.anim_endseq_objA      = ANIM_ENDSEQ_OBJA_J;
        adr.anim_endseq_objB      = ANIM_ENDSEQ_OBJB_J;
        adr.anim_end_table        = ANIM_END_TABLE_J;
        adr.shadow_data           = SPRITE_SHADOW_DATA_J;
        adr.shadow_frames         = SPRITE_SHDW_FRAMES_J;
        adr.sprite_shadow_small   = SPRITE_SHDW_SMALL_J;
        adr.sprite_logo_bg        = SPRITE_LOGO_BG_J;
        adr.sprite_logo_car       = SPRITE_LOGO_CAR_J;
        adr.sprite_logo_bird1     = SPRITE_LOGO_BIRD1_J;
        adr.sprite_logo_bird2     = SPRITE_LOGO_BIRD2_J;
        adr.sprite_logo_base      = SPRITE_LOGO_BASE_J;
        adr.sprite_logo_text      = SPRITE_LOGO_TEXT_J;
        adr.sprite_logo_palm1     = SPRITE_LOGO_PALM1_J;
        adr.sprite_logo_palm2     = SPRITE_LOGO_PALM2_J;
        adr.sprite_logo_palm3     = SPRITE_LOGO_PALM3_J;
        adr.sprite_fm_left        = SPRITE_FM_LEFT_J;
        adr.sprite_fm_centre      = SPRITE_FM_CENTRE_J;
        adr.sprite_fm_right       = SPRITE_FM_RIGHT_J;
        adr.sprite_dial_left      = SPRITE_DIAL_LEFT_J;
        adr.sprite_dial_centre    = SPRITE_DIAL_CENTRE_J;
        adr.sprite_dial_right     = SPRITE_DIAL_RIGHT_J;
        adr.sprite_eq             = SPRITE_EQ_J;
        adr.sprite_radio          = SPRITE_RADIO_J;
        adr.sprite_hand_left      = SPRITE_HAND_LEFT_J;
        adr.sprite_hand_centre    = SPRITE_HAND_CENTRE_J;
        adr.sprite_hand_right     = SPRITE_HAND_RIGHT_J;
        adr.sprite_coursemap_top  = SPRITE_COURSEMAP_TOP_J;
        adr.sprite_coursemap_bot  = SPRITE_COURSEMAP_BOT_J;
        adr.sprite_coursemap_end  = SPRITE_COURSEMAP_END_J;
        adr.sprite_minicar_right  = SPRITE_MINICAR_RIGHT_J;
        adr.sprite_minicar_up     = SPRITE_MINICAR_UP_J;
        adr.sprite_minicar_down   = SPRITE_MINICAR_DOWN_J;
        adr.anim_seq_flag         = ANIM_SEQ_FLAG_J;
        adr.anim_ferrari_curr     = ANIM_FERRARI_CURR_J;
        adr.anim_ferrari_next     = ANIM_FERRARI_NEXT_J;
        adr.anim_pass1_curr       = ANIM_PASS1_CURR_J;
        adr.anim_pass1_next       = ANIM_PASS1_NEXT_J;
        adr.anim_pass2_curr       = ANIM_PASS2_CURR_J;
        adr.anim_pass2_next       = ANIM_PASS2_NEXT_J;
        adr.traffic_props         = TRAFFIC_PROPS_J;
        adr.traffic_data          = TRAFFIC_DATA_J;
        adr.sprite_porsche        = SPRITE_PORSCHE_J;
        adr.sprite_coursemap      = SPRITE_COURSEMAP_J;
        adr.road_seg_table        = ROAD_SEG_TABLE_J;
        adr.road_seg_end          = ROAD_SEG_TABLE_END_J;
        adr.road_seg_split        = ROAD_SEG_TABLE_SPLIT_J;

        // Sub CPU
        adr.road_height_lookup    = ROAD_HEIGHT_LOOKUP_J;
    }
    else
    {
        roms.rom0p = &roms.rom0;
        roms.rom1p = &roms.rom1;

        // Main CPU
        adr.tiles_def_lookup      = TILES_DEF_LOOKUP;
        adr.tiles_table           = TILES_TABLE;
        adr.sprite_master_table   = SPRITE_MASTER_TABLE;
        adr.sprite_type_table     = SPRITE_TYPE_TABLE;
        adr.sprite_def_props1     = SPRITE_DEF_PROPS1;
        adr.sprite_def_props2     = SPRITE_DEF_PROPS2;
        adr.sprite_cloud          = SPRITE_CLOUD_FRAMES;
        adr.sprite_minitree       = SPRITE_MINITREE_FRAMES;
        adr.sprite_grass          = SPRITE_GRASS_FRAMES;
        adr.sprite_sand           = SPRITE_SAND_FRAMES;
        adr.sprite_stone          = SPRITE_STONE_FRAMES;
        adr.sprite_water          = SPRITE_WATER_FRAMES;
        adr.sprite_ferrari_frames = SPRITE_FERRARI_FRAMES;
        adr.sprite_skid_frames    = SPRITE_SKID_FRAMES;
        adr.sprite_pass_frames    = SPRITE_PASS_FRAMES;
        adr.sprite_pass1_skidl    = SPRITE_PASS1_SKIDL;
        adr.sprite_pass1_skidr    = SPRITE_PASS1_SKIDR;
        adr.sprite_pass2_skidl    = SPRITE_PASS2_SKIDL;
        adr.sprite_pass2_skidr    = SPRITE_PASS2_SKIDR;
        adr.sprite_crash_spin1    = SPRITE_CRASH_SPIN1;
        adr.sprite_crash_spin2    = SPRITE_CRASH_SPIN2;
        adr.sprite_bump_data1     = SPRITE_BUMP_DATA1;
        adr.sprite_bump_data2     = SPRITE_BUMP_DATA2;
        adr.sprite_crash_man1     = SPRITE_CRASH_MAN1;
        adr.sprite_crash_girl1    = SPRITE_CRASH_GIRL1;
        adr.sprite_crash_flip     = SPRITE_CRASH_FLIP;
        adr.sprite_crash_flip_m1  = SPRITE_CRASH_FLIP_MAN1;
        adr.sprite_crash_flip_g1  = SPRITE_CRASH_FLIP_GIRL1;
        adr.sprite_crash_flip_m2  = SPRITE_CRASH_FLIP_MAN2;
        adr.sprite_crash_flip_g2  = SPRITE_CRASH_FLIP_GIRL2;
        adr.sprite_crash_man2     = SPRITE_CRASH_MAN2;
        adr.sprite_crash_girl2    = SPRITE_CRASH_GIRL2;
        adr.smoke_data            = SMOKE_DATA;
        adr.spray_data            = SPRAY_DATA;
        adr.shadow_data           = SPRITE_SHADOW_DATA;
        adr.shadow_frames         = SPRITE_SHDW_FRAMES;
        adr.sprite_shadow_small   = SPRITE_SHDW_SMALL;
        adr.sprite_logo_bg        = SPRITE_LOGO_BG;
        adr.sprite_logo_car       = SPRITE_LOGO_CAR;
        adr.sprite_logo_bird1     = SPRITE_LOGO_BIRD1;
        adr.sprite_logo_bird2     = SPRITE_LOGO_BIRD2;
        adr.sprite_logo_base      = SPRITE_LOGO_BASE;
        adr.sprite_logo_text      = SPRITE_LOGO_TEXT;
        adr.sprite_logo_palm1     = SPRITE_LOGO_PALM1;
        adr.sprite_logo_palm2     = SPRITE_LOGO_PALM2;
        adr.sprite_logo_palm3     = SPRITE_LOGO_PALM3;
        adr.sprite_fm_left        = SPRITE_FM_LEFT;
        adr.sprite_fm_centre      = SPRITE_FM_CENTRE;
        adr.sprite_fm_right       = SPRITE_FM_RIGHT;
        adr.sprite_dial_left      = SPRITE_DIAL_LEFT;
        adr.sprite_dial_centre    = SPRITE_DIAL_CENTRE;
        adr.sprite_dial_right     = SPRITE_DIAL_RIGHT;
        adr.sprite_eq             = SPRITE_EQ;
        adr.sprite_radio          = SPRITE_RADIO;
        adr.sprite_hand_left      = SPRITE_HAND_LEFT;
        adr.sprite_hand_centre    = SPRITE_HAND_CENTRE;
        adr.sprite_hand_right     = SPRITE_HAND_RIGHT;
        adr.sprite_coursemap_top  = SPRITE_COURSEMAP_TOP;
        adr.sprite_coursemap_bot  = SPRITE_COURSEMAP_BOT;
        adr.sprite_coursemap_end  = SPRITE_COURSEMAP_END;
        adr.sprite_minicar_right  = SPRITE_MINICAR_RIGHT;
        adr.sprite_minicar_up     = SPRITE_MINICAR_UP;
        adr.sprite_minicar_down   = SPRITE_MINICAR_DOWN;
        adr.anim_seq_flag         = ANIM_SEQ_FLAG;
        adr.anim_ferrari_curr     = ANIM_FERRARI_CURR;
        adr.anim_ferrari_next     = ANIM_FERRARI_NEXT;
        adr.anim_pass1_curr       = ANIM_PASS1_CURR;
        adr.anim_pass1_next       = ANIM_PASS1_NEXT;
        adr.anim_pass2_curr       = ANIM_PASS2_CURR;
        adr.anim_pass2_next       = ANIM_PASS2_NEXT;
        adr.anim_ferrari_frames   = ANIM_FERRARI_FRAMES;
        adr.anim_endseq_obj1      = ANIM_ENDSEQ_OBJ1;
        adr.anim_endseq_obj2      = ANIM_ENDSEQ_OBJ2;
        adr.anim_endseq_obj3      = ANIM_ENDSEQ_OBJ3;
        adr.anim_endseq_obj4      = ANIM_ENDSEQ_OBJ4;
        adr.anim_endseq_obj5      = ANIM_ENDSEQ_OBJ5;
        adr.anim_endseq_obj6      = ANIM_ENDSEQ_OBJ6;
        adr.anim_endseq_obj7      = ANIM_ENDSEQ_OBJ7;
        adr.anim_endseq_obj8      = ANIM_ENDSEQ_OBJ8;
        adr.anim_endseq_objA      = ANIM_ENDSEQ_OBJA;
        adr.anim_endseq_objB      = ANIM_ENDSEQ_OBJB;
        adr.anim_end_table        = ANIM_END_TABLE;
        adr.shadow_data           = SPRITE_SHADOW_DATA;
        adr.shadow_frames         = SPRITE_SHDW_FRAMES;
        adr.sprite_shadow_small   = SPRITE_SHDW_SMALL;
        adr.traffic_props         = TRAFFIC_PROPS;
        adr.traffic_data          = TRAFFIC_DATA;
        adr.sprite_porsche        = SPRITE_PORSCHE;
        adr.sprite_coursemap      = SPRITE_COURSEMAP;
        adr.road_seg_table        = ROAD_SEG_TABLE;
        adr.road_seg_end          = ROAD_SEG_TABLE_END;
        adr.road_seg_split        = ROAD_SEG_TABLE_SPLIT;

        // Sub CPU
        adr.road_height_lookup    = ROAD_HEIGHT_LOOKUP;
    }

    trackloader.init(jap);

    // Use Prototype Coconut Beach Track
    trackloader.stage_data[0] = prototype ? 0x3A : 0x3C;
}
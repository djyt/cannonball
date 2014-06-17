/***************************************************************************
    Best Outrunners Name Entry & Display.
    Used in attract mode, and at game end.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "setup.hpp"
#include "main.hpp"
#include "engine/ohud.hpp"
#include "engine/oinputs.hpp"
#include "engine/ostats.hpp"
#include "engine/outils.hpp"
#include "engine/ohiscore.hpp"

OHiScore ohiscore;

OHiScore::OHiScore(void)
{
}

OHiScore::~OHiScore(void)
{
}

// Clear score variables (not scores themselves)
//
// Source: 0xCE74
void OHiScore::init()
{
    //ostats.score = 0x04100000; // hack

    best_or_state     = 0;
    state             = STATE_GETPOS;
    score_pos         = -1;
    initial_selected  = 0;
    letter_selected   = 0;
    acc_curr          = 0;
    acc_prev          = 0;
    flash             = 0;
    score_display_pos = 0;
    dest_total        = 0;
}

// Setup palette for Best Outrunners High Score Entry
// This is the shaded red background for the hi-score entry
// Source: 0x360C
void OHiScore::setup_pal_best()
{
    uint32_t src = PAL_BESTOR;
    uint32_t dst = 0x120F00;

    for (int i = 0; i <= 0x1F; i++)
        video.write_pal32(&dst, roms.rom0.read32(&src));
}

// Setup road colour for Best Outrunners High Score Entry
// This is a pure black road for the hi-score entry
// Source: 0x3624
void OHiScore::setup_road_best()
{
    uint32_t dst = 0x120800;

    for (int i = 0; i <= 0x1F; i++)
        video.write_pal32(&dst, 0);
}

// Initalize Default Score Table
// Source: 0xD17A
void OHiScore::init_def_scores()
{
    uint32_t adr = DEFAULT_SCORES;

    for (int i = 0; i < NO_SCORES; i++)
    {
        // Read default score
        scores[i].score = roms.rom0.read32(&adr);

        // Read initials
        uint32_t initials = roms.rom0.read32(&adr);
        scores[i].initial1 = (initials >> 24) & 0xFF;
        scores[i].initial2 = (initials >> 16) & 0xFF;
        scores[i].initial3 = (initials >> 8) & 0xFF;

        // Read default time
        scores[i].time = roms.rom0.read16(&adr);
        //scores[i].time = (i & 1) ? 0x4321 : 0x1234; // hack to display 4m 43 51 or 1m 16 56
        // Read map tiles
        scores[i].maptiles = roms.rom0.read32(&adr);
        //scores[i].maptiles = 0xe5c8c2d1; // hack to populate map tiles for testing
    }
}

// Hi Score Processing Logic
//
// Source: 0xD1C4
void OHiScore::tick()
{
    switch (state & 3)
    {
        // Detect Score Position, Insert Score, Init Table
        case STATE_GETPOS:   
            get_score_pos();

            // New High Score
            if (score_pos != -1)
            {
                osoundint.queue_sound(sound::PCM_WAVE);
                #ifdef COMPILE_SOUND_CODE
                if (config.sound.custom_music[3].enabled)
                    cannonball::audio.load_wav(config.sound.custom_music[3].filename.c_str());
                else
                #endif
                osoundint.queue_sound(sound::MUSIC_LASTWAVE);
                insert_score();               
            }
            // Not a High Score
            else
            {
                ostats.time_counter = 5;
            }
            set_display_pos();
            acc_prev = -1;
            state = STATE_DISPLAY;
            video.enabled = true;
            break;

        // Display Basic High Score Table
        case STATE_DISPLAY: 
            display_scores();
            if (best_or_state >= 2)
                state = STATE_ENTRY; // Only allow name entry when minicars have animation finished
            break;

        // Init Name Entry
        case STATE_ENTRY:
            check_name_entry();
            break;

        // Score Done
        case STATE_DONE:
            return;
    }
}

// Calculate high score position.
// Source: D318
void OHiScore::get_score_pos()
{
    for (int i = 0; i < NO_SCORES; i++)
    {
        if (ostats.score > scores[i].score)
        {
            score_pos = i;
            set_display_pos();
            return;
        }
    }

    score_pos = -1; // Not a new high-score
}

// - Insert Score Entry
// - Move Other Entries Down In Memory
// - Calculate Completed Time
// - Setup Appropriate Minimap Tiles
//
// Source: 0xD2C0
void OHiScore::insert_score()
{
    // Move entries down in memory
    for (int i = NO_SCORES - 1; i > score_pos; i--)
    {
        scores[i] = scores[i-1];
    }

    scores[score_pos].score    = ostats.score;
    scores[score_pos].initial1 = 0x20;
    scores[score_pos].initial2 = 0x20;
    scores[score_pos].initial3 = 0x20;

    // Calculate total time if game completed. Store result in $20
    if (ostats.game_completed)
    {
        const uint8_t entries = outrun.cannonball_mode == Outrun::MODE_ORIGINAL ? 5 : 15;

        scores[score_pos].time = 0;

        for (int i = 0; i < entries; i++)
            scores[score_pos].time += ostats.stage_counters[i];
    }
    else
    {
        scores[score_pos].time = 0;
        ostats.game_completed = false;
    }

    // Setup Appropriate Minimap Tiles
    scores[score_pos].maptiles = roms.rom0.read32(ohud.setup_mini_map());
}

// Set Table Position To Display Score From. Store Result in $26
// Source: 0xD298
void OHiScore::set_display_pos()
{
    if (score_pos < 0)
    {
        score_display_pos = 13;
    }
    else
    {
        score_display_pos = score_pos - 3;

        if (score_display_pos < 0)
            score_display_pos = 0;
        else if (score_display_pos > 13)
            score_display_pos = 13;
    } 
    //score_display_pos = 0; // HACK!
}

// Check whether to perform name entry.
// Print alphabet and other stuff if necessary.
// Source: 0xD252
void OHiScore::check_name_entry()
{
    // No High Score
    if (score_pos == -1)
    {
        ohud.blit_text1(TEXT1_YOURSCORE);
        ohud.draw_score(0x110BDA, ostats.score, 3); // Select font 3 and print score
        state = STATE_DONE;
    }
    else
    {
        // Get text ram address of score to blit
        uint32_t score_adr = get_score_adr();
        // Blit Alphabet. Highlight selected letter red.
        blit_alphabet();
        // Flash current initial that is being entered
        flash_entry(score_adr);   
        // Draw big red countdown timer
        const uint16_t BIG_RED_FONT = 0x8080;
        ohud.draw_timer2(ostats.time_counter, 0x1101EC, BIG_RED_FONT);
        // Input from controls
        do_input(score_adr);
        
        // Save new score info
        if (state == STATE_DONE)
            config.save_scores(outrun.cannonball_mode == Outrun::MODE_ORIGINAL ? FILENAME_SCORES : FILENAME_CONT);
    }
}

// Get Address in text ram at which to output score
// Source: 0xD542
uint32_t OHiScore::get_score_adr()
{
    if (score_pos < 3)
        return 0x110452 + (score_pos << 8); // top 3 positions
    if (score_pos >= 17)
        return 0x110A52 + ((score_pos - 19) << 8); // last 3 positions

    return 0x110752; // middle positions
}

// Blit Alphabet. Highlight selected letter red.
// Source: 0xD45A
void OHiScore::blit_alphabet()
{
    // Print Text: "ABCDEFGHIJK..."
    ohud.blit_text2(TEXT2_ALPHABET); 

    // Address in text ram for characters
    uint32_t adr = 0x110BF0;

    video.write_text16(&adr,       0x8D00); // Full Stop
    video.write_text16(adr + 0x7E, 0x8D01);
    video.write_text16(&adr,       0x8D04); // Arrow
    video.write_text16(adr + 0x7E, 0x8D05);
    video.write_text16(&adr,       0x8D02); // ED
    video.write_text16(adr + 0x7E, 0x8D03);

    // Colour selected tile red
    const uint16_t RED = 0x80;
    adr = 0x110BBC + (letter_selected << 1);
    video.write_text8(adr,        (video.read_text8(adr) & 1) | RED);
    video.write_text8(adr + 0x80, (video.read_text8(adr + 0x80) & 1) | RED);
}

// Flash current initial that is being entered
//
// Takes address of score entry as input
//
// Source: 0xD42C
void OHiScore::flash_entry(uint32_t adr)
{
    uint16_t tile = 0x20; // Default blank tile
    flash++; // Increment flashing counter

    if (flash & BIT_3)
    {
        tile = (roms.rom0.read8(letter_selected + TILES_ALPHABET) & 0xFF) | 0x8600;
    }

    video.write_text16(adr + (initial_selected << 1), tile);
}

// High Score Input
//
// Source: 0xD33A
void OHiScore::do_input(uint32_t adr)
{
    // Read Steering Left / Right & Denote Letter To Be Highlighted

    const uint8_t ENTRIES = 28; // 28 Possible entries we can select from
    
    int16_t position = read_controls() + letter_selected;

    if (position > ENTRIES)
        letter_selected = position = 0;
    else if (position < 0)
        letter_selected = position = ENTRIES;
    else
        letter_selected = position;

    // Check accelerator for press and depress
    if (!acc_curr || !(acc_prev ^ acc_curr)) return;

    // End option selected
    if (letter_selected == ENTRIES)
    {
        video.write_text16(adr + (initial_selected << 1), 0x20); // Write blank tile to ram
        ostats.frame_counter = 0;
        ostats.time_counter = 0;
        state = STATE_DONE;
    }
    // Delete option selected
    else if (letter_selected == ENTRIES - 1)
    {
        // Delete if not at first position
        if (initial_selected != 0)
        {
            if (initial_selected == 1)
                scores[score_pos].initial2 = 0x20;
            else if (initial_selected == 2)
                scores[score_pos].initial3 = 0x20;

            video.write_text16(adr + (initial_selected << 1), 0x20); // Write blank tile to ram

            initial_selected--;
        }
    }
    // Normal character selected
    else
    {
        uint8_t tile = roms.rom0.read8(TILES_ALPHABET + letter_selected);
        
        // Store initial to score structure
        if (initial_selected == 0)
            scores[score_pos].initial1 = tile;
        else if (initial_selected == 1)
            scores[score_pos].initial2 = tile;
        else if (initial_selected == 2)
            scores[score_pos].initial3 = tile;

        video.write_text16(adr + (initial_selected << 1), tile | 0x8600); // Write initial tile to ram

        // Final Initial
        if (++initial_selected >= 3)
        {
            state = STATE_DONE;
            ostats.frame_counter = ostats.frame_reset;
            ostats.time_counter  = 2;
            // code to enable easter egg if YU. is inputted goes here.
        }
    }
}

// Read controls for high score input screen
//
// Output:
//  0 = No Movement
// -1 = Left
//  1 = Right
//
// Source: 0xD4DA
int8_t OHiScore::read_controls()
{
    // Determine when accelerator has been pressed then depressed
    if (oinputs.input_acc < 0x30)
    {
        acc_prev = acc_curr;
        acc_curr = 0;
    }
    else if (oinputs.input_acc < 0x60)
    {
        acc_curr = acc_prev;
    }
    else
    {
        acc_prev = acc_curr;
        acc_curr = -1;
    }

    // Check Steering Wheel
    int8_t movement = 1; // default to right
    int16_t steering = (oinputs.input_steering & 0xFF) - 0x80;
    if (steering < 0)
    {
        steering = -steering;
        movement = -1; // left
    }

    // Set increment to potentially advance to next letter.
    // This depends on how far the steering wheel is turned.
    if (steering >= 0x30)
        steer += 5;
    else if (steering >= 0x10)
        steer += 1;

    if (steer >= 0x14)
        steer = 0;
    else
        movement = 0; // no movement
   
    return movement;
}

// Display Best Outrunners in attract mode and name entry screen
//
// Source: 0xCE84
void OHiScore::display_scores()
{
    switch (best_or_state)
    {
        // Init
        case 0:
            video.clear_text_ram();
            setup_minicars();
            blit_score_table();
            best_or_state = 1; // Set State to TICK
            break;

        // Tick
        case 1:
            tick_minicars();
            // Have all mini-cars reached their destination?
            if (dest_total >= 7)
                 best_or_state = 2; // Set State to DONE 
            break;

        // Return
        case 2:
            return;
    }
}

// ------------------------------------------------------------------------------------------------
//                                       Mini car Movement
// ------------------------------------------------------------------------------------------------

// Setup minicars before they move across screen
// Source: 0xCED2
void OHiScore::setup_minicars()
{
    for (int i = 0; i < NO_MINICARS; i++)
    {
        minicars[i].pos         = 0x100;
        minicars[i].dst_reached = 0;
        minicars[i].speed       = (outils::random() & 0x180) | 0xF0;
        minicars[i].base_speed  = (outils::random() & 0x7) | 0x01;
    }
}

// Move minicars across screen on text ram layer
// Source: 0xCF0E
void OHiScore::tick_minicars()
{
    // Destination in text ram
    uint32_t dst = 0x11047C;

    // Source tile data
    uint32_t tiles_adr = TILES_MINICARS1;

    // There are seven lines / entries to blit
    for (int i = 0; i < NO_MINICARS; i++)
    {
        minicar_entry* minicar = &minicars[i];
        
        // Minicar is on-screen
        if (!minicar->dst_reached & BIT_0)
        {
            // Minicar has reached destination position (off-screen)
            if ((minicar->pos >> 8) >= 0x5A)
            {
                minicar->dst_reached |= BIT_0;
                dest_total++; // Increment total minicars that have reached destination
            }

            minicar->speed += minicar->base_speed;

            if (minicar->speed >= 0x200)
                minicar->speed = 0x180;

            minicar->pos += minicar->speed;

            setup_minicars_pal(minicar);

            // Masked off the lower bit
            int16_t pos = (minicar->pos >> 8) & 0xFFFE;

            // Get final address in text ram for minicar based on position
            uint32_t textram_adr = dst - pos;

            // Address for following smoke tiles
            uint32_t tiles_smoke_adr = TILES_MINICARS2;

            // The minicar is two tiles wide.
            // Two versions of routine, one that only blits the car in two tiles
            if ((minicar->pos >> 8) & BIT_0)
            {
                video.write_text32(&textram_adr, roms.rom0.read32(tiles_adr)); // blit car in 2 tiles
                video.write_text32(&textram_adr, roms.rom0.read32(&tiles_smoke_adr)); // smoke trail tile 1
                video.write_text16(&textram_adr, roms.rom0.read16(&tiles_smoke_adr)); // smoke trail tile 2
            }
            // Blit at an offset
            // The second blits the mini-car at an offset halfway into the tile (and hence takes 3 tiles)
            else
            {
                video.write_text32(&textram_adr, roms.rom0.read32(4 + tiles_adr)); // blit car in 3 tiles
                video.write_text16(&textram_adr, roms.rom0.read16(8 + tiles_adr)); // blit car in 3 tiles
                video.write_text32(&textram_adr, roms.rom0.read32(&tiles_smoke_adr)); // smoke trail tile 1
                video.write_text16(&textram_adr, roms.rom0.read16(&tiles_smoke_adr)); // smoke trail tile 2
            }

            // Erase Minicar tiles (0xCFB2)
            // Reveal info from tile ram by copying to text ram

            // Bottom Line
            uint16_t tile_bits = video.read_tile8(textram_adr - 0x2000 + 1) | minicar->tile_props;
            video.write_text16(textram_adr, tile_bits);
            // Top Line
            tile_bits = video.read_tile8(textram_adr - 0x2000 - 0x7F) | minicar->tile_props;
            video.write_text16(textram_adr - 0x80, tile_bits);
        }

        dst += 0x100; // Advance to next row in text ram
        tiles_adr += 0x0A; // Advance to next block of minicar data
    }
}

// Setup palette and priority data for the copied tiles behind the minicar.
// The palette & priority used for the text depends on the position.
// Source: 0xCFCC
void OHiScore::setup_minicars_pal(minicar_entry* minicar)
{
    uint8_t pos = minicar->pos >> 8;

    // Lap Time Tile Properties
    minicar->tile_props = 0x8400;
    if (pos <= 0x20) return; // Was 0x1F in original: Changed to handle longer times

    // Route Tile Properties
    minicar->tile_props = 0x8B00;
    if (pos <= 0x2D) return;

    // Initial Tile Properties
    minicar->tile_props = 0x8200;
    if (pos <= 0x39) return;

    // Score Tile Properties
    minicar->tile_props = 0x8400;
    if (pos <= 0x4A) return;

    // 1.2.3. Tile Properties
    minicar->tile_props = 0x8600;
}

// ------------------------------------------------------------------------------------------------
//                                     Score Table Rendering
// ------------------------------------------------------------------------------------------------

// Source: 0xD00C
void OHiScore::blit_score_table()
{
    // Clear tile table ready for High Score Display
    uint32_t tile_addr = 0x10E000; // Tile Table 15
    for (int i = 0; i <= 0x3FF; i++)
        video.write_tile32(&tile_addr, 0x200020);

    ohud.blit_text2(TEXT2_BEST_OR);   // Print "BEST OUTRUNNERS"
    ohud.blit_text1(TEXT1_SCORE_ETC); // Print Score, Name, Route, Record
    blit_digit();                     // Blit 1. 2. 3. etc.
    blit_scores();                    // Blit list of scores
    blit_initials();                  // Blit initials attached to those scores
    if (outrun.cannonball_mode != Outrun::MODE_CONT)
        blit_route_map();            // Blit Mini Route Map
    blit_lap_time();
}

// Blit 7x single digit at start of score table (1. 2. 3. 4. 5. 6. 7.)
// Source: 0xD03A
void OHiScore::blit_digit()
{
    // Destination in tile ram for digit
    uint32_t dst = 0x10E438;

    // Starting display position
    int16_t pos = score_display_pos + 1;

    // Display numbers 1 to 7
    for (int i = 0; i < 7; i++)
    {
        int32_t tile = (pos / 10) | ((pos % 10) << 16);

        // Draw blank
        if (!(tile & 0xFFFF))
        {
            tile = (tile & 0xFFFF0000) | 0x20;
            outils::swap32(tile);
            tile |= 0x30;
        }
        // Draw tile
        else
        {
            outils::swap32(tile);
            tile |= 0x300030;
        }

        video.write_tile32(dst, tile);      // Output number digit
        video.write_tile16(4 + dst, 0x5B);  // Output full stop following digit

        dst += 0x100; // Advance to next text row
        pos++;
    }
}

// Blit High Scores
//
// Source: 0xD078
void OHiScore::blit_scores()
{
    // Destination in tile ram for digit
    uint32_t dst = 0x10E43E;

    // Starting display position
    int16_t pos = score_display_pos;

    // Display scores 1 to 7
    for (int i = 0; i < 7; i++)
    {
        ohud.draw_score_tile(dst, scores[pos++].score, 0);
        dst += 0x100; // Advance to next text row
    }
}

// Blit Initials
//
// Source: 0xD0A4
void OHiScore::blit_initials()
{
    // Destination in tile ram for digit
    uint32_t dst = 0x10E452;

    // Starting display position
    int16_t pos = score_display_pos;

    // Write 3 initials for entries 1 to 7
    for (int i = 0; i < 7; i++)
    {
        video.write_tile8(dst + 1, scores[pos].initial1);
        video.write_tile8(dst + 3, scores[pos].initial2);
        video.write_tile8(dst + 5, scores[pos].initial3);
        pos++;
        dst += 0x100; // Advance to next text row
    }
}

// Blit mini route map
//
// Source: 0xD0D8
void OHiScore::blit_route_map()
{
    // Destination in tile ram for digit
    uint32_t dst = 0x10E45E;

    // Starting display position
    int16_t pos = score_display_pos;

    // Write 7 map entries
    for (int i = 0; i < 7; i++)
    {
        uint32_t tiles = scores[pos++].maptiles;

        // eg e5 c8 c2 d1 (4 tile indexes of route map)
        video.write_tile8(dst - 0x7F, (tiles >> 24) & 0xFF);
        video.write_tile8(dst - 0x7D, (tiles >> 16) & 0xFF);
        video.write_tile8(dst + 0x01, (tiles >> 8) & 0xFF);
        video.write_tile8(dst + 0x03, tiles & 0xFF);

        dst += 0x100; // Advance to next text row
    }
}

// Blit laptime
//
// Source: 0xD112
void OHiScore::blit_lap_time()
{
    // Destination in tile ram for digit
    uint32_t dst = 0x10E46A;

    // Starting display position
    int16_t pos = score_display_pos;

    // Write 7 lap entries
    for (int i = 0; i < 7; i++)
    {
        uint16_t time = scores[pos++].time;

        if (time)
        {
            convert_lap_time(time);

            // Write laptime
            if (laptime[0] != TILE_PROPS)
            {
                video.write_tile16(dst - 0x2, laptime[0]); // Minutes Digit 1
            }
            
            video.write_tile16(0x0 + dst, laptime[1]); // Minutes Digit 2
            video.write_tile16(0x2 + dst, 0x5E);       // '
            video.write_tile16(0x4 + dst, laptime[2]); // Seconds Digit 1
            video.write_tile16(0x6 + dst, laptime[3]); // Seconds Digit 2
            video.write_tile16(0x8 + dst, 0x5F);       // '
            video.write_tile16(0xA + dst, laptime[4]); // Milliseconds Digit 1
            video.write_tile16(0xC + dst, laptime[5]); // Milliseconds Digit 2
        }

        dst += 0x100; // Advance to next text row
    }
}

// Convert laptime to tile data and store in laptime array.
// Enhanced routine to handle minutes > 9
//
// Source: 0x806C
void OHiScore::convert_lap_time(uint16_t time)
{
    const uint16_t MINUTE = 3600;

    int32_t src_time = time; // laptime copy [d0] 
    int16_t minutes = -1;     // Store number of minutes

    // Calculate Minutes
    do
    {
        src_time -= MINUTE;
        minutes++;
    }
    while (src_time >= 0);
    
    src_time += MINUTE;
    minutes = outils::convert16_dechex(minutes);

    // Store Millisecond Lookup
    uint16_t ms_lookup = src_time & 0x3F; 
    
    // Calculate Seconds
    uint16_t seconds   = src_time >> 6;   // Store Seconds

    uint16_t s1 = seconds & 0xF; // First digit [d1]
    uint16_t s2 = seconds >> 4;  // Second digit [d2]

    if (s1 > 9)
        seconds += 6;

    s2 = outils::bcd_add(s2, s2);
    int16_t d3 = s2;
    s2 = outils::bcd_add(s2, s2);
    s2 = outils::bcd_add(s2, d3);
    seconds = outils::bcd_add(s2, seconds);

    // Output Milliseconds
    laptime[5] = (ostats.lap_ms[ms_lookup] & 0xF) | TILE_PROPS;
    laptime[4] = ((ostats.lap_ms[ms_lookup] & 0xF0) >> 4) | TILE_PROPS;

    // Output Seconds
    laptime[3] = (seconds & 0xF) | TILE_PROPS;
    laptime[2] = ((seconds & 0xF0) >> 4) | TILE_PROPS;

    // Output Minutes
    laptime[1] = (minutes & 0xF) | TILE_PROPS;
    laptime[0] = ((minutes & 0xF0) >> 4) | TILE_PROPS;
}
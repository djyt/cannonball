/***************************************************************************
    In-Game Statistics.
    - Stage Timers
    - Route Info
    - Speed to Score Conversion
    - Bonus Time Increment
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/outils.hpp"
#include "engine/ostats.hpp"

OStats ostats;

OStats::OStats(void)
{
}

OStats::~OStats(void)
{
}

void OStats::clear_stage_times()
{
    stage_counters[0] = stage_counters[1] = stage_counters[2] = stage_counters[3] = stage_counters[4] = 0;

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 3; j++)
            stage_times[i][j] = 0;
}

void OStats::clear_route_info()
{
    route_info = 0;
    routes[0] = routes[1] = routes[2] = routes[3] = 
    routes[4] = routes[5] = routes[6] = routes[7] = 0;
}

// Increment Counters, Stage Timers & Print Stage Timers
//
// Source: 0x7F12
void OStats::do_timers()
{
    if (outrun.game_state != GS_INGAME) return;

    // Each stage has a standard counter that just increments. Do this here.
    stage_counters[cur_stage]++;

    inc_lap_timer();
    ohud.draw_lap_timer(0x11016C, stage_times[cur_stage], ms_value);
}

// Increment and store lap timer for each stage.
//
// Source: 0x7F4C
void OStats::inc_lap_timer()
{
    // Add MS (Not actual milliseconds, as these are looked up from the table below)
    if (++stage_times[cur_stage][2] >= 0x40)
    {
        // Looped MS, so add a second
        stage_times[cur_stage][2] = 0;
        stage_times[cur_stage][1] = outils::bcd_add(stage_times[cur_stage][1], 1);

        // Loop seconds, so add a minute
        if (stage_times[cur_stage][1] == 0x60)
        {
            stage_times[cur_stage][1] = 0;
            stage_times[cur_stage][0] = outils::bcd_add(stage_times[cur_stage][0], 1);
        }
    }

    // Get MS Value
    ms_value = LAP_MS[stage_times[cur_stage][2]];
}

// Conversion table from 0 to 64 -> Millisecond value
const uint8_t OStats::LAP_MS[] = 
{
    0x00, 0x01, 0x03, 0x04, 0x06, 0x07, 0x09, 0x10, 0x12, 0x14, 0x15, 0x17, 0x18, 0x20, 0x21, 0x23,
    0x25, 0x26, 0x28, 0x29, 0x31, 0x32, 0x34, 0x35, 0x37, 0x39, 0x40, 0x42, 0x43, 0x45, 0x46, 0x48,
    0x50, 0x51, 0x53, 0x54, 0x56, 0x57, 0x59, 0x60, 0x62, 0x64, 0x65, 0x67, 0x68, 0x70, 0x71, 0x73,
    0x75, 0x76, 0x78, 0x79, 0x81, 0x82, 0x84, 0x85, 0x87, 0x89, 0x90, 0x92, 0x93, 0x95, 0x96, 0x98,
};

// Source: 0xBE4E
void OStats::convert_speed_score(uint16_t speed)
{
    // 0x960 is the last value in this table to be actively used
    const uint16_t CONVERT[] = 
    {
        0x0,   0x10,  0x20,  0x30,  0x40,  0x50,  0x60,  0x80,  0x110, 0x150,
        0x200, 0x260, 0x330, 0x410, 0x500, 0x600, 0x710, 0x830, 0x960, 0x1100,
        0x1250,
    };

    uint16_t score = CONVERT[(speed >> 4)];
    update_score(score);
}

// Update In-Game Score. Adds Value To Overall Score.
//
// Source: 0x7340
void OStats::update_score(uint32_t value)
{
    score = outils::bcd_add(value, score);

    if (score > 0x99999999)
        score = 0x99999999;

    ohud.draw_score_ingame(score);
}

// Initialize Next Level
//
// In-Game Only:
//
// 1/ Show Extend Play Timer
// 2/ Add correct time extend for time adjustment setting from dips
// 3/ Setup next level with relevant number of enemies
// 4/ Blit some info to the screen
//
// Source: 0x8FAC

void OStats::init_next_level()
{
    if (extend_play_timer)
    {
        // End Extend Play: Clear Text From Screen
        if (--extend_play_timer <= 0)
        {
            ohud.blit_text1(TEXT1_EXTEND_CLEAR1);
            ohud.blit_text1(TEXT1_EXTEND_CLEAR2);
            ohud.blit_text1(TEXT1_LAPTIME_CLEAR1);
            ohud.blit_text1(TEXT1_LAPTIME_CLEAR2);
        }
        // Extend Play: Flash Text
        else
        {
            int16_t do_blit = ((extend_play_timer - 1) ^ extend_play_timer) & BIT_3;

            if (do_blit)
            {
                if (extend_play_timer & BIT_3)
                {
                    ohud.blit_text1(TEXT1_EXTEND1);
                    ohud.blit_text1(TEXT1_EXTEND2);
                }
                else
                {
                    ohud.blit_text1(TEXT1_EXTEND_CLEAR1);
                    ohud.blit_text1(TEXT1_EXTEND_CLEAR2);
                }
            }
        }
    }
    else if (outrun.game_state == GS_INGAME && oinitengine.checkpoint_marker)
    {
        oinitengine.checkpoint_marker = 0;

        uint16_t time_lookup = (DIP_TIME * 40) + oroad.stage_lookup_off;
        time_counter = outils::bcd_add(time_counter, TIME[time_lookup]);
        otraffic.set_max_traffic();
        osoundint.queue_sound(sound::YM_CHECKPOINT);
        osoundint.queue_sound(sound::VOICE_CHECKPOINT);
        extend_play_timer = 0x80;
        ohud.blit_text1(TEXT1_LAPTIME1);
        ohud.blit_text1(TEXT1_LAPTIME2);

        // Draw last laptime
        // Note there is a bug in the original code here, where the current ms value is displayed, instead of the ms value from the last lap time
        ohud.draw_lap_timer(0x110554, stage_times[cur_stage-1], FIX_BUGS ? LAP_MS[stage_times[cur_stage-1][2]] : ms_value);

        // Update Stage Number on HUD
        ohud.draw_digits(0x110d76, cur_stage+1);

        // No need to redraw the stage info as that was a bug in the original game
    }
}

// Time Tables
//
// - Show how much time will be incremented to the counter at each stage
// - Rightmost routes first
// - Note there appears to be an error with the Stage 3a Normal entry
//
//         | Easy | Norm | Hard | VHar |
//         '------'------'------'------'
//Stage 1  |  80     75     72     70  |
//         '---------------------------'
//Stage 2a |  65     65     65     65  |
//Stage 2b |  62     62     62     62  |
//         '---------------------------'
//Stage 3a |  57     55     57     57  |
//Stage 3b |  62     60     60     60  |
//Stage 3c |  60     60     59     58  |
//         '---------------------------'
//Stage 4a |  66     65     64     62  |
//Stage 4b |  63     62     60     60  |
//Stage 4c |  61     60     58     58  |
//Stage 4d |  65     65     63     63  |
//         '---------------------------'
//Stage 5a |  58     56     54     54  |
//Stage 5b |  55     56     54     54  |
//Stage 5c |  56     56     54     54  |
//Stage 5d |  58     56     54     54  |
//Stage 5e |  56     56     56     56  |
//         '---------------------------'


const uint8_t OStats::TIME[] =
{
    // Easy
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x65, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x57, 0x62, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x66, 0x63, 0x61, 0x65, 0x00, 0x00, 0x00, 0x00, 
    0x58, 0x55, 0x56, 0x58, 0x56, 0x00, 0x00, 0x00,

    // Normal 
    0x75, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x65, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x55, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x65, 0x62, 0x60, 0x65, 0x00, 0x00, 0x00, 0x00,
    0x56, 0x56, 0x56, 0x56, 0x56, 0x00, 0x00, 0x00, 

    // Hard
    0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x65, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x57, 0x60, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x64, 0x60, 0x58, 0x63, 0x00, 0x00, 0x00, 0x00,
    0x54, 0x54, 0x54, 0x54, 0x56, 0x00, 0x00, 0x00, 

    // Hardest
    0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x65, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x57, 0x60, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x62, 0x60, 0x58, 0x63, 0x00, 0x00, 0x00, 0x00, 
    0x54, 0x54, 0x54, 0x54, 0x56, 0x00, 0x00, 0x00,
};

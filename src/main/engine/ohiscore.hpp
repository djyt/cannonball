/***************************************************************************
    Best Outrunners Name Entry & Display.
    Used in attract mode, and at game end.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.hpp"

struct score_entry
{
    uint32_t score;
    uint8_t initial1;
    uint8_t initial2;
    uint8_t initial3;
    uint32_t maptiles;
    uint16_t time;
};

class OHiScore
{
public:
    // Number of score entries in table
    const static uint8_t NO_SCORES = 20;
    
    // 20 Score Entries
    score_entry scores[NO_SCORES];

    OHiScore(void);
    ~OHiScore(void);

    void init();
    void init_def_scores();
    void tick();
    void setup_pal_best();
    void setup_road_best();
    void display_scores();

private:
    const static uint16_t TILE_PROPS = 0x8030;

    // +C : Best OutRunners State
    uint8_t best_or_state;

    // +14: State of score logic
    uint8_t state;

    // +16: High Score Position In Table
    int8_t score_pos;

    // +17 Selected Initial (0-2)
    int8_t initial_selected;

    // +18: Selected Letter
    int16_t letter_selected;

    // +1A: Acceleration Value Current
    int16_t acc_curr;

    // +1C: Acceleration Value Previous
    int16_t acc_prev;

    // +1E: Steering Value
    int16_t steer;

    // +22: Flashing counter
    uint8_t flash;

    // +24: Total number of minicars that have reached destination
    int8_t dest_total;

    // +26: High Score Table Display Position
    int8_t score_display_pos;

    enum
    {
        STATE_GETPOS,   // Detect Score Position, Insert Score, Init Table
        STATE_DISPLAY,  // Display Basic High Score Table
        STATE_ENTRY,    // Init Name Entry
        STATE_DONE      // Score Done
    };

    // Mini-car data format. 
    // These are the mini cars that move across and reveal the high score entries
    struct minicar_entry
    {
        int16_t pos;            // [+0] Word 0: Position
        int16_t speed;          // [+2] Word 1: Speed (increments over time)
        int16_t base_speed;     // [+4] Word 2: Base Speed
        int16_t dst_reached;    // [+6] Word 3: Set when reached destination
        uint16_t tile_props;    // [+8] Word 4: Palette/Priority bits for tile
    };

    // Number of minicar entries
    const static uint8_t NO_MINICARS = 7;

    // 20 Score Entries
    minicar_entry minicars[NO_MINICARS];

    // Stores Laptime conversion
    // +0: Minutes Digit 1
    // +1: Minutes Digit 2
    // +2: Seconds Digit 1
    // +3: Seconds Digit 2
    // +4: Milliseconds Digit 1
    // +5: Milliseconds Digit 2
    uint16_t laptime[6];

    void get_score_pos();
    void insert_score();
    void set_display_pos();
    void check_name_entry();
    uint32_t get_score_adr();
    void blit_alphabet();
    void flash_entry(uint32_t adr);
    void do_input(uint32_t adr);
    int8_t read_controls();
    void setup_minicars();
    void tick_minicars();
    void setup_minicars_pal(minicar_entry*);
    void blit_score_table();
    void blit_scores();
    void blit_digit();
    void blit_initials();
    void blit_route_map();
    void blit_lap_time();
    void convert_lap_time(uint16_t);
};

extern OHiScore ohiscore;
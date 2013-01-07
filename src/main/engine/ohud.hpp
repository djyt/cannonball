/***************************************************************************
    Heads-Up Display (HUD) Code
    
    - Score Rendering
    - Timer Rendering
    - Rev Rendering
    - Minimap Rendering
    - Text Rendering
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

class OHud
{
public:

    // Colours for default text palette
    enum 
    {
        GREY  = 0x84,
        PINK  = 0x86,
        GREEN = 0x92,
    } colors;

    // Base for digits, for fast digit drawing
    const static uint16_t DIGIT_BASE = 0x30;

    OHud(void);
    ~OHud(void);

    void draw_main_hud();
    void clear_timetrial_text();
    void do_mini_map();
    void draw_timer1(uint16_t);
    void draw_timer2(uint16_t, uint32_t, uint16_t);
    void draw_lap_timer(uint32_t, uint8_t*, uint8_t);
    void draw_score_ingame(uint32_t);
    void draw_score(uint32_t, const uint32_t, const uint8_t);
    void draw_score_tile(uint32_t, const uint32_t, const uint8_t);
    void draw_digits(uint32_t, uint8_t, uint16_t col = GREEN);
    void draw_rev_counter();
	void blit_text1(uint32_t);
    void blit_text1(uint8_t x, uint8_t y, uint32_t src_addr);
	void blit_text2(uint32_t);
    void blit_text_big(const uint8_t Y, const char* text, bool do_notes = false);
    void blit_text_new(uint16_t, uint16_t, const char* text, uint16_t col = GREY);
    void blit_speed(uint32_t, uint16_t);
    void blit_large_digit(uint32_t*, uint8_t);
	void draw_copyright_text();
    void draw_insert_coin();
    void draw_credits();
    uint32_t setup_mini_map();
    uint32_t translate(const uint16_t x, const uint16_t y, const uint32_t BASE_POS = 0x110030);
    void blit_debug();

private:
    void draw_mini_map(uint32_t);
};

extern OHud ohud;
/***************************************************************************
    Bonus Points Code.
    
    This is the code that displays your bonus points on completing the game.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/ostats.hpp"
#include "engine/ohud.hpp"
#include "engine/outils.hpp"
#include "engine/obonus.hpp"

OBonus obonus;

OBonus::OBonus(void)
{
}

OBonus::~OBonus(void)
{
}

void OBonus::init()
{
    bonus_control = BONUS_DISABLE;
    bonus_state   = BONUS_TEXT_INIT;

    bonus_counter = 0;
}

// Display Text and countdown time for bonus mode
// Source: 0x99E0
void OBonus::do_bonus_text()
{
    switch (bonus_state)
    {
        case BONUS_TEXT_INIT:
            init_bonus_text();
            break;

        case BONUS_TEXT_SECONDS:
            decrement_bonus_secs();
            break;

        case BONUS_TEXT_CLEAR:
        case BONUS_TEXT_DONE:
            if (bonus_counter < 60)
                bonus_counter++;
            else
            {
                ohud.blit_text2(TEXT2_BONUS_CLEAR1);
                ohud.blit_text2(TEXT2_BONUS_CLEAR2);
                ohud.blit_text2(TEXT2_BONUS_CLEAR3);
            }
            break;
    }
}

// Calculate Bonus Seconds. This uses the seconds remaining for every lap raced.
// Print stuff to text layer for bonus mode.
//
// Source: 0x9A9C
void OBonus::init_bonus_text()
{
    bonus_state = BONUS_TEXT_SECONDS;
    
    int16_t time_counter_bak = ostats.time_counter << 8;
    ostats.time_counter = 0x30;

    uint16_t total_time = 0;

    if (outrun.cannonball_mode == Outrun::MODE_ORIGINAL)
    {
        // Add milliseconds remaining from previous stage times
        for (int i = 0; i < 5; i++)
        {
            total_time = outils::bcd_add(outils::DEC_TO_HEX[ostats.stage_times[i][2]], total_time);
        }
    }

    // Mask on top digit of lap milliseconds
    total_time &= 0xF0;

    if (total_time)
    {
        time_counter_bak |= (10 - (total_time >> 4));
    }
    // So 60 seconds remaining on the clock and 3 from lap_seconds would be 0x0603

    // Extract individual 3 digits
    uint16_t digit_mid = (time_counter_bak >> 8  & 0x0F) * 10;
    uint16_t digit_top = (time_counter_bak >> 12 & 0x0F) * 100;
    uint16_t digit_bot = (time_counter_bak & 0x0F);

    // Write them back to final bonus seconds value
    bonus_secs = digit_bot + digit_mid + digit_top;

    // Write to text layer
    ohud.blit_text2(TEXT2_BONUS_POINTS); // Print "BONUS POINTS"
    ohud.blit_text1(TEXT1_BONUS_STOP);   // Print full stop after Bonus Points text
    ohud.blit_text1(TEXT1_BONUS_SEC);    // Print "SEC"
    ohud.blit_text1(TEXT1_BONUS_X);      // Print 'X' symbol after SEC
    ohud.blit_text1(TEXT1_BONUS_PTS);    // Print "PTS"

    // Blit big 100K number
    uint32_t src_addr = TEXT1_BONUS_100K;
    uint32_t dst_addr = 0x11065A;
    int8_t count = roms.rom0.read8(&src_addr);

    for (int8_t i = 0; i <= count; i++)
        ohud.blit_large_digit(&dst_addr, (roms.rom0.read8(&src_addr) - 0x30) << 1);

    blit_bonus_secs();
}

// Decrement bonus seconds. Blit seconds remaining.
// Source: 9A08
void OBonus::decrement_bonus_secs()
{
    if (bonus_counter < 60)
    {
        bonus_counter++;
        return;
    }

    // Play Signal 1 Sound In A Steady Fashion
    if ((((bonus_counter - 1) ^ bonus_counter) & BIT_2) == 0)
        osoundint.queue_sound(sound::SIGNAL1);

    // Increment Score by 100K points
    ostats.update_score(0x100000);
    
    // Blit bonus seconds remaining
    blit_bonus_secs();
    
    if (--bonus_secs < 0)
    {
        bonus_counter = -1;
        bonus_state = BONUS_TEXT_CLEAR;
    }
    else
        bonus_counter++;

}

// Blit large yellow second remaining value e.g.: 23.3
// Source: 0x9B7C
void OBonus::blit_bonus_secs()
{
    const uint8_t COL2 = 0x80;
    const uint16_t TILE_DOT = 0x8C2E;
    const uint16_t TILE_ZERO = 0x8420;

    uint32_t d1 = (bonus_secs / 100) << 8;
    uint32_t d4 = (bonus_secs / 100) * 100;

    uint32_t d2 = (bonus_secs - d4) / 10;
    uint32_t d3 = bonus_secs - d4;

    d4 = d2;
    d2 <<= 4;
    d4 *= 10;
    d3 -= d4;
    d1 += d2;
    d1 += d3;

    d3 = (d1 & 0xF)   << 1;
    d2 = (d1 & 0xF0)  >> 3;
    d1 = (d1 & 0xF00) >> 7;

    uint32_t text_addr = 0x110644;

    // Blit Digit 1
    if (d1)
    {
        ohud.blit_large_digit(&text_addr, d1);
    }
    else
    {
        video.write_text16(text_addr, TILE_ZERO);
        video.write_text16(text_addr | COL2, TILE_ZERO);
        text_addr += 2;        
    }

    // Blit Digit 2
    ohud.blit_large_digit(&text_addr, d2);
    // Blit Dot
    video.write_text16(text_addr | COL2, TILE_DOT);
    text_addr += 2;
    // Blit Digit 3
    ohud.blit_large_digit(&text_addr, d3);
}

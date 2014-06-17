/***************************************************************************
    Cabinet Diagnostics.

    Diagnostics Modes. Based On Original Code, with modifications.
    Mostly of use for real cabinets.

    - CRT Check
    - CannonBoard Interface Check
    - Motor Hardware Test
    - Brake/Start Lamp Test
    - Control Input Test
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "cabdiag.hpp"

#include "../sdl/input.hpp"
#include "utils.hpp"
#include "../video.hpp"
#include "../cannonboard/interface.hpp"
#include "../engine/outrun.hpp"
#include "../engine/ooutputs.hpp"
#include "../engine/ohud.hpp"
#include "../engine/otiles.hpp"

const static uint32_t PAL_CRT[] = {0xF, 0xF000FF, 0xF000F0F, 0xFF00FFF, 0xFFF, 0xEEE0DDD, 0xCCC0BBB,
                                   0xAAA0999, 0x888, 0x0, 0x0, 0x0, 0x777, 0x6660555, 0x4440333, 0x2220111};

CabDiag::CabDiag(Interface* cannonboard)
{
    this->cannonboard = cannonboard;
}

CabDiag::~CabDiag(void)
{

}

void CabDiag::reset()
{
    done    = false;
    counter = 0;

    video.clear_text_ram();
    otiles.fill_tilemap_color(0x4F60);  
    osprites.disable_sprites();

    oroad.horizon_set    = 1;
    oroad.horizon_base   = -0x3FF;

    // Write Palette To RAM
    uint32_t dst = 0x120000;
    const static uint32_t PAL_SERVICE[] = {0xFF, 0xFF00FF, 0xFF00FF, 0xFF0000};
    video.write_pal32(&dst, PAL_SERVICE[0]);
    video.write_pal32(&dst, PAL_SERVICE[1]);
    video.write_pal32(&dst, PAL_SERVICE[2]);
    video.write_pal32(&dst, PAL_SERVICE[3]);
}

void CabDiag::set(uint8_t state)
{
    this->state = state;
    init = false;
}

bool CabDiag::tick(Packet* packet)
{
    // Initialize State
    if (!init)
    {
        init = true;
        press_start_to_exit = true;
        reset();

        switch (state)
        {
            case STATE_INTERFACE:
                init_interface();
                break;

            case STATE_OUTPUT:
                init_output();
                break;

            case STATE_INPUT:
                init_input();
                break;

            case STATE_CRT:
                init_crt();
                break;

            case STATE_MOTORT:
                init_motor_test();
                press_start_to_exit = false; // Not skippable
                break;
        }
    }

    if (counter == 60)
        ohud.blit_text_new(7, 23, "PRESS START BUTTON TO EXIT", 0x84);

    if (press_start_to_exit && counter >= 60 && input.is_pressed(Input::START))
    {
        done = true;
    }

    // Tick State
    switch (state)
    {
        case STATE_INTERFACE:
            tick_interface(packet);
            break;

        case STATE_OUTPUT:
            tick_output();
            break;

        case STATE_INPUT:
            tick_input(packet);
            break;

        case STATE_CRT:
            break;

        case STATE_MOTORT:
            press_start_to_exit = outrun.outputs->diag_motor(packet->ai1, packet->mci, 0);
            break;
    }
    osprites.sprite_copy();
    osprites.update_sprites();
    otiles.write_tilemap_hw();
    oroad.tick();
    
    if (press_start_to_exit)
        counter++;

    return done;
}

// ------------------------------------------------------------------------------------------------
// INTERFACE DIAGNOSTICS
// ------------------------------------------------------------------------------------------------

void CabDiag::init_interface()
{
    //cannonboard->reset_stats();
    int x = 10;
    int y = 2;
    blit_box();
    ohud.blit_text_new(3, y, "CANNONBOARD INTERFACE DIAGNOSTICS", 0x86);

    y = 5;
    ohud.blit_text_new(x, y++, "SERIAL PORT", 0x84); y+=2;

    ohud.blit_text_new(4, y++, "- INBOUND PACKET INFORMATION  -", 0x82); y++;
    ohud.blit_text_new(x, y++, "GOOD", 0x84);
    ohud.blit_text_new(x, y++, "BAD", 0x84);
    ohud.blit_text_new(x, y++, "NOT FOUND", 0x84); y+=2;

    ohud.blit_text_new(4, y++, "- OUTBOUND PACKET INFORMATION -", 0x82); y++;
    ohud.blit_text_new(x, y++, "GOOD", 0x84);
    ohud.blit_text_new(x, y++, "BAD", 0x84);
    ohud.blit_text_new(x, y++, "MISSED", 0x84);
}

void CabDiag::tick_interface(Packet* packet)
{
    int x = 23;
    int y = 5;
    ohud.blit_text_new(x, y, cannonboard->started() ? "READY" : "ERROR", 0x80);

    y = 10;
    ohud.blit_text_new(x, y++, Utils::to_hex_string(cannonboard->stats_found_in).c_str(), 0x80);
    ohud.blit_text_new(x, y++, Utils::to_hex_string(cannonboard->stats_error_in).c_str(), 0x80);
    ohud.blit_text_new(x, y++, Utils::to_hex_string(cannonboard->stats_notfound_in).c_str(), 0x80);
    y += 4;
    ohud.blit_text_new(x, y++, Utils::to_hex_string(cannonboard->stats_found_out).c_str(), 0x80);
    ohud.blit_text_new(x, y++, Utils::to_hex_string(cannonboard->stats_error_out).c_str(), 0x80);
    ohud.blit_text_new(x, y++, Utils::to_hex_string(cannonboard->stats_missed_out).c_str(), 0x80);
}

// ------------------------------------------------------------------------------------------------
// OUTPUT TEST
// ------------------------------------------------------------------------------------------------

void CabDiag::init_output()
{
    blit_box();

    // Draw Text
    ohud.blit_text_new(15, 2, "DIAGNOSTIC", 0x86);
    ohud.blit_text_new(14, 4, "OUTPUT  TEST", 0x80);
    ohud.blit_text_new(13, 6, "START LAMP", 0x84);
    ohud.blit_text_new(13, 7, "BRAKE LAMP", 0x84);
}

void CabDiag::tick_output()
{
    if (counter & BIT_5)
    {
        ohud.blit_text_new(24, 6, " ON", 0x80);
        ohud.blit_text_new(24, 7, " ON", 0x80);
        outrun.outputs->set_digital(OOutputs::D_START_LAMP);
        outrun.outputs->set_digital(OOutputs::D_BRAKE_LAMP);
    }
    else
    {
        ohud.blit_text_new(24, 6, "OFF", 0x86);
        ohud.blit_text_new(24, 7, "OFF", 0x86);
        outrun.outputs->clear_digital(OOutputs::D_START_LAMP);
        outrun.outputs->clear_digital(OOutputs::D_BRAKE_LAMP);
    }

    if (done)
    {
        outrun.outputs->clear_digital(OOutputs::D_START_LAMP);
        outrun.outputs->clear_digital(OOutputs::D_BRAKE_LAMP);
    }
}

// ------------------------------------------------------------------------------------------------
// INPUT TEST
// ------------------------------------------------------------------------------------------------

void CabDiag::init_input()
{
    blit_box();

    // Draw Text
    ohud.blit_text_new(15, 2, "DIAGNOSTIC", 0x86);
    ohud.blit_text_new(15, 4, "INPUT TEST", 0x80);

    int x = 14;
    int y = 8;
    ohud.blit_text_new(x, y++, "COIN #1", 0x84);
    ohud.blit_text_new(x, y++, "COIN #2", 0x84); y++;
    ohud.blit_text_new(x, y++, "SERVICE", 0x84);
    ohud.blit_text_new(x, y++, "START", 0x84); y += 2;
    ohud.blit_text_new(x, y++, "GEAR", 0x84); y++;
    ohud.blit_text_new(x, y++, "WHEEL", 0x84);
    ohud.blit_text_new(x, y++, "BRAKE", 0x84);
    ohud.blit_text_new(x, y++, "ACCEL", 0x84);
}

void CabDiag::tick_input(Packet* packet)
{
    int x = 23;
    int y = 8;
    ohud.blit_text_new(x, y++, (packet->di1 & 0x40) ? "ON " : "OFF", 0x80);  // COIN 1
    ohud.blit_text_new(x, y++, (packet->di1 & 0x80) ? "ON " : "OFF", 0x80);  // COIN 2
    y++;
    ohud.blit_text_new(x, y++, (packet->di1 & 0x04) ? "ON " : "OFF", 0x80);  // SERVICE
    ohud.blit_text_new(x, y++, (packet->di1 & 0x08) ? "ON " : "OFF", 0x80);  // START
    y += 2;
    ohud.blit_text_new(x, y++, (packet->di1 & 0x10) ? "LOW " : "HIGH", 0x80); // GEAR
    y++;
    ohud.blit_text_new(x, y, "  H", 0x80);
    ohud.blit_text_new(x, y++, Utils::to_hex_string(packet->ai2).c_str(), 0x80); // WHEEL
    ohud.blit_text_new(x, y, "  H", 0x80);
    ohud.blit_text_new(x, y++, Utils::to_hex_string(packet->ai0).c_str(), 0x80); // BRAKE
    ohud.blit_text_new(x, y, "  H", 0x80);
    ohud.blit_text_new(x, y++, Utils::to_hex_string(packet->ai3).c_str(), 0x80); // ACCEL
}

// ------------------------------------------------------------------------------------------------
// CRT POSITION AND COLOUR TEST
// ------------------------------------------------------------------------------------------------

void CabDiag::init_crt()
{
    // Draw Text
    ohud.blit_text_new(15, 2, "DIAGNOSTIC", 0x86);
    ohud.blit_text_new(15, 4, "C.R.T.TEST", 0x80);
    ohud.blit_text_new(15, 6, "COLOR BAR", 0x84);
    ohud.blit_text_new(11, 26,"C.R.T.POSITION CHECK", 0x84);

    blit_box();

    // Initalize Palette
    uint32_t dst = 0x120040;
    for (uint8_t i = 0; i < 16; i++)
        video.write_pal32(&dst, PAL_CRT[i]);

    dst = 0x110438;
    blit1_block(dst,  0x8CF78CF7); dst += 8;
    blit7_block(&dst, 0x88F688F6);
    dst = 0x1106B8;
    blit7_block(&dst, 0x8AF68AF6);
    blit1_block(dst,  0x8CF68CF6);
    dst = 0x110938;
    blit7_block(&dst, 0x8EF68EF6);
    blit1_block(dst,  0x8CF78CF7);
}

void CabDiag::blit_box()
{
    // Draw Top Border
    uint32_t dst = 0x110030;
    video.write_text16(&dst, 0x8001);
    for (uint8_t i = 0; i < 38; i++)
        video.write_text16(&dst, 0x8002);
    video.write_text16(&dst, 0x8003);

    // Draw Bottom Border
    dst = 0x110DB0;
    video.write_text16(&dst, 0x8006);
    for (uint8_t i = 0; i < 38; i++)
        video.write_text16(&dst, 0x8007);
    video.write_text16(&dst, 0x8008);

    // Draw Left & Right Border
    dst = 0x1100B0;
    for (uint8_t i = 0; i < 26; i++)
    {
        video.write_text16(dst,        0x8004);
        video.write_text16(dst + 0x4E, 0x8005);
        dst += 0x80;
    }
}

void CabDiag::blit1_block(uint32_t adr, uint32_t data)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        video.write_text32(adr + 0, data);
        video.write_text32(adr + 4, data);
        adr += 0x80;
    }
}
void CabDiag::blit7_block(uint32_t* adr, uint32_t data)
{
    for (uint8_t i = 0; i < 7; i++)
    {
        blit1_block(*adr, data);
        *adr += 8;
        data += 0x10001;
    }
}

// ------------------------------------------------------------------------------------------------
// MOTOR DIAGNOSTICS TEST
// ------------------------------------------------------------------------------------------------

void CabDiag::init_motor_test()
{
    blit_box();

    // Draw Text
    ohud.blit_text_new(15, 2, "DIAGNOSTIC", 0x86);
    ohud.blit_text_new(15, 4, "MOTOR TEST", 0x80);
}
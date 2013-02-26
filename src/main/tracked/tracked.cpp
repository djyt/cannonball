#include <iostream>
#include <sstream>

#include "main.hpp"
#include "sdl/input.hpp"
#include "sdl/video.hpp"
#include "frontend/config.hpp"
#include "tracked/tracked.hpp"
#include "engine/outrun.hpp"
#include "engine/ohud.hpp"
#include "engine/olevelobjs.hpp"
#include "engine/otiles.hpp"

/*

    Block End Position is when the block is deemed to end.
    This is when the scenery is closest to the camera. 
    Road pos is close to the block end position.

    The block that is displayed is the stuff closest to the camera.

*/

Tracked::Tracked(void)
{

}

Tracked::~Tracked(void)
{

}

void Tracked::init()
{
    state = INIT;
}

void Tracked::tick()
{
    switch (state)
    {
        case INIT:
            outrun.init();
            video.enabled = true;
            cannonball::frame = 0;
            outrun.game_state = GS_INGAME;
            state = TICK;
            break;

        case TICK:
            if (cannonball::tick_frame) controls();
            tick_track();
            video.clear_text_ram();
            //display_sprite_info();
            display_path_info();
            break;
    }
}

void Tracked::tick_track()
{
    // CPU 1
    if (cannonball::tick_frame)
    {
        oinitengine.update_road();
        oinitengine.update_shadow_offset();
        uint32_t result = 0x12F * (oinitengine.car_increment >> 16);
        oroad.road_pos_change = result;
        oroad.road_pos += result;
        oinitengine.set_granular_position();
        oinitengine.set_fine_position();

        oroad.road_width_bak = oroad.road_width >> 16; 
        oroad.car_x_bak = oroad.road_width_bak; 
        oinitengine.car_x_pos = oroad.car_x_bak;
    }
    osprites.tick();
    olevelobjs.do_sprite_routine();
    osprites.sprite_copy();

    // CPU 2
    oroad.tick();

    // Vertical Int
    otiles.write_tilemap_hw();
    osprites.update_sprites();
    otiles.update_tilemaps();
    oinitengine.set_granular_position();
}

void Tracked::display_path_info()
{
    static const int TX = 16; // Text X Offset
    uint32_t addr = oroad.stage_addr + oroad.road_data_offset;

    const int16_t x1 = (int16_t) roms.rom1p->read16(addr);
    const int16_t x2 = (int16_t) roms.rom1p->read16(addr + 2);
    const uint16_t road_pos    = oroad.road_pos >> 16;

    ohud.blit_text_new(0,  0, "ROAD POS", OHud::GREEN);
    ohud.blit_text_new(TX, 0, config.to_string(road_pos).c_str(), OHud::GREEN);

    ohud.blit_text_new(0,  3, "X1     ", OHud::GREEN);
    ohud.blit_text_new(TX, 3, config.to_string(x1).c_str(), OHud::GREEN);
    ohud.blit_text_new(0,  4, "X2     ", OHud::GREEN);
    ohud.blit_text_new(TX, 4, config.to_string(x2).c_str(), OHud::GREEN);
}

void Tracked::display_sprite_info()
{
    static const int TX = 16; // Text X Offset

    uint16_t road_pos    = oroad.road_pos >> 16;
    if (road_pos < 8) return;
    uint32_t sprite_adr  = oinitengine.road_seg_addr1 - 4;
    uint16_t block_start = roms.rom0p->read16(&sprite_adr);                // Road pos of block start
    uint16_t block_end   = roms.rom0p->read16(oinitengine.road_seg_addr1); // Road pos of block end
    uint8_t total_spr    = roms.rom0p->read8(&sprite_adr);                 // Number of sprites in block
    uint8_t sprite_id    = roms.rom0p->read8(&sprite_adr);                 // Sprite Block ID (Used to lookup block)

    // Master Sprite Table
    //
    // Each one of the following addresses contains the following:
    // [+0] Sprite Frequency Value Bitmask [Word]
    // [+2] Reload Value For Sprite Info Offset [Word]
    // [+4] Start of table with x,y,type,palette etc.
    uint32_t master_adr = roms.rom0p->read32(outrun.adr.sprite_master_table + (sprite_id * 4));
    uint16_t seg_sprite_freq = roms.rom0p->read16(master_adr);
    uint16_t seg_spr_offset2 = roms.rom0p->read16(master_adr + 2);
    uint32_t xy_table_adr    = master_adr + 4;

    // Sprite Properties Table
    //
    // [+0] [Byte] Bit 0 = H-Flip Sprite
    //             Bit 1 = Enable Shadows
    //            
    //             Bits 4-7 = Routine Draw Number
    // [+1] [Byte] Sprite X World Position
    // [+2] [Word] Sprite Y World Position
    // [+5] [Byte] Sprite Type
    // [+7] [Byte] Sprite Palette

    uint16_t sprite_x = roms.rom0p->read8 (xy_table_adr + 1);// << 4;
    uint16_t sprite_y = roms.rom0p->read16(xy_table_adr + 2);// << 7;
    uint8_t  sprite_t = roms.rom0p->read8 (xy_table_adr + 5);// << 2;
    uint8_t  sprite_p = roms.rom0p->read8 (xy_table_adr + 7);

    ohud.blit_text_new(0,  0, "ROAD POS", OHud::GREEN);
    ohud.blit_text_new(TX, 0, config.to_hex_string(road_pos).c_str(), OHud::GREEN);

    ohud.blit_text_new(0,  2, "SPRITE INFORMATION", OHud::PINK);
    ohud.blit_text_new(0,  3, "BLOCK START");
    ohud.blit_text_new(TX, 3, config.to_hex_string(block_start).c_str());
    ohud.blit_text_new(0,  4, "BLOCK END");
    ohud.blit_text_new(TX, 4, config.to_hex_string(block_end).c_str());
    ohud.blit_text_new(0,  5, "TOTAL");
    ohud.blit_text_new(TX, 5, config.to_hex_string(total_spr).c_str());
    ohud.blit_text_new(0,  6, "MASTER INDEX");
    ohud.blit_text_new(TX, 6, config.to_hex_string(sprite_id).c_str());
    ohud.blit_text_new(0,  7, "ADDRESS");
    ohud.blit_text_new(TX, 7, config.to_hex_string(master_adr).c_str());

    ohud.blit_text_new(0,  9,  "FREQUENCY TABLE", OHud::PINK);
    ohud.blit_text_new(0,  10, "FREQUENCY");
    ohud.blit_text_new(TX, 10, dec_to_bin(seg_sprite_freq).c_str());
    ohud.blit_text_new(0,  11, "SPRITES DEFINED"); // Number of sprites in this particular block of data
    ohud.blit_text_new(TX, 11, config.to_hex_string(seg_spr_offset2 / 8).c_str()); // 0 to this is the sprite address of block
    ohud.blit_text_new(0,  12, "SPRITE ADDRESS");
    ohud.blit_text_new(TX, 12, config.to_hex_string(xy_table_adr).c_str()); // 0 to this is the sprite address of block

    ohud.blit_text_new(0,  14, "SPRITE PROPERTIES", OHud::PINK);
    ohud.blit_text_new(0,  15, "X Y");
    ohud.blit_text_new(TX, 15, config.to_hex_string(sprite_x).c_str());
    ohud.blit_text_new(TX + 3, 15, config.to_hex_string(sprite_y).c_str());
    ohud.blit_text_new(0,  16, "TYPE");
    ohud.blit_text_new(TX, 16, config.to_hex_string(sprite_t).c_str());
    ohud.blit_text_new(0,  17, "PALETTE");
    ohud.blit_text_new(TX, 17, config.to_hex_string(sprite_p).c_str());
}



std::string Tracked::dec_to_bin(int nValue, bool bReverse)
{
    std::string sBin;  
    while(nValue != 0)
    {
       sBin += (nValue & 1) ? '1' : '0';
       nValue >>= 1;
    }

    if(!bReverse)        
        std::reverse(sBin.begin(),sBin.end());

    return sBin;
}

void Tracked::controls()
{
    if (input.is_pressed(Input::LEFT))
        oinitengine.camera_x_off += 5;
    if (input.is_pressed(Input::RIGHT))
        oinitengine.camera_x_off -= 5;

    // Start/Stop 
    if (input.has_pressed(Input::GEAR))
    {
        if (oinitengine.car_increment != 0x80 << 16)
            oinitengine.car_increment = 0x80 << 16;
        else
            oinitengine.car_increment = 0;
    }

    if (input.is_pressed(Input::UP))
        oinitengine.car_increment = 0x20 << 16;
    else if (oinitengine.car_increment != 0x80 << 16)
        oinitengine.car_increment = 0;

    if (input.is_pressed(Input::ACCEL))
    {
        oroad.horizon_base += -20;
        if (oroad.horizon_base < 0x100)
            oroad.horizon_base = 0x100;
    }

    if (input.is_pressed(Input::BRAKE))
    {
        oroad.horizon_base += 20;
        if (oroad.horizon_base > 0x6A0)
            oroad.horizon_base = 0x6A0;
    }

    input.frame_done();
}
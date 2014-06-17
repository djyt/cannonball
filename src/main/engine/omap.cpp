/***************************************************************************
    Course Map Logic & Rendering. 
    
    This is the full-screen map that is displayed at the end of the game. 
    
    The logo is built from multiple sprite components.
    
    The course map itself is made up of sprites and pieced together. 
    It's not a tilemap.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/oferrari.hpp"
#include "engine/omap.hpp"
#include "engine/otiles.hpp"
#include "engine/otraffic.hpp"
#include "engine/ostats.hpp"

OMap omap;

// Position of Ferrari in Jump Table
const uint8_t SPRITE_FERRARI = 25;

OMap::OMap(void)
{
}


OMap::~OMap(void)
{
}

void OMap::init()
{
    oferrari.car_ctrl_active = false; // -1
    video.clear_text_ram();
    osprites.disable_sprites();
    otraffic.disable_traffic();
    osprites.clear_palette_data();
    oinitengine.car_increment = 0;
    oferrari.car_inc_old      = 0;
    osprites.spr_cnt_main     = 0;
    osprites.spr_cnt_shadow   = 0;
    oroad.road_ctrl           = ORoad::ROAD_BOTH_P0;
    oroad.horizon_base        = -0x3FF;
    otiles.fill_tilemap_color(0xABD); //  Paint pinkish colour on tilemap 16
    init_sprites = true;
}

// Process route through levels
// Process end position on final level
// Source: 0x345E
void OMap::tick()
{
    // 60 FPS Code to simply render sprites
    if (!outrun.tick_frame)
    {
        blit();
        return;
    }

    // Initialize Course Map Sprites if necessary
    if (init_sprites)
    {
        load_sprites();
        init_sprites = false;
        return;
    }

    switch (map_state)
    {
        // Initialise Route Info
        case MAP_INIT:
            video.sprite_layer->set_x_clip(false); // Don't clip the area in wide-screen mode
            map_route  = roms.rom0.read8(MAP_ROUTE_LOOKUP + ostats.routes[1]);
            map_pos    = 0;
            map_stage1 = 0;
            map_stage2 = ostats.cur_stage;
            if (map_stage2 > 0)
                map_state = MAP_ROUTE;
            else
            {
                map_state = MAP_ROUTE_FINAL;
                do_route_final();
                break;
            }

        // Do Route [Note map is displayed from this point on]
        case MAP_ROUTE:
            if (++map_pos > 0x1B)
            {
                if (--map_stage2 <= 0)
                {   //map_end_route
                    map_pos = 0;
                    map_stage1++;
                    uint16_t route_info = ostats.routes[1 + map_stage1];
                    if (route_info)
                    {
                        map_route = roms.rom0.read8(MAP_ROUTE_LOOKUP + route_info);                      
                    }
                    else
                    {
                        map_route = roms.rom0.read8(MAP_ROUTE_LOOKUP + ostats.routes[0 + map_stage1] + 0x10);
                    }

                    map_state = MAP_ROUTE_FINAL;
                    do_route_final();
                }
                else
                {
                    map_pos = 0;
                    map_stage1++;
                    map_route = roms.rom0.read8(MAP_ROUTE_LOOKUP + ostats.routes[1 + map_stage1]);
                }
            }
            break;
        
        // Do Final Segment Of Route [Car still moving]
        case MAP_ROUTE_FINAL:
            do_route_final();
            break;

        // Route Concluded       
        case MAP_ROUTE_DONE:
            end_route();
            break;
        
        // Init Delay Counter For Map Display
        case MAP_INIT_DELAY:
            init_map_delay();
            break;

        // Display Map
        case MAP_DISPLAY:
            map_display();
            break;

        // Clear Course Map        
        case MAP_CLEAR:
            outrun.init_best_outrunners();
            return;
    }

    draw_course_map();
}

// Render sprites only. No Logic
void OMap::blit()
{
    for (uint8_t i = 0; i <= MAP_PIECES; i++)
    {
        oentry* sprite = &osprites.jump_table[i];
        if (sprite->control & OSprites::ENABLE)
            osprites.do_spr_order_shadows(sprite);
    }
}

void OMap::draw_course_map()
{
    oentry* sprite = osprites.jump_table;

    // Draw Road Components
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_vert_bottom(sprite++);
    draw_vert_top   (sprite++);
    draw_horiz_end  (sprite++);
    draw_horiz_end  (sprite++);
    draw_horiz_end  (sprite++);
    draw_horiz_end  (sprite++);
    draw_horiz_end  (sprite++);

    // Draw Mini Car
    move_mini_car   (sprite++);

    // Draw Backdrop Map Pieces
    for (uint8_t i = 26; i <= MAP_PIECES; i++)
    {
        if (sprite->control & OSprites::ENABLE)
            osprites.do_spr_order_shadows(sprite++);
    }
}


void OMap::position_ferrari(uint8_t index)
{
    oentry* segment = &osprites.jump_table[index];
    osprites.jump_table[SPRITE_FERRARI].x = segment->x - 8;
    osprites.jump_table[SPRITE_FERRARI].y = segment->y;
}

// Initalize Course Map Sprites
// 
// Notes: Index 26 is start of water that needs to be changed for widescreen
// 
// Source: 0x33F4
void OMap::load_sprites()
{
    // hacks
    /*ostats.cur_stage = 4;
    ostats.routes[0] = 4;
    ostats.routes[1] = 0x08;
    ostats.routes[2] = 0x18;
    ostats.routes[3] = 0x28;
    ostats.routes[4] = 0x38;
    oinitengine.rd_split_state = 0x16;
    oroad.road_pos = 0x192 << 16;*/
    // end hacks

    uint32_t adr = outrun.adr.sprite_coursemap;

    for (uint8_t i = 0; i <= MAP_PIECES; i++)
    {
        oentry* sprite     = &osprites.jump_table[i];
        sprite->id         = i+1;
        sprite->control    = roms.rom0p->read8(&adr);
        sprite->draw_props = roms.rom0p->read8(&adr);
        sprite->shadow     = roms.rom0p->read8(&adr);
        sprite->zoom       = roms.rom0p->read8(&adr);
        sprite->pal_src    = (uint8_t) roms.rom0p->read16(&adr);
        sprite->priority   = sprite->road_priority = roms.rom0p->read16(&adr);
        sprite->x          = roms.rom0p->read16(&adr);
        sprite->y          = roms.rom0p->read16(&adr);
        sprite->addr       = roms.rom0p->read32(&adr);
        sprite->counter    = 0;  
        
        adr += 4; // throw this address away

        osprites.map_palette(sprite);
    }

    // Wide-screen hack to extend sea to edge of screen.
    if (config.s16_x_off != 0 || config.engine.fix_bugs)
    {
        for (uint8_t i = 26; i <= 30; i++)
        {
            oentry* sprite = &osprites.jump_table[i];
            sprite->addr   = osprites.jump_table[31].addr;
            sprite->x      -= 64;
            sprite->zoom   = 0x7F;
        }
    }

    // Minicar initalization moved here
    minicar_enable = 0;
    osprites.jump_table[SPRITE_FERRARI].x = -0x80;
    osprites.jump_table[SPRITE_FERRARI].y = 0x78;
    map_state = MAP_INIT;
}

// Source: 0x355A
void OMap::do_route_final()
{
    int16_t pos = oroad.road_pos >> 16;
    if (oinitengine.rd_split_state)
        pos += 0x79C;   

    pos = (pos * 0x1B) / 0x94D;
    map_pos_final = pos;

    map_state = MAP_ROUTE_DONE;
    end_route();
}

// Source: 0x3584
void OMap::end_route()
{
    map_pos++;

    if (map_pos_final < map_pos)
    {
        // 359C
        map_pos = map_pos_final;
        minicar_enable = 1;
        map_state = MAP_INIT_DELAY;
        init_map_delay();
    }
}

// Source: 0x35B6
void OMap::init_map_delay()
{
    map_route = 0;
    map_delay = 0x80;
    map_state = MAP_DISPLAY;
    map_display();
}

// Source: 0x35CC
void OMap::map_display()
{
    // Init Best OutRunners
    if (--map_delay <= 0)
    {
        map_state = MAP_CLEAR;
        outrun.init_best_outrunners();
    }
}

// ------------------------------------------------------------------------------------------------
// Colour sprite based road as car moves over it on mini-map
// ------------------------------------------------------------------------------------------------

// Source: 0x3740
void OMap::draw_vert_top(oentry* sprite)
{
    if (sprite->control & OSprites::ENABLE)
        draw_piece(sprite, outrun.adr.sprite_coursemap_top);
}

// Source: 0x3736
void OMap::draw_vert_bottom(oentry* sprite)
{
    if (sprite->control & OSprites::ENABLE)
        draw_piece(sprite, outrun.adr.sprite_coursemap_bot);
}

// Source: 0x372C
void OMap::draw_horiz_end(oentry* sprite)
{
    if (sprite->control & OSprites::ENABLE)
        draw_piece(sprite, outrun.adr.sprite_coursemap_end);
}

// Source: 0x3746
void OMap::draw_piece(oentry* sprite, uint32_t adr)
{
    // Update palette of background piece, to highlight route as minicar passes over it
    if (map_route == sprite->id)
    {
        sprite->priority = 0x102;
        sprite->road_priority = 0x102;

        adr += (map_pos << 3);

        sprite->addr    = roms.rom0p->read32(adr);
        sprite->pal_src = roms.rom0p->read8(4 + adr);
        osprites.map_palette(sprite);
    }

    osprites.do_spr_order_shadows(sprite);
}

// Move mini car sprite on Course Map Screen
// Source: 0x3696
void OMap::move_mini_car(oentry* sprite)
{
    // Move Mini Car
    if (!minicar_enable)
    {
        // Remember that the minimap is angled, so we still need to adjust both the x and y positions
        uint32_t movement_table = (map_route & 1) ? MAP_MOVEMENT_RIGHT : MAP_MOVEMENT_LEFT;
        
        int16_t pos = (map_stage1 < 4) ? map_pos : map_pos >> 1;
        pos <<= 1; // do not try to merge with previous line

        sprite->x += roms.rom0.read16(movement_table + pos);
        int16_t y_change = roms.rom0.read16(movement_table + pos + 0x40);
        sprite->y -= y_change;

        if (y_change == 0)
            sprite->addr = outrun.adr.sprite_minicar_right;
        else if (y_change < 0)
            sprite->addr = outrun.adr.sprite_minicar_down;
        else
            sprite->addr = outrun.adr.sprite_minicar_up;
    }

    osprites.do_spr_order_shadows(sprite);
}
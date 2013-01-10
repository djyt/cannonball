#include "sdl/input.hpp"
#include "frontend/ttrial.hpp"

#include "engine/omap.hpp"
#include "engine/ostats.hpp"

// Notes: Set the car up as follows to snap it to the relevant map piece
//  osprites.jump_table[25].x = osprites.jump_table[z].x;
//  osprites.jump_table[25].y = osprites.jump_table[z].y;

// Where z is 1,3,5,7,9,11,13,15 based on a stage in the route map.
// We could also change the colour of the road here. 

// Track Selection: Ferrari Position Per Track
// This is a link to a sprite object that represents part of the course map.
static const uint8_t FERRARI_POS[] = 
{
    1,5,3,11,9,7,19,17,15,13,24,23,22,21,20
};

// Map Stage Number to Internal Lookup 
static const uint8_t STAGE_LOOKUP[] = 
{
    0x00,
    0x09, 0x08,
    0x12, 0x11, 0x10,
    0x1B, 0x1A, 0x19, 0x18,
    0x24, 0x23, 0x22, 0x21, 0x20
};

TTrial::TTrial(void)
{
    // Reset all times to default
    for (uint8_t i = 0; i < 15; i++)
    {
        best_times[i][0] = 0x01;
        best_times[i][1] = 0x15;
        best_times[i][2] = 0x00;
    }
}

TTrial::~TTrial(void)
{

}

void TTrial::init()
{
    state = INIT_COURSEMAP;
}

bool TTrial::tick()
{
    switch (state)
    {
        case INIT_COURSEMAP:
            osprites.init();
            video.enabled = true;
            video.sprite_layer->set_x_clip(true);
            omap.init();
            omap.load_sprites();
            omap.position_ferrari(FERRARI_POS[level_selected = 0]);
            ohud.blit_text_big(1, "STEER TO SELECT TRACK");
            ohud.blit_text1(2, 25, TEXT1_LAPTIME1);
            ohud.blit_text1(2, 26, TEXT1_LAPTIME2);
            osoundint.queue_sound(sound::PCM_WAVE);
            outrun.ttrial.laps    = config.ttrial.laps;
            outrun.ttrial.traffic = config.ttrial.traffic;
            state = TICK_COURSEMAP;

        case TICK_COURSEMAP:
        {
            if (input.has_pressed(Input::LEFT))
            {
                if (--level_selected < 0)
                    level_selected = sizeof(FERRARI_POS) - 1;
            }
            else if (input.has_pressed(Input::RIGHT))
            {
                if (++level_selected > sizeof(FERRARI_POS) - 1)
                    level_selected = 0;
            }
            else if (input.has_pressed(Input::START) || input.has_pressed(Input::ACCEL))
            {
                outrun.ttrial.enabled          = true;
                outrun.ttrial.level            = STAGE_LOOKUP[level_selected];
                outrun.ttrial.current_lap      = 0;
                outrun.ttrial.best_lap_counter = 10000;
                outrun.ttrial.best_lap[0]      = best_times[level_selected][0];
                outrun.ttrial.best_lap[1]      = best_times[level_selected][1];
                outrun.ttrial.best_lap[2]      = best_times[level_selected][2];
                outrun.ttrial.new_high_score   = false;
                ostats.credits = 1;
                return true;
            }
            omap.position_ferrari(FERRARI_POS[level_selected]);
            ohud.draw_lap_timer(ohud.translate(7, 26), best_times[level_selected], OStats::LAP_MS[best_times[level_selected][2]]);

            omap.blit();
            oroad.tick();
            osprites.sprite_copy();
            osprites.update_sprites();
            otiles.write_tilemap_hw();
            otiles.update_tilemaps();
        }
            break;
    }

    return false;
}

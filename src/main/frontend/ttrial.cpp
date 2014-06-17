/***************************************************************************
    Time Trial Mode Front End.

    This file is part of Cannonball. 
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "sdl/input.hpp"
#include "frontend/ttrial.hpp"

#include "engine/ohud.hpp"
#include "engine/oinputs.hpp"
#include "engine/outils.hpp"
#include "engine/omap.hpp"
#include "engine/ostats.hpp"
#include "engine/otiles.hpp"

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

TTrial::TTrial(uint16_t* best_times)
{
    this->best_times = best_times;
}

TTrial::~TTrial(void)
{

}

void TTrial::init()
{
    state = INIT_COURSEMAP;
}

int TTrial::tick()
{
    switch (state)
    {
        case INIT_COURSEMAP:
            outrun.select_course(config.engine.jap != 0, config.engine.prototype != 0); // Need to setup correct course map graphics.
            config.load_tiletrial_scores();
            osprites.init();
            video.enabled = true;
            video.sprite_layer->set_x_clip(false);
            omap.init();
            omap.load_sprites();
            omap.position_ferrari(FERRARI_POS[level_selected = 0]);
            ohud.blit_text_big(1, "STEER TO SELECT TRACK");
            ohud.blit_text1(2, 25, TEXT1_LAPTIME1);
            ohud.blit_text1(2, 26, TEXT1_LAPTIME2);
            osoundint.queue_sound(sound::PCM_WAVE);
            outrun.ttrial.laps    = config.ttrial.laps;
            outrun.custom_traffic = config.ttrial.traffic;
            state = TICK_COURSEMAP;

        case TICK_COURSEMAP:
            {
                if (input.has_pressed(Input::MENU))
                {
                    return BACK_TO_MENU;
                }
                else if (input.has_pressed(Input::LEFT) || oinputs.is_analog_l())
                {
                    if (--level_selected < 0)
                        level_selected = sizeof(FERRARI_POS) - 1;
                }
                else if (input.has_pressed(Input::RIGHT)|| oinputs.is_analog_r())
                {
                    if (++level_selected > sizeof(FERRARI_POS) - 1)
                        level_selected = 0;
                }
                else if (input.has_pressed(Input::START) || input.has_pressed(Input::ACCEL) || oinputs.is_analog_select())
                {
                    outils::convert_counter_to_time(best_times[level_selected], best_converted);

                    outrun.cannonball_mode         = Outrun::MODE_TTRIAL;
                    outrun.ttrial.level            = STAGE_LOOKUP[level_selected];
                    outrun.ttrial.current_lap      = 0;
                    outrun.ttrial.best_lap_counter = 10000;
                    outrun.ttrial.best_lap[0]      = best_converted[0];
                    outrun.ttrial.best_lap[1]      = best_converted[1];
                    outrun.ttrial.best_lap[2]      = best_converted[2];
                    outrun.ttrial.best_lap_counter = best_times[level_selected];
                    outrun.ttrial.new_high_score   = false;
                    outrun.ttrial.overtakes        = 0;
                    outrun.ttrial.crashes          = 0;
                    outrun.ttrial.vehicle_cols     = 0;
                    ostats.credits = 1;
                    return INIT_GAME;
                }
                omap.position_ferrari(FERRARI_POS[level_selected]);
                outils::convert_counter_to_time(best_times[level_selected], best_converted);
                ohud.draw_lap_timer(ohud.translate(7, 26), best_converted, best_converted[2]);
                omap.blit();
                oroad.tick();
                osprites.sprite_copy();
                osprites.update_sprites();
                otiles.write_tilemap_hw();
                otiles.update_tilemaps(0);
            }
            break;
    }

    return CONTINUE;
}

void TTrial::update_best_time()
{
    best_times[level_selected] = outrun.ttrial.best_lap_counter;
    config.save_tiletrial_scores();
}
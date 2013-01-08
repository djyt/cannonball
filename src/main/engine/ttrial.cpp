#include "sdl/input.hpp"
#include "engine/ttrial.hpp"
#include "engine/omap.hpp"

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

TTrial::TTrial(void)
{

}

TTrial::~TTrial(void)
{

}

void TTrial::init()
{
    osprites.init();
    state = INIT_COURSEMAP;
}

void TTrial::tick()
{
    switch (state)
    {
        case INIT_COURSEMAP:
            video.enabled = true;
            omap.init();
            omap.load_sprites();
            omap.position_ferrari(FERRARI_POS[level_selected = 0]);
            ohud.blit_text_big(1, "STEER TO SELECT TRACK");
            state = TICK_COURSEMAP;

        case TICK_COURSEMAP:           
            if (input.has_pressed(Input::LEFT))
            {
                if (--level_selected < 0)
                    level_selected = sizeof(FERRARI_POS) - 1;
                omap.position_ferrari(FERRARI_POS[level_selected]);
            }
            else if (input.has_pressed(Input::RIGHT))
            {
                if (++level_selected > sizeof(FERRARI_POS) - 1)
                    level_selected = 0;
                std::cout << (int) level_selected << std::endl;
                omap.position_ferrari(FERRARI_POS[level_selected]);
            }
            omap.blit();
            oroad.tick();
            osprites.sprite_copy();
            osprites.update_sprites();
            otiles.write_tilemap_hw();
            
            break;
    }
}

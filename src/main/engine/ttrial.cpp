#include "engine/ttrial.hpp"
#include "engine/omap.hpp"

// Notes: Set the car up as follows to snap it to the relevant map piece
//  osprites.jump_table[25].x = osprites.jump_table[z].x;
//  osprites.jump_table[25].y = osprites.jump_table[z].y;

// Where z is 1,3,5,7,9,11,13,15 based on a stage in the route map.
// We could also change the colour of the road here. 

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
            ohud.blit_text_big(1, "STEER TO SELECT TRACK");
            state = TICK_COURSEMAP;

        case TICK_COURSEMAP:
            omap.blit();
            oroad.tick();
            osprites.sprite_copy();
            osprites.update_sprites();
            otiles.write_tilemap_hw();
            break;
    }
}

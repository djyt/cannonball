/***************************************************************************
    Level Object Logic
    
    This class handles rendering most of the objects that comprise a typical
    level. 
    
    - Configures rendering properties (co-ordinates, zoom etc.) 
    - Object specific logic, including collision checks & start lights etc.

    The original codebase contains a large amount of code duplication,
    much of which is duplicated here.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

class OLevelObjs
{
    public:
        // Spray Counter (Going Through Water). Might need to be signed?
        uint16_t spray_counter;

        // Wheel Spray Type
        // 00 = Water
        // 04 = Yellow Stuff
        // 08 = Green Stuff
        // 0c = Pink stuff
        // 10 = Smoke
        uint16_t spray_type;

	    //	Collision With Sprite Has Ocurred
	    //
	    // 0 = No Collision
	    // 1 = Collision (and increments for every additional collision in this crash cycle)
	    uint8_t collision_sprite;

	    // Sprite Collision Counter (Hitting Scenery)
	    int16_t sprite_collision_counter;

        OLevelObjs(void);
        ~OLevelObjs(void);

        void default_entries();
        void hiscore_entries();
        void setup_sprites(uint32_t);
        void do_sprite_routine();
        void hide_sprite(oentry*);

    private:
        // Default sprite entries for stage 1 initialization
        const static uint8_t DEF_SPRITE_ENTRIES = 0x44;
        
        // Hi-Score Sprite Entries
        const static uint8_t HISCORE_SPRITE_ENTRIES = 0x40;

        const static uint8_t COLLISION_RESET = 4;
        const static uint16_t SPRAY_RESET = 0xC;

        void init_entries(uint32_t, uint8_t);
	    void setup_sprite(oentry*, uint32_t);
	    void setup_sprite_routine(oentry*);		
        void sprite_collision_z1c(oentry*);
        void sprite_lights(oentry*);
        void sprite_lights_countdown(oentry*);
        void sprite_grass(oentry* sprite);
        void sprite_water(oentry* sprite);
        void sprite_rocks(oentry* sprite);
        void sprite_debris(oentry* sprite);
        void sprite_minitree(oentry* sprite);
        void do_thickness_sprite(oentry* sprite, const uint32_t);
        void sprite_clouds(oentry* sprite);
	    void sprite_normal(oentry*, uint8_t);
	    void set_spr_zoom_priority(oentry*, uint8_t);
        void set_spr_zoom_priority2(oentry*, uint8_t);
        void set_spr_zoom_priority_rocks(oentry*, uint8_t);
};

extern OLevelObjs olevelobjs;
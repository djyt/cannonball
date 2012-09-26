#pragma once

#include "outrun.hpp"

class OSmoke
{
public:
    // Load smoke sprites for next level?
    int8_t load_smoke_data;

    OSmoke(void);
    ~OSmoke(void);
    void init();
    void setup_smoke_sprite(bool);
    void draw_ferrari_smoke(oentry*);
    void draw(oentry*);

private:
    // Ferrari wheel smoke type on road
    uint16_t smoke_type_onroad;

    // Ferrari wheel smoke type off road
    uint16_t smoke_type_offroad;

    // Ferrari wheel smoke type after car collision
    uint16_t smoke_type_slip;

    void tick_smoke_anim(oentry*, int8_t, uint32_t);
   
};

extern OSmoke osmoke;


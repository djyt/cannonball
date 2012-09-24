#pragma once

#include "outrun.hpp"

class OMusic
{
public:
    OMusic(void);
    ~OMusic(void);

    void enable();
    void disable();
    void tick();
    void blit();
    void check_start();

private:
    uint16_t entry_start;
    
	void setup_sprite1();
	void setup_sprite2();
	void setup_sprite3();
	void setup_sprite4();
	void setup_sprite5();
    void blit_music_select();
};

extern OMusic omusic;

